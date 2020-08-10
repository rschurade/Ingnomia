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
#include "../eventconnector.h"
#include "TileInfoModel.h"

#include <QDebug>
#include <QPainter>

ProxyTileInfo::ProxyTileInfo( QObject* parent ) :
	QObject( parent )
{
	connect( EventConnector::getInstance().aggregatorTileInfo(), &AggregatorTileInfo::signalUpdateTileInfo, this, &ProxyTileInfo::onUpdateTileInfo, Qt::QueuedConnection );
	connect( this, &ProxyTileInfo::signalTerrainCommand, &EventConnector::getInstance(), &EventConnector::onTerrainCommand, Qt::QueuedConnection );
	connect( this, &ProxyTileInfo::signalManageCommand, &EventConnector::getInstance(), &EventConnector::onManageCommand, Qt::QueuedConnection );
	connect( this, &ProxyTileInfo::signalRequestStockpileItems, EventConnector::getInstance().aggregatorTileInfo(), &AggregatorTileInfo::onRequestStockpileItems, Qt::QueuedConnection );

	connect( EventConnector::getInstance().aggregatorTileInfo(), &AggregatorTileInfo::signalUpdateSPInfo, this,  &ProxyTileInfo::onUpdateStockpileInfo, Qt::QueuedConnection );
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
