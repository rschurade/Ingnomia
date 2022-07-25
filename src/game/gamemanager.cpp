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

#include <QPointer>
#include <QDateTime>
#include <QDir>
#include <QJsonDocument>
#include <QStandardPaths>

#include "spdlog/spdlog.h"

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
	spdlog::debug("GameManger: New game");

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
	auto folder = IO::getDataFolder() / "save";

	QDir dir( QString::fromStdString(folder.string()) );
	dir.setFilter( QDir::Dirs | QDir::NoDotAndDotDot );
	dir.setSorting( QDir::Time );
	if ( !dir.entryList().isEmpty() )
	{
		auto kingdomDir = dir.entryList().first().toStdString();

		folder = IO::getDataFolder() / "save" / kingdomDir;
		QDir dir2( QString::fromStdString(folder.string()) );
		dir2.setFilter( QDir::Dirs | QDir::NoDotAndDotDot );
		dir2.setSorting( QDir::Time );
		if ( !dir2.entryList().isEmpty() )
		{
			auto gameDir = dir2.entryList().first().toStdString();

			if ( IO::saveCompatible( folder / gameDir ) )
			{
				loadGame( folder / gameDir );
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
		spdlog::debug("Failed to init translation.");
		abort();
	}
}

void GameManager::loadGame( const fs::path& folder )
{
	init();
	
	m_game = new Game( this );
	m_eventConnector->setGamePtr( m_game );

	IO io( m_game, this) ;
	io.signalStatus.connect( &GameManager::onGeneratorMessage, this );
	if ( io.load( folder ) )
	{
		Global::util = new Util( m_game );
		Global::sel = new Selection( m_game );

		postCreationInit();
		m_eventConnector->sendLoadGameDone( true );
	}
	else
	{
		spdlog::debug("failed to load");
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

	m_game->fm()->signalFarmChanged.connect(&AggregatorAgri::onUpdateFarm, m_eventConnector->aggregatorAgri()); // TODO: Qt::QueuedConnection
	m_game->fm()->signalPastureChanged.connect(&AggregatorAgri::onUpdatePasture, m_eventConnector->aggregatorAgri()); // TODO: Qt::QueuedConnection
	m_eventConnector->aggregatorDebug()->signalTriggerEvent.connect(&EventManager::onDebugEvent, m_game->em());
	m_game->spm()->signalStockpileAdded.connect(&AggregatorStockpile::onOpenStockpileInfo, m_eventConnector->aggregatorStockpile()); // TODO: Qt::QueuedConnection
	m_game->spm()->signalStockpileContentChanged.connect(&AggregatorStockpile::onUpdateStockpileContent, m_eventConnector->aggregatorStockpile()); // TODO: Qt::QueuedConnection
	m_game->wsm()->signalJobListChanged.connect(&AggregatorWorkshop::onCraftListChanged, m_eventConnector->aggregatorWorkshop()); // TODO: Qt::QueuedConnection

	m_game->sm()->signalPlayEffect.connect(&AggregatorSound::onPlayEffect, m_eventConnector->aggregatorSound()); // TODO: Qt::QueuedConnection

	m_game->em()->signalUpdateMission.connect( &AggregatorNeighbors::onUpdateMission, m_eventConnector->aggregatorNeighbors() ); // TODO: Qt::QueuedConnection
	m_game->em()->signalCenterCamera.connect( &AggregatorRenderer::onCenterCamera, m_eventConnector->aggregatorRenderer() );    // TODO: Qt::QueuedConnection

	m_game->inv()->signalAddItem.connect(&AggregatorInventory::onAddItem, m_eventConnector->aggregatorInventory()); // TODO: Qt::QueuedConnection
	m_game->inv()->signalRemoveItem.connect(&AggregatorInventory::onRemoveItem, m_eventConnector->aggregatorInventory()); // TODO: Qt::QueuedConnection

	m_game->signalTimeAndDate.connect(&EventConnector::onTimeAndDate, m_eventConnector);
	m_game->signalKingdomInfo.connect(&EventConnector::onKingdomInfo, m_eventConnector);
	m_game->signalHeartbeat.connect(&EventConnector::onHeartbeat, m_eventConnector);

	Global::util->initAllowedInContainer();
	m_eventConnector->onViewLevel( GameState::viewLevel );
	m_eventConnector->emitInMenu( false );

	m_eventConnector->stopGame.connect(&AggregatorRenderer::onWorldParametersChanged, m_eventConnector->aggregatorRenderer());
	m_eventConnector->startGame.connect(&Game::start, m_game);

	m_eventConnector->signalCameraPosition.connect(&AggregatorSound::onCameraPosition, m_eventConnector->aggregatorSound()); // TODO: Qt::QueuedConnection


	m_game->signalUpdateTileInfo.connect(&AggregatorTileInfo::onUpdateAnyTileInfo, m_eventConnector->aggregatorTileInfo());
	m_game->signalUpdateStockpile.connect(&AggregatorStockpile::onUpdateAfterTick, m_eventConnector->aggregatorStockpile());
	m_game->signalUpdateTileInfo.connect(&AggregatorRenderer::onUpdateAnyTileInfo, m_eventConnector->aggregatorRenderer());
	m_game->signalTimeAndDate.connect(&EventConnector::onTimeAndDate, m_eventConnector);
	m_game->sendTime();

	Global::sel->signalActionChanged.connect(&AggregatorSelection::onActionChanged, m_eventConnector->aggregatorSelection()); // TODO: Qt::QueuedConnection
	Global::sel->signalFirstClick.connect(&AggregatorSelection::onUpdateFirstClick, m_eventConnector->aggregatorSelection()); // TODO: Qt::QueuedConnection
	Global::sel->signalSize.connect(&AggregatorSelection::onUpdateSize, m_eventConnector->aggregatorSelection()); // TODO: Qt::QueuedConnection
	Global::sel->updateGui();

	m_eventConnector->aggregatorInventory()->update();

	m_eventConnector->emitPause( m_game->paused() );
	m_eventConnector->emitStartGame();
}

void GameManager::onGeneratorMessage( QString message )
{
	spdlog::debug( "{}", message.toStdString() );
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
void GameManager::setHeartbeatResponse( int value )
{
	if( m_game )
	{
		m_game->setHeartbeatResponse( value );
	}
}

Game* GameManager::game()
{ 
	return m_game; 
}