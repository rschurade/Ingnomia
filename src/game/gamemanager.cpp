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
#include "gamemanager.h"

#include "../base/config.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/io.h"
#include "../base/pathfinder.h"
#include "../base/util.h"
#include "../game/game.h"
#include "../game/mechanismmanager.h"
#include "../game/militarymanager.h"
#include "../game/newgamesettings.h"
#include "../game/world.h"
#include "../game/worldgenerator.h"
#include "../gfx/spritefactory.h"
#include "../gui/eventconnector.h"
#include "../gui/mainwindow.h"
#include "../gui/mainwindowrenderer.h"
#include "../gui/strings.h"

#include "../game/inventory.h"
#include "../game/creaturemanager.h"
#include "../game/eventmanager.h"
#include "../game/farmingmanager.h"
#include "../game/fluidmanager.h"
#include "../game/gnomemanager.h"
#include "../game/mechanismmanager.h"
#include "../game/roommanager.h"
#include "../game/stockpilemanager.h"
#include "../game/workshopmanager.h"

#include "../gui/aggregatoragri.h"
#include "../gui/aggregatorcreatureinfo.h"
#include "../gui/aggregatordebug.h"
#include "../gui/aggregatorinventory.h"
#include "../gui/aggregatorpopulation.h"
#include "../gui/aggregatorrenderer.h"
#include "../gui/aggregatorstockpile.h"
#include "../gui/aggregatortileinfo.h"
#include "../gui/aggregatorworkshop.h"
#include "../gui/aggregatorneighbors.h"
#include "../gui/aggregatormilitary.h"
#include "../gui/aggregatorsettings.h"
#include "../gui/aggregatorloadgame.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QJsonDocument>
#include <QStandardPaths>

GameManager::GameManager( QObject* parent ) :
	QObject( parent )
{
	qRegisterMetaType<GameSpeed>();

	m_sf = new SpriteFactory();
	m_eventConnector = new EventConnector( this );
	m_newGameSettings = new NewGameSettings( this );

	GameState::init();

}

GameManager::~GameManager()
{
	if ( m_game )
	{
		delete m_game;
	}
}

EventConnector* GameManager::eventConnector()
{
	return m_eventConnector;
}

void GameManager::setShowMainMenu( bool value )
{
	m_eventConnector->emitInMenu( value );
}

void GameManager::startNewGame()
{
	qDebug() << "GameManger: New game";

	// create new random kingdom name

	// save current settings for fast create new game
	m_newGameSettings->save();
	
	// check if folder exists, set new save folder name if yes
	createNewGame();
	
	m_eventConnector->sendResume();
}

void GameManager::setUpNewGame()
{
	// check if folder exists, set new save folder name if yes
}

void GameManager::continueLastGame()
{
	//get last save
	QString folder = QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/My Games/Ingnomia/save/";

	QDir dir( folder );
	dir.setFilter( QDir::Dirs | QDir::NoDotAndDotDot );
	dir.setSorting( QDir::Time );
	if ( !dir.entryList().isEmpty() )
	{
		auto kingdomDir = dir.entryList().first();

		folder = QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/My Games/Ingnomia/save/" + kingdomDir + "/";
		QDir dir2( folder );
		dir2.setFilter( QDir::Dirs | QDir::NoDotAndDotDot );
		dir2.setSorting( QDir::Time );
		if ( !dir2.entryList().isEmpty() )
		{
			auto gameDir = dir2.entryList().first();

			if ( IO::saveCompatible( folder + gameDir + "/" ) )
			{
				loadGame( folder + gameDir + "/" );
				return;
			}
		}
	}
	m_eventConnector->sendLoadGameDone( false );
}

void GameManager::init()
{
	// Temporarily stop thread, so we can safely destroy the game
	if ( m_game )
	{
		delete m_game;
	}
	// reset everything and initialize components;
	Global::reset();

	m_eventConnector->emitStopGame();

	GameState::init();

	if ( !S::gi().init() )
	{
		qDebug() << "Failed to init translation.";
		abort();
	}

	if ( !m_sf->init() )
	{
		qDebug() << "Failed to init SpriteFactory.";
		abort();
	}
}

void GameManager::loadGame( QString folder )
{
	/*
	init();

	Config::getInstance().set( "NoRender", true );

	IO io;
	connect( &io, &IO::signalStatus, this, &GameManager::onGeneratorMessage );
	if ( io.load( folder ) )
	{
		m_game = new Game();
		connect( m_eventConnector, &EventConnector::stopGame, m_eventConnector->aggregatorRenderer(), &AggregatorRenderer::onWorldParametersChanged );
		connect( m_eventConnector, &EventConnector::startGame, m_game, &Game::start );

		qRegisterMetaType<QSet<unsigned int>>();
		connect( m_game, &Game::signalUpdateTileInfo, m_eventConnector->aggregatorTileInfo(), &AggregatorTileInfo::onUpdateAnyTileInfo );
		connect( m_game, &Game::signalUpdateStockpile, m_eventConnector->aggregatorStockpile(), &AggregatorStockpile::onUpdateAfterTick );

		connect( m_game, &Game::signalUpdateTileInfo, m_eventConnector->aggregatorRenderer(), &AggregatorRenderer::onUpdateAnyTileInfo );

		connect( m_game, &Game::signalTimeAndDate, m_eventConnector, &EventConnector::onTimeAndDate );
		connect( m_game, &Game::signalKingdomInfo, m_eventConnector, &EventConnector::onKingdomInfo );
		m_game->sendTime();
		m_eventConnector->onViewLevel( GameState::viewLevel );

		Config::getInstance().set( "NoRender", false );

		Config::getInstance().set( "gameRunning", true );

		m_eventConnector->emitInMenu( false );

		m_eventConnector->emitInitView();
		m_eventConnector->emitStartGame();

		m_eventConnector->emitPause( m_game->paused() );
		m_eventConnector->sendLoadGameDone( true );
	}
	else
	{
		qDebug() << "failed to load";
		m_eventConnector->sendLoadGameDone( false );
	}
	*/
}

void GameManager::createNewGame()
{
	init();

	WorldGenerator wg( m_newGameSettings, this );
	connect( &wg, &WorldGenerator::signalStatus, this, &GameManager::onGeneratorMessage );
	World* world = wg.generate();

	m_game = new Game( m_sf, world, this );

	connect( m_game->fm(), &FarmingManager::signalFarmChanged, Global::eventConnector->aggregatorAgri(), &AggregatorAgri::onUpdateFarm, Qt::QueuedConnection );
	connect( m_game->fm(), &FarmingManager::signalPastureChanged, Global::eventConnector->aggregatorAgri(), &AggregatorAgri::onUpdatePasture, Qt::QueuedConnection );
	connect( Global::eventConnector->aggregatorDebug(), &AggregatorDebug::signalTriggerEvent, m_game->em(), &EventManager::onDebugEvent );

	m_eventConnector->aggregatorAgri()->init( m_game );
	m_eventConnector->aggregatorCreatureInfo()->init( m_game );
	m_eventConnector->aggregatorInventory()->init( m_game );


	GameState::peaceful = m_newGameSettings->isPeaceful();
	Util::initAllowedInContainer();
	Config::getInstance().set( "NoRender", false );
	m_eventConnector->onViewLevel( GameState::viewLevel );
	Config::getInstance().set( "gameRunning", true );
	m_eventConnector->emitInMenu( false );

	connect( m_eventConnector, &EventConnector::stopGame, m_eventConnector->aggregatorRenderer(), &AggregatorRenderer::onWorldParametersChanged );
	connect( m_eventConnector, &EventConnector::startGame, m_game, &Game::start );

	qRegisterMetaType<QSet<unsigned int>>();
	connect( m_game, &Game::signalUpdateTileInfo,  m_eventConnector->aggregatorTileInfo(), &AggregatorTileInfo::onUpdateAnyTileInfo );
	connect( m_game, &Game::signalUpdateStockpile, m_eventConnector->aggregatorStockpile(), &AggregatorStockpile::onUpdateAfterTick );
	connect( m_game, &Game::signalUpdateTileInfo,  m_eventConnector->aggregatorRenderer(), &AggregatorRenderer::onUpdateAnyTileInfo );
	connect( m_game, &Game::signalTimeAndDate,     m_eventConnector, &EventConnector::onTimeAndDate );
	m_game->sendTime();

	//thread1->setPriority( QThread::HighPriority );
	m_eventConnector->emitPause( m_game->paused() );
	m_eventConnector->emitStartGame();
}

void GameManager::onGeneratorMessage( QString message )
{
	qDebug() << message;
}

void GameManager::saveGame()
{
	if( m_game )
	{
		bool paused = m_game->paused();
		m_game->setPaused( true );
		IO::save();
		m_game->setPaused( paused );

		m_eventConnector->sendResume();
	}
}

GameSpeed GameManager::gameSpeed()
{
	if( m_game )
	{
		return m_game->gameSpeed();
	}
	return GameSpeed::Normal;
}
void GameManager::setGameSpeed( GameSpeed speed )
{
	if( m_game )
	{
		if( m_game->gameSpeed() != speed )
		{
			m_game->setGameSpeed( speed );
			m_eventConnector->emitGameSpeed( m_game->gameSpeed() );
		}
	}
}

bool GameManager::paused()
{
	if( m_game )
	{
		return m_game->paused();
	}
	return true;
}

void GameManager::setPaused( bool value )
{
	if( m_game )
	{
		if( m_game->paused() != value )
		{
			m_game->setPaused( value );
			m_eventConnector->emitPause( value );
		}
	}
}