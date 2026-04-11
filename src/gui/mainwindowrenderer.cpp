/*
	This file is part of Ingnomia https://github.com/rschurade/Ingnomia
    Copyright (C) 2017-2020  Ralph Schurade, Ingnomia Team

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/** @file mainwindowrenderer.cpp
 *  @brief MainWindowRenderer implementation: shader/texture/VBO setup, per-frame draw
 *         ordering (compute tile update → tile draw → selection → thought bubbles → axles),
 *         and camera/view-level controls.
 */
#include "mainwindowrenderer.h"

#include "../game/game.h" //TODO only temporary

#include "../base/config.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/util.h"
#include "../base/vptr.h"
#include "../game/gamemanager.h"
#include "../game/plant.h"
#include "../game/world.h"
#include "../gfx/sprite.h"
#include "../gfx/spritefactory.h"
#include "eventconnector.h"
#include "mainwindow.h"
#include "aggregatorselection.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QImage>
#include <QMessageBox>
#include <QTimer>
#include <QOpenGLContext>

#include <glad/gl.h>

#include <cstring>
#include <unordered_map>

namespace
{
/// @brief RAII helper that opens a GL debug-output group on construction and pops it on
///        destruction, so debug tools like RenderDoc show hierarchical scopes.
class DebugScope
{
public:
	DebugScope() = delete;
	DebugScope( const char* c )
	{
		static GLuint counter = 0;
		glPushDebugGroup( GL_DEBUG_SOURCE_APPLICATION, counter++, static_cast<GLsizei>( strlen( c ) ), c );
	}
	~DebugScope()
	{
		glPopDebugGroup();
	}
};
} // namespace

/// @brief Constructs the renderer and wires up the signal chain with AggregatorRenderer,
///        EventConnector, and AggregatorSelection. initializeGL() must be called after the
///        parent's GL context is current.
/// @param parent Owning MainWindow.
MainWindowRenderer::MainWindowRenderer( MainWindow* parent ) :
	QObject( parent ),
	m_parent( parent )
{
	connect( Global::eventConnector->aggregatorRenderer(), &AggregatorRenderer::signalWorldParametersChanged, this, &MainWindowRenderer::cleanupWorld );

	connect( Global::eventConnector->aggregatorRenderer(), &AggregatorRenderer::signalTileUpdates, this, &MainWindowRenderer::onTileUpdates );
	connect( Global::eventConnector->aggregatorRenderer(), &AggregatorRenderer::signalAxleData, this, &MainWindowRenderer::onAxelData );
	connect( Global::eventConnector->aggregatorRenderer(), &AggregatorRenderer::signalThoughtBubbles, this, &MainWindowRenderer::onThoughtBubbles );
	connect( Global::eventConnector->aggregatorRenderer(), &AggregatorRenderer::signalCenterCamera, this, &MainWindowRenderer::onCenterCameraPosition );
	connect( Global::eventConnector, &EventConnector::signalInMenu, this, &MainWindowRenderer::onSetInMenu );

	connect( Global::eventConnector->aggregatorSelection(), &AggregatorSelection::signalUpdateSelection, this, &MainWindowRenderer::onUpdateSelection, Qt::QueuedConnection );

	// Full polling of initial state on load
	connect( this, &MainWindowRenderer::fullDataRequired, Global::eventConnector->aggregatorRenderer(), &AggregatorRenderer::onAllTileInfo );
	connect( this, &MainWindowRenderer::fullDataRequired, Global::eventConnector->aggregatorRenderer(), &AggregatorRenderer::onThoughtBubbleUpdate );
	connect( this, &MainWindowRenderer::fullDataRequired, Global::eventConnector->aggregatorRenderer(), &AggregatorRenderer::onAxleDataUpdate );

	connect( this, &MainWindowRenderer::signalCameraPosition, Global::eventConnector, &EventConnector::onCameraPosition );

	qDebug() << "initialize GL ...";
	connect( m_parent->context(), &QOpenGLContext::aboutToBeDestroyed, this, &MainWindowRenderer::cleanup );
	connect( this, &MainWindowRenderer::redrawRequired, m_parent, &MainWindow::redraw );
}

/// @brief Destructor.
MainWindowRenderer ::~MainWindowRenderer()
{
}

/// @brief One-time GL resource setup: installs a debug log handler, builds the shared
///        vertex/index buffers for the tile quad, compiles shaders, and creates the sprite
///        atlas array textures.
void MainWindowRenderer::initializeGL()
{
	qDebug() << "[OpenGL]" << reinterpret_cast<char const*>( glGetString( GL_VENDOR ) );
	qDebug() << "[OpenGL]" << reinterpret_cast<char const*>( glGetString( GL_VERSION ) );
	qDebug() << "[OpenGL]" << reinterpret_cast<char const*>( glGetString( GL_RENDERER ) );

	qDebug() << m_parent->context()->format();

	GLDEBUGPROC logHandler = []( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam ) -> void
	{
		static const std::unordered_map<GLenum, const char*> debugTypes = {
			{ GL_DEBUG_TYPE_ERROR, "Error" },
			{ GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, "DeprecatedBehavior" },
			{ GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR, "UndefinedBehavior" },
			{ GL_DEBUG_TYPE_PORTABILITY, "Portability" },
			{ GL_DEBUG_TYPE_PERFORMANCE, "Performance" },
			{ GL_DEBUG_TYPE_MARKER, "Marker" },
			{ GL_DEBUG_TYPE_OTHER, "Other" },
			{ GL_DEBUG_TYPE_PUSH_GROUP, "Push" },
			{ GL_DEBUG_TYPE_POP_GROUP, "Pop" }
		};
		static const std::unordered_map<GLenum, const char*> severities = {
			{ GL_DEBUG_SEVERITY_LOW, "low" },
			{ GL_DEBUG_SEVERITY_MEDIUM, "medium" },
			{ GL_DEBUG_SEVERITY_HIGH, "high" },
			{ GL_DEBUG_SEVERITY_NOTIFICATION, "notify" },
		};
		if ( severity == GL_DEBUG_SEVERITY_NOTIFICATION && !Global::debugOpenGL )
			return;
		// Only want to handle these from dedicated graphic debugger
		if ( type == GL_DEBUG_TYPE_PUSH_GROUP || type == GL_DEBUG_TYPE_POP_GROUP )
			return;
		// Rate-limit GL error messages to avoid log spam
		// Known issue: Noesis GUI generates GL_INVALID_OPERATION errors
		static int glErrorCount = 0;
		if ( type == GL_DEBUG_TYPE_ERROR )
		{
			++glErrorCount;
			if ( glErrorCount <= 5 )
			{
				qDebug() << "[OpenGL]" << debugTypes.at( type ) << " " << severities.at(severity) << ":" << message;
			}
			else if ( glErrorCount == 6 )
			{
				qDebug() << "[OpenGL] Suppressing further GL_ERROR messages";
			}
			return;
		}
		qDebug() << "[OpenGL]" << debugTypes.at( type ) << " " << severities.at(severity) << ":" << message;
	};
	glEnable( GL_DEBUG_OUTPUT );
	glDebugMessageCallback( logHandler, nullptr );

	float vertices[] = {
		// Wall layer
		0.f, .8f, 1.f, // top left
		0.f, .2f, 1.f, // bottom left
		1.f, .8f, 1.f, // top right
		1.f, .2f, 1.f, // bottom right

		// floor layer
		0.f, .5f, 0.f, // top left
		0.f, .2f, 0.f, // bottom left
		1.f, .5f, 0.f, // top right
		1.f, .2f, 0.f, // bottom right
	};

	constexpr GLushort indices[] = {
		0, 1, 3, // Wall 1
		2, 0, 3, // Wall 2
		4, 5, 7, // Floor 1
		6, 4, 7, // Floor 2
		// Wall again for BTF rendering
		0, 1, 3, // Wall 1
		2, 0, 3, // Wall 2
	};

	if ( !initShaders() )
	{
		//qCritical() << "failed to init shaders - exiting";
	}

	glGenVertexArrays( 1, &m_vao );
	glBindVertexArray( m_vao );

	glGenBuffers( 1, &m_vbo );
	glBindBuffer( GL_ARRAY_BUFFER, m_vbo );
	glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_STATIC_DRAW );

	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( float ), (void*)0 );
	glEnableVertexAttribArray( 0 );

	glGenBuffers( 1, &m_vibo );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_vibo );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ), indices, GL_STATIC_DRAW );

	glBindVertexArray( 0 );

	updateRenderParams();

	qDebug() << "initialize GL - done";
}

/// @brief Queues a shader reload for the next paintWorld() call (debug hotkey entry point).
void MainWindowRenderer::reloadShaders()
{
	m_reloadShaders = true;
}

/// @brief Releases all GL resources (buffers, shaders, textures). Invoked when the parent
///        context is about to be destroyed.
void MainWindowRenderer::cleanup()
{
	m_parent->makeCurrent();

	if ( m_worldShader ) { glDeleteProgram( m_worldShader ); m_worldShader = 0; }
	if ( m_worldUpdateShader ) { glDeleteProgram( m_worldUpdateShader ); m_worldUpdateShader = 0; }
	if ( m_thoughtBubbleShader ) { glDeleteProgram( m_thoughtBubbleShader ); m_thoughtBubbleShader = 0; }
	if ( m_selectionShader ) { glDeleteProgram( m_selectionShader ); m_selectionShader = 0; }
	if ( m_axleShader ) { glDeleteProgram( m_axleShader ); m_axleShader = 0; }

	glDeleteBuffers( 1, &m_vbo );
	glDeleteBuffers( 1, &m_vibo );
	if ( m_vao ) { glDeleteVertexArrays( 1, &m_vao ); m_vao = 0; }

	m_parent->doneCurrent();

	m_reloadShaders = true;
}

/// @brief Releases per-world GL resources (tile buffer objects) so a new world can be
///        loaded without leaking GPU memory. Triggered by signalWorldParametersChanged.
void MainWindowRenderer::cleanupWorld()
{
	m_parent->makeCurrent();
	glDeleteTextures( 32, m_textures );
	memset( m_textures, 0, sizeof( m_textures ) );
	glDeleteBuffers( 1, &m_tileBo );
	m_tileBo = 0;
	glDeleteBuffers( 1, &m_tileUpdateBo );
	m_tileUpdateBo     = 0;
	m_texesInitialized = false;

	m_parent->doneCurrent();

	m_pendingUpdates.clear();
	m_selectionData.clear();
	m_thoughBubbles = ThoughtBubbleInfo();
	m_axleData      = AxleDataInfo();
}

/// @brief Slot: enqueues an incoming tile-update batch to be uploaded and applied next frame.
/// @param updates Batch of per-tile TileDataUpdate packets.
void MainWindowRenderer::onTileUpdates( const TileDataUpdateInfo& updates )
{
	m_pendingUpdates.push_back( updates.updates );
	emit redrawRequired();
}

/// @brief Slot: stores the current set of thought bubbles for the next paint.
/// @param bubbles New thought bubble batch.
void MainWindowRenderer::onThoughtBubbles( const ThoughtBubbleInfo& bubbles )
{
	m_thoughBubbles = bubbles;
	emit redrawRequired();
}

/// @brief Slot: stores the current axle power data for the next paint.
/// @param data New axle data batch.
void MainWindowRenderer::onAxelData( const AxleDataInfo& data )
{
	m_axleData = data;
	emit redrawRequired();
}

/// @brief Reads a shader source file from content/shaders/@p name and returns its contents.
/// @param name Filename without directory.
/// @return Shader source as a QString, or empty on failure.
QString MainWindowRenderer::copyShaderToString( QString name )
{
	QFile file( Global::cfg->get( "dataPath" ).toString() + "/shaders/" + name + ".glsl" );
	file.open( QIODevice::ReadOnly );
	QTextStream in( &file );
	QString code( "" );
	while ( !in.atEnd() )
	{
		code += in.readLine();
		code += "\n";
	}

	return code;
}

/// @brief Compiles and links a vertex/fragment shader pair with the given base @p name
///        (<name>.vert / <name>.frag under content/shaders).
/// @param name Shader base filename (without extension).
/// @return GL program handle, or 0 on compile/link failure.
GLuint MainWindowRenderer::initShader( QString name )
{
	QString vs = copyShaderToString( name + "_v" );
	QString fs = copyShaderToString( name + "_f" );

	// Create and compile vertex shader
	GLuint vertexShader = glCreateShader( GL_VERTEX_SHADER );
	QByteArray vsBytes = vs.toUtf8();
	const char* vsSource = vsBytes.constData();
	glShaderSource( vertexShader, 1, &vsSource, nullptr );
	glCompileShader( vertexShader );

	GLint success;
	glGetShaderiv( vertexShader, GL_COMPILE_STATUS, &success );
	if ( !success )
	{
		char infoLog[512];
		glGetShaderInfoLog( vertexShader, 512, nullptr, infoLog );
		qCritical() << "Vertex shader compilation failed for" << name << ":" << infoLog;
		glDeleteShader( vertexShader );
		return 0;
	}

	// Create and compile fragment shader
	GLuint fragmentShader = glCreateShader( GL_FRAGMENT_SHADER );
	QByteArray fsBytes = fs.toUtf8();
	const char* fsSource = fsBytes.constData();
	glShaderSource( fragmentShader, 1, &fsSource, nullptr );
	glCompileShader( fragmentShader );

	glGetShaderiv( fragmentShader, GL_COMPILE_STATUS, &success );
	if ( !success )
	{
		char infoLog[512];
		glGetShaderInfoLog( fragmentShader, 512, nullptr, infoLog );
		qCritical() << "Fragment shader compilation failed for" << name << ":" << infoLog;
		glDeleteShader( vertexShader );
		glDeleteShader( fragmentShader );
		return 0;
	}

	// Create program and link shaders
	GLuint program = glCreateProgram();
	glAttachShader( program, vertexShader );
	glAttachShader( program, fragmentShader );
	glLinkProgram( program );

	glGetProgramiv( program, GL_LINK_STATUS, &success );
	if ( !success )
	{
		char infoLog[512];
		glGetProgramInfoLog( program, 512, nullptr, infoLog );
		qCritical() << "Shader program linking failed for" << name << ":" << infoLog;
		glDeleteShader( vertexShader );
		glDeleteShader( fragmentShader );
		glDeleteProgram( program );
		return 0;
	}

	// Clean up shaders (they're now in the program)
	glDeleteShader( vertexShader );
	glDeleteShader( fragmentShader );

	return program;
}

/// @brief Compiles and links a compute shader with the given base @p name (<name>.comp).
/// @param name Shader base filename (without extension).
/// @return GL program handle, or 0 on compile/link failure.
GLuint MainWindowRenderer::initComputeShader( QString name )
{
	QString cs = copyShaderToString( name + "_c" );

	// Create and compile compute shader
	GLuint computeShader = glCreateShader( GL_COMPUTE_SHADER );
	QByteArray csBytes = cs.toUtf8();
	const char* csSource = csBytes.constData();
	glShaderSource( computeShader, 1, &csSource, nullptr );
	glCompileShader( computeShader );

	GLint success;
	glGetShaderiv( computeShader, GL_COMPILE_STATUS, &success );
	if ( !success )
	{
		char infoLog[512];
		glGetShaderInfoLog( computeShader, 512, nullptr, infoLog );
		qCritical() << "Compute shader compilation failed for" << name << ":" << infoLog;
		glDeleteShader( computeShader );
		return 0;
	}

	// Create program and link shader
	GLuint program = glCreateProgram();
	glAttachShader( program, computeShader );
	glLinkProgram( program );

	glGetProgramiv( program, GL_LINK_STATUS, &success );
	if ( !success )
	{
		char infoLog[512];
		glGetProgramInfoLog( program, 512, nullptr, infoLog );
		qCritical() << "Compute shader program linking failed for" << name << ":" << infoLog;
		glDeleteShader( computeShader );
		glDeleteProgram( program );
		return 0;
	}

	// Clean up shader
	glDeleteShader( computeShader );

	return program;
}

/// @brief Loads and compiles every shader used by the renderer. Called at init and on a
///        debug-hotkey shader reload.
/// @return true if every shader compiled successfully.
bool MainWindowRenderer::initShaders()
{
	m_worldShader = initShader( "world" );
	m_worldUpdateShader = initComputeShader( "worldupdate" );
	m_thoughtBubbleShader = initShader( "thoughtbubble" );
	m_selectionShader = initShader( "selection" );
	m_axleShader = initShader( "axle" );

	if ( !m_worldShader || !m_worldUpdateShader || !m_thoughtBubbleShader || !m_selectionShader || !m_axleShader )
	{
		// Can't proceed, and need to know what happened!
		abort();
		return false;
	}

	m_reloadShaders = false;

	return true;
}

/// @brief Allocates a new 32×64 RGBA8 array texture with @p depth slices on texture unit @p unit.
/// @param unit  GL texture unit index.
/// @param depth Number of array slices.
void MainWindowRenderer::createArrayTexture( int unit, int depth )
{
	glActiveTexture( GL_TEXTURE0 + unit );
	glGenTextures( 1, &m_textures[unit] );
	glBindTexture( GL_TEXTURE_2D_ARRAY, m_textures[unit] );
	glTexStorage3D(
		GL_TEXTURE_2D_ARRAY,
		1,             // No mipmaps
		GL_RGBA8,      // Internal format
		32, 64,        // width,height
		depth          // Number of layers
	);
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0 );
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
}

/// @brief Uploads @p depth RGBA8 slices from @p data into an existing array texture.
/// @param unit  GL texture unit index.
/// @param depth Number of slices to upload.
/// @param data  Source pixel data (depth × 32 × 64 × 4 bytes).
void MainWindowRenderer::uploadArrayTexture( int unit, int depth, const uint8_t* data )
{
	glActiveTexture( GL_TEXTURE0 + unit );
	glTexSubImage3D(
		GL_TEXTURE_2D_ARRAY,
		0,                // Mipmap number
		0, 0, 0,          // xoffset, yoffset, zoffset
		32, 64, depth,    // width, height, depth
		GL_RGBA,          // format
		GL_UNSIGNED_BYTE, // type
		data
	);
}

/// @brief Creates all sprite atlas array textures used by the renderer.
void MainWindowRenderer::initTextures()
{
	GLint max_layers;
	glGetIntegerv( GL_MAX_ARRAY_TEXTURE_LAYERS, &max_layers );

	qDebug() << "max array size: " << max_layers;
	qDebug() << "used " << Global::eventConnector->game()->sf()->size() << " sprites";

	int maxArrayTextures = Global::cfg->get( "MaxArrayTextures" ).toInt();

	for ( int i = 0; i < 32; ++i )
	{
		createArrayTexture( i, maxArrayTextures );
	}

	m_texesInitialized = true;
}

/// @brief Allocates the per-world tile data SSBO (m_tileBo) sized for the current world
///        dimensions. Called after a new world is generated or loaded.
void MainWindowRenderer::initWorld()
{
	QElapsedTimer timer;
	timer.start();

	glGenBuffers( 1, &m_tileBo );
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, m_tileBo );
	glBufferData( GL_SHADER_STORAGE_BUFFER, TD_SIZE * sizeof( unsigned int ) * Global::eventConnector->game()->w()->world().size(), nullptr, GL_DYNAMIC_DRAW );
	const uint8_t zero = 0;
	glClearBufferData( GL_SHADER_STORAGE_BUFFER, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &zero );
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 ); // unbind

	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 0, m_tileBo );

	glGenBuffers( 1, &m_tileUpdateBo );

	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 0, m_tileBo );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1, m_tileUpdateBo );

	m_texesInitialized = true;

	m_rotation = 0;

	emit fullDataRequired();
}

/// @brief Rebuilds the orthographic projection matrix and the RenderVolume bounds from
///        the current camera parameters.
void MainWindowRenderer::updateRenderParams()
{
	m_renderSize = qMin( Global::dimX, (int)( ( sqrt( m_width * m_width + m_height * m_height ) / 12 ) / m_scale ) );

	m_renderDepth = Global::cfg->get( "renderDepth" ).toInt();

	m_viewLevel = GameState::viewLevel;

	m_volume.min = { 0, 0, qMin( qMax( m_viewLevel - m_renderDepth, 0 ), Global::dimZ - 1 ) };
	m_volume.max = { Global::dimX - 1, Global::dimY - 1, qMin( m_viewLevel, Global::dimZ - 1 ) };

	m_lightMin = Global::cfg->get( "lightMin" ).toFloat();
	if ( m_lightMin < 0.01 )
		m_lightMin = 0.3f;

	m_debug   = Global::debugMode;

	m_projectionMatrix.setToIdentity();
	m_projectionMatrix.ortho( -m_width / 2, m_width / 2, -m_height / 2, m_height / 2, -( m_volume.max.x + m_volume.max.y + m_volume.max.z + 1 ), -m_volume.min.z );
	m_projectionMatrix.scale( m_scale, m_scale );
	m_projectionMatrix.translate( m_moveX, -m_moveY );
}

// Helper functions for setting uniforms
/// @brief Sets an int uniform by name on a GL program.
/// @param shader Program handle.
/// @param name   Uniform name.
/// @param value  Integer value.
void MainWindowRenderer::setUniformi( GLuint shader, const char* name, GLint value )
{
	GLint loc = glGetUniformLocation( shader, name );
	if ( loc >= 0 ) glUniform1i( loc, value );
}

/// @brief Sets a float uniform by name on a GL program.
/// @param shader Program handle.
/// @param name   Uniform name.
/// @param value  Float value.
void MainWindowRenderer::setUniformf( GLuint shader, const char* name, GLfloat value )
{
	GLint loc = glGetUniformLocation( shader, name );
	if ( loc >= 0 ) glUniform1f( loc, value );
}

/// @brief Sets an unsigned int uniform by name on a GL program.
/// @param shader Program handle.
/// @param name   Uniform name.
/// @param value  Unsigned integer value.
void MainWindowRenderer::setUniformui( GLuint shader, const char* name, GLuint value )
{
	GLint loc = glGetUniformLocation( shader, name );
	if ( loc >= 0 ) glUniform1ui( loc, value );
}

/// @brief Sets a mat4 uniform by name on a GL program, converting from QMatrix4x4.
/// @param shader Program handle.
/// @param name   Uniform name.
/// @param matrix Matrix value (Qt column-major).
void MainWindowRenderer::setUniformMatrix4fv( GLuint shader, const char* name, const QMatrix4x4& matrix )
{
	GLint loc = glGetUniformLocation( shader, name );
	if ( loc >= 0 ) glUniformMatrix4fv( loc, 1, GL_FALSE, matrix.constData() );
}

/// @brief Main draw function for the world view. Clears the framebuffer, optionally reloads
///        shaders, applies pending tile updates via the compute shader, then draws tiles,
///        selection preview, thought bubbles, and axles in order.
void MainWindowRenderer::paintWorld()
{
	DebugScope s( "paint world" );
	QElapsedTimer timer;
	//timer.start();
	{
		DebugScope s( "clear" );
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glEnable( GL_DEPTH_TEST );
		glDepthFunc( GL_LEQUAL );
		glDisable( GL_STENCIL_TEST );
		glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
		glDisable( GL_SCISSOR_TEST );

		glDepthMask( true );
		glStencilMask( 0xFFFFFFFF );
		glClearStencil( 0 );
		glClearDepth( 1 );
		glClearColor( 0.0, 0.0, 0.0, 1.0 );
		glColorMask( true, true, true, true );
		//glClearDepth( 1 );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	}

	if ( m_inMenu )
	{
		return;
	}

	if ( m_reloadShaders )
	{
		DebugScope s( "init shaders" );
		if ( !initShaders() )
		{
			return;
		}
	}

	if ( !m_texesInitialized )
	{
		DebugScope s( "init textures" );
		initWorld();
		initTextures();
		updateRenderParams();
	}
	updateWorld();

	// Rebind correct textures to texture units
	for ( auto unit = 0; unit < 32; ++unit )
	{
		glActiveTexture( GL_TEXTURE0 + unit );
		glBindTexture( GL_TEXTURE_2D_ARRAY, m_textures[unit] );
	}

	timer.start();
	updateTextures();

	glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT );

	QString msg = "render time: " + QString::number( timer.elapsed() ) + " ms";
	//emit sendOverlayMessage( 1, msg );

	{
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glEnable( GL_DEPTH_TEST );
		glDepthFunc( GL_LEQUAL );
		glDisable( GL_STENCIL_TEST );

		glDepthMask( true );
		glStencilMask( 0xFFFFFFFF );
		glClearStencil( 0 );
		glClearDepth( 1 );
		glClearColor( 0.0, 0.0, 0.0, 1.0 );
		glColorMask( true, true, true, true );
		//glClearDepth( 1 );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

		glBindVertexArray( m_vao );

		paintTiles();

		paintSelection();

		paintThoughtBubbles();

		if ( Global::showAxles )
		{
			paintAxles();
		}

		glBindVertexArray( 0 );
	}

	//glFinish();

	bool pause = Global::cfg->get( "Pause" ).toBool();

	if ( pause != m_pause )
	{
		m_pause = pause;
	}
}

/// @brief Slot invoked when camera parameters change: recomputes render params and emits
///        signalCameraPosition so listeners (e.g. AggregatorSound) can react.
void MainWindowRenderer::onRenderParamsChanged()
{
	updateRenderParams();
	emit redrawRequired();
	emit signalCameraPosition(m_moveX, m_moveY, m_viewLevel, m_rotation, m_scale);

}

/// @brief Uploads the common uniforms shared by every draw shader (projection, camera,
///        view level, daylight, light floor, rotation, …) onto the given program.
/// @param shader Target shader program.
void MainWindowRenderer::setCommonUniforms( GLuint shader )
{
	GLint indexTotal = glGetUniformLocation( shader, "uWorldSize" );
	if ( indexTotal >= 0 )
	{
		glUniform3ui( indexTotal, Global::dimX, Global::dimY, Global::dimZ );
	}
	GLint indexMin = glGetUniformLocation( shader, "uRenderMin" );
	if ( indexMin >= 0 )
	{
		glUniform3ui( indexMin, m_volume.min.x, m_volume.min.y, m_volume.min.z );
	}
	GLint indexMax = glGetUniformLocation( shader, "uRenderMax" );
	if ( indexMax >= 0 )
	{
		glUniform3ui( indexMax, m_volume.max.x, m_volume.max.y, m_volume.max.z );
	}
	setUniformMatrix4fv( shader, "uTransform", m_projectionMatrix );
	setUniformi( shader, "uWorldRotation", (GLint)m_rotation );
	setUniformi( shader, "uTickNumber", (GLint)GameState::tick );
}

/// @brief Draws the main world tile layer: binds m_worldShader, the tile SSBO, and the
///        sprite array textures, then issues an instanced draw of the tile quad per tile
///        in the render volume.
void MainWindowRenderer::paintTiles()
{
	DebugScope s( "paint tiles" );

	glUseProgram( m_worldShader );
	setCommonUniforms( m_worldShader );

	for ( int i = 0; i < m_texesUsed; ++i )
	{
		auto texNum = "uTexture[" + QString::number( i ) + "]";
		setUniformi( m_worldShader, texNum.toStdString().c_str(), i );
	}

	//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	setUniformi( m_worldShader, "uOverlay", Global::showDesignations ? 1 : 0 );
	setUniformi( m_worldShader, "uShowJobs", Global::showJobs ? 1 : 0 );
	setUniformi( m_worldShader, "uDebug", m_debug ? 1 : 0 );
	setUniformi( m_worldShader, "uWallsLowered", Global::wallsLowered ? 1 : 0 );

	setUniformi( m_worldShader, "uUndiscoveredTex", Global::undiscoveredUID * 4 );
	setUniformi( m_worldShader, "uWaterTex", Global::waterSpriteUID * 4 );

	if ( GameState::daylight )
	{
		m_daylight = qMin( 1.0, m_daylight + 0.025 );
	}
	else
	{
		m_daylight = qMax( 0.0, m_daylight - 0.025 );
	}
	setUniformf( m_worldShader, "uDaylight", m_daylight );
	setUniformf( m_worldShader, "uLightMin", m_lightMin );
	//setUniformf( m_worldShader, "uDaylight", 1.0f );

	auto volume   = m_volume.size();
	GLsizei tiles = volume.x * volume.y * volume.z;

	// First pass is pure front-to-back of opaque blocks, alpha doesn't even work
	glDisable( GL_BLEND );
	glDepthMask( true );

	setUniformi( m_worldShader, "uPaintFrontToBack", 1 );
	glDrawElementsInstanced( GL_TRIANGLES, 12, GL_UNSIGNED_SHORT, 0, tiles );

	//!TODO Transparency pass is too early, all the stuff rendered later is still missing
	// Second pass includes transparency
	setUniformi( m_worldShader, "uPaintFrontToBack", 0 );
	glEnable( GL_BLEND );
	glDrawElementsInstanced( GL_TRIANGLES, 12, GL_UNSIGNED_SHORT, (void*)( sizeof( GLushort ) * 6 ), tiles );

	// All done with depth writes, everything beyond is layered
	glDepthMask( false );

	glUseProgram( 0 );
}

/// @brief Draws the placement cursor preview grid using m_selectionShader. Optionally
///        disables depth test for stair placement previews.
void MainWindowRenderer::paintSelection()
{
	// TODO this is a workaround until some transparency solution is implemented
	if( m_selectionNoDepthTest )
	{
		glDisable( GL_DEPTH_TEST );
	}
	DebugScope s( "paint selection" );
	glUseProgram( m_selectionShader );
	setCommonUniforms( m_selectionShader );

	for ( int i = 0; i < m_texesUsed; ++i )
	{
		auto texNum = "uTexture[" + QString::number( i ) + "]";
		setUniformi( m_selectionShader, texNum.toStdString().c_str(), i );
	}

	for ( const auto& sd : m_selectionData )
	{
		GLint tile = glGetUniformLocation( m_selectionShader, "tile" );
		glUniform3ui( tile, sd.pos.x, sd.pos.y, sd.pos.z );
		setUniformi( m_selectionShader, "uSpriteID", (GLint)sd.spriteID );
		setUniformi( m_selectionShader, "uRotation", (GLint)sd.localRot );
		setUniformi( m_selectionShader, "uValid", sd.valid ? 1 : 0 );

		glDrawArraysInstancedBaseInstance( GL_TRIANGLE_STRIP, 0, 4, 1, 0 );
	}

	glUseProgram( 0 );
	if( m_selectionNoDepthTest )
	{
		glEnable( GL_DEPTH_TEST );
	}
}

/// @brief Draws all active thought bubbles above their gnomes using m_thoughtBubbleShader.
void MainWindowRenderer::paintThoughtBubbles()
{
	DebugScope s( "paint thoughts" );

	glUseProgram( m_thoughtBubbleShader );
	setCommonUniforms( m_thoughtBubbleShader );

	setUniformi( m_thoughtBubbleShader, "uTexture0", 0 );

	for ( const auto& thoughtBubble : m_thoughBubbles.thoughtBubbles )
	{
		if ( thoughtBubble.pos.z <= m_viewLevel )
		{
			GLint tile = glGetUniformLocation( m_thoughtBubbleShader, "tile" );
			glUniform3ui( tile, thoughtBubble.pos.x, thoughtBubble.pos.y, thoughtBubble.pos.z );
			setUniformi( m_thoughtBubbleShader, "uType", (GLint)thoughtBubble.sprite );
			glDrawArraysInstancedBaseInstance( GL_TRIANGLE_STRIP, 0, 4, 1, 0 );
		}
	}

	glUseProgram( 0 );
}

/// @brief Draws the spinning axle overlays using m_axleShader, if Global::showAxles is on.
void MainWindowRenderer::paintAxles()
{
	DebugScope s( "paint axles" );

	glUseProgram( m_axleShader );
	setCommonUniforms( m_axleShader );
	setUniformi( m_axleShader, "uTickNumber", (GLint)GameState::tick );

	for ( int i = 0; i < m_texesUsed; ++i )
	{
		auto texNum = "uTexture[" + QString::number( i ) + "]";
		setUniformi( m_axleShader, texNum.toStdString().c_str(), i );
	}

	for ( const auto& ad : m_axleData.data )
	{
		if ( !ad.isVertical && ad.pos.z <= m_viewLevel )
		{
			GLint tile = glGetUniformLocation( m_axleShader, "tile" );
			glUniform3ui( tile, ad.pos.x, ad.pos.y, ad.pos.z );
			setUniformi( m_axleShader, "uSpriteID", (GLint)ad.spriteID );
			setUniformi( m_axleShader, "uRotation", (GLint)ad.localRot );
			setUniformf( m_axleShader, "uAnim", ad.anim );

			glDrawArraysInstancedBaseInstance( GL_TRIANGLE_STRIP, 0, 4, 1, 0 );
		}
	}

	glUseProgram( 0 );
}

/// @brief Slot: viewport resize handler. Updates m_width/m_height and the projection matrix.
/// @param w New width in pixels.
/// @param h New height in pixels.
void MainWindowRenderer::resize( int w, int h )
{
	m_width  = w;
	m_height = h;
	onRenderParamsChanged();
}

/// @brief Rotates the camera by @p direction steps (positive = CW, negative = CCW).
///        Updates camera offsets so the centre stays fixed, then requests a redraw.
/// @param direction Rotation delta.
void MainWindowRenderer::rotate( int direction )
{
	direction  = qBound( -1, direction, 1 );
	m_rotation = ( 4 + m_rotation + direction ) % 4;

	if( direction == 1 )
	{
		updatePositionAfterCWRotation( m_moveX, m_moveY );
	}
	else
	{
		updatePositionAfterCWRotation( m_moveX, m_moveY );
		updatePositionAfterCWRotation( m_moveX, m_moveY );
		updatePositionAfterCWRotation( m_moveX, m_moveY );
	}
	onRenderParamsChanged();
}

/// @brief Sets the camera position to the given absolute world-space offsets. Used when
///        restoring a saved view on load so the renderer's leftover m_moveX/m_moveY from
///        the previous game doesn't compound the result.
void MainWindowRenderer::setMove( float x, float y )
{
	if ( !Global::dimX )
		return;
	m_moveX = x;
	m_moveY = y;
	GameState::moveX = m_moveX;
	GameState::moveY = m_moveY;
	onRenderParamsChanged();
}

/// @brief Pans the camera by (x, y) screen pixels.
/// @param x X delta in pixels.
/// @param y Y delta in pixels.
void MainWindowRenderer::move( float x, float y )
{
	if ( !Global::dimX )
		return;

	m_moveX += x / m_scale;
	m_moveY += y / m_scale;

	const auto centerY = -Global::dimX * 8.f;
	const auto centerX = 0;

	float oldX, oldY;
	do
	{
		oldX   = m_moveX;
		oldY   = m_moveY;
		const auto rangeY = Global::dimX * 8.f - abs( m_moveX - centerX ) / 2.f;
		const auto rangeX = Global::dimX * 16.f - abs( m_moveY - centerY ) * 2.f;
		m_moveX           = qBound( centerX - rangeX, m_moveX, centerX + rangeX );
		m_moveY           = qBound( centerY - rangeY, m_moveY, centerY + rangeY );
	} while ( oldX != m_moveX || oldY != m_moveY );

	GameState::moveX = m_moveX;
	GameState::moveY = m_moveY;

	onRenderParamsChanged();
}

/// @brief Multiplies the current zoom factor by @p factor (mouse wheel zoom).
/// @param factor Zoom multiplier.
void MainWindowRenderer::scale( float factor )
{
	m_scale *= factor;
	m_scale = qBound( 0.25f, m_scale, 15.f );
	GameState::scale = m_scale;
	onRenderParamsChanged();
}

/// @brief Sets the zoom factor to an absolute value.
/// @param scale New zoom factor.
void MainWindowRenderer::setScale( float scale )
{
	m_scale = qBound( 0.25f, scale, 15.f );
	onRenderParamsChanged();
}

/// @brief Sets the top z-level being displayed.
/// @param level New view level (z coordinate).
void MainWindowRenderer::setViewLevel( int level )
{
	m_viewLevel = level;
	onRenderParamsChanged();
}

/// @brief Uploads any queued tile updates to m_tileUpdateBo and runs the compute shader
///        to apply them into m_tileBo.
void MainWindowRenderer::updateWorld()
{
	if ( !m_pendingUpdates.empty() )
	{
		DebugScope s( "update world" );
		for ( const auto& update : m_pendingUpdates )
		{
			uploadTileData( update );
		}
		m_pendingUpdates.clear();
	}
}

/// @brief Uploads one batch of TileDataUpdate packets to m_tileUpdateBo ready for the
///        compute shader to consume.
/// @param tileData Batch of tile-update records.
void MainWindowRenderer::uploadTileData( const QVector<TileDataUpdate>& tileData )
{
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, m_tileUpdateBo );
	glBufferData( GL_SHADER_STORAGE_BUFFER, sizeof( TileDataUpdate ) * tileData.size(), tileData.data(), GL_STREAM_DRAW );
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 );

	glUseProgram( m_worldUpdateShader );
	setUniformi( m_worldUpdateShader, "uUpdateSize", (GLint)tileData.size() );
	glDispatchCompute( ( tileData.size() + 63 ) / 64, 1, 1 );
	glUseProgram( 0 );
}

/// @brief Re-uploads sprite atlas slices from SpriteFactory when new sprites have been
///        created (signalled via SpriteFactory::textureAdded/creatureTextureAdded).
void MainWindowRenderer::updateTextures()
{
	if ( Global::eventConnector->game()->sf()->textureAdded() || Global::eventConnector->game()->sf()->creatureTextureAdded() )
	{
		DebugScope s( "update textures" );

		m_texesUsed = Global::eventConnector->game()->sf()->texesUsed();

		int maxArrayTextures = Global::cfg->get( "MaxArrayTextures" ).toInt();

		for ( int i = 0; i < m_texesUsed; ++i )
		{
			uploadArrayTexture( i, maxArrayTextures, Global::eventConnector->game()->sf()->pixelData( i ).constData() );
		}
	}
}

/// @brief Slot: caches the latest placement cursor data from AggregatorSelection.
/// @param data        New per-tile SelectionData map.
/// @param noDepthTest True to draw the cursor with depth test disabled.
void MainWindowRenderer::onUpdateSelection( const QMap<unsigned int, SelectionData>& data, bool noDepthTest )
{
	m_selectionData.clear();
	for( const auto& key : data.keys() )
	{
		m_selectionData.insert( key, data[key] );
	}
	m_selectionNoDepthTest = noDepthTest;
}

/// @brief Slot: centres the camera on the given world position (used by "jump to gnome"
///        and event-triggered camera moves).
/// @param target World position to centre on.
void MainWindowRenderer::onCenterCameraPosition( const Position& target )
{
	m_moveX     = 16 * (-target.x + target.y);
	m_moveY     = 8 * ( -target.x - target.y );
	m_viewLevel = target.z;
	onRenderParamsChanged();
	// Notify aggregators (selection, sound, …) that the camera moved — otherwise
	// the cursor-tile mapping in AggregatorSelection stays at its zero defaults
	// until the player nudges the wheel or resizes the window.
	if ( m_parent )
	{
		m_parent->pushRenderParams();
	}
}

/// @brief Helper used by rotate() to adjust camera offsets after a 90° clockwise rotation
///        so the centre point stays in place.
/// @param x In/out X offset.
/// @param y In/out Y offset.
void MainWindowRenderer::updatePositionAfterCWRotation( float& x, float& y )
{
	constexpr int tileHeight = 8; //tiles are assumed to be 8 pixels high (and twice as wide)
	int tmp = x;
	x = -2 * ( Global::dimX * tileHeight + y );
	y = -( Global::dimY - 2 ) * tileHeight + tmp/2;
}

/// @brief Slot: sets the in-menu flag. When true, the renderer runs at menu tick rate (16 ms)
///        instead of the uncapped gameplay loop.
/// @param value True while the main menu is active.
void MainWindowRenderer::onSetInMenu( bool value )
{
	m_inMenu = value;
}
