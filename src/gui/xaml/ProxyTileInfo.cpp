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

#include <QDebug>
#include <QPainter>

ProxyTileInfo::ProxyTileInfo( QObject* parent ) :
	QObject( parent )
{
	connect( Global::eventConnector->aggregatorTileInfo(), &AggregatorTileInfo::signalUpdateTileInfo, this, &ProxyTileInfo::onUpdateTileInfo, Qt::QueuedConnection );
	connect( this, &ProxyTileInfo::signalTerrainCommand, Global::eventConnector, &EventConnector::onTerrainCommand, Qt::QueuedConnection );
	connect( this, &ProxyTileInfo::signalManageCommand, Global::eventConnector, &EventConnector::onManageCommand, Qt::QueuedConnection );
	connect( this, &ProxyTileInfo::signalRequestStockpileItems, Global::eventConnector->aggregatorTileInfo(), &AggregatorTileInfo::onRequestStockpileItems, Qt::QueuedConnection );

	connect( this, &ProxyTileInfo::signalSetTennant, Global::eventConnector->aggregatorTileInfo(), &AggregatorTileInfo::onSetTennant, Qt::QueuedConnection );
	connect( this, &ProxyTileInfo::signalSetAlarm, Global::eventConnector->aggregatorTileInfo(), &AggregatorTileInfo::onSetAlarm, Qt::QueuedConnection );

	connect( Global::eventConnector->aggregatorTileInfo(), &AggregatorTileInfo::signalUpdateSPInfo, this,  &ProxyTileInfo::onUpdateStockpileInfo, Qt::QueuedConnection );

	connect( this, &ProxyTileInfo::signalToggleMechActive, Global::eventConnector->aggregatorTileInfo(), &AggregatorTileInfo::onToggleMechActive, Qt::QueuedConnection );
	connect( this, &ProxyTileInfo::signalToggleMechInvert, Global::eventConnector->aggregatorTileInfo(), &AggregatorTileInfo::onToggleMechInvert, Qt::QueuedConnection );

	connect( this, &ProxyTileInfo::signalSetAutomatonRefuel, Global::eventConnector->aggregatorTileInfo(), &AggregatorTileInfo::onSetAutomatonRefuel, Qt::QueuedConnection );
	connect( this, &ProxyTileInfo::signalSetAutomatonCore, Global::eventConnector->aggregatorTileInfo(), &AggregatorTileInfo::onSetAutomatonCore, Qt::QueuedConnection );
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
	emit signalTerrainCommand( tileID, cmd );
}

void ProxyTileInfo::sendManageCommand( unsigned int tileID )
{
	emit signalManageCommand( tileID );
}

void ProxyTileInfo::requestStockpileItems( unsigned int tileID )
{
	emit signalRequestStockpileItems( tileID );
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
	emit signalSetTennant( designationID, gnomeID );
}

void ProxyTileInfo::setAlarm( unsigned int designationID, bool value )
{
	emit signalSetAlarm( designationID, value );
}

void ProxyTileInfo::toggleMechActive( unsigned int id )
{
	emit signalToggleMechActive( id );
}
	
void ProxyTileInfo::toggleMechInvert( unsigned int id )
{
	emit signalToggleMechInvert( id );
}

void ProxyTileInfo::setAutomatonRefuel( unsigned int id, bool refuel )
{

	emit signalSetAutomatonRefuel( id, refuel );
}
	
void ProxyTileInfo::setAutomatonCore( unsigned int id, QString core )
{
	emit signalSetAutomatonCore( id, core );
}