//
// Created by Arcnor on 25/07/2022.
// Copyright (c) 2022 Reversed Games. All rights reserved.
//

#include "debugmanager.h"

#include "../base/config.h"
#include "../game/game.h"
#include "../game/gnomemanager.h"
#include "../gfx/constants.h"
#include "../gfx/spritefactory.h"
#include "../gui/eventconnector.h"
#include "../gui/xaml/debugproxy.h"

#include <SDL_image.h>
#include <imgui.h>

#include <fmt/format.h>

static const std::string debugSpawning[] = {
	"Gnome",
	"Trader",
	"Goblin"
};

constexpr ImVec2 SpawnButtonSize( 150, 0 );

DebugProxy* _debugProxy;

void DebugManager::showDebug()
{
	auto* game = Global::eventConnector->game();
	if ( game == nullptr )
		return;

	if ( _debugProxy == nullptr )
	{
		_debugProxy = new DebugProxy( nullptr );
	}

	ImGui::Begin( "Debug" );

	if ( ImGui::BeginTabBar( "mainDebugTabBar" ) )
	{
		renderGeneralTab();

		renderGnomeManagerTab();

		ImGui::EndTabBar();
	}
	ImGui::End();
}

void DebugManager::renderGeneralTab()
{
	if ( ImGui::BeginTabItem( "General" ) )
	{
		auto* game = Global::eventConnector->game();

		const auto texUsed = game->sf()->texesUsed();

		ImGui::Text( "Used textures: %d", texUsed );
		ImGui::Separator();
		for ( const auto& item : debugSpawning )
		{
			const auto lbl = fmt::format( "Spawn {}", item );
			if ( ImGui::Button( lbl.c_str(), SpawnButtonSize ) )
			{
				_debugProxy->spawnCreature( item );
			}
		}
		ImGui::Separator();
		if ( ImGui::Button( "Dump textures" ) )
		{
			int depth = Global::cfg->get<int>( "MaxArrayTextures" );
			fs::path outPath( "/tmp/ing" );
			fs::create_directories( outPath );

			constexpr auto imageBytes = SpriteWidth * SpriteHeight * SpriteBytesPerPixel;
			for ( int j = 0; j < texUsed; ++j )
			{
				auto* pixData    = game->sf()->pixelData( j );
				fs::path outFile = outPath / fmt::format( "{}.png", j );
				IMG_SavePNG( pixData, outFile.c_str() );
			}
		}

		ImGui::EndTabItem();
	}
}

void DebugManager::renderGnomeManagerTab()
{
	if ( ImGui::BeginTabItem( "Gnome Manager" ) )
	{
		auto* game = Global::eventConnector->game();

		game->gm()->showDebug();
		ImGui::EndTabItem();
	}
}