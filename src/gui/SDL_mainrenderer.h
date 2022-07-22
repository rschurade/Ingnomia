//
// Created by Arcnor on 21/07/2022.
// Copyright (c) 2022 Reversed Games. All rights reserved.
//

#ifndef INGNOMIA_SDL_MAINRENDERER_H
#define INGNOMIA_SDL_MAINRENDERER_H

#include <sigslot/signal.hpp>

#include "../base/position.h"
#include "../base/tile.h"
#include "../gui/aggregatorrenderer.h"

#include <bgfx/bgfx.h>

class SDL_MainRenderer
{
public:
	SDL_MainRenderer();

	void initialize();

	void render();

	void cleanupWorld();

	void onTileUpdates( const TileDataUpdateInfo& updates );
	void onThoughtBubbles( const ThoughtBubbleInfo& bubbles );
	void onAxelData( const AxleDataInfo& data );

	void onUpdateSelection( const absl::btree_map<unsigned int, SelectionData>& data, bool noDepthTest );

	void onCenterCameraPosition( const Position& target );

	void onSetInMenu( bool value ) {
		m_inMenu = value;
	}

private:
	bool m_inMenu = true;
	bool m_initialized = false;

	bgfx::UniformHandle m_uWorldSize, m_uRenderMin, m_uRenderMax, m_uTransform, m_uWorldRotation, m_uTickNumber;
	bgfx::UniformHandle m_uUpdateSize;
	bgfx::DynamicVertexBufferHandle m_tileDataHandle, m_tileDataUpdateHandle;

	std::vector<std::vector<TileDataUpdate>> m_pendingUpdates;
	ThoughtBubbleInfo m_thoughBubbles;
	AxleDataInfo m_axleData;

	absl::btree_map<unsigned int, SelectionData> m_selectionData;
	bool m_selectionNoDepthTest = false;

	void paintTiles( bgfx::ViewId viewId );
	void paintSelection( bgfx::ViewId viewId );
	void paintThoughtBubbles( bgfx::ViewId viewId );
	void paintAxles( bgfx::ViewId viewId );

	void updateWorld();
	void uploadTileData( const std::vector<TileDataUpdate>& tileData );

public: // signals:
	sigslot::signal<> redrawRequired;
	sigslot::signal<> fullDataRequired;
	sigslot::signal<float /*x*/, float /*y*/, float /*z*/, int /*r*/, float /*scale*/> signalCameraPosition;
};

#endif // INGNOMIA_SDL_MAINRENDERER_H
