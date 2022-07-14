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
#include "StockpileProxy.h"

#include "../../base/db.h"
#include "../../base/gamestate.h"
#include "../../gfx/sprite.h"
#include "../../gfx/spritefactory.h"
#include "../../base/global.h"
#include "../eventconnector.h"
#include "TileInfoModel.h"

#include <QPainter>

StockpileProxy::StockpileProxy( QObject* parent ) :
	QObject( parent )
{
	Global::eventConnector->aggregatorStockpile()->signalUpdateInfo.connect(&StockpileProxy::onUpdateInfo, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorStockpile()->signalUpdateContent.connect(&StockpileProxy::onUpdateContent, this); // TODO: Qt::QueuedConnection

	this->signalSetBasicOptions.connect(&AggregatorStockpile::onSetBasicOptions, Global::eventConnector->aggregatorStockpile()); // TODO: Qt::QueuedConnection
	this->signalSetActive.connect(&AggregatorStockpile::onSetActive, Global::eventConnector->aggregatorStockpile()); // TODO: Qt::QueuedConnection
}

StockpileProxy::~StockpileProxy()
{
}

void StockpileProxy::setParent( IngnomiaGUI::StockpileModel* parent )
{
	m_parent = parent;
}

void StockpileProxy::onUpdateInfo( const GuiStockpileInfo& info )
{
	if ( m_parent )
	{
		m_parent->onUpdateInfo( info );
	}
}

void StockpileProxy::onUpdateContent( const GuiStockpileInfo& info )
{
	if ( m_parent )
	{
		m_parent->onUpdateContent( info );
	}
}

void StockpileProxy::setBasicOptions( unsigned int stockpileID, QString name, int priority, bool suspended, bool pull, bool allowPull )
{
	signalSetBasicOptions( stockpileID, name, priority, suspended, pull, allowPull );
}

void StockpileProxy::setActive( unsigned int stockpileID, bool active, QString category, QString group, QString item, QString material )
{
	signalSetActive( stockpileID, active, category, group, item, material );
}
