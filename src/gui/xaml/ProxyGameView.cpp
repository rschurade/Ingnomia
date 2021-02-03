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
#include "ProxyGameView.h"

#include "../aggregatoragri.h"
#include "../aggregatortileinfo.h"
#include "../aggregatorneighbors.h"
#include "../aggregatorpopulation.h"
#include "../aggregatorcreatureinfo.h"
#include "../aggregatormilitary.h"
#include "../aggregatorworkshop.h"
#include "../eventconnector.h"

#include "ViewModel.h"

#include <QDebug>
#include <QPainter>

ProxyGameView::ProxyGameView( QObject* parent ) :
	QObject( parent )
{
	connect( Global::eventConnector, &EventConnector::signalKeyEsc, this, &ProxyGameView::onKeyEscape, Qt::QueuedConnection );
	connect( this, &ProxyGameView::signalPropagateEscape, Global::eventConnector, &EventConnector::onPropagateEscape );

	connect( Global::eventConnector, &EventConnector::signalTimeAndDate, this, &ProxyGameView::onTimeAndDate, Qt::QueuedConnection );
	connect( Global::eventConnector, &EventConnector::signalKingdomInfo, this, &ProxyGameView::onKingdomInfo, Qt::QueuedConnection );
	connect( Global::eventConnector, &EventConnector::signalViewLevel, this, &ProxyGameView::onViewLevel, Qt::QueuedConnection );
	
	connect( Global::eventConnector, &EventConnector::signalHeartbeat, this, &ProxyGameView::onHeartbeat, Qt::QueuedConnection );

	connect( Global::eventConnector, &EventConnector::signalUpdatePause, this, &ProxyGameView::onUpdatePause, Qt::QueuedConnection );
	connect( Global::eventConnector, &EventConnector::signalUpdateGameSpeed, this, &ProxyGameView::onUpdateGameSpeed, Qt::QueuedConnection );

	connect( Global::eventConnector, &EventConnector::signalBuild, this, &ProxyGameView::onBuild, Qt::QueuedConnection );
	connect( Global::eventConnector->aggregatorTileInfo(), &AggregatorTileInfo::signalShowTileInfo, this, &ProxyGameView::onShowTileInfo, Qt::QueuedConnection );

	connect( Global::eventConnector->aggregatorStockpile(), &AggregatorStockpile::signalOpenStockpileWindow, this, &ProxyGameView::onOpenStockpileWindow, Qt::QueuedConnection );
	connect( Global::eventConnector->aggregatorWorkshop(), &AggregatorWorkshop::signalOpenWorkshopWindow, this, &ProxyGameView::onOpenWorkshopWindow, Qt::QueuedConnection );
	//connect( Global::eventConnector->aggregatorAgri(), &AggregatorAgri::signalShowAgri, this, &ProxyGameView::onOpenAgriWindow, Qt::QueuedConnection );

	connect( this, &ProxyGameView::signalCloseStockpileWindow, Global::eventConnector->aggregatorStockpile(), &AggregatorStockpile::onCloseWindow, Qt::QueuedConnection );
	connect( this, &ProxyGameView::signalCloseWorkshopWindow, Global::eventConnector->aggregatorWorkshop(), &AggregatorWorkshop::onCloseWindow, Qt::QueuedConnection );
	connect( this, &ProxyGameView::signalCloseAgricultureWindow, Global::eventConnector->aggregatorAgri(), &AggregatorAgri::onCloseWindow, Qt::QueuedConnection );

	connect( this, &ProxyGameView::signalRequestPopulationUpdate, Global::eventConnector->aggregatorPopulation(), &AggregatorPopulation::onRequestPopulationUpdate, Qt::QueuedConnection );
	connect( this, &ProxyGameView::signalRequestCreatureUpdate, Global::eventConnector->aggregatorCreatureInfo(), &AggregatorCreatureInfo::onRequestCreatureUpdate, Qt::QueuedConnection );
	connect( this, &ProxyGameView::signalRequestNeighborsUpdate, Global::eventConnector->aggregatorNeighbors(), &AggregatorNeighbors::onRequestNeighborsUpdate, Qt::QueuedConnection );
	connect( this, &ProxyGameView::signalRequestMissionsUpdate, Global::eventConnector->aggregatorNeighbors(), &AggregatorNeighbors::onRequestMissions, Qt::QueuedConnection );

	connect( this, &ProxyGameView::signalRequestMilitaryUpdate, Global::eventConnector->aggregatorMilitary(), &AggregatorMilitary::onRequestMilitary, Qt::QueuedConnection );
	connect( this, &ProxyGameView::signalRequestInventoryUpdate, Global::eventConnector->aggregatorInventory(), &AggregatorInventory::onRequestCategories, Qt::QueuedConnection );

	connect( Global::eventConnector, &EventConnector::signalEvent, this, &ProxyGameView::onEvent, Qt::QueuedConnection );
	connect( this, &ProxyGameView::signalEventAnswer, Global::eventConnector, &EventConnector::onAnswer, Qt::QueuedConnection );

	connect( this, &ProxyGameView::signalSetPaused, Global::eventConnector, &EventConnector::onSetPause, Qt::QueuedConnection );
	connect( this, &ProxyGameView::signalSetGameSpeed, Global::eventConnector, &EventConnector::onSetGameSpeed, Qt::QueuedConnection );

	connect( this, &ProxyGameView::signalSetRenderOptions, Global::eventConnector, &EventConnector::onSetRenderOptions, Qt::QueuedConnection );
	connect( Global::eventConnector, &EventConnector::signalUpdateRenderOptions, this, &ProxyGameView::onUpdateRenderOptions, Qt::QueuedConnection );

	connect( this, &ProxyGameView::signalRequestBuildItems, Global::eventConnector->aggregatorInventory(), &AggregatorInventory::onRequestBuildItems, Qt::QueuedConnection );
	connect( Global::eventConnector->aggregatorInventory(), &AggregatorInventory::signalBuildItems, this,  &ProxyGameView::onBuildItems, Qt::QueuedConnection );
	connect( Global::eventConnector->aggregatorInventory(), &AggregatorInventory::signalWatchList, this,  &ProxyGameView::onWatchList, Qt::QueuedConnection );
	connect( this, &ProxyGameView::signalRequestCmdBuild, Global::eventConnector, &EventConnector::onCmdBuild, Qt::QueuedConnection );
	connect( this, &ProxyGameView::signalSetSelectionAction, Global::eventConnector, &EventConnector::onSetSelectionAction, Qt::QueuedConnection );
	connect( this, &ProxyGameView::signalHeartbeatResponse, Global::eventConnector, &EventConnector::onHeartbeatResponse, Qt::QueuedConnection );
	
}

ProxyGameView::~ProxyGameView()
{
}

void ProxyGameView::setParent( IngnomiaGUI::GameModel* parent )
{
	m_parent = parent;
}

void ProxyGameView::onTimeAndDate( int minute, int hour, int day, QString season, int year, QString sunStatus )
{
	if ( m_parent )
	{
		m_parent->setTimeAndDate( minute, hour, day, season, year, sunStatus );
	}
}

void ProxyGameView::onKingdomInfo( QString name, QString info1, QString info2, QString info3 )
{
	if( m_parent )
	{
		m_parent->updateKingdomInfo( name, info1, info2, info3 );
	}
}

void ProxyGameView::onHeartbeat( int value )
{
	emit signalHeartbeatResponse(value);

}

void ProxyGameView::onViewLevel( int level )
{
	if ( m_parent )
	{
		m_parent->setViewLevel( level );
	}
}

void ProxyGameView::onUpdatePause( bool value )
{
	if ( m_parent )
	{
		m_parent->updatePause( value );
	}
}

void ProxyGameView::onUpdateGameSpeed( GameSpeed speed )
{
	if ( m_parent )
	{
		m_parent->updateGameSpeed( speed );
	}
}

void ProxyGameView::onBuild()
{
	if ( m_parent )
	{
		m_parent->onBuild();
	}
}

void ProxyGameView::onShowTileInfo( unsigned int tileID )
{
	if ( m_parent )
	{
		m_parent->onShowTileInfo( tileID );
	}
}

void ProxyGameView::onOpenStockpileWindow( unsigned int stockpileID )
{
	if ( m_parent )
	{
		m_parent->onShowStockpileInfo( stockpileID );
	}
}

void ProxyGameView::onOpenWorkshopWindow( unsigned int workshopID )
{
	if ( m_parent )
	{
		m_parent->onShowWorkshopInfo( workshopID );
	}
}

void ProxyGameView::onOpenAgriWindow( unsigned int ID )
{
	if ( m_parent )
	{
		m_parent->onShowAgriculture( ID );
	}
}

void ProxyGameView::closeStockpileWindow()
{
	emit signalCloseStockpileWindow();
}

void ProxyGameView::closeWorkshopWindow()
{
	emit signalCloseWorkshopWindow();
}

void ProxyGameView::closeAgricultureWindow()
{
	emit signalCloseAgricultureWindow();
}

void ProxyGameView::requestPopulationUpdate()
{
	emit signalRequestPopulationUpdate();
}

void ProxyGameView::requestNeighborsUpdate()
{
	emit signalRequestNeighborsUpdate();
}

void ProxyGameView::requestMissionsUpdate()
{
	emit signalRequestMissionsUpdate();
}

void ProxyGameView::requestCreatureUpdate( unsigned int id )
{
	emit signalRequestCreatureUpdate( id );
}

void ProxyGameView::onEvent( unsigned int id, QString title, QString msg, bool pause, bool yesno )
{
	if( m_parent )
	{
		m_parent->eventMessage( id, title, msg, pause, yesno );
	}
}

void ProxyGameView::sendEventAnswer( unsigned int eventID, bool answer )
{
	emit signalEventAnswer( eventID, answer );
}

void ProxyGameView::requestMilitaryUpdate()
{
	emit signalRequestMilitaryUpdate();
}

void ProxyGameView::requestInventoryUpdate()
{
	emit signalRequestInventoryUpdate();
}


void ProxyGameView::onKeyEscape()
{
	if( m_parent )
	{
		m_parent->OnCmdBack( nullptr );
	}
}
	
void ProxyGameView::propagateEscape()
{
	emit signalPropagateEscape();
}

void ProxyGameView::setGameSpeed( GameSpeed speed )
{
	emit signalSetGameSpeed( speed );
}
	
void ProxyGameView::setPaused( bool paused )
{
	emit signalSetPaused( paused );
}

void ProxyGameView::setRenderOptions( bool designations, bool jobs, bool walls, bool axles )
{
	emit signalSetRenderOptions( designations, jobs, walls, axles );
}

void ProxyGameView::onUpdateRenderOptions( bool designation, bool jobs, bool walls, bool axles )
{
	if( m_parent )
	{
		m_parent->updateRenderOptions( designation, jobs, walls, axles );
	}
}

void ProxyGameView::requestBuildItems( BuildSelection buildSelection, QString category )
{
	emit signalRequestBuildItems( buildSelection, category );
}

void ProxyGameView::onBuildItems( const QList<GuiBuildItem>& items )
{
	if( m_parent )
	{
		m_parent->updateBuildItems( items );
	}
}

void ProxyGameView::requestCmdBuild( BuildItemType type, QString param, QString item, QStringList mats )
{
	emit signalRequestCmdBuild( type, param, item, mats );
}

void ProxyGameView::setSelectionAction( QString action )
{
	emit signalSetSelectionAction( action );
}

void ProxyGameView::onWatchList( const QList<GuiWatchedItem>& watchlist )
{
	if( m_parent )
	{
		m_parent->updateWatchList( watchlist );
	}
}