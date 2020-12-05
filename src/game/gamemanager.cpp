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

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QJsonDocument>
#include <QStandardPaths>

GameManager::GameManager( QObject* parent ) :
	QObject( parent )
{
	qRegisterMetaType<GameSpeed>();

	m_gameThread.start();
	GameState::init();

	connect( this, &GameManager::signalUpdatePaused, &EventConnector::getInstance(), &EventConnector::onUpdatePause );
	connect( this, &GameManager::signalUpdateGameSpeed, &EventConnector::getInstance(), &EventConnector::onUpdateGameSpeed );

	EventConnector::getInstance().moveToThread( &m_gameThread );
}

GameManager::~GameManager()
{
	m_gameThread.terminate();
	m_gameThread.wait();

	if ( m_game )
	{
		delete m_game;
	}
}

void GameManager::setShowMainMenu( bool value )
{
	m_showMainMenu = value;
}

void GameManager::startNewGame( std::function<void( void )> callback )
{
	qDebug() << "GameManger: New game";

	// create new random kingdom name

	// save current settings for fast create new game
	NewGameSettings::getInstance().save();
	
	// check if folder exists, set new save folder name if yes
	createNewGame();
	callback();
}

void GameManager::setUpNewGame()
{
	// check if folder exists, set new save folder name if yes
}

void GameManager::continueLastGame( std::function<void( bool )> callback )
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
				loadGame( folder + gameDir + "/", callback );
				return;
			}
		}
	}

	callback( false );
}

void GameManager::init()
{
	// Temporarily stop thread, so we can safely destroy the game
	m_gameThread.quit();
	m_gameThread.wait();

	if ( m_game )
	{
		delete m_game;
	}

	m_gameThread.start();

	// reset everything and initialize components;
	Global::reset();

	emit stopGame();

	GameState::init();

	if ( !S::gi().init() )
	{
		qDebug() << "Failed to init translation.";
		abort();
	}

	if ( !Global::sf().init() )
	{
		qDebug() << "Failed to init SpriteFactory.";
		abort();
	}
}

void GameManager::loadGame( QString folder, std::function<void( bool )> callback )
{
	init();

	Config::getInstance().set( "NoRender", true );

	IO io;
	connect( &io, &IO::signalStatus, this, &GameManager::onGeneratorMessage );
	if ( io.load( folder ) )
	{
		m_game = new Game( true );
		connect( this, &GameManager::stopGame, EventConnector::getInstance().aggregatorRenderer(), &AggregatorRenderer::onWorldParametersChanged );
		connect( this, &GameManager::startGame, m_game, &Game::start );

		qRegisterMetaType<QSet<unsigned int>>();
		connect( m_game, &Game::signalUpdateTileInfo, EventConnector::getInstance().aggregatorTileInfo(), &AggregatorTileInfo::onUpdateAnyTileInfo );
		connect( m_game, &Game::signalUpdateStockpile, EventConnector::getInstance().aggregatorStockpile(), &AggregatorStockpile::onUpdateAfterTick );

		connect( m_game, &Game::signalUpdateTileInfo, EventConnector::getInstance().aggregatorRenderer(), &AggregatorRenderer::onUpdateAnyTileInfo );

		connect( m_game, &Game::signalTimeAndDate, &EventConnector::getInstance(), &EventConnector::onTimeAndDate );
		connect( m_game, &Game::signalKingdomInfo, &EventConnector::getInstance(), &EventConnector::onKingdomInfo );
		m_game->sendTime();
		EventConnector::getInstance().onViewLevel( GameState::viewLevel );

		Config::getInstance().set( "NoRender", false );

		m_game->moveToThread( &m_gameThread );

		Config::getInstance().set( "gameRunning", true );

		m_showMainMenu = false;

		emit signalInitView();

		emit startGame();
		callback( true );
	}
	else
	{
		qDebug() << "failed to load";
		callback( false );
	}
}

void GameManager::createNewGame()
{
	init();

	m_game = new Game();

	World& world = Global::w();

	WorldGenerator wg;
	connect( &wg, &WorldGenerator::signalStatus, this, &GameManager::onGeneratorMessage );
	wg.generate();

	GameState::peaceful = NewGameSettings::getInstance().isPeaceful();
	Global::nm().reset();
	Global::mcm().init();
	Global::mil().init();
	Global::w().regionMap().initRegions();
	PathFinder::getInstance().init();
	Util::initAllowedInContainer();

	Config::getInstance().set( "NoRender", false );
	
	EventConnector::getInstance().onViewLevel( GameState::viewLevel );
	m_game->moveToThread( &m_gameThread );

	Config::getInstance().set( "gameRunning", true );

	m_showMainMenu = false;

	connect( this, &GameManager::stopGame, EventConnector::getInstance().aggregatorRenderer(), &AggregatorRenderer::onWorldParametersChanged );
	connect( this, &GameManager::startGame, m_game, &Game::start );

	qRegisterMetaType<QSet<unsigned int>>();
	connect( m_game, &Game::signalUpdateTileInfo, EventConnector::getInstance().aggregatorTileInfo(), &AggregatorTileInfo::onUpdateAnyTileInfo );
	connect( m_game, &Game::signalUpdateStockpile, EventConnector::getInstance().aggregatorStockpile(), &AggregatorStockpile::onUpdateAfterTick );

	connect( m_game, &Game::signalUpdateTileInfo, EventConnector::getInstance().aggregatorRenderer(), &AggregatorRenderer::onUpdateAnyTileInfo );

	connect( m_game, &Game::signalTimeAndDate, &EventConnector::getInstance(), &EventConnector::onTimeAndDate );
	m_game->sendTime();

	//thread1->setPriority( QThread::HighPriority );
	emit startGame();
}

void GameManager::onGeneratorMessage( QString message )
{
	qDebug() << message;
}

void GameManager::saveGame()
{
	bool paused = m_paused;

	m_paused = true;
	IO::save();

	m_paused = paused;
}

GameSpeed GameManager::gameSpeed()
{
	return m_gameSpeed;
}
void GameManager::setGameSpeed( GameSpeed speed )
{
	qDebug() << (int)speed;
	if( m_gameSpeed != speed )
	{
		m_gameSpeed = speed;
		emit signalUpdateGameSpeed( m_gameSpeed );
	}
}

bool GameManager::paused()
{
	return m_paused;
}
void GameManager::trySetPaused( bool value )
{
	emit signalUpdatePaused( value );
}

void GameManager::setPaused( bool value )
{
	if( m_paused != value )
	{
		m_paused = value;
		emit signalUpdatePaused( value );
	}
}