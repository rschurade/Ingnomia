//
// Created by Arcnor on 21/07/2022.
// Copyright (c) 2022 Reversed Games. All rights reserved.
//

#include "SDL_mainrenderer.h"

#include "../base/global.h"
#include "../bgfxUtils.h"
#include "../gui/aggregatorrenderer.h"
#include "../gui/aggregatorselection.h"
#include "../gui/eventconnector.h"
#include "../game/game.h"
#include "../game/world.h"
#include "spdlog/spdlog.h"

SDL_MainRenderer::SDL_MainRenderer() : m_initialized(false)
{
	Global::eventConnector->aggregatorRenderer()->signalWorldParametersChanged.connect( &SDL_MainRenderer::cleanupWorld, this );

	Global::eventConnector->aggregatorRenderer()->signalTileUpdates.connect( &SDL_MainRenderer::onTileUpdates, this );
	Global::eventConnector->aggregatorRenderer()->signalAxleData.connect( &SDL_MainRenderer::onAxelData, this );
	Global::eventConnector->aggregatorRenderer()->signalThoughtBubbles.connect( &SDL_MainRenderer::onThoughtBubbles, this );
	Global::eventConnector->aggregatorRenderer()->signalCenterCamera.connect( &SDL_MainRenderer::onCenterCameraPosition, this );
	Global::eventConnector->signalInMenu.connect( &SDL_MainRenderer::onSetInMenu, this );

	Global::eventConnector->aggregatorSelection()->signalUpdateSelection.connect(&SDL_MainRenderer::onUpdateSelection, this); // TODO: This was Qt::QueuedConnection

	// Full polling of initial state on load
	this->fullDataRequired.connect(&AggregatorRenderer::onAllTileInfo, Global::eventConnector->aggregatorRenderer());
	this->fullDataRequired.connect(&AggregatorRenderer::onThoughtBubbleUpdate, Global::eventConnector->aggregatorRenderer());
	this->fullDataRequired.connect(&AggregatorRenderer::onAxleDataUpdate, Global::eventConnector->aggregatorRenderer());

	this->signalCameraPosition.connect(&EventConnector::onCameraPosition, Global::eventConnector);

//	this->redrawRequired.connect(&MainWindow::redraw, m_parent);
}

constexpr auto m_ProgramAxle = 0;
constexpr auto m_ProgramSelection = 1;
constexpr auto m_ProgramThoughtBubble = 2;
constexpr auto m_ProgramWorld = 3;
constexpr auto m_ProgramWorldUpdate = 4;

constexpr std::array<std::string_view, 4> VertexShaders {
	"axle",
	"selection",
	"thoughtbubble",
	"world",
};

constexpr std::array<std::string_view, 4> FragmentShaders {
	"axle",
	"selection",
	"thoughtbubble",
	"world",
};

constexpr std::array<std::string_view, 1> ComputeShaders {
	"worldupdate",
};

std::array<bgfx::ShaderHandle, VertexShaders.size()> m_VertexShaders;
std::array<bgfx::ShaderHandle, FragmentShaders.size()> m_FragmentShaders;
std::array<bgfx::ShaderHandle, ComputeShaders.size()> m_ComputeShaders;

std::array<bgfx::ProgramHandle, VertexShaders.size() + ComputeShaders.size()> m_Programs;

void SDL_MainRenderer::initialize()
{
	static const fs::path shaderDir { Global::exePath / "content" / "shaders" };

	LoadShaders<VertexShaders.size()>(VertexShaders, m_VertexShaders, "v", shaderDir);
	LoadShaders<FragmentShaders.size()>(FragmentShaders, m_FragmentShaders, "f", shaderDir);
	LoadShaders<ComputeShaders.size()>(ComputeShaders, m_ComputeShaders, "c", shaderDir);

	for ( auto j = 0; j < FragmentShaders.size(); ++j )
	{
		m_Programs[j] = bgfx::createProgram( m_VertexShaders[j], m_FragmentShaders[j], true );
		if ( !bgfx::isValid( m_Programs[j] ) ) {
			spdlog::critical("Cannot compile shader!");
			abort();
		}
	}

	for ( int j = 0; j < ComputeShaders.size(); ++j )
	{
		m_Programs[j + FragmentShaders.size()] = bgfx::createProgram( m_ComputeShaders[j], true );
	}

	m_uWorldSize = bgfx::createUniform("uWorldSize", bgfx::UniformType::Vec4);
	m_uRenderMin = bgfx::createUniform("uRenderMin", bgfx::UniformType::Vec4);
	m_uRenderMax = bgfx::createUniform("uRenderMax", bgfx::UniformType::Vec4);
	m_uTransform = bgfx::createUniform("uTransform", bgfx::UniformType::Mat4);
//	m_uWorldRotation = bgfx::createUniform("uWorldRotation", bgfx::UniformType::Vec4);
//	m_uTickNumber = bgfx::createUniform("uTickNumber", bgfx::UniformType::Vec4);

	m_uUpdateSize = bgfx::createUniform("uUpdateSize", bgfx::UniformType::Vec4);

	bgfx::VertexLayout vl;
	vl.begin().add(bgfx::Attrib::Position, sizeof(TileDataUpdate) / sizeof(float), bgfx::AttribType::Float).end();
	m_tileDataUpdateHandle = bgfx::createDynamicVertexBuffer((1 << 16) + 2, vl);

}

void SDL_MainRenderer::render() {
	if (m_inMenu) {
		return;
	}

	if (!m_initialized) {
		fullDataRequired();

		bgfx::VertexLayout vl;
		vl.begin().add(bgfx::Attrib::Position, sizeof(TileData) / sizeof(float), bgfx::AttribType::Float).end();
		m_tileDataHandle = bgfx::createDynamicVertexBuffer(Global::eventConnector->game()->w()->world().size(), vl);

		m_initialized = true;
	}

	updateWorld();

	auto viewId = 1;

	bgfx::setViewClear(viewId, BGFX_CLEAR_DEPTH | BGFX_CLEAR_STENCIL | BGFX_CLEAR_COLOR);

	paintTiles(viewId++);
	paintSelection(viewId++);
	paintThoughtBubbles(viewId++);

	if ( Global::showAxles )
	{
		paintAxles(viewId++);
	}
}

void SDL_MainRenderer::paintTiles( bgfx::ViewId viewId )
{
#if _DEBUG
	bgfx::setViewName(viewId, "Tiles");
#endif
	bgfx::touch(viewId);

	bgfx::submit(viewId, m_Programs[m_ProgramWorld]);
}

void SDL_MainRenderer::paintSelection( bgfx::ViewId viewId )
{
#if _DEBUG
	bgfx::setViewName(viewId, "Selection");
#endif
	bgfx::touch(viewId);

	bgfx::submit(viewId, m_Programs[m_ProgramSelection]);
}

void SDL_MainRenderer::paintThoughtBubbles( bgfx::ViewId viewId )
{
#if _DEBUG
	bgfx::setViewName(viewId, "ThoughtBubbles");
#endif
	bgfx::touch(viewId);

	bgfx::submit(viewId, m_Programs[m_ProgramThoughtBubble]);
}

void SDL_MainRenderer::paintAxles( bgfx::ViewId viewId )
{
#if _DEBUG
	bgfx::setViewName(viewId, "Axles");
#endif
	bgfx::touch(viewId);

	bgfx::submit(viewId, m_Programs[m_ProgramAxle]);
}

void SDL_MainRenderer::cleanupWorld()
{
//	m_parent->makeCurrent();
//	glDeleteTextures( 32, m_textures );
//	memset( m_textures, 0, sizeof( m_textures ) );
//	glDeleteBuffers( 1, &m_tileBo );
//	m_tileBo = 0;
//	glDeleteBuffers( 1, &m_tileUpdateBo );
//	m_tileUpdateBo     = 0;
//	m_texesInitialized = false;

//	m_parent->doneCurrent();

	m_pendingUpdates.clear();
	m_selectionData.clear();
	m_thoughBubbles = ThoughtBubbleInfo();
	m_axleData      = AxleDataInfo();
}

void SDL_MainRenderer::onTileUpdates( const TileDataUpdateInfo& updates )
{
	m_pendingUpdates.push_back( updates.updates );
	redrawRequired();
}

void SDL_MainRenderer::onThoughtBubbles( const ThoughtBubbleInfo& bubbles )
{
	m_thoughBubbles = bubbles;
	redrawRequired();
}

void SDL_MainRenderer::onAxelData( const AxleDataInfo& data )
{
	m_axleData = data;
	redrawRequired();
}

void SDL_MainRenderer::onUpdateSelection( const absl::btree_map<unsigned int, SelectionData>& data, bool noDepthTest )
{
	m_selectionData.clear();
	m_selectionData.insert(data.begin(), data.end());
	m_selectionNoDepthTest = noDepthTest;
}

void SDL_MainRenderer::onCenterCameraPosition( const Position& target )
{
//	m_moveX     = 16 * (-target.x + target.y);
//	m_moveY     = 8 * ( -target.x - target.y );
//	m_viewLevel = target.z;
//	onRenderParamsChanged();
}


void SDL_MainRenderer::updateWorld()
{
	if ( !m_pendingUpdates.empty() )
	{
//		DebugScope s( "update world" );
		for ( const auto& update : m_pendingUpdates )
		{
			uploadTileData( update );
		}
		m_pendingUpdates.clear();
	}
}

void SDL_MainRenderer::uploadTileData( const std::vector<TileDataUpdate>& tileData )
{
	bgfx::setBuffer(0, m_tileDataHandle, bgfx::Access::ReadWrite);
	bgfx::setBuffer(1, m_tileDataUpdateHandle, bgfx::Access::Read);
	auto* mem = bgfx::copy(tileData.data(), sizeof(TileDataUpdate) * tileData.size());
	bgfx::update(m_tileDataUpdateHandle, 0, mem);

	const float updateSize[4] { static_cast<float>(tileData.size()), 0.0f, 0.0f, 0.0f };
	bgfx::setUniform(m_uUpdateSize, &updateSize );
	bgfx::dispatch(0, m_Programs[m_ProgramWorldUpdate], ( tileData.size() + 63 ) / 64, 1, 1 );
}