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

#include "aggregatoragri.h"
#include "aggregatorcreatureinfo.h"
#include "aggregatordebug.h"
#include "aggregatorinventory.h"
#include "aggregatorpopulation.h"
#include "aggregatorrenderer.h"
#include "aggregatorstockpile.h"
#include "aggregatortileinfo.h"
#include "aggregatorworkshop.h"
#include "aggregatorneighbors.h"
#include "aggregatormilitary.h"
#include "aggregatorsettings.h"
#include "aggregatorloadgame.h"

#include "../base/db.h"
#include "../base/config.h"
#include "../base/global.h"
#include "../base/selection.h"
#include "../game/gamemanager.h"
#include "../game/game.h"
#include "../game/job.h"
#include "../game/jobmanager.h"
#include "../game/plant.h"
#include "../game/world.h"

#include <QDebug>

EventConnector::EventConnector( GameManager* parent ) :
	gm( parent ),
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
	m_settingsAggregator	 = new AggregatorSettings( this );
	m_inventoryAggregator    = new AggregatorInventory( this );
	m_loadGameAggregator	 = new AggregatorLoadGame( this );
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

void EventConnector::onKingdomInfo( QString name, QString info1, QString info2, QString info3 )
{
	emit signalKingdomInfo( name, info1, info2, info3 );
}


void EventConnector::onViewLevel( int level )
{
	emit signalViewLevel( level );
}

void EventConnector::onSetPause( bool paused )
{
	gm->setPaused( paused );
}

void EventConnector::onSetGameSpeed( GameSpeed speed )
{
	gm->setGameSpeed( speed );
}

void EventConnector::onKeyPress( int key )
{
	switch ( key )
	{
		case Qt::Key_Escape:
			emit signalKeyEsc();
			break;
	}
}

void EventConnector::onTogglePause()
{
	emit signalUpdatePause( !gm->paused() );
}

void EventConnector::onPropagateEscape()
{
	emit signalPropagateKeyEsc();
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
		if ( gm->game() && gm->game()->world() && gm->game()->world()->plants().contains( tileID ) )
		{
			Plant& plant = gm->game()->world()->plants()[tileID];
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
	if ( gm->game() && gm->game()->world() )
	{
		Tile& tile = gm->game()->world()->getTile( tileID );

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
}


void EventConnector::onSetRenderOptions( bool designations, bool jobs, bool walls, bool axles )
{
	
	Global::wallsLowered = walls;
	Global::showDesignations = designations;
	Global::showJobs = jobs;
	Global::showAxles = axles;
	
	
}

void EventConnector::onUpdateRenderOptions()
{
	emit signalUpdateRenderOptions( Global::showDesignations, Global::showJobs, Global::wallsLowered, Global::showAxles );
}

void EventConnector::emitStartGame()
{
	emit startGame();
}
	
void EventConnector::emitStopGame()
{
	emit stopGame();
}

void EventConnector::emitInitView()
{
	emit signalInitView();
}

void EventConnector::emitInMenu( bool value )
{
	emit signalInMenu( value );
}
	
void EventConnector::onStartNewGame()
{
	gm->startNewGame();
}

void EventConnector::onContinueLastGame()
{
	gm->continueLastGame();
}

void EventConnector::onLoadGame( QString folder )
{
	gm->loadGame( folder );
}

void EventConnector::onSaveGame()
{
	gm->saveGame();
}

void EventConnector::onSetShowMainMenu( bool value )
{
	gm->setShowMainMenu( value );
}

void EventConnector::sendResume()
{
	emit signalResume();
}
	
void EventConnector::sendLoadGameDone( bool value )
{
	emit signalLoadGameDone( value );
}

void EventConnector::emitPause( bool paused )
{
	emit signalUpdatePause( paused );
}
	
void EventConnector::emitGameSpeed( GameSpeed speed )
{
	emit signalUpdateGameSpeed( speed );
}

void EventConnector::onSetSelectionAction( QString action )
{
	Selection::getInstance().setAction( action );
}
	
void EventConnector::onSetSelectionItem( QString item )
{
	Selection::getInstance().setItemID( item );
}

void EventConnector::onSetSelectionMaterials( QStringList mats )
{
	Selection::getInstance().setMaterials( mats );
}

void EventConnector::onCmdBuild( BuildItemType type, QString param, QString item, QStringList mats )
{
	switch ( type )
	{
		case BuildItemType::Workshop:
		{
			Selection::getInstance().setAction( "BuildWorkshop" );
		}
		break;
		case BuildItemType::Terrain:
		{
			QString type = DB::select( "Type", "Constructions", item ).toString();
			
			if( !param.isEmpty() )
			{
				if( param == "FillHole" )
				{
					Selection::getInstance().setAction( param );
				}
				else
				{
					Selection::getInstance().setAction( param + type );
				}
			}
			else
			{
				if( type == "Stairs" && item == "Scaffold" )
				{
					Selection::getInstance().setAction( "BuildScaffold" );
				}
				else
				{
					Selection::getInstance().setAction( "Build" + type );
				}
			}
		}
		break;
		case BuildItemType::Item:
		{
			Selection::getInstance().setAction( "BuildItem" );
		}
		break;
	}

	Selection::getInstance().setMaterials( mats );
	Selection::getInstance().setItemID( item );

	emit signalBuild();
}