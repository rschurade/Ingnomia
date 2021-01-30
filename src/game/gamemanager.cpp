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
#include "../base/selection.h"

#include "../game/game.h"
#include "../game/mechanismmanager.h"
#include "../game/militarymanager.h"
#include "../game/newgamesettings.h"
#include "../game/world.h"
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
#include "../game/soundmanager.h"
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
#include "../gui/aggregatorselection.h"
#include "../gui/aggregatorsound.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QJsonDocument>
#include <QStandardPaths>

GameManager::GameManager( QObject* parent ) :
	QObject( parent )
{
	qRegisterMetaType<GameSpeed>();

	m_eventConnector = new EventConnector( this );
	Global::eventConnector = m_eventConnector;
	Global::util = new Util( nullptr );

	Global::newGameSettings = new NewGameSettings( this );

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
	m_eventConnector->emitPause( true );
	m_eventConnector->emitInMenu( value );
	
	if( m_game )
	{
		m_game->setPaused( true );
	}
}

void GameManager::endCurrentGame()
{
	m_eventConnector->emitStopGame();

	if ( m_game )
	{
		delete m_game;
		Global::sel = nullptr;
		Global::util = new Util( nullptr );
	}
}

void GameManager::startNewGame()
{
	qDebug() << "GameManger: New game";

	// create new random kingdom name

	// save current settings for fast create new game
	Global::newGameSettings->save();
	
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
	m_eventConnector->emitStopGame();

	if ( m_game )
	{
		delete m_game;
	}
	// reset everything and initialize components;
	Global::reset();
	
	GameState::init();

	if ( !S::gi().init() )
	{
		qDebug() << "Failed to init translation.";
		abort();
	}
}

void GameManager::loadGame( QString folder )
{
	init();
	
	m_game = new Game( this );
	m_eventConnector->setGamePtr( m_game );

	IO io( m_game, this) ;
	connect( &io, &IO::signalStatus, this, &GameManager::onGeneratorMessage );
	if ( io.load( folder ) )
	{
		Global::util = new Util( m_game );
		Global::sel = new Selection( m_game );

		postCreationInit();
		m_eventConnector->sendLoadGameDone( true );
	}
	else
	{
		qDebug() << "failed to load";
		m_eventConnector->sendLoadGameDone( false );
	}
}

void GameManager::createNewGame()
{
	init();
	m_game = new Game( this );
	m_eventConnector->setGamePtr( m_game );
	m_game->generateWorld( Global::newGameSettings );
	
	Global::util = new Util( m_game );
	Global::sel = new Selection( m_game );

	GameState::peaceful = Global::newGameSettings->isPeaceful();

	postCreationInit();
}


void GameManager::postCreationInit()
{
	m_game->mil()->init();

	m_eventConnector->aggregatorAgri()->init( m_game );
	m_eventConnector->aggregatorCreatureInfo()->init( m_game );
	m_eventConnector->aggregatorInventory()->init( m_game );
	m_eventConnector->aggregatorMilitary()->init( m_game );
	m_eventConnector->aggregatorNeighbors()->init( m_game );
	m_eventConnector->aggregatorPopulation()->init( m_game );
	m_eventConnector->aggregatorRenderer()->init( m_game );
	m_eventConnector->aggregatorStockpile()->init( m_game );
	m_eventConnector->aggregatorTileInfo()->init( m_game );
	m_eventConnector->aggregatorWorkshop()->init( m_game );
	m_eventConnector->aggregatorSound()->init( m_game );

	connect( m_game->fm(), &FarmingManager::signalFarmChanged, m_eventConnector->aggregatorAgri(), &AggregatorAgri::onUpdateFarm, Qt::QueuedConnection );
	connect( m_game->fm(), &FarmingManager::signalPastureChanged, m_eventConnector->aggregatorAgri(), &AggregatorAgri::onUpdatePasture, Qt::QueuedConnection );
	connect( m_eventConnector->aggregatorDebug(), &AggregatorDebug::signalTriggerEvent, m_game->em(), &EventManager::onDebugEvent );
	connect( m_game->spm(), &StockpileManager::signalStockpileAdded, m_eventConnector->aggregatorStockpile(), &AggregatorStockpile::onOpenStockpileInfo, Qt::QueuedConnection );
	connect( m_game->spm(), &StockpileManager::signalStockpileContentChanged, m_eventConnector->aggregatorStockpile(), &AggregatorStockpile::onUpdateStockpileContent, Qt::QueuedConnection );
	connect( m_game->wsm(), &WorkshopManager::signalJobListChanged, m_eventConnector->aggregatorWorkshop(), &AggregatorWorkshop::onCraftListChanged, Qt::QueuedConnection );

	connect( m_game->em(), &EventManager::signalUpdateMission, m_eventConnector->aggregatorNeighbors(), &AggregatorNeighbors::onUpdateMission, Qt::QueuedConnection );

	connect( m_game->inv(), &Inventory::signalAddItem, m_eventConnector->aggregatorInventory(), &AggregatorInventory::onAddItem, Qt::QueuedConnection );
	connect( m_game->inv(), &Inventory::signalRemoveItem, m_eventConnector->aggregatorInventory(), &AggregatorInventory::onRemoveItem, Qt::QueuedConnection );

	connect( m_game, &Game::signalTimeAndDate, m_eventConnector, &EventConnector::onTimeAndDate );
	connect( m_game, &Game::signalKingdomInfo, m_eventConnector, &EventConnector::onKingdomInfo );
	
	connect( m_game->sm(), &SoundManager::signalPlayEffect, m_eventConnector->aggregatorSound(), &AggregatorSound::onPlayEffect, Qt::QueuedConnection );

	
	Global::util->initAllowedInContainer();
	m_eventConnector->onViewLevel( GameState::viewLevel );
	m_eventConnector->emitInMenu( false );

	connect( m_eventConnector, &EventConnector::stopGame, m_eventConnector->aggregatorRenderer(), &AggregatorRenderer::onWorldParametersChanged );
	connect( m_eventConnector, &EventConnector::startGame, m_game, &Game::start );

	qRegisterMetaType<QSet<unsigned int>>();
	connect( m_game, &Game::signalUpdateTileInfo,  m_eventConnector->aggregatorTileInfo(), &AggregatorTileInfo::onUpdateAnyTileInfo );
	connect( m_game, &Game::signalUpdateStockpile, m_eventConnector->aggregatorStockpile(), &AggregatorStockpile::onUpdateAfterTick );
	connect( m_game, &Game::signalUpdateTileInfo,  m_eventConnector->aggregatorRenderer(), &AggregatorRenderer::onUpdateAnyTileInfo );
	connect( m_game, &Game::signalTimeAndDate,     m_eventConnector, &EventConnector::onTimeAndDate );
	m_game->sendTime();

	connect( Global::sel, &Selection::signalActionChanged, m_eventConnector->aggregatorSelection(), &AggregatorSelection::onActionChanged, Qt::QueuedConnection );
	connect( Global::sel, &Selection::signalFirstClick, m_eventConnector->aggregatorSelection(), &AggregatorSelection::onUpdateFirstClick, Qt::QueuedConnection );
	connect( Global::sel, &Selection::signalSize, m_eventConnector->aggregatorSelection(), &AggregatorSelection::onUpdateSize, Qt::QueuedConnection );
	Global::sel->updateGui();

	m_eventConnector->aggregatorInventory()->update();

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
		IO io( m_game, this );
		io.save();
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

Game* GameManager::game()
{ 
	return m_game; 
}