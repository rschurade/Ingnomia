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
#include "ProxyTileInfo.h"

#include "../../base/db.h"
#include "../../base/gamestate.h"
#include "../../gfx/sprite.h"
#include "../../gfx/spritefactory.h"
#include "../../base/global.h"
#include "../eventconnector.h"
#include "TileInfoModel.h"

#include <QPainter>

ProxyTileInfo::ProxyTileInfo( QObject* parent ) :
	QObject( parent )
{
	Global::eventConnector->aggregatorTileInfo()->signalUpdateTileInfo.connect(&ProxyTileInfo::onUpdateTileInfo, this); // TODO: Qt::QueuedConnection
	this->signalTerrainCommand.connect(&EventConnector::onTerrainCommand, Global::eventConnector); // TODO: Qt::QueuedConnection
	this->signalManageCommand.connect(&EventConnector::onManageCommand, Global::eventConnector); // TODO: Qt::QueuedConnection
	this->signalRequestStockpileItems.connect(&AggregatorTileInfo::onRequestStockpileItems, Global::eventConnector->aggregatorTileInfo()); // TODO: Qt::QueuedConnection

	this->signalSetTennant.connect(&AggregatorTileInfo::onSetTennant, Global::eventConnector->aggregatorTileInfo()); // TODO: Qt::QueuedConnection
	this->signalSetAlarm.connect(&AggregatorTileInfo::onSetAlarm, Global::eventConnector->aggregatorTileInfo()); // TODO: Qt::QueuedConnection

	Global::eventConnector->aggregatorTileInfo()->signalUpdateSPInfo.connect(&ProxyTileInfo::onUpdateStockpileInfo, this); // TODO: Qt::QueuedConnection

	this->signalToggleMechActive.connect(&AggregatorTileInfo::onToggleMechActive, Global::eventConnector->aggregatorTileInfo()); // TODO: Qt::QueuedConnection
	this->signalToggleMechInvert.connect(&AggregatorTileInfo::onToggleMechInvert, Global::eventConnector->aggregatorTileInfo()); // TODO: Qt::QueuedConnection

	this->signalSetAutomatonRefuel.connect(&AggregatorTileInfo::onSetAutomatonRefuel, Global::eventConnector->aggregatorTileInfo()); // TODO: Qt::QueuedConnection
	this->signalSetAutomatonCore.connect(&AggregatorTileInfo::onSetAutomatonCore, Global::eventConnector->aggregatorTileInfo()); // TODO: Qt::QueuedConnection
}

ProxyTileInfo::~ProxyTileInfo()
{
}

void ProxyTileInfo::setParent( IngnomiaGUI::TileInfoModel* parent )
{
	m_parent = parent;
}

void ProxyTileInfo::onUpdateTileInfo( const GuiTileInfo& info )
{
	if ( m_parent )
	{
		m_parent->onUpdateTileInfo( info );
	}
}

void ProxyTileInfo::sendTerrainCommand( unsigned int tileID, QString cmd )
{
	signalTerrainCommand( tileID, cmd );
}

void ProxyTileInfo::sendManageCommand( unsigned int tileID )
{
	signalManageCommand( tileID );
}

void ProxyTileInfo::requestStockpileItems( unsigned int tileID )
{
	signalRequestStockpileItems( tileID );
}

void ProxyTileInfo::onUpdateStockpileInfo( const GuiStockpileInfo& info )
{
	if ( m_parent )
	{
		m_parent->updateMiniStockpile( info );
	}
}

void ProxyTileInfo::setTennant( unsigned int designationID, unsigned int gnomeID )
{
	signalSetTennant( designationID, gnomeID );
}

void ProxyTileInfo::setAlarm( unsigned int designationID, bool value )
{
	signalSetAlarm( designationID, value );
}

void ProxyTileInfo::toggleMechActive( unsigned int id )
{
	signalToggleMechActive( id );
}
	
void ProxyTileInfo::toggleMechInvert( unsigned int id )
{
	signalToggleMechInvert( id );
}

void ProxyTileInfo::setAutomatonRefuel( unsigned int id, bool refuel )
{

	signalSetAutomatonRefuel( id, refuel );
}
	
void ProxyTileInfo::setAutomatonCore( unsigned int id, QString core )
{
	signalSetAutomatonCore( id, core );
}