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
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QTimer>

namespace
{
class DebugScope
{
public:
	DebugScope() = delete;
	DebugScope( const char* c )
	{
		static GLuint counter    = 0;
		QOpenGLExtraFunctions* f = QOpenGLContext::currentContext()->extraFunctions();
		f->glPushDebugGroup( GL_DEBUG_SOURCE_APPLICATION, counter++, static_cast<GLsizei>( strlen( c ) ), c );
	}
	~DebugScope()
	{
		QOpenGLExtraFunctions* f = QOpenGLContext::currentContext()->extraFunctions();
		f->glPopDebugGroup();
	}
};
} // namespace

MainWindowRenderer::MainWindowRenderer( MainWindow* parent ) :
	QObject( parent ),
	m_parent( parent )
{
	connect( Global::eventConnector->aggregatorRenderer(), &AggregatorRenderer::signalWorldParametersChanged, this, &MainWindowRenderer::cleanupWorld );

	connect( Global::eventConnector->aggregatorRenderer(), &AggregatorRenderer::signalTileUpdates, this, &MainWindowRenderer::onTileUpdates );
	connect( Global::eventConnector->aggregatorRenderer(), &AggregatorRenderer::signalAxleData, this, &MainWindowRenderer::onAxelData );
	connect( Global::eventConnector->aggregatorRenderer(), &AggregatorRenderer::signalThoughtBubbles, this, &MainWindowRenderer::onThoughtBubbles );
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

	if ( !initializeOpenGLFunctions() )
	{
		qDebug() << "failed to initialize OpenGL - make sure your graphics card and driver support OpenGL 4.3";
		qCritical() << "failed to initialize OpenGL functions core 4.3 - exiting";
		QMessageBox msgBox;
		msgBox.setText( "Failed to initialize OpenGL - make sure your graphics card and driver support OpenGL 4.3" );
		msgBox.exec();
		exit( 0 );
	}

	qDebug() << "[OpenGL]" << reinterpret_cast<char const*>( glGetString( GL_VENDOR ) );
	qDebug() << "[OpenGL]" << reinterpret_cast<char const*>( glGetString( GL_VERSION ) );
	qDebug() << "[OpenGL]" << reinterpret_cast<char const*>( glGetString( GL_RENDERER ) );

	qDebug() << m_parent->context()->format();

	QOpenGLExtraFunctions* f = QOpenGLContext::currentContext()->extraFunctions();
	GLDEBUGPROC logHandler   = []( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam ) -> void
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
		qDebug() << "[OpenGL]" << debugTypes.at( type ) << " " << severities.at(severity) << ":" << message;
	};
	glEnable( GL_DEBUG_OUTPUT );
	f->glDebugMessageCallback( logHandler, nullptr );
}
MainWindowRenderer ::~MainWindowRenderer()
{
}

void MainWindowRenderer::initializeGL()
{
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

	m_vao.create();
	QOpenGLVertexArrayObject::Binder vaoBinder( &m_vao );

	glGenBuffers( 1, &m_vbo );
	glBindBuffer( GL_ARRAY_BUFFER, m_vbo );
	glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_STATIC_DRAW );

	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( float ), (void*)0 );
	glEnableVertexAttribArray( 0 );

	glGenBuffers( 1, &m_vibo );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_vibo );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ), indices, GL_STATIC_DRAW );

	updateRenderParams();

	qDebug() << "initialize GL - done";
}

void MainWindowRenderer::reloadShaders()
{
	m_reloadShaders = true;
}

void MainWindowRenderer::cleanup()
{
	m_parent->makeCurrent();
	m_worldShader.reset();
	m_worldUpdateShader.reset();
	m_thoughtBubbleShader.reset();
	m_selectionShader.reset();
	m_axleShader.reset();

	glDeleteBuffers( 1, &m_vbo );
	glDeleteBuffers( 1, &m_vibo );

	m_parent->doneCurrent();

	m_reloadShaders = true;
}

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

void MainWindowRenderer::onTileUpdates( const TileDataUpdateInfo& updates )
{
	m_pendingUpdates.push_back( updates.updates );
	emit redrawRequired();
}

void MainWindowRenderer::onThoughtBubbles( const ThoughtBubbleInfo& bubbles )
{
	m_thoughBubbles = bubbles;
	emit redrawRequired();
}

void MainWindowRenderer::onAxelData( const AxleDataInfo& data )
{
	m_axleData = data;
	emit redrawRequired();
}

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

QOpenGLShaderProgram* MainWindowRenderer::initShader( QString name )
{
	QString vs = copyShaderToString( name + "_v" );
	QString fs = copyShaderToString( name + "_f" );

	QScopedPointer<QOpenGLShaderProgram> shader(new QOpenGLShaderProgram);

	bool ok = true;
	ok &= shader->addShaderFromSourceCode( QOpenGLShader::Vertex, vs );
	ok &= shader->addShaderFromSourceCode( QOpenGLShader::Fragment, fs );
	if ( !ok )
	{
		qCritical() << "failed to add shader source code" << name;
		return nullptr;
	}

	ok &= shader->link();

	if ( !ok )
	{
		qCritical() << "failed to link shader" << name;
		return nullptr;
	}

	ok &= shader->bind();
	if ( !ok )
	{
		qCritical() << "failed to bind shader" << name;
		return nullptr;
	}

	shader->release();

	return shader.take();
}

QOpenGLShaderProgram* MainWindowRenderer::initComputeShader( QString name )
{
	QString cs = copyShaderToString( name + "_c" );

	QScopedPointer<QOpenGLShaderProgram> shader( new QOpenGLShaderProgram );

	bool ok = true;
	ok &= shader->addShaderFromSourceCode( QOpenGLShader::Compute, cs );
	if ( !ok )
	{
		qCritical() << "failed to add shader source code";
		return nullptr;
	}

	ok &= shader->link();

	if ( !ok )
	{
		qCritical() << "failed to link shader";
		return nullptr;
	}

	ok &= shader->bind();
	if ( !ok )
	{
		qCritical() << "failed to bind shader";
		return nullptr;
	}

	shader->release();

	return shader.take();
}

bool MainWindowRenderer::initShaders()
{
	m_worldShader.reset(initShader( "world" ));
	m_worldUpdateShader.reset(initComputeShader( "worldupdate" ));
	m_thoughtBubbleShader.reset(initShader( "thoughtbubble" ));
	m_selectionShader.reset(initShader( "selection" ));
	m_axleShader.reset(initShader( "axle" ));

	if ( !m_worldShader || !m_worldUpdateShader || !m_thoughtBubbleShader || !m_selectionShader || !m_axleShader )
	{
		// Can't proceed, and need to know what happened!
		abort();
		return false;
	}

	m_reloadShaders = false;

	return true;
}

void MainWindowRenderer::createArrayTexture( int unit, int depth )
{
	//GLint max_layers;
	//glGetIntegerv ( GL_MAX_ARRAY_TEXTURE_LAYERS, &max_layers );

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

	/*
	QString msg = "Move: " + QString::number( m_moveX ) + ", " + QString::number( m_moveY ) + " z-Level: " + QString::number( m_viewLevel ); 
	qDebug() << msg;
	//emit sendOverlayMessage( 5, msg );
	msg = "Window size: " + QString::number( m_width ) + "x" + QString::number( m_height ) + " Scale " + QString::number( m_scale ) + " Rotation " + QString::number( m_rotation );
	qDebug() << msg;
	//emit sendOverlayMessage( 4, msg );
	*/
}

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

		QOpenGLVertexArrayObject::Binder vaoBinder( &m_vao );

		paintTiles();

		paintSelection();

		paintThoughtBubbles();

		if ( Global::showAxles )
		{
			paintAxles();
		}
	}

	//glFinish();

	bool pause = Global::cfg->get( "Pause" ).toBool();

	if ( pause != m_pause )
	{
		m_pause = pause;
	}
}

void MainWindowRenderer::onRenderParamsChanged()
{
	updateRenderParams();
	emit redrawRequired();
	emit signalCameraPosition(m_moveX, m_moveY, m_viewLevel, m_rotation, m_scale);
	
}

void MainWindowRenderer::setCommonUniforms( QOpenGLShaderProgram* shader )
{
	auto indexTotal = shader->uniformLocation( "uWorldSize" );
	if ( indexTotal >= 0 )
	{
		glUniform3ui( indexTotal, Global::dimX, Global::dimY, Global::dimZ );
	}
	auto indexMin = shader->uniformLocation( "uRenderMin" );
	if ( indexMin >= 0 )
	{
		glUniform3ui( indexMin, m_volume.min.x, m_volume.min.y, m_volume.min.z );
	}
	auto indexMax = shader->uniformLocation( "uRenderMax" );
	if ( indexMax >= 0 )
	{
		glUniform3ui( indexMax, m_volume.max.x, m_volume.max.y, m_volume.max.z );
	}
	shader->setUniformValue( "uTransform", m_projectionMatrix );
	shader->setUniformValue( "uWorldRotation", (GLuint)m_rotation );
	shader->setUniformValue( "uTickNumber", (GLuint)GameState::tick );
}

void MainWindowRenderer::paintTiles()
{
	DebugScope s( "paint tiles" );

	m_worldShader->bind();
	setCommonUniforms( m_worldShader.get() );

	for ( int i = 0; i < m_texesUsed; ++i )
	{
		auto texNum = "uTexture[" + QString::number( i ) + "]";
		m_worldShader->setUniformValue( texNum.toStdString().c_str(), i );
	}

	//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	m_worldShader->setUniformValue( "uOverlay", Global::showDesignations );
	m_worldShader->setUniformValue( "uShowJobs", Global::showJobs );
	m_worldShader->setUniformValue( "uDebug", m_debug );
	m_worldShader->setUniformValue( "uWallsLowered", Global::wallsLowered );

	m_worldShader->setUniformValue( "uUndiscoveredTex", Global::undiscoveredUID * 4 );
	m_worldShader->setUniformValue( "uWaterTex", Global::waterSpriteUID * 4 );

	if ( GameState::daylight )
	{
		m_daylight = qMin( 1.0, m_daylight + 0.025 );
	}
	else
	{
		m_daylight = qMax( 0.0, m_daylight - 0.025 );
	}
	m_worldShader->setUniformValue( "uDaylight", m_daylight );
	m_worldShader->setUniformValue( "uLightMin", m_lightMin );
	//m_worldShader->setUniformValue( "uDaylight", 1.0f );

	auto volume   = m_volume.size();
	GLsizei tiles = volume.x * volume.y * volume.z;

	// First pass is pure front-to-back of opaque blocks, alpha doesn't even work
	glDisable( GL_BLEND );
	glDepthMask( true );

	m_worldShader->setUniformValue( "uPaintFrontToBack", true );
	glDrawElementsInstanced( GL_TRIANGLES, 12, GL_UNSIGNED_SHORT, 0, tiles );

	//!TODO Transparency pass is too early, all the stuff rendered later is still missing
	// Second pass includes transparency
	m_worldShader->setUniformValue( "uPaintFrontToBack", false );
	glEnable( GL_BLEND );
	glDrawElementsInstanced( GL_TRIANGLES, 12, GL_UNSIGNED_SHORT, (void*)( sizeof( GLushort ) * 6 ), tiles );

	// All done with depth writes, everything beyond is layered
	glDepthMask( false );

	m_worldShader->release();
}

void MainWindowRenderer::paintSelection()
{
	// TODO this is a workaround until some transparency solution is implemented
	if( m_selectionNoDepthTest )
	{
		glDisable( GL_DEPTH_TEST );
	}
	DebugScope s( "paint selection" );
	m_selectionShader->bind();
	setCommonUniforms( m_selectionShader.get() );

	for ( int i = 0; i < m_texesUsed; ++i )
	{
		auto texNum = "uTexture[" + QString::number( i ) + "]";
		m_selectionShader->setUniformValue( texNum.toStdString().c_str(), i );
	}

	for ( const auto& sd : m_selectionData )
	{
		GLint tile = m_axleShader->uniformLocation( "tile" );
		glUniform3ui( tile, sd.pos.x, sd.pos.y, sd.pos.z );
		m_selectionShader->setUniformValue( "uSpriteID", sd.spriteID );
		m_selectionShader->setUniformValue( "uRotation", sd.localRot );
		m_selectionShader->setUniformValue( "uValid", sd.valid );

		glDrawArraysInstancedBaseInstance( GL_TRIANGLE_STRIP, 0, 4, 1, 0 );
	}

	m_selectionShader->release();
	if( m_selectionNoDepthTest )
	{
		glEnable( GL_DEPTH_TEST );
	}
}

void MainWindowRenderer::paintThoughtBubbles()
{
	DebugScope s( "paint thoughts" );

	m_thoughtBubbleShader->bind();
	setCommonUniforms( m_thoughtBubbleShader.get() );

	m_thoughtBubbleShader->setUniformValue( "uTexture0", 0 );

	for ( const auto& thoughtBubble : m_thoughBubbles.thoughtBubbles )
	{
		if ( thoughtBubble.pos.z <= m_viewLevel )
		{
			GLint tile = m_axleShader->uniformLocation( "tile" );
			glUniform3ui( tile, thoughtBubble.pos.x, thoughtBubble.pos.y, thoughtBubble.pos.z );
			m_thoughtBubbleShader->setUniformValue( "uType", thoughtBubble.sprite );
			glDrawArraysInstancedBaseInstance( GL_TRIANGLE_STRIP, 0, 4, 1, 0 );
		}
	}

	m_thoughtBubbleShader->release();
}

void MainWindowRenderer::paintAxles()
{
	DebugScope s( "paint axles" );

	m_axleShader->bind();
	setCommonUniforms( m_axleShader.get() );
	m_axleShader->setUniformValue( "uTickNumber", (unsigned int)GameState::tick );

	for ( int i = 0; i < m_texesUsed; ++i )
	{
		auto texNum = "uTexture[" + QString::number( i ) + "]";
		m_axleShader->setUniformValue( texNum.toStdString().c_str(), i );
	}

	for ( const auto& ad : m_axleData.data )
	{
		if ( !ad.isVertical && ad.pos.z <= m_viewLevel )
		{
			GLint tile = m_axleShader->uniformLocation( "tile" );
			glUniform3ui( tile, ad.pos.x, ad.pos.y, ad.pos.z );
			m_axleShader->setUniformValue( "uSpriteID", ad.spriteID );
			m_axleShader->setUniformValue( "uRotation", ad.localRot );
			m_axleShader->setUniformValue( "uAnim", ad.anim );

			glDrawArraysInstancedBaseInstance( GL_TRIANGLE_STRIP, 0, 4, 1, 0 );
		}
	}

	m_axleShader->release();
}

void MainWindowRenderer::resize( int w, int h )
{
	m_width  = w;
	m_height = h;
	onRenderParamsChanged();
}

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

void MainWindowRenderer::scale( float factor )
{
	m_scale *= factor;
	m_scale = qBound( 0.25f, m_scale, 15.f );
	GameState::scale = m_scale;
	onRenderParamsChanged();
}

void MainWindowRenderer::setScale( float scale )
{
	m_scale = qBound( 0.25f, scale, 15.f );
	onRenderParamsChanged();
}

void MainWindowRenderer::setViewLevel( int level )
{
	m_viewLevel = level;
	onRenderParamsChanged();
}

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

void MainWindowRenderer::uploadTileData( const QVector<TileDataUpdate>& tileData )
{
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, m_tileUpdateBo );
	glBufferData( GL_SHADER_STORAGE_BUFFER, sizeof( TileDataUpdate ) * tileData.size(), tileData.data(), GL_STREAM_DRAW );
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 );

	m_worldUpdateShader->bind();
	m_worldUpdateShader->setUniformValue( "uUpdateSize", (GLint)tileData.size() );
	glDispatchCompute( ( tileData.size() + 63 ) / 64, 1, 1 );
	m_worldUpdateShader->release();
}

void MainWindowRenderer::updateTextures()
{
	if ( Global::eventConnector->game()->sf()->textureAdded() || Global::eventConnector->game()->sf()->creatureTextureAdded() )
	{
		DebugScope s( "update textures" );

		m_texesUsed = Global::eventConnector->game()->sf()->texesUsed();

		int maxArrayTextures = Global::cfg->get( "MaxArrayTextures" ).toInt();

		for ( int i = 0; i < m_texesUsed; ++i )
		{
			uploadArrayTexture( i, maxArrayTextures, Global::eventConnector->game()->sf()->pixelData( i ).cbegin() );
		}
	}
}

void MainWindowRenderer::onUpdateSelection( const QMap<unsigned int, SelectionData>& data, bool noDepthTest )
{
	m_selectionData.clear();
	for( const auto& key : data.keys() )
	{
		m_selectionData.insert( key, data[key] );
	}
	m_selectionNoDepthTest = noDepthTest;
}

unsigned int MainWindowRenderer::posToInt( Position pos, quint8 rotation )
{
	//return x + Global::dimX * y + Global::dimX * Global::dimX * z;

	switch ( rotation )
	{
		case 0:
			return pos.toInt();
			break;
		case 1:
			return pos.x + Global::dimX * ( Global::dimX - pos.y ) + Global::dimX * Global::dimX * pos.z;
			break;
		case 2:
			return ( Global::dimX - pos.x ) + Global::dimX * ( Global::dimX - pos.y ) + Global::dimX * Global::dimX * pos.z;
			break;
		case 3:
			return ( Global::dimX - pos.x ) + Global::dimX * pos.y + Global::dimX * Global::dimX * pos.z;
			break;
	}
	return 0;
}

void MainWindowRenderer::updatePositionAfterCWRotation( float& x, float& y )
{
	constexpr int tileHeight = 8; //tiles are assumed to be 8 pixels high (and twice as wide)
	int tmp = x;
	x = -2 * ( Global::dimX * tileHeight + y );
	//y = -Global::dimY * tileHeight + tmp/2;
	y = -( Global::dimY - 2 ) * tileHeight + tmp/2;
}

void MainWindowRenderer::onSetInMenu( bool value )
{
	m_inMenu = value;
}