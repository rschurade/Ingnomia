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
#include "eventconnector.h"

#include "../base/global.h"
#include "../game/job.h"
#include "../game/jobmanager.h"
#include "../game/plant.h"
#include "../game/world.h"

#include <QDebug>

EventConnector::EventConnector( QObject* parent ) :
	QObject( parent )
{
	m_tiAggregator           = new AggregatorTileInfo( this );
	m_spAggregator           = new AggregatorStockpile( this );
	m_wsAggregator           = new AggregatorWorkshop( this );
	m_acAggregator           = new AggregatorAgri( this );
	m_rAggregator            = new AggregatorRenderer( this );
	m_popAggregator          = new AggregatorPopulation( this );
	m_creatureInfoAggregator = new AggregatorCreatureInfo( this );
	m_debugAggregator        = new AggregatorDebug( this );
	m_neighborsAggregator    = new AggregatorNeighbors( this );
	m_militaryAggregator     = new AggregatorMilitary( this );
}

EventConnector::~EventConnector()
{
}

void EventConnector::onExit()
{
	emit signalExit();
}

void EventConnector::onWindowSize( int w, int h )
{
	emit signalWindowSize( w, h );
}

void EventConnector::onTimeAndDate( int minute, int hour, int day, QString season, int year, QString sunStatus )
{
	emit signalTimeAndDate( minute, hour, day, season, year, sunStatus );
}

void EventConnector::onViewLevel( int level )
{
	emit signalViewLevel( level );
}

void EventConnector::onUpdatePause()
{
	emit signalUpdatePause();
}

void EventConnector::onUpdateGameSpeed()
{
	emit signalUpdateGameSpeed();
}

void EventConnector::onKeyEsc()
{
	emit signalKeyEsc();
}

void EventConnector::onBuild()
{
	emit signalBuild();
}

void EventConnector::onTerrainCommand( unsigned int tileID, QString cmd )
{
	if ( cmd == "Mine" )
		Global::jm().addJob( "Mine", Position( tileID ), 0 );
	else if ( cmd == "Remove" )
		Global::jm().addJob( "RemoveFloor", Position( tileID ), 0 );
	else if ( cmd == "Fell" )
		Global::jm().addJob( "FellTree", Position( tileID ), 0 );
	else if ( cmd == "Destroy" )
		Global::jm().addJob( "RemovePlant", Position( tileID ), 0 );
	else if ( cmd == "Harvest" )
	{
		if ( Global::w().plants().contains( tileID ) )
		{
			Plant& plant = Global::w().plants()[tileID];
			if ( plant.isTree() )
			{
				Global::jm().addJob( "HarvestTree", Position( tileID ), 0 );
			}
			else
			{
				Global::jm().addJob( "Harvest", Position( tileID ), 0 );
			}
		}
	}
}

void EventConnector::onManageCommand( unsigned int tileID )
{
	Tile& tile = Global::w().getTile( tileID );

	TileFlag designation = tile.flags - ~( TileFlag::TF_WORKSHOP + TileFlag::TF_STOCKPILE + TileFlag::TF_GROVE + TileFlag::TF_FARM + TileFlag::TF_PASTURE + TileFlag::TF_ROOM );
	switch ( designation )
	{
		case TileFlag::TF_WORKSHOP:
			m_wsAggregator->onOpenWorkshopInfoOnTile( tileID );
			break;
		case TileFlag::TF_STOCKPILE:
		{
			m_spAggregator->onOpenStockpileInfoOnTile( tileID );
		}
		break;
		case TileFlag::TF_GROVE:
		case TileFlag::TF_FARM:
		case TileFlag::TF_PASTURE:
		{
			m_acAggregator->onOpen( designation, tileID );
		}
		break;
		case TileFlag::TF_ROOM:
			break;
	}
}
