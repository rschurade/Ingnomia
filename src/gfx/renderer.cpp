#include "renderer.h"

#include "../base/config.h"

#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QCoreApplication>

#include <QFile>

Renderer::Renderer( int width, int height ) :
	m_width( width ),
	m_height( height )
{
}
	
Renderer::~Renderer()
{
}

void Renderer::render()
{
	QOpenGLVertexArrayObject::Binder vaoBinder( &m_vao );

	glViewport(0, 0, m_width, m_height );
	glClearStencil(0);
	glClearColor( 0.0, 0.0, 0.0, 0.0 );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
		
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_ALWAYS );
	
	if( m_shader )	
	{
		m_shader->bind();	
		
		m_shader->setUniformValue( "transform", m_projectionMatrix );

		glDrawArraysInstancedBaseInstance( GL_TRIANGLES, 0, 18, 1, 0 );	

		m_shader->release();
	}
}

bool Renderer::init()
{
	

	std::string glVendor = reinterpret_cast< char const * >( glGetString( GL_VENDOR ) );
	std::string glVersion = reinterpret_cast< char const * >( glGetString( GL_VERSION ) );
	std::string glRenderer = reinterpret_cast< char const * >( glGetString( GL_RENDERER ) );
	
	qDebug() << glVendor.c_str();
	qDebug() << glRenderer.c_str();
	qDebug() << glVersion.c_str();

	float vertices[] = {
		0.,  1., -10.,  // top left
		0, 0, -10,  // bottom left
		1., 1., -10.0f,  // top right
		
		0.,  0., -10.,  // bottom left
		1.,  0, -10.0f,   // bottom right
		1.,  1., -10.  // top right
	};
	
	if( !initShaders() )
	{
		qCritical() << "failed to init shaders - exiting";
		return false;
	}

	m_vao.create();
	QOpenGLVertexArrayObject::Binder vaoBinder( &m_vao );

	glGenBuffers( 1, &m_vbo );  
	glBindBuffer( GL_ARRAY_BUFFER, m_vbo );  
	glBufferData( GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW );

	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0 );
	glEnableVertexAttribArray( 0 );  

	glClearColor( 0, 0, 0, 1 );
	glClear( GL_COLOR_BUFFER_BIT );
	glEnable( GL_DEPTH_TEST );

	return true;
}

void Renderer::resizeGL( int w, int h )
{
	m_width = w;
	m_height = h;
	//qDebug() << "resize gl";

	glViewport( 0, 0, w, h );

	m_projectionMatrix.setToIdentity();
	m_projectionMatrix.ortho( -w / 2, w / 2, -h/ 2, h / 2, -1000, +1000 );
}

QString Renderer::copyShaderToString( QString name )
{
	QFile file( Config::getInstance().get("dataPath").toString() + "/shaders/" + name + ".glsl" );
	file.open( QIODevice::ReadOnly );
	QTextStream in( &file );
	QString code( "" );
	while( !in.atEnd() )
	{
		code += in.readLine();
		code += "\n";
	}

	return code;
}

QOpenGLShaderProgram* Renderer::initShader( QString name )
{
	QString vs = copyShaderToString( name + "_v" );
	QString fs = copyShaderToString( name + "_f" );

	QOpenGLShaderProgram* shader = new QOpenGLShaderProgram;

	bool ok = true; 
	ok &= shader->addShaderFromSourceCode( QOpenGLShader::Vertex, vs );
	ok &= shader->addShaderFromSourceCode( QOpenGLShader::Fragment, fs );
	if( !ok )
	{
		qDebug() << "failed to add shader source code";
		return nullptr;
	}

	ok &= shader->link();

	if( !ok )
	{
		qDebug() << "failed to link shader";
		return nullptr;
	}

	ok &= shader->bind();
	if( !ok )
	{
		qDebug() << "failed to bind shader";
		return nullptr;
	}
	
	shader->release();

	return shader;
}

bool Renderer::initShaders()
{
	m_shader = initShader( "menu" );
	if( !m_shader )
	{
		return false;
	}
	else
	{
		qDebug() << "init shader menu success";
	}

	return true;
}