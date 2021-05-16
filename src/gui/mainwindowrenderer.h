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
#pragma once

#include "aggregatorrenderer.h"

#include <NsGui/IView.h>

#include <QElapsedTimer>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWindow>

#include <QOpenGLFunctions_4_3_Core>

struct Position;
class QOpenGLTexture;
class MainWindow;

class MainWindowRenderer : public QObject, protected QOpenGLFunctions_4_3_Core
{
	Q_OBJECT

public:
	MainWindowRenderer( MainWindow* parent = Q_NULLPTR );
	~MainWindowRenderer();

	int rotation() { return m_rotation; }
	float scale() { return m_scale; }
	int moveX() { return m_moveX; }
	int moveY() { return m_moveY; }

protected:
	QOpenGLVertexArrayObject m_vao;
	QScopedPointer<QOpenGLShaderProgram> m_worldShader;
	QScopedPointer<QOpenGLShaderProgram> m_worldUpdateShader;
	QScopedPointer<QOpenGLShaderProgram> m_thoughtBubbleShader;
	QScopedPointer<QOpenGLShaderProgram> m_selectionShader;
	QScopedPointer<QOpenGLShaderProgram> m_axleShader;

	GLuint m_textures[32] = { 0 };
	GLuint m_tileBo       = 0;
	GLuint m_tileUpdateBo = 0;
	GLuint m_vbo          = 0;
	GLuint m_vibo         = 0;

	QString m_selectedAction = "";

private:
	QString copyShaderToString( QString name );
	QOpenGLShaderProgram* initShader( QString name );
	QOpenGLShaderProgram* initComputeShader( QString name );
	bool initShaders();

	unsigned int posToInt( Position pos, quint8 rotation );

	void initTextures();
	void initWorld();

	void paintTiles();
	void setCommonUniforms( QOpenGLShaderProgram* shader );
	void paintSelection();
	void paintThoughtBubbles();
	void paintAxles();
	void updateWorld();
	void uploadTileData( const QVector<TileDataUpdate>& tileData );
	void updateTextures();
	void updateRenderParams();

	void updatePositionAfterCWRotation( float& x, float& y );

	void createArrayTexture( int unit, int depth );
	void uploadArrayTexture( int unit, int depth, const uint8_t* data );
	int m_texesUsed         = 0;
	bool m_texesInitialized = false;

	MainWindow* m_parent;

	QMatrix4x4 m_projectionMatrix;

	int m_width  = 0;
	int m_height = 0;

	bool m_rendering = false;
	bool m_inMenu    = true;

	float m_moveX       = 0;
	float m_moveY       = 0;
	float m_scale       = 1.0;
	int m_rotation      = 0;
	int m_renderSize    = 100;
	int m_viewLevel     = 100;
	int m_renderDepth   = 10;
	bool m_debug        = false;

	struct RenderVolume
	{
		Position min;
		Position max;
		inline Position size() const
		{
			// Min and max are inclusive
			return max - min + Position( 1, 1, 1 );
		}
	};
	RenderVolume m_volume;

	bool m_paintCreatures = true;

	float m_daylight = 1.0;

	int m_countRenders = 0;

	bool m_pause     = false;
	float m_lightMin = 0.3f;

	bool m_reloadShaders = false;

	QMap<unsigned int, SelectionData> m_selectionData;
	bool m_selectionNoDepthTest = false;

	ThoughtBubbleInfo m_thoughBubbles;
	AxleDataInfo m_axleData;
	QVector<QVector<TileDataUpdate>> m_pendingUpdates;

public slots:
	void initializeGL();
	void reloadShaders();
	void resize( int w, int h );
	void rotate( int direction );
	void move( float x, float y );
	void scale( float factor );
	void setScale( float scale );
	void setViewLevel( int level );
	void cleanup();
	void cleanupWorld();

	void onTileUpdates( const TileDataUpdateInfo& updates );
	void onThoughtBubbles( const ThoughtBubbleInfo& bubbles );
	void onAxelData( const AxleDataInfo& data );

	void paintWorld();
	void onRenderParamsChanged();

	void onSetInMenu( bool value );

	void onUpdateSelection( const QMap<unsigned int, SelectionData>& data, bool noDepthTest );

signals:
	void redrawRequired();
	void fullDataRequired();
	void signalCameraPosition(float x, float y, float z, int r, float scale );
};
