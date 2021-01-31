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
#include "game.h"

#include "../base/config.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/io.h"
#include "../base/pathfinder.h"
#include "../base/util.h"
#include "../game/animal.h"

#include "../game/inventory.h"
#include "../game/creaturemanager.h"
#include "../game/eventmanager.h"
#include "../game/farmingmanager.h"
#include "../game/fluidmanager.h"
#include "../game/gnomemanager.h"
#include "../game/mechanismmanager.h"
#include "../game/militarymanager.h"
#include "../game/newgamesettings.h"
#include "../game/roommanager.h"
#include "../game/stockpilemanager.h"
#include "../game/workshopmanager.h"
#include "../gfx/spritefactory.h"

#include "../game/gamemanager.h"
#include "../game/gnome.h"

#include "../game/itemhistory.h"
#include "../game/object.h"
#include "../game/plant.h"
#include "../game/techtree.h"
#include "../game/world.h"
#include "../game/worldgenerator.h"

#include "../gui/eventconnector.h"
#include "../gui/strings.h"
#include "../gui/aggregatorcreatureinfo.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QTimer>

#include <time.h>

Game::Game( QObject* parent ) :
	QObject( parent )
{
	qDebug() << "init game...";
	
	m_upsTimer.start();

	m_sf.reset( new SpriteFactory() );
	
	#pragma region initStuff
	DB::select( "Value_", "Time", "MillisecondsSlow" );

	m_millisecondsSlow = DB::select( "Value_", "Time", "MillisecondsSlow" ).toInt();
	m_millisecondsFast = DB::select( "Value_", "Time", "MillisecondsFast" ).toInt();

	GameState::tick = 1;
	GameState::hour = 9;
	GameState::minute = 0;
	GameState::day    = 1;
	GameState::season = 0; // zero based, 0-numSeasons, typically 4 can be modded
	GameState::year   = 1;

	QString dt = QString( "Day %1, %2:%3" ).arg( GameState::day, 2, 10, QChar( ' ' ) ).arg( GameState::hour, 2, 10, QChar( '0' ) ).arg( GameState::minute, 2, 10, QChar( '0' ) );
	GameState::currentDayTime = dt;
	GameState::seasonString = "Spring";
	GameState::currentYearAndSeason = "Year " + QString::number( GameState::year ) + ", " + S::s( "$SeasonName_" + GameState::seasonString );

	calcDaylight();
	GameState::daylight = true;

	for( auto t : DB::ids( "Tech" ) )
	{
		GameState::techs.insert( t, 1 );
	}
#pragma endregion
	m_inv   			= new Inventory( this );
	
	m_spm				= new StockpileManager( this );
	m_farmingManager	= new FarmingManager( this );
	m_workshopManager	= new WorkshopManager( this );
	m_roomManager		= new RoomManager( this );

	m_jobManager		= new JobManager( this );
	
	m_creatureManager	= new CreatureManager( this );
	m_gnomeManager		= new GnomeManager( this );
	m_militaryManager	= new MilitaryManager( this );

	m_mechanismManager	= new MechanismManager( this );
	m_fluidManager		= new FluidManager( this );
	
	m_neighborManager	= new NeighborManager( this );
	m_eventManager		= new EventManager( this );

	qDebug() << "init game done";
}


Game::~Game()
{
}

void Game::generateWorld( NewGameSettings* ngs )
{
	m_inv->loadFilter();

	WorldGenerator wg( ngs, this );
	connect( &wg, &WorldGenerator::signalStatus, dynamic_cast<GameManager*>( parent() ), &GameManager::onGeneratorMessage );
	m_world.reset( wg.generateTopology() );	
	wg.addLife();

	m_pf.reset( new PathFinder( m_world.get(), this ) );
}

void Game::setWorld( int dimX, int dimY, int dimZ )
{
	m_world.reset( new World( dimX, dimY, dimZ, this ) );
	m_pf.reset( new PathFinder( m_world.get(), this ) );
}

void Game::start()
{
	qDebug() << "Starting game";

	if ( GameState::tick == 0 && !GameState::initialSave )
	{
		Global::cfg->set( "DaysToNextAutoSave", 0 );
		autoSave();
		Global::cfg->set( "Pause", true );
		emit signalPause( true );
	}

	if ( m_timer )
	{
		stop();
	}
	m_timer = new QTimer( this );
	connect( m_timer, &QTimer::timeout, this, &Game::loop );
	m_timer->start( m_millisecondsSlow );
}

void Game::stop()
{
	qDebug() << "Stop game";
	if ( m_timer )
	{
		m_timer->stop();
		delete m_timer;
	}
}

void Game::loop()
{
	QElapsedTimer timer;
	timer.start();
	if (m_guiHeartbeat <= m_guiHeartbeatResponse+20)
	{
		m_upsCounter1++;
		if ( m_upsTimer.elapsed() > 1000 ) 
		{
			m_upsTimer.restart();
			//printf(" gameloop ups %d avg ms %d\n", m_upsCounter1, m_avgLoopTime/m_upsCounter1);
			m_upsCounter = m_upsCounter1;
			m_upsCounter1 = 0;
			m_avgLoopTime = 0;
		}
		int ms2 = 0;
		
		if ( !m_paused )
		{
			
			
			emit sendOverlayMessage( 6, "tick " + QString::number( GameState::tick ) );
			//printf("   game tick %d\n",GameState::tick );
			
			sendClock();

			// process grass
			m_world->processGrass();
			// process plants
			processPlants();

			// process animals

			m_creatureManager->onTick( GameState::tick, GameState::seasonChanged, GameState::dayChanged, GameState::hourChanged, GameState::minuteChanged );

			// process gnomes
			QElapsedTimer timer2;
			timer2.start();
			m_gnomeManager->onTick( GameState::tick, GameState::seasonChanged, GameState::dayChanged, GameState::hourChanged, GameState::minuteChanged );
			ms2 = timer2.elapsed();
			// process jobs
			m_jobManager->onTick();
			// process stockpiles
			m_spm->onTick( GameState::tick );
			m_farmingManager->onTick( GameState::tick, GameState::seasonChanged, GameState::dayChanged, GameState::hourChanged, GameState::minuteChanged );
			m_workshopManager->onTick( GameState::tick );
			m_roomManager->onTick( GameState::tick );
			m_inv->itemHistory()->onTick( GameState::dayChanged );
			m_eventManager->onTick( GameState::tick, GameState::seasonChanged, GameState::dayChanged, GameState::hourChanged, GameState::minuteChanged );
			m_mechanismManager->onTick( GameState::tick, GameState::seasonChanged, GameState::dayChanged, GameState::hourChanged, GameState::minuteChanged );
			m_fluidManager->onTick( GameState::tick, GameState::seasonChanged, GameState::dayChanged, GameState::hourChanged, GameState::minuteChanged );
			m_neighborManager->onTick( GameState::tick, GameState::seasonChanged, GameState::dayChanged, GameState::hourChanged, GameState::minuteChanged );

			m_world->processWater();

			m_pf->findPaths();

			++GameState::tick;
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//
		// update gui
		//
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	
		auto updates = m_world->updatedTiles();
		if ( !updates.empty() )
		{
			signalUpdateTileInfo( std::move( updates ) );
		}
		emit signalUpdateStockpile();
	
		Global::eventConnector->aggregatorCreatureInfo()->update();
	
		int ms        = timer.elapsed();
		m_maxLoopTime = qMax( ms2, m_maxLoopTime );
	
		auto numString = QString::number( ms );
		while ( numString.size() < 5 )
			numString.prepend( '0' );
	
		QString msg = "game loop time: " + numString;
		if ( Global::debugMode )
			msg += " ms (max gnome time:" + QString::number( m_maxLoopTime ) + "ms)";
		emit sendOverlayMessage( 3, msg );
	
		
		emit signalKingdomInfo( GameState::kingdomName, 
			"Gnomes: " + QString::number( gm()->numGnomes() ), 
			"Animals: " + QString::number( fm()->countAnimals() ),
			"Items: "  + QString::number( inv()->numItems() ) );

		m_guiHeartbeat = m_guiHeartbeat + 1;
		emit signalHeartbeat(m_guiHeartbeat);
	}

	
	m_avgLoopTime += timer.elapsed();
	
}

void Game::sendClock()
{
	GameState::minuteChanged = false;
	GameState::hourChanged   = false;
	GameState::dayChanged    = false;
	GameState::seasonChanged = false;

	if ( GameState::tick % Global::util->ticksPerMinute == 0 )
	{
		++GameState::minute;
		GameState::minuteChanged = true;
	}
	if ( GameState::minute == Global::util->minutesPerHour )
	{
		GameState::minute = 0;
		++GameState::hour;
		GameState::hourChanged = true;
	}
	if ( GameState::hour == Global::util->hoursPerDay )
	{
		GameState::hour = 0;
		++GameState::day;
		GameState::dayChanged = true;
	}

	if ( GameState::dayChanged )
	{
		int daysThisSeason = DB::select( "NumDays", "Seasons", GameState::seasonString ).toInt();
		if ( GameState::day > daysThisSeason )
		{
			GameState::day = 1;
			++GameState::season;
			GameState::seasonChanged    = true;
			QString nextSeason = DB::select( "NextSeason", "Seasons", GameState::seasonString ).toString();
			//qDebug() << "Now it's " << nextSeason;
			GameState::seasonString = nextSeason;

			Global::util->daysPerSeason = DB::select( "NumDays", "Seasons", nextSeason ).toInt();

			int numSeasonsPerYear = DB::numRows( "Seasons" );
			if ( GameState::season == numSeasonsPerYear )
			{
				GameState::season = 0;
				++GameState::year;
			}
		}

		calcDaylight();

		autoSave();
	}
	if ( GameState::seasonChanged )
	{
		auto gm = dynamic_cast<GameManager*>( parent() );
		m_sf->forceUpdate();
	}

	QString sunStatus;
	int currentTimeInt = GameState::hour * Global::util->minutesPerHour + GameState::minute;
	if ( currentTimeInt < GameState::sunrise )
	{
		//time between midnight and sunrise
		sunStatus        = "Sunrise: " + intToTime( GameState::sunrise );
		GameState::daylight = false;
	}
	else if ( currentTimeInt < GameState::sunset )
	{
		// day time
		sunStatus        = "Sunset: " + intToTime( GameState::sunset );
		GameState::daylight = true;
	}
	else
	{
		// time after sunset
		sunStatus        = "Sunrise: " + intToTime( GameState::nextSunrise );
		GameState::daylight = false;
	}
	QString dt = QString( "Day %1, %2:%3" ).arg( GameState::day, 2, 10, QChar( ' ' ) ).arg( GameState::hour, 2, 10, QChar( '0' ) ).arg( GameState::minute, 2, 10, QChar( '0' ) );
	GameState::currentDayTime = dt;
	GameState::currentYearAndSeason = "Year " + QString::number( GameState::year ) + ", " + S::s( "$SeasonName_" + GameState::seasonString );
	emit signalTimeAndDate( GameState::minute, GameState::hour, GameState::day, S::s( "$SeasonName_" + GameState::seasonString ), GameState::year, sunStatus );
}

void Game::sendTime()
{
	emit signalTimeAndDate( GameState::minute, GameState::hour, GameState::day, S::s( "$SeasonName_" + GameState::seasonString ), GameState::year, "" );
}

void Game::calcDaylight()
{
	QString currentSeason = GameState::seasonString;
	QString sunrise       = DB::select( "SunRiseFirst", "Seasons", currentSeason ).toString();
	QString sunset        = DB::select( "SunSetFirst", "Seasons", currentSeason ).toString();
	QString nextSeason    = DB::select( "NextSeason", "Seasons", currentSeason ).toString();

	QString nextSunrise = DB::select( "SunRiseFirst", "Seasons", nextSeason ).toString();
	QString nextSunset  = DB::select( "SunSetFirst", "Seasons", nextSeason ).toString();

	int numDays = DB::select( "NumDays", "Seasons", currentSeason ).toInt();

	int sr1 = timeToInt( sunrise );
	int sr2 = timeToInt( nextSunrise );
	int ss1 = timeToInt( sunset );
	int ss2 = timeToInt( nextSunset );

	GameState::sunrise     = sr1 + ( ( sr2 - sr1 ) / numDays ) * ( GameState::day - 1 );
	GameState::nextSunrise = sr1 + ( ( sr2 - sr1 ) / numDays ) * ( GameState::day );
	GameState::sunset      = ss1 + ( ( ss2 - ss1 ) / numDays ) * ( GameState::day - 1 );

	//qDebug() << "sunrise: " << intToTime( m_sunrise ) << " sunset:" << intToTime( m_sunset );
}

int Game::timeToInt( QString time )
{
	QStringList tl = time.split( ":" );
	return tl[0].toInt() * Global::util->minutesPerHour + tl[1].toInt();
}

QString Game::intToTime( int time )
{
	int hour    = time / Global::util->minutesPerHour;
	int minute  = time - ( hour * Global::util->minutesPerHour );
	QString out = "";
	if ( hour < 10 )
		out += "0";
	out += QString::number( hour );
	out += ":";
	if ( minute < 10 )
		out += "0";
	out += QString::number( minute );
	return out;
}

void Game::processPlants()
{
	QList<Position> toRemove;
	for ( auto& p : m_world->plants() )
	{
		switch ( p.onTick( GameState::tick, GameState::dayChanged, GameState::seasonChanged ) )
		{
			case OnTickReturn::DESTROY:
				toRemove.push_back( p.getPos() );
				break;
			case OnTickReturn::UPDATE:
				break;
		}
	}
	for ( auto p : toRemove )
	{
		m_world->removePlant( p );
	}
}

void Game::autoSave()
{
	int daysToNext = Global::cfg->get( "DaysToNextAutoSave" ).toInt();

	if ( daysToNext == 0 )
	{
		Global::cfg->set( "Pause", true );
		emit signalStartAutoSave();
		emit signalPause( true );
		IO io( this, this );
		io.save( true );
		emit signalEndAutoSave();

		if ( Global::cfg->get( "AutoSaveContinue" ).toBool() )
		{
			emit signalPause( false );
			Global::cfg->set( "Pause", false );
		}

		Global::cfg->set( "DaysToNextAutoSave", Global::cfg->get( "AutoSaveInterval" ).toInt() - 1 );
	}
	else
	{
		--daysToNext;
		Global::cfg->set( "DaysToNextAutoSave", daysToNext );
	}
}

void Game::save()
{
	Global::cfg->set( "Pause", true );
	emit signalStartAutoSave();
	emit signalPause( true );
	IO io( this, this );
	io.save( true );
	emit signalEndAutoSave();

	if ( Global::cfg->get( "AutoSaveContinue" ).toBool() )
	{
		emit signalPause( false );
		Global::cfg->set( "Pause", false );
	}
}

	
GameSpeed Game::gameSpeed()
{
	return m_gameSpeed;
}

void Game::setGameSpeed( GameSpeed speed )
{
	m_gameSpeed = speed;
	switch( m_gameSpeed )
	{
		case GameSpeed::Normal:
			m_timer->setInterval( m_millisecondsSlow );
			break;
		case GameSpeed::Fast:
			m_timer->setInterval( m_millisecondsFast );
			break;
	}
}

bool Game::paused()
{
	return m_paused;
}

void Game::setPaused( bool value )
{
	m_paused = value;
	if (m_paused) 
	{
		m_timer->setInterval( m_millisecondsSlow*2 );
	}
	else {
		switch( m_gameSpeed )
		{
			case GameSpeed::Normal:
				m_timer->setInterval( m_millisecondsSlow );
				break;
			case GameSpeed::Fast:
				m_timer->setInterval( m_millisecondsFast );
				break;
		}
	}
	
}
void Game::setHeartbeatResponse( int value )
{
	//printf("heartbeatresponse %d", value);
	m_guiHeartbeatResponse = value;
}

Inventory*			Game::inv(){ return m_inv; }
ItemHistory*		Game::ih(){ return m_inv->itemHistory(); }
JobManager*			Game::jm(){ return m_jobManager; }
StockpileManager*	Game::spm(){ return m_spm; }
FarmingManager*		Game::fm(){ return m_farmingManager; }
WorkshopManager*	Game::wsm(){ return m_workshopManager; }
World*				Game::w(){ return m_world.get(); }
SpriteFactory*		Game::sf(){ return m_sf.get(); }
RoomManager*		Game::rm(){ return m_roomManager; }
GnomeManager*		Game::gm(){ return m_gnomeManager; }
CreatureManager*	Game::cm(){ return m_creatureManager; }
EventManager*		Game::em(){ return m_eventManager; }
MechanismManager*	Game::mcm(){ return m_mechanismManager; }
FluidManager*		Game::flm(){ return m_fluidManager; }
NeighborManager*	Game::nm(){ return m_neighborManager; }
MilitaryManager*	Game::mil(){ return m_militaryManager; }
PathFinder*			Game::pf(){ return m_pf.get(); }
World*				Game::world() { return m_world.get(); }
