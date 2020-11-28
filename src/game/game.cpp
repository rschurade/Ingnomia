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
#include "../game/creaturemanager.h"
#include "../game/eventmanager.h"
#include "../game/farmingmanager.h"
#include "../game/fluidmanager.h"
#include "../game/gamemanager.h"
#include "../game/gnome.h"
#include "../game/gnomemanager.h"
#include "../game/inventory.h"
#include "../game/itemhistory.h"
#include "../game/mechanismmanager.h"
#include "../game/object.h"
#include "../game/plant.h"
#include "../game/roommanager.h"
#include "../game/stockpilemanager.h"
#include "../game/techtree.h"
#include "../game/workshopmanager.h"
#include "../game/world.h"
#include "../gfx/spritefactory.h"
#include "../gui/eventconnector.h"
#include "../gui/strings.h"
#include "../gui/aggregatorcreatureinfo.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QTimer>

#include <time.h>

Game::Game()
{
	qDebug() << "init game...";

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

	qDebug() << "init game done";

	m_speed = (int)GameSpeed::Normal;

	/*
	int lastlvl = 1;
	for( int i = 1; i < 10000000; ++i )
	{
		int lvl = Util::reverseFib( i );
		if ( lvl > lastlvl )
		{
			qDebug() << lvl << i;
			lastlvl = lvl;
		}
	}
	*/

	//TechTree tt;
	//tt.create();
}

Game::Game( bool isLoaded )
{
	m_millisecondsSlow = DB::select( "Value_", "Time", "MillisecondsSlow" ).toInt();
	m_millisecondsFast = DB::select( "Value_", "Time", "MillisecondsFast" ).toInt();

	Global::xpMod = Config::getInstance().get( "XpMod" ).toDouble();
}


Game::~Game()
{
}

void Game::start()
{
	qDebug() << "Starting game";

	if ( GameState::tick == 0 && !GameState::initialSave )
	{
		Config::getInstance().set( "DaysToNextAutoSave", 0 );
		autoSave();
		Config::getInstance().set( "Pause", true );
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
	bool pause = GameManager::getInstance().paused();

	emit sendOverlayMessage( 6, "tick " + QString::number( GameState::tick ) );

	QElapsedTimer timer;
	timer.start();
	int ms2 = 0;
	if ( !pause )
	{
		sendClock();

		// process grass
		Global::w().processGrass();
		// process plants
		processPlants();

		// process animals

		Global::cm().onTick( GameState::tick, GameState::seasonChanged, GameState::dayChanged, GameState::hourChanged, GameState::minuteChanged );

		// process gnomes
		QElapsedTimer timer2;
		timer2.start();
		Global::gm().onTick( GameState::tick, GameState::seasonChanged, GameState::dayChanged, GameState::hourChanged, GameState::minuteChanged );
		ms2 = timer2.elapsed();
		// process jobs
		Global::jm().onTick();
		// process stockpiles
		Global::spm().onTick( GameState::tick );
		Global::fm().onTick( GameState::tick, GameState::seasonChanged, GameState::dayChanged, GameState::hourChanged, GameState::minuteChanged );
		Global::wsm().onTick( GameState::tick );
		Global::rm().onTick( GameState::tick );
		Global::ih().onTick( GameState::dayChanged );
		Global::em().onTick( GameState::tick, GameState::seasonChanged, GameState::dayChanged, GameState::hourChanged, GameState::minuteChanged );
		Global::mcm().onTick( GameState::tick, GameState::seasonChanged, GameState::dayChanged, GameState::hourChanged, GameState::minuteChanged );
		Global::flm().onTick( GameState::tick, GameState::seasonChanged, GameState::dayChanged, GameState::hourChanged, GameState::minuteChanged );
		Global::nm().onTick( GameState::tick, GameState::seasonChanged, GameState::dayChanged, GameState::hourChanged, GameState::minuteChanged );

		Global::w().processWater();

		PathFinder::getInstance().findPaths();

		++GameState::tick;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// update gui
	//
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	auto updates = Global::w().updatedTiles();
	if ( !updates.empty() )
	{
		signalUpdateTileInfo( std::move( updates ) );
	}
	emit signalUpdateStockpile();

	EventConnector::getInstance().aggregatorCreatureInfo()->update();

	int ms        = timer.elapsed();
	m_maxLoopTime = qMax( ms2, m_maxLoopTime );

	auto numString = QString::number( ms );
	while ( numString.size() < 5 )
		numString.prepend( '0' );

	QString msg = "game loop time: " + numString;
	if ( Global::debugMode )
		msg += " ms (max gnome time:" + QString::number( m_maxLoopTime ) + "ms)";
	emit sendOverlayMessage( 3, msg );

	emit signalKingdomInfo( GameState::kingdomName, "", "", "" );

	if ( (int)GameManager::getInstance().gameSpeed() != (int)m_speed )
	{
		m_timer->stop();
		m_speed = (int)GameManager::getInstance().gameSpeed();
		if ( m_speed == (int)GameSpeed::Normal )
		{
			m_timer->start( m_millisecondsSlow );
		}
		else
		{
			m_timer->start( m_millisecondsFast );
		}
	}
}

void Game::sendClock()
{
	GameState::minuteChanged = false;
	GameState::hourChanged   = false;
	GameState::dayChanged    = false;
	GameState::seasonChanged = false;

	if ( GameState::tick % Util::ticksPerMinute == 0 )
	{
		++GameState::minute;
		GameState::minuteChanged = true;
	}
	if ( GameState::minute == Util::minutesPerHour )
	{
		GameState::minute = 0;
		++GameState::hour;
		GameState::hourChanged = true;
	}
	if ( GameState::hour == Util::hoursPerDay )
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

			Util::daysPerSeason = DB::select( "NumDays", "Seasons", nextSeason ).toInt();

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
		Global::sf().forceUpdate();
	}

	QString sunStatus;
	int currentTimeInt = GameState::hour * Util::minutesPerHour + GameState::minute;
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
	return tl[0].toInt() * Util::minutesPerHour + tl[1].toInt();
}

QString Game::intToTime( int time )
{
	int hour    = time / Util::minutesPerHour;
	int minute  = time - ( hour * Util::minutesPerHour );
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
	for ( auto& p : Global::w().plants() )
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
		Global::w().removePlant( p );
	}
}

void Game::autoSave()
{
	int daysToNext = Config::getInstance().get( "DaysToNextAutoSave" ).toInt();

	if ( daysToNext == 0 )
	{
		Config::getInstance().set( "Pause", true );
		emit signalStartAutoSave();
		emit signalPause( true );
		IO::save( true );
		emit signalEndAutoSave();

		if ( Config::getInstance().get( "AutoSaveContinue" ).toBool() )
		{
			emit signalPause( false );
			Config::getInstance().set( "Pause", false );
		}

		Config::getInstance().set( "DaysToNextAutoSave", Config::getInstance().get( "AutoSaveInterval" ).toInt() - 1 );
	}
	else
	{
		--daysToNext;
		Config::getInstance().set( "DaysToNextAutoSave", daysToNext );
	}
}

void Game::save()
{
	Config::getInstance().set( "Pause", true );
	emit signalStartAutoSave();
	emit signalPause( true );
	IO::save( true );
	emit signalEndAutoSave();

	if ( Config::getInstance().get( "AutoSaveContinue" ).toBool() )
	{
		emit signalPause( false );
		Config::getInstance().set( "Pause", false );
	}
}