//
// Created by Arcnor on 21/07/2022.
// Copyright (c) 2022 Reversed Games. All rights reserved.
//

#include "SDL_mainrenderer.h"

#include "../base/config.h"
#include "../base/global.h"
#include "../bgfxUtils.h"
#include "../game/game.h"
#include "../game/world.h"
#include "../gamestate.h"
#include "../gui/aggregatorrenderer.h"
#include "../gui/aggregatorselection.h"
#include "../gui/eventconnector.h"
#include "../gfx/spritefactory.h"
#include "../gfx/constants.h"
#include "spdlog/spdlog.h"

#include <bx/math.h>

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
	static const fs::path shaderDir { Global::exePath / "content" / "shaders" / "game" };

	LoadShaders<VertexShaders.size()>(VertexShaders, m_VertexShaders, "VS", shaderDir);
	LoadShaders<FragmentShaders.size()>(FragmentShaders, m_FragmentShaders, "FS", shaderDir);
	LoadShaders<ComputeShaders.size()>(ComputeShaders, m_ComputeShaders, "CS", shaderDir);

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

	// TODO: Destroy resources on shutdown
	m_uWorldSize = bgfx::createUniform("uWorldSize", bgfx::UniformType::Vec4);
	m_uRenderMin = bgfx::createUniform("uRenderMin", bgfx::UniformType::Vec4);
	m_uRenderMax = bgfx::createUniform("uRenderMax", bgfx::UniformType::Vec4);
//	m_uWorldRotation = bgfx::createUniform("uWorldRotation", bgfx::UniformType::Vec4);
//	m_uTickNumber = bgfx::createUniform("uTickNumber", bgfx::UniformType::Vec4);

	m_uUpdateSize = bgfx::createUniform("uUpdateSize", bgfx::UniformType::Vec4);
	m_uTexture = bgfx::createUniform("uTexture", bgfx::UniformType::Sampler);

	int textureSize = Global::cfg->get<int>( "TextureSize" );

	const bgfx::Caps* caps = bgfx::getCaps();
	if (caps->limits.maxTextureSize < textureSize) {
		throw std::runtime_error(fmt::format("Cannot support {} textures, max. texture size is {}, GPU is probably too old!", textureSize, caps->limits.maxTextureSize));
	}
	if (caps->limits.maxTextureLayers < TextureLayers) {
		throw std::runtime_error(fmt::format("Cannot support {} texture layers, max. texture layers is {}, GPU is probably too old!", TextureLayers, caps->limits.maxTextureLayers));
	}
	// TODO: Check texture format support, we don't want to have it emulated...
	m_texture = bgfx::createTexture2D(textureSize, textureSize, false, TextureLayers, bgfx::TextureFormat::RGBA8, BGFX_SAMPLER_UVW_CLAMP | BGFX_SAMPLER_POINT);

	bgfx::VertexLayout vl;
	vl.begin().add(bgfx::Attrib::Position, sizeof(TileDataUpdate) / sizeof(float), bgfx::AttribType::Float).end();
	m_tileDataUpdateHandle = bgfx::createDynamicVertexBuffer((1 << 16) + 2, vl);

	constexpr std::array<float, 3 * 8> vertices = {
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

	constexpr std::array<uint16_t, 3 * 6> indices = {
		0, 1, 3, // Wall 1
		2, 0, 3, // Wall 2
		4, 5, 7, // Floor 1
		6, 4, 7, // Floor 2
		// Wall again for BTF rendering
		0, 1, 3, // Wall 1
		2, 0, 3, // Wall 2
	};

	bgfx::VertexLayout layout;
	layout
		.begin()
		.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
		.end();
	auto* vertexMem = bgfx::copy(&vertices, vertices.size() * sizeof(vertices[0]));
	m_vertexBuffer = bgfx::createVertexBuffer(vertexMem, layout);

	auto* indexMem = bgfx::copy(&indices, indices.size() * sizeof(indices[0]));
	m_indicesBuffer = bgfx::createIndexBuffer(indexMem);

	updateRenderParams();
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
	updateTextures();

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

void SDL_MainRenderer::updateTextures()
{
	auto* sf = Global::eventConnector->game()->sf();
	if ( sf->textureAdded() || sf->creatureTextureAdded() )
	{
#ifdef _DEBUG
		bgfx::setMarker( "update textures" );
#endif

		const auto m_texesUsed = sf->texesUsed();

		for ( int layerIdx = 0; layerIdx < m_texesUsed; ++layerIdx )
		{
			auto* pixData = sf->pixelData( layerIdx );

			// TODO: We can also update only sprites that have changed, instead of a few dozen MB each time...
			SDL_LockSurface(pixData);
			auto *mem = bgfx::copy(pixData->pixels, pixData->pitch * pixData->h);
			bgfx::updateTexture2D(m_texture, layerIdx, 0, 0, 0, pixData->w, pixData->h, mem);
			SDL_UnlockSurface(pixData);
		}
	}
}

void SDL_MainRenderer::paintTiles( bgfx::ViewId viewId )
{
#if _DEBUG
	bgfx::setViewName(viewId, "Tiles");
#endif
	bgfx::touch(viewId);
	bgfx::setViewRect(viewId, 0, 0, m_fbWidth, m_fbHeight);
	bgfx::setViewTransform(viewId, nullptr, m_projection);
	bgfx::setTexture(0, m_uTexture, m_texture);

	bgfx::setIndexBuffer(m_indicesBuffer);
	bgfx::setVertexBuffer(0, m_vertexBuffer);

	bgfx::setBuffer(2, m_tileDataHandle, bgfx::Access::Read);

	bgfx::submit(viewId, m_Programs[m_ProgramWorld]);
}

void SDL_MainRenderer::paintSelection( bgfx::ViewId viewId )
{
#if _DEBUG
	bgfx::setViewName(viewId, "Selection");
#endif
	bgfx::touch(viewId);
	bgfx::setViewRect(viewId, 0, 0, m_fbWidth, m_fbHeight);
	bgfx::setViewTransform(viewId, nullptr, m_projection);

	bgfx::submit(viewId, m_Programs[m_ProgramSelection]);
}

void SDL_MainRenderer::paintThoughtBubbles( bgfx::ViewId viewId )
{
#if _DEBUG
	bgfx::setViewName(viewId, "ThoughtBubbles");
#endif
	bgfx::touch(viewId);
	bgfx::setViewRect(viewId, 0, 0, m_fbWidth, m_fbHeight);
	bgfx::setViewTransform(viewId, nullptr, m_projection);

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
	m_moveX     = 16 * (-target.x + target.y);
	m_moveY     = 8 * ( -target.x - target.y );
	m_viewLevel = target.z;
	onRenderParamsChanged();
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

void SDL_MainRenderer::resize(int fbWidth, int fbHeight)
{
	m_fbWidth = fbWidth;
	m_fbHeight = fbHeight;

	onRenderParamsChanged();
}

void SDL_MainRenderer::onRenderParamsChanged()
{
	updateRenderParams();
	// TODO
}

void SDL_MainRenderer::updateRenderParams()
{
	m_renderSize = qMin( Global::dimX, (int)( ( sqrt( m_fbWidth * m_fbWidth + m_fbHeight * m_fbHeight ) / 12 ) / m_scale ) );

	m_renderDepth = Global::cfg->get_or_default<int>( "renderDepth" , 0 );

	m_viewLevel = GameState::viewLevel;

	m_volume.min = { 0, 0, qMin( qMax( m_viewLevel - m_renderDepth, 0 ), Global::dimZ - 1 ) };
	m_volume.max = { Global::dimX - 1, Global::dimY - 1, qMin( m_viewLevel, Global::dimZ - 1 ) };

	m_lightMin = Global::cfg->get_or_default<double>( "lightMin" , 0 );
	if ( m_lightMin < 0.01 )
		m_lightMin = 0.3f;

	m_debug   = Global::debugMode;

	const bgfx::Caps* caps = bgfx::getCaps();

	//	constexpr auto centering = 0.5f;
	bx::mtxOrtho(
		m_projection
		, -m_fbWidth / 2, m_fbWidth / 2
		, -m_fbHeight / 2, m_fbHeight / 2
		, -( m_volume.max.x + m_volume.max.y + m_volume.max.z + 1 ), -m_volume.min.z
		, 0.0f
		, caps->homogeneousDepth
	);
	bx::mtxScale( m_projection, m_scale );
	bx::mtxTranslate( m_projection, m_moveX, -m_moveY, 0 );
}