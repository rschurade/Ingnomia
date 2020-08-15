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

#include "../base/config.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/selection.h"
#include "../base/util.h"
#include "../base/vptr.h"
#include "../game/gamemanager.h"
#include "../game/plant.h"
#include "../game/world.h"
#include "../gfx/sprite.h"
#include "../gfx/spritefactory.h"
#include "eventconnector.h"
#include "mainwindow.h"

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
	connect( EventConnector::getInstance().aggregatorRenderer(), &AggregatorRenderer::signalWorldParametersChanged, this, &MainWindowRenderer::cleanupWorld );

	connect( EventConnector::getInstance().aggregatorRenderer(), &AggregatorRenderer::signalTileUpdates, this, &MainWindowRenderer::onTileUpdates );
	connect( EventConnector::getInstance().aggregatorRenderer(), &AggregatorRenderer::signalAxleData, this, &MainWindowRenderer::onAxelData );
	connect( EventConnector::getInstance().aggregatorRenderer(), &AggregatorRenderer::signalThoughtBubbles, this, &MainWindowRenderer::onThoughtBubbles );

	// Full polling of initial state on load
	connect( this, &MainWindowRenderer::fullDataRequired, EventConnector::getInstance().aggregatorRenderer(), &AggregatorRenderer::onAllTileInfo );
	connect( this, &MainWindowRenderer::fullDataRequired, EventConnector::getInstance().aggregatorRenderer(), &AggregatorRenderer::onThoughtBubbleUpdate );
	connect( this, &MainWindowRenderer::fullDataRequired, EventConnector::getInstance().aggregatorRenderer(), &AggregatorRenderer::onAxleDataUpdate );
}
MainWindowRenderer ::~MainWindowRenderer()
{
}

void MainWindowRenderer::initializeGL()
{
	qDebug() << "initialize GL ...";
	connect( m_parent->context(), &QOpenGLContext::aboutToBeDestroyed, this, &MainWindowRenderer::cleanup );
	connect( this, &MainWindowRenderer::redrawRequired, m_parent, &MainWindow::redraw );

	if ( !initializeOpenGLFunctions() )
	{
		QMessageBox msgBox;
		msgBox.setText( "Failed to initialize OpenGL - make sure your graphics card and driver support OpenGL 4.2" );
		msgBox.exec();
		qDebug() << "failed to initialize OpenGL - make sure your graphics card and driver support OpenGL 4.2";
		qCritical() << "failed to initialize OpenGL functions core 4.2 - exiting";
		exit( 0 );
	}

	std::string glVendor   = reinterpret_cast<char const*>( glGetString( GL_VENDOR ) );
	std::string glVersion  = reinterpret_cast<char const*>( glGetString( GL_VERSION ) );
	std::string glRenderer = reinterpret_cast<char const*>( glGetString( GL_RENDERER ) );

	qDebug() << glVendor.c_str();
	qDebug() << glRenderer.c_str();
	qDebug() << glVersion.c_str();

	qDebug() << m_parent->context()->format();

	float vertices[] = {
		// Wall layer
		0.f, 1.f, 1.f, // top left
		0.f, .2f, 1.f, // bottom left
		1.f, 1.f, 1.f, // top right
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
	if ( m_worldShader == nullptr )
	{
		return;
	}
	m_parent->makeCurrent();
	delete m_worldShader;
	m_worldShader = nullptr;
	delete m_worldUpdateShader;
	m_worldUpdateShader = nullptr;
	delete m_thoughtBubbleShader;
	m_thoughtBubbleShader = nullptr;
	delete m_selectionShader;
	m_selectionShader = nullptr;
	delete m_axleShader;
	m_axleShader = nullptr;

	glDeleteBuffers( 1, &m_vbo );
	glDeleteBuffers( 1, &m_vibo );

	m_parent->doneCurrent();

	m_reloadShaders = true;
}

void MainWindowRenderer::cleanupWorld()
{
	m_parent->makeCurrent();
	glDeleteTextures( 8, m_textures );
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
	QFile file( Config::getInstance().get( "dataPath" ).toString() + "/shaders/" + name + ".glsl" );
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

	QOpenGLShaderProgram* shader = new QOpenGLShaderProgram;

	bool ok = true;
	ok &= shader->addShaderFromSourceCode( QOpenGLShader::Vertex, vs );
	ok &= shader->addShaderFromSourceCode( QOpenGLShader::Fragment, fs );
	if ( !ok )
	{
		qDebug() << "failed to add shader source code";
		return nullptr;
	}

	ok &= shader->link();

	if ( !ok )
	{
		qDebug() << "failed to link shader";
		return nullptr;
	}

	ok &= shader->bind();
	if ( !ok )
	{
		qDebug() << "failed to bind shader";
		return nullptr;
	}

	shader->release();

	return shader;
}

QOpenGLShaderProgram* MainWindowRenderer::initComputeShader( QString name )
{
	QString cs = copyShaderToString( name + "_c" );

	QOpenGLShaderProgram* shader = new QOpenGLShaderProgram;

	bool ok = true;
	ok &= shader->addShaderFromSourceCode( QOpenGLShader::Compute, cs );
	if ( !ok )
	{
		qDebug() << "failed to add shader source code";
		return nullptr;
	}

	ok &= shader->link();

	if ( !ok )
	{
		qDebug() << "failed to link shader";
		return nullptr;
	}

	ok &= shader->bind();
	if ( !ok )
	{
		qDebug() << "failed to bind shader";
		return nullptr;
	}

	shader->release();

	return shader;
}

bool MainWindowRenderer::initShaders()
{
	m_worldShader = initShader( "world" );
	if ( !m_worldShader )
	{
		return false;
	}
	m_worldUpdateShader = initComputeShader( "worldupdate" );
	if ( !m_worldUpdateShader )
	{
		return false;
	}
	m_thoughtBubbleShader = initShader( "thoughtbubble" );
	if ( !m_thoughtBubbleShader )
	{
		return false;
	}
	m_selectionShader = initShader( "selection" );
	if ( !m_selectionShader )
	{
		return false;
	}
	m_axleShader = initShader( "axle" );
	if ( !m_axleShader )
	{
		return false;
	}

	m_reloadShaders = false;

	return true;
}

void MainWindowRenderer::createArrayTexture( int unit, int depth, const QVector<uint8_t>& data )
{
	//GLint max_layers;
	//glGetIntegerv ( GL_MAX_ARRAY_TEXTURE_LAYERS, &max_layers );

	glActiveTexture( GL_TEXTURE0 + unit );
	glGenTextures( 1, &m_textures[unit] );
	glBindTexture( GL_TEXTURE_2D_ARRAY, m_textures[unit] );
	glTexImage3D( GL_TEXTURE_2D_ARRAY,
				  0,                // mipmap level
				  GL_RGBA8,         // gpu texel format
				  32,               // width
				  64,               // height
				  depth,            // depth
				  0,                // border
				  GL_RGBA,          // cpu pixel format
				  GL_UNSIGNED_BYTE, // cpu pixel coord type
				  &data[0] );       // pixel data

	//glGenerateMipmap( GL_TEXTURE_2D_ARRAY );
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	//glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 4 );
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
}

void MainWindowRenderer::initTextures()
{
	GLint max_layers;
	glGetIntegerv( GL_MAX_ARRAY_TEXTURE_LAYERS, &max_layers );

	SpriteFactory& sf = Global::sf();

	qDebug() << "max array size: " << max_layers;
	qDebug() << "used " << sf.size() << " sprites";

	m_texesUsed = sf.texesUsed();

	int maxArrayTextures = Config::getInstance().get( "MaxArrayTextures" ).toInt();

	for ( int i = 0; i < m_texesUsed; ++i )
	{
		createArrayTexture( i, maxArrayTextures, sf.pixelData( i ) );
	}

	//qDebug() << "stored " << maxArrayTextures << " pixmaps in array texture";

	m_texesInitialized = true;
}

void MainWindowRenderer::initWorld()
{
	QElapsedTimer timer;
	timer.start();

	int dim  = Global::dimX;
	int dim2 = dim * dim;
	int dimZ = Global::dimZ;

	glGenBuffers( 1, &m_tileBo );
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, m_tileBo );
	glBufferData( GL_SHADER_STORAGE_BUFFER, TD_SIZE * sizeof( unsigned int ) * Global::w().world().size(), nullptr, GL_DYNAMIC_DRAW );
	const uint8_t zero = 0;
	glClearBufferData( GL_SHADER_STORAGE_BUFFER, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &zero );
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 ); // unbind

	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 0, m_tileBo );

	glGenBuffers( 1, &m_tileUpdateBo );

	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 0, m_tileBo );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1, m_tileUpdateBo );

	m_texesInitialized = true;

	emit fullDataRequired();
}

void MainWindowRenderer::updateRenderParams()
{
	m_renderSize = qMin( Global::dimX, (int)( ( sqrt( m_width * m_width + m_height * m_height ) / 12 ) / m_scale ) );

	m_renderDepth = Config::getInstance().get( "renderDepth" ).toInt();

	m_waterQuality = Config::getInstance().get( "waterQuality" ).toInt();

	m_viewLevel = Config::getInstance().get( "viewLevel" ).toInt();

	m_volume.min = { 0, 0, qMin( qMax( m_viewLevel - m_renderDepth, 0 ), Global::dimZ - 1 ) };
	m_volume.max = { Global::dimX - 1, Global::dimY - 1, qMin( m_viewLevel, Global::dimZ - 1 ) };

	m_lightMin = Config::getInstance().get( "lightMin" ).toFloat();
	if ( m_lightMin < 0.01 )
		m_lightMin = 0.3f;

	m_rotation = Config::getInstance().get( "rotation" ).toInt();

	m_overlay      = Config::getInstance().get( "overlay" ).toBool();
	m_debug        = Global::debugMode;
	m_debugOverlay = false; // Config::getInstance().get( "debugOverlay" ).toBool();

	m_renderDown = Config::getInstance().get( "renderMode" ).toString() == "down";

	m_projectionMatrix.setToIdentity();
	m_projectionMatrix.ortho( -m_width / 2, m_width / 2, -m_height / 2, m_height / 2, -( m_volume.max.x + m_volume.max.y + m_volume.max.z + 1 ), -m_volume.min.z );
	m_projectionMatrix.scale( m_scale, m_scale );
	m_projectionMatrix.translate( m_moveX, -m_moveY );

	m_paintCreatures = Config::getInstance().get( "renderCreatures" ).toBool();
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

	if ( GameManager::getInstance().showMainMenu() )
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
	updateSelection();

	timer.start();
	updateTextures();

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

		if ( m_paintCreatures )
		{
			paintThoughtBubbles();
		}

		if ( Global::showAxles )
		{
			paintAxles();
		}
	}

	//glFinish();

	bool pause = Config::getInstance().get( "Pause" ).toBool();

	if ( pause != m_pause )
	{
		m_pause = pause;
	}
}

void MainWindowRenderer::onRenderParamsChanged()
{
	updateRenderParams();
	emit redrawRequired();
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
	setCommonUniforms( m_worldShader );

	for ( int i = 0; i < m_texesUsed; ++i )
	{
		auto texNum = "uTexture[" + QString::number( i ) + "]";
		m_worldShader->setUniformValue( texNum.toStdString().c_str(), i );
	}

	//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	m_worldShader->setUniformValue( "uOverlay", m_overlay );
	m_worldShader->setUniformValue( "uDebug", m_debug );
	m_worldShader->setUniformValue( "uDebugOverlay", m_debugOverlay );
	m_worldShader->setUniformValue( "uWallsLowered", Global::wallsLowered );

	m_worldShader->setUniformValue( "uUndiscoveredTex", Global::undiscoveredUID * 4 );
	m_worldShader->setUniformValue( "uWaterTex", Global::waterSpriteUID * 4 );

	m_worldShader->setUniformValue( "uPaintCreatures", m_paintCreatures );

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
	m_worldShader->setUniformValue( "uPaintSolid", true );
	m_worldShader->setUniformValue( "uPaintWater", m_waterQuality == 0 );

	glDrawElementsInstanced( GL_TRIANGLES, 12, GL_UNSIGNED_SHORT, 0, tiles );

	// All done with depth writes, everything beyond is layered
	glEnable( GL_BLEND );
	glDepthMask( false );

	if ( m_waterQuality >= 1 )
	{
		m_worldShader->setUniformValue( "uPaintFrontToBack", false );
		m_worldShader->setUniformValue( "uPaintSolid", false );
		m_worldShader->setUniformValue( "uPaintCreatures", false );
		m_worldShader->setUniformValue( "uPaintWater", true );
		glDrawElementsInstanced( GL_TRIANGLES, 12, GL_UNSIGNED_SHORT, (void*)( sizeof( GLushort ) * 6 ), tiles );
	}

	m_worldShader->release();
}

void MainWindowRenderer::paintSelection()
{
	DebugScope s( "paint selection" );
	m_selectionShader->bind();
	setCommonUniforms( m_selectionShader );

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
}

void MainWindowRenderer::paintThoughtBubbles()
{
	DebugScope s( "paint thoughts" );

	m_thoughtBubbleShader->bind();
	setCommonUniforms( m_thoughtBubbleShader );

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
	setCommonUniforms( m_axleShader );
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
	Config::getInstance().set( "rotation", m_rotation );
	onRenderParamsChanged();
}

void MainWindowRenderer::move( int x, int y )
{
	m_moveX += x / m_scale;
	m_moveY += y / m_scale;
	onRenderParamsChanged();
}

void MainWindowRenderer::scale( float factor )
{
	m_scale *= factor;
	m_scale = qBound( 0.25f, m_scale, 15.f );
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
		glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
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
	SpriteFactory& sf = Global::sf();

	if ( Global::sf().textureAdded() || Global::sf().creatureTextureAdded() )
	{
		DebugScope s( "update textures" );

		glDeleteTextures( m_texesUsed, &m_textures[0] );

		m_texesUsed = sf.texesUsed();

		int maxArrayTextures = Config::getInstance().get( "MaxArrayTextures" ).toInt();

		for ( int i = 0; i < m_texesUsed; ++i )
		{
			createArrayTexture( i, maxArrayTextures, sf.pixelData( i ) );
		}
	}
}

void MainWindowRenderer::updateSelection()
{
	if ( Selection::getInstance().changed() )
	{
		DebugScope s( "update selection" );

		SpriteFactory& sf = Global::sf();

		QString action = Selection::getInstance().action();
		m_selectionData.clear();
		if ( !action.isEmpty() )
		{
			bool isFloor = Selection::getInstance().isFloor();

			QList<QPair<Position, bool>> selection = Selection::getInstance().getSelection();

			QList<QPair<Sprite*, QPair<Position, unsigned char>>> sprites;
			QList<QPair<Sprite*, QPair<Position, unsigned char>>> spritesInv;

			int rotation = Selection::getInstance().rotation();

			QList<QVariantMap> spriteIDs;

			if ( action == "BuildWall" || action == "BuildFancyWall" || action == "BuildFloor" || action == "BuildFancyFloor" || action == "BuildRamp" || action == "BuildRampCorner" || action == "BuildStairs" )
			{
				spriteIDs = DB::selectRows( "Constructions_Sprites", "ID", Selection::getInstance().itemID() );
			}
			else if ( action == "BuildWorkshop" )
			{
				spriteIDs = DB::selectRows( "Workshops_Components", "ID", Selection::getInstance().itemID() );
			}
			else if ( action == "BuildItem" )
			{
				QVariantMap sprite;
				sprite.insert( "SpriteID", DBH::spriteID( Selection::getInstance().itemID() ) );
				sprite.insert( "Offset", "0 0 0" );
				sprite.insert( "Type", "Furniture" );
				sprite.insert( "Material", Selection::getInstance().material() );
				spriteIDs.push_back( sprite );
			}
			else
			{
				spriteIDs = DB::selectRows( "Actions_Tiles", "ID", action );
			}
			for ( auto asi : spriteIDs )
			{
				QVariantMap entry = asi;
				if ( !entry.value( "SpriteID" ).toString().isEmpty() )
				{
					if ( entry.value( "SpriteID" ).toString() == "none" )
					{
						continue;
					}
					if ( !entry.value( "SpriteIDOverride" ).toString().isEmpty() )
					{
						entry.insert( "SpriteID", entry.value( "SpriteIDOverride" ).toString() );
					}

					// TODO repair rot
					unsigned char localRot = Util::rotString2Char( entry.value( "WallRotation" ).toString() );

					Sprite* addSpriteValid = nullptr;

					QStringList mats;
					for ( auto mv : entry.value( "Material" ).toList() )
					{
						mats.push_back( mv.toString() );
					}

					if ( entry.contains( "Material" ) )
					{
						addSpriteValid = sf.createSprite( entry["SpriteID"].toString(), mats );
					}
					else
					{
						addSpriteValid = sf.createSprite( entry["SpriteID"].toString(), { "None" } );
					}
					Position offset( 0, 0, 0 );
					if ( entry.contains( "Offset" ) )
					{
						QString os      = entry["Offset"].toString();
						QStringList osl = os.split( " " );

						if ( osl.size() == 3 )
						{
							offset.x = osl[0].toInt();
							offset.y = osl[1].toInt();
							offset.z = osl[2].toInt();
						}
						int rotX = offset.x;
						int rotY = offset.y;
						switch ( rotation )
						{
							case 1:
								offset.x = -1 * rotY;
								offset.y = rotX;
								break;
							case 2:
								offset.x = -1 * rotX;
								offset.y = -1 * rotY;
								break;
							case 3:
								offset.x = rotY;
								offset.y = -1 * rotX;
								break;
						}
					}
					sprites.push_back( QPair<Sprite*, QPair<Position, unsigned char>>( addSpriteValid, { offset, localRot } ) );
					spritesInv.push_back( QPair<Sprite*, QPair<Position, unsigned char>>( addSpriteValid, { offset, localRot } ) );
				}
				else
				{
					QString os      = entry["Offset"].toString();
					QStringList osl = os.split( " " );
					Position offset( 0, 0, 0 );
					if ( osl.size() == 3 )
					{
						offset.x = osl[0].toInt();
						offset.y = osl[1].toInt();
						offset.z = osl[2].toInt();
					}
					int rotX = offset.x;
					int rotY = offset.y;
					switch ( rotation )
					{
						case 1:
							offset.x = -1 * rotY;
							offset.y = rotX;
							break;
						case 2:
							offset.x = -1 * rotX;
							offset.y = -1 * rotY;
							break;
						case 3:
							offset.x = rotY;
							offset.y = -1 * rotX;
							break;
					}
					sprites.push_back( QPair<Sprite*, QPair<Position, unsigned char>>( sf.createSprite( "SolidSelectionFloor", { "None" } ), { offset, 0 } ) );
					spritesInv.push_back( QPair<Sprite*, QPair<Position, unsigned char>>( sf.createSprite( "SolidSelectionFloor", { "None" } ), { offset, 0 } ) );
				}
			}
			unsigned int tileID = 0;
			for ( auto p : selection )
			{
				if ( p.second )
				{
					for ( auto as : sprites )
					{
						if ( as.first )
						{
							SelectionData sd;
							sd.spriteID = as.first->uID;
							sd.localRot = ( ( rotation + as.second.second ) % 4 );
							sd.pos      = Position( p.first + as.second.first );
							sd.pos.setToBounds();
							sd.isFloor = isFloor;
							sd.valid   = true;
							m_selectionData.insert( posToInt( sd.pos, m_rotation ), sd );
						}
					}
				}
				else
				{
					for ( auto as : spritesInv )
					{
						if ( as.first )
						{
							SelectionData sd;
							sd.spriteID = as.first->uID;
							sd.localRot = ( ( rotation + as.second.second ) % 4 );
							sd.pos      = Position( p.first + as.second.first );
							sd.pos.setToBounds();
							sd.isFloor = isFloor;
							sd.valid   = false;
							m_selectionData.insert( posToInt( sd.pos, m_rotation ), sd );
						}
					}
				}
			}
		}
	}
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

Position MainWindowRenderer::calcCursor( int mouseX, int mouseY, bool useViewLevel ) const
{
	Position cursorPos;
	int dim = Global::dimX;
	if ( dim == 0 )
	{
		cursorPos = Position( 0, 0, 0 );
		return cursorPos;
	}
	int viewLevel = m_viewLevel;
	int w2        = ( m_width / m_scale ) / 2;
	int h2        = ( m_height / m_scale ) / 2;

	int x0 = m_moveX + w2;
	int y0 = m_moveY + h2 + 360;

	int rot = m_rotation;

	int dimZ = Global::dimZ;

	World& world     = Global::w();
	bool zFloorFound = false;

	int origViewLevel = viewLevel;
	int zDiff         = 0;
	while ( !zFloorFound && zDiff < 20 )
	{
		zDiff  = origViewLevel - viewLevel;
		int z0 = qMax( 0, viewLevel - ( viewLevel - 20 ) );

		int m_mouseXScaled = (double)( mouseX ) / m_scale;
		int m_mouseYScaled = (double)( mouseY ) / m_scale - ( zDiff * 4 ) - 3;

		int column = ( m_mouseXScaled - x0 ) / 32;
		if ( m_mouseXScaled < x0 )
			column -= 1;
		int row = ( m_mouseYScaled - y0 - 8 + z0 * 20 ) / 16;

		float quadX = ( m_mouseXScaled - x0 ) % 32;
		float quadY = ( ( m_mouseYScaled - y0 - 8 + z0 * 20 ) % 16 ) * 2;

		if ( quadX < 0 )
		{
			if ( quadX == -32 )
				quadX = 0;
			quadX = 31 + quadX;
		}
		if ( quadY < 0 )
			quadY = 31 + quadY;

		bool lower  = ( quadX / quadY ) >= 1.0;
		bool lower2 = ( ( 32. - quadX ) / quadY ) > 1.0;

		bool north = lower && lower2;
		bool south = !lower && !lower2;
		bool east  = lower && !lower2;
		bool west  = !lower && lower2;

		int selX = 0;
		int selY = 0;

		if ( south )
		{
			//qDebug() << "south" << quadX << " " << quadY << " " << row << " " <<  column;
			selX = row + 1 + column;
			selY = row + 1 - column;
		}
		if ( west )
		{
			//qDebug() << "west" << quadX << " " << quadY << " " << row << " " <<  column;
			selX = row + column;
			selY = row + 1 - column;
		}
		if ( north )
		{
			//qDebug() << "north" << quadX << " " << quadY << " " << row << " " <<  column;
			selX = row + column;
			selY = row - column;
		}
		if ( east )
		{
			//qDebug() << "east" << quadX << " " << quadY << " " << row << " " <<  column;
			selX = row + 1 + column;
			selY = row - column;
		}

		switch ( rot )
		{
			case 0:
				cursorPos.x = qMin( qMax( 0, selX - zDiff - 1 ), dim - 1 );
				cursorPos.y = qMin( qMax( 0, selY - zDiff - 1 ), dim - 1 );
				cursorPos.z = qMin( qMax( 0, viewLevel ), dimZ - 1 );

				if ( !Global::wallsLowered && cursorPos.valid() && Global::w().getTile( cursorPos.seOf() ).wallType & WallType::WT_SOLIDWALL )
				{
					cursorPos.x += 1;
					cursorPos.y += 1;
				}

				break;
			case 1:
				cursorPos.x = qMin( qMax( 0, selY - zDiff - 1 ), dim - 1 );
				cursorPos.y = qMin( qMax( 0, dim - selX + zDiff ), dim - 1 );
				cursorPos.z = qMin( qMax( 0, viewLevel ), dimZ - 1 );
				if ( !Global::wallsLowered && cursorPos.valid() && Global::w().getTile( cursorPos.neOf() ).wallType & WallType::WT_SOLIDWALL )
				{
					cursorPos.x += 1;
					cursorPos.y -= 1;
				}
				break;
			case 2:
				cursorPos.x = qMin( qMax( 0, dim - selX + zDiff ), dim - 1 );
				cursorPos.y = qMin( qMax( 0, dim - selY + zDiff ), dim - 1 );
				cursorPos.z = qMin( qMax( 0, viewLevel ), dimZ - 1 );
				if ( !Global::wallsLowered && cursorPos.valid() && Global::w().getTile( cursorPos.nwOf() ).wallType & WallType::WT_SOLIDWALL )
				{
					cursorPos.x -= 1;
					cursorPos.y -= 1;
				}
				break;
			case 3:
				cursorPos.x = qMin( qMax( 0, dim - selY + zDiff ), dim - 1 );
				cursorPos.y = qMin( qMax( 0, selX - zDiff - 1 ), dim - 1 );
				cursorPos.z = qMin( qMax( 0, viewLevel ), dimZ - 1 );
				if ( !Global::wallsLowered && cursorPos.valid() && Global::w().getTile( cursorPos.swOf() ).wallType & WallType::WT_SOLIDWALL )
				{
					cursorPos.x -= 1;
					cursorPos.y += 1;
				}
				break;
		}

		const Tile& tile = world.getTile( cursorPos );
		if ( cursorPos.z > 0 )
		{
			Tile& tileBelow = world.getTile( cursorPos.x, cursorPos.y, cursorPos.z - 1 );
			if ( tile.floorType != FloorType::FT_NOFLOOR || tileBelow.wallType == WallType::WT_SOLIDWALL || useViewLevel )
			{
				zFloorFound = true;
			}
			else
			{
				--viewLevel;
				if ( viewLevel == 1 )
				{
					zFloorFound = true;
				}
			}
		}
		else
		{
			zFloorFound = true;
		}
#if 0
		QString text = QString::number( m_mouseXScaled ) + " " + QString::number( m_mouseYScaled );
		text += "\n";
		text += "col:" + QString::number( column ) + " row: " + QString::number( row );
		text += "\n";
		//text += "qx:" + QString::number( quadX ) + " qy: " + QString::number( quadY );
		//text += "\n";
		text += "rot:" + QString::number( rot );
		text += "\n";
		text += "selx:" + QString::number( selX ) + " sely: " + QString::number( selY );
		text += "\n";
		text += m_cursorPos.toString();
		QToolTip::showText( mapToGlobal( QPoint( m_mouseX + 50, m_mouseY + 50 ) ), text , this, rect() );
#endif
	}

	if ( useViewLevel )
	{
		cursorPos.z = viewLevel;
	}
	return cursorPos;
}
