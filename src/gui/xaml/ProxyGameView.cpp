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
	Global::eventConnector->signalKeyEsc.connect(&ProxyGameView::onKeyEscape, this); // TODO: Qt::QueuedConnection
this->signalPropagateEscape.connect(&EventConnector::onPropagateEscape, Global::eventConnector);

	Global::eventConnector->signalTimeAndDate.connect(&ProxyGameView::onTimeAndDate, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->signalKingdomInfo.connect(&ProxyGameView::onKingdomInfo, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->signalViewLevel.connect(&ProxyGameView::onViewLevel, this); // TODO: Qt::QueuedConnection
	
	Global::eventConnector->signalHeartbeat.connect(&ProxyGameView::onHeartbeat, this); // TODO: Qt::QueuedConnection

	Global::eventConnector->signalUpdatePause.connect(&ProxyGameView::onUpdatePause, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->signalUpdateGameSpeed.connect(&ProxyGameView::onUpdateGameSpeed, this); // TODO: Qt::QueuedConnection

	Global::eventConnector->signalBuild.connect(&ProxyGameView::onBuild, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorTileInfo()->signalShowTileInfo.connect(&ProxyGameView::onShowTileInfo, this); // TODO: Qt::QueuedConnection

	Global::eventConnector->aggregatorStockpile()->signalOpenStockpileWindow.connect(&ProxyGameView::onOpenStockpileWindow, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorWorkshop()->signalOpenWorkshopWindow.connect(&ProxyGameView::onOpenWorkshopWindow, this); // TODO: Qt::QueuedConnection
	//connect( Global::eventConnector->aggregatorAgri(), &AggregatorAgri::signalShowAgri, this, &ProxyGameView::onOpenAgriWindow, Qt::QueuedConnection );

	this->signalCloseStockpileWindow.connect(&AggregatorStockpile::onCloseWindow, Global::eventConnector->aggregatorStockpile()); // TODO: Qt::QueuedConnection
	this->signalCloseWorkshopWindow.connect(&AggregatorWorkshop::onCloseWindow, Global::eventConnector->aggregatorWorkshop()); // TODO: Qt::QueuedConnection
	this->signalCloseAgricultureWindow.connect(&AggregatorAgri::onCloseWindow, Global::eventConnector->aggregatorAgri()); // TODO: Qt::QueuedConnection

	this->signalRequestPopulationUpdate.connect(&AggregatorPopulation::onRequestPopulationUpdate, Global::eventConnector->aggregatorPopulation()); // TODO: Qt::QueuedConnection
	this->signalRequestCreatureUpdate.connect(&AggregatorCreatureInfo::onRequestCreatureUpdate, Global::eventConnector->aggregatorCreatureInfo()); // TODO: Qt::QueuedConnection
	this->signalRequestNeighborsUpdate.connect(&AggregatorNeighbors::onRequestNeighborsUpdate, Global::eventConnector->aggregatorNeighbors()); // TODO: Qt::QueuedConnection
	this->signalRequestMissionsUpdate.connect(&AggregatorNeighbors::onRequestMissions, Global::eventConnector->aggregatorNeighbors()); // TODO: Qt::QueuedConnection

	this->signalRequestMilitaryUpdate.connect(&AggregatorMilitary::onRequestMilitary, Global::eventConnector->aggregatorMilitary()); // TODO: Qt::QueuedConnection
	this->signalRequestInventoryUpdate.connect(&AggregatorInventory::onRequestCategories, Global::eventConnector->aggregatorInventory()); // TODO: Qt::QueuedConnection

	Global::eventConnector->signalEvent.connect(&ProxyGameView::onEvent, this); // TODO: Qt::QueuedConnection
	this->signalEventAnswer.connect(&EventConnector::onAnswer, Global::eventConnector); // TODO: Qt::QueuedConnection

	this->signalSetPaused.connect(&EventConnector::onSetPause, Global::eventConnector); // TODO: Qt::QueuedConnection
	this->signalSetGameSpeed.connect(&EventConnector::onSetGameSpeed, Global::eventConnector); // TODO: Qt::QueuedConnection

	this->signalSetRenderOptions.connect(&EventConnector::onSetRenderOptions, Global::eventConnector); // TODO: Qt::QueuedConnection
	Global::eventConnector->signalUpdateRenderOptions.connect(&ProxyGameView::onUpdateRenderOptions, this); // TODO: Qt::QueuedConnection

	this->signalRequestBuildItems.connect(&AggregatorInventory::onRequestBuildItems, Global::eventConnector->aggregatorInventory()); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorInventory()->signalBuildItems.connect(&ProxyGameView::onBuildItems, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorInventory()->signalWatchList.connect(&ProxyGameView::onWatchList, this); // TODO: Qt::QueuedConnection
	this->signalRequestCmdBuild.connect(&EventConnector::onCmdBuild, Global::eventConnector); // TODO: Qt::QueuedConnection
	this->signalSetSelectionAction.connect(&EventConnector::onSetSelectionAction, Global::eventConnector); // TODO: Qt::QueuedConnection
	this->signalHeartbeatResponse.connect(&EventConnector::onHeartbeatResponse, Global::eventConnector); // TODO: Qt::QueuedConnection
	
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
	signalHeartbeatResponse(value);

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
	signalCloseStockpileWindow();
}

void ProxyGameView::closeWorkshopWindow()
{
	signalCloseWorkshopWindow();
}

void ProxyGameView::closeAgricultureWindow()
{
	signalCloseAgricultureWindow();
}

void ProxyGameView::requestPopulationUpdate()
{
	signalRequestPopulationUpdate();
}

void ProxyGameView::requestNeighborsUpdate()
{
	signalRequestNeighborsUpdate();
}

void ProxyGameView::requestMissionsUpdate()
{
	signalRequestMissionsUpdate();
}

void ProxyGameView::requestCreatureUpdate( unsigned int id )
{
	signalRequestCreatureUpdate( id );
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
	signalEventAnswer( eventID, answer );
}

void ProxyGameView::requestMilitaryUpdate()
{
	signalRequestMilitaryUpdate();
}

void ProxyGameView::requestInventoryUpdate()
{
	signalRequestInventoryUpdate();
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
	signalPropagateEscape();
}

void ProxyGameView::setGameSpeed( GameSpeed speed )
{
	signalSetGameSpeed( speed );
}
	
void ProxyGameView::setPaused( bool paused )
{
	signalSetPaused( paused );
}

void ProxyGameView::setRenderOptions( bool designations, bool jobs, bool walls, bool axles )
{
	signalSetRenderOptions( designations, jobs, walls, axles );
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
	signalRequestBuildItems( buildSelection, category );
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
	signalRequestCmdBuild( type, param, item, mats );
}

void ProxyGameView::setSelectionAction( QString action )
{
	signalSetSelectionAction( action );
}

void ProxyGameView::onWatchList( const QList<GuiWatchedItem>& watchlist )
{
	if( m_parent )
	{
		m_parent->updateWatchList( watchlist );
	}
}