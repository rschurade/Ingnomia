//
// Created by Arcnor on 15/07/2022.
// Copyright (c) 2022 Reversed Games. All rights reserved.
//

#include "base/config.h"
#include "base/crashhandler.h"
#include "base/db.h"
#include "base/global.h"
#include "base/io.h"
#include "bgfx/bgfx.h"
#include "game/gamemanager.h"
#include "gui/SDL_mainwindow.h"
#include "gui/strings.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"
#include "version.h"

#include <SDL.h>
#include <filesystem>

namespace fs = std::filesystem;

bool verboseLog = false;


fs::path getLogPath()
{
	const auto& folder   = IO::getDataFolder();
	std::string fileName = "log.txt";

	if ( fs::exists( folder ) )
	{
		fileName = ( folder / fileName ).string();
	}

	return fileName;
}

void clearLog( const fs::path& logPath )
{
	if ( fs::exists( logPath ) )
	{
		fs::resize_file( logPath, 0 );
	}
}

void redirectLogToFile( const fs::path& logPath )
{
	try
	{
		auto file_logger = spdlog::basic_logger_mt( "basic_logger", logPath.string() );
		spdlog::set_default_logger( file_logger );
	}
	catch ( const spdlog::spdlog_ex& ex )
	{
		fmt::print( "Log init failed: {}\n", ex.what() );
	}
}

void parseArgs( int argc, char* argv[] )
{
	for ( int i = 1; i < argc; ++i )
	{
		const auto arg = std::string( argv[i] );
		if ( arg == "-h" || arg == "?" )
		{
			spdlog::debug( "Command line options:" );
			spdlog::debug( "-h : displays this message" );
			spdlog::debug( "-v : toggles verbose mode, warning: this will spam your console with messages" );
			spdlog::debug( "---" );
		}
		else if ( arg == "-v" )
		{
			verboseLog = true;
		}
		else if ( arg == "-ds" )
		{
			Global::debugSound = true;
		}
	}
}


int main( int argc, char* argv[] )
//int oldMain( int argc, char* argv[] )
{
	setupCrashHandler();

	if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS |
				   SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC |
				   SDL_INIT_GAMECONTROLLER | SDL_INIT_SENSOR) != 0) {
		spdlog::critical("Cannot initialize SDL: {}", SDL_GetError());
		return 10;
	}
	const auto logPath = getLogPath();
	clearLog( logPath );
	//	redirectLogToFile(logPath);
	spdlog::set_level( spdlog::level::debug );

	spdlog::info( "{} version {}", PROJECT_NAME, PROJECT_VERSION );

	const auto gamePathStr = SDL_GetBasePath();
	const auto prefPathStr = SDL_GetPrefPath( "Roest", PROJECT_NAME );

	const auto gamePath = fs::path( std::string( gamePathStr ) );
	const auto prefPath = fs::path( std::string( prefPathStr ) );

	SDL_free( gamePathStr );
	SDL_free( prefPathStr );

	Global::exePath = gamePath;
	Global::cfg = new Config();

	if ( !Global::cfg->valid() )
	{
		spdlog::debug("Failed to init Config.");
		abort();
	}

	DB::init();
	DB::initStructs();

	if ( !S::gi().init() )
	{
		spdlog::debug("Failed to init translation.");
		abort();
	}

	Global::cfg->set( "CurrentVersion", PROJECT_VERSION );

	parseArgs( argc, argv );

	auto* gm = new GameManager();

	SDL_MainWindow win;

	win.mainLoop();

	delete gm;

	return 0;
}

#ifdef _WIN32
#ifndef _WINDEF_
typedef uint32_t DWORD;
#endif

extern "C"
{
	// Request use of dedicated GPUs for NVidia/AMD/iGPU mixed setups
	__declspec( dllexport ) DWORD NvOptimusEnablement                  = 1;
	__declspec( dllexport ) DWORD AmdPowerXpressRequestHighPerformance = 1;
}
#endif // _WIN32