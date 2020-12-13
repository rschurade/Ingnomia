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
    connect( Global::eventConnector->aggregatorInventory(), &AggregatorInventory::signalInventoryGroups, this, &InventoryProxy::onGroupsUpdate, Qt::QueuedConnection );
    connect( Global::eventConnector->aggregatorInventory(), &AggregatorInventory::signalInventoryItems, this, &InventoryProxy::onItemsUpdate, Qt::QueuedConnection );

    connect( this, &InventoryProxy::signalRequestCategories, Global::eventConnector->aggregatorInventory(), &AggregatorInventory::onRequestCategories, Qt::QueuedConnection );
    connect( this, &InventoryProxy::signalRequestGroups, Global::eventConnector->aggregatorInventory(), &AggregatorInventory::onRequestGroups, Qt::QueuedConnection );
    connect( this, &InventoryProxy::signalRequestItems, Global::eventConnector->aggregatorInventory(), &AggregatorInventory::onRequestItems, Qt::QueuedConnection );
}

void InventoryProxy::setParent( IngnomiaGUI::InventoryModel* parent )
{
	m_parent = parent;
}

    
void InventoryProxy::requestCategories()
{
    emit signalRequestCategories();
}

void InventoryProxy::requestGroups( QString category )
{
    emit signalRequestGroups( category );
}

void InventoryProxy::requestItems( QString category, QString group )
{
    emit signalRequestItems( category, group );
}

	
void InventoryProxy::onCategoryUpdate( const QList<GuiInventoryCategory>& categories )
{
    if( m_parent )
	{
        m_parent->updateCategories( categories );
	}
}

void InventoryProxy::onGroupsUpdate( const QList<GuiInventoryGroup>& groups )
{
    if( m_parent )
	{
        m_parent->updateGroups( groups );
	}
}

void InventoryProxy::onItemsUpdate( const QList<GuiInventoryItem>& items )
{
    if( m_parent )
	{
        m_parent->updateItems( items );
	}
}