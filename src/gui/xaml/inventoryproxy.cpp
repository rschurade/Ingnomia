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
#include "inventoryproxy.h"

#include "../../base/global.h"
#include "../eventconnector.h"

#include <QDebug>

InventoryProxy::InventoryProxy( QObject* parent ) :
	QObject( parent )
{
	
    connect( Global::eventConnector->aggregatorInventory(), &AggregatorInventory::signalInventoryCategories, this, &InventoryProxy::onCategoryUpdate, Qt::QueuedConnection );

    connect( this, &InventoryProxy::signalRequestCategories, Global::eventConnector->aggregatorInventory(), &AggregatorInventory::onRequestCategories, Qt::QueuedConnection );
    connect( this, &InventoryProxy::signalSetActive, Global::eventConnector->aggregatorInventory(), &AggregatorInventory::onSetActive, Qt::QueuedConnection );
}

void InventoryProxy::setParent( IngnomiaGUI::InventoryModel* parent )
{
	m_parent = parent;
}

    
void InventoryProxy::requestCategories()
{
    emit signalRequestCategories();
}
	
void InventoryProxy::onCategoryUpdate( const QList<GuiInventoryCategory>& categories )
{
    if( m_parent )
	{
        m_parent->updateCategories( categories );
	}
}

void InventoryProxy::setActive( bool active, const GuiWatchedItem& gwi )
{
    emit signalSetActive( active, gwi );
}