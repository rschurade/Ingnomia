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
/** @file mainwindowrenderer.h
 *  @brief MainWindowRenderer: the raw OpenGL 4.3 Core renderer that draws tiles, creatures,
 *         thought bubbles, axles, and selection previews via a compute-shader tile-update
 *         pipeline. Owned by MainWindow.
 */
#pragma once

#include "aggregatorrenderer.h"

#include <NsGui/IView.h>

#include <QElapsedTimer>
#include <QMatrix4x4>
#include <QObject>

#include <glad/gl.h>

struct Position;
class MainWindow;

/// @brief GLAD-based renderer for the world view. Owns all the GL objects (shaders, buffer
///        objects, array textures), applies incoming tile updates to a GPU-side buffer via
///        a compute shader, and draws the world + overlays each frame.
class MainWindowRenderer : public QObject
{
	Q_OBJECT

public:
	MainWindowRenderer( MainWindow* parent = Q_NULLPTR );
	~MainWindowRenderer();

	/// @brief Returns the current camera rotation index (0–3).
	int rotation() { return m_rotation; }
	/// @brief Returns the current zoom factor.
	float scale() { return m_scale; }
	/// @brief Returns the current camera X offset.
	int moveX() { return m_moveX; }
	/// @brief Returns the current camera Y offset.
	int moveY() { return m_moveY; }
	/// @brief Returns true when the renderer is in main-menu (idle) mode.
	bool isInMenu() const { return m_inMenu; }

protected:
	GLuint m_vao = 0;                         ///< Vertex array object for the world quad.
	GLuint m_worldShader = 0;                 ///< Tile-draw vertex/fragment program.
	GLuint m_worldUpdateShader = 0;           ///< Compute shader that applies tile-data updates to m_tileBo.
	GLuint m_thoughtBubbleShader = 0;         ///< Thought bubble vertex/fragment program.
	GLuint m_selectionShader = 0;             ///< Placement-cursor vertex/fragment program.
	GLuint m_axleShader = 0;                  ///< Axle-spinner vertex/fragment program.

	GLuint m_textures[32] = { 0 };            ///< Array textures holding sprite atlas slices.
	GLuint m_tileBo       = 0;                ///< Persistent tile data SSBO on the GPU.
	GLuint m_tileUpdateBo = 0;                ///< Scratch SSBO of incoming tile updates.
	GLuint m_vbo          = 0;                ///< Shared vertex buffer object.
	GLuint m_vibo         = 0;                ///< Shared index buffer object.

	QString m_selectedAction = "";            ///< Currently active cursor action (informational).

private:
	QString copyShaderToString( QString name );
	GLuint initShader( QString name );
	GLuint initComputeShader( QString name );
	bool initShaders();

	void initTextures();
	void initWorld();

	void paintTiles();
	void setCommonUniforms( GLuint shader );
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
	int m_texesUsed         = 0;               ///< Number of array texture slices actually populated.
	bool m_texesInitialized = false;            ///< True once the sprite atlas has been uploaded.

	MainWindow* m_parent;                       ///< Owning MainWindow.

	QMatrix4x4 m_projectionMatrix;              ///< Current orthographic projection.

	int m_width  = 0;                           ///< Viewport width in pixels.
	int m_height = 0;                           ///< Viewport height in pixels.

	bool m_rendering = false;                   ///< Re-entrancy guard for paintWorld().
	bool m_inMenu    = true;                    ///< True while the main menu is visible.

	float m_moveX       = 0;                    ///< Camera X offset.
	float m_moveY       = 0;                    ///< Camera Y offset.
	float m_scale       = 1.0;                  ///< Camera zoom factor.
	int m_rotation      = 0;                    ///< Camera rotation index 0–3.
	int m_renderSize    = 100;                  ///< Tiles per side of the rendered slice.
	int m_viewLevel     = 100;                  ///< Top z-level being displayed.
	int m_renderDepth   = 10;                   ///< Number of z-levels rendered below m_viewLevel.
	bool m_debug        = false;                ///< Debug overlay toggle.

	/// @brief Axis-aligned box of tiles currently being rendered.
	struct RenderVolume
	{
		Position min;  ///< Inclusive lower corner.
		Position max;  ///< Inclusive upper corner.
		/// @brief Returns the size in tiles of the inclusive volume.
		inline Position size() const
		{
			// Min and max are inclusive
			return max - min + Position( 1, 1, 1 );
		}
	};
	RenderVolume m_volume;                      ///< Current render volume bounds.

	bool m_paintCreatures = true;               ///< Debug toggle to hide creature sprites.

	float m_daylight = 1.0;                     ///< Current daylight factor 0–1.

	int m_countRenders = 0;                     ///< Counter for debug frame logging.

	bool m_pause     = false;                   ///< True when the simulation is paused.
	float m_lightMin = 0.3f;                    ///< Minimum ambient light level.

	bool m_reloadShaders = false;               ///< Flag set by the debug hotkey to reload shaders next frame.

	QMap<unsigned int, SelectionData> m_selectionData; ///< Current placement-cursor preview tiles.
	bool m_selectionNoDepthTest = false;        ///< True to draw the cursor without depth test (e.g. stair previews).

	ThoughtBubbleInfo m_thoughBubbles;          ///< Pending thought bubble batch.
	AxleDataInfo m_axleData;                    ///< Pending axle data batch.
	QVector<QVector<TileDataUpdate>> m_pendingUpdates; ///< Tile update batches waiting to be uploaded.

	// Shader uniform setting helpers
	void setUniformi( GLuint shader, const char* name, GLint value );
	void setUniformf( GLuint shader, const char* name, GLfloat value );
	void setUniformui( GLuint shader, const char* name, GLuint value );
	void setUniformMatrix4fv( GLuint shader, const char* name, const QMatrix4x4& matrix );

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

	void onCenterCameraPosition( const Position& target );

signals:
	void redrawRequired();
	void fullDataRequired();
	void signalCameraPosition(float x, float y, float z, int r, float scale );
};
