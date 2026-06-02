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
/** @file debugproxy.cpp
 *  @brief DebugProxy implementation: wires every signal/slot pair between DebugModel and
 *         AggregatorDebug, then provides thin pass-throughs.
 */
#include "debugproxy.h"

#include "../../base/global.h"
#include "../eventconnector.h"

#include <NsGui/ObservableCollection.h>
#include <NsCore/Ptr.h>

#include <QDebug>

/// @brief Constructs the proxy and connects every signal pair between DebugModel and AggregatorDebug.
DebugProxy::DebugProxy( QObject* parent ) :
	QObject( parent )
{
	auto agg = Global::eventConnector->aggregatorDebug();
	connect( this, &DebugProxy::signalSpawnCreature, agg, &AggregatorDebug::onSpawnCreature, Qt::QueuedConnection );
	connect( this, &DebugProxy::signalSetWindowSize, agg, &AggregatorDebug::onSetWindowSize, Qt::QueuedConnection );
	connect( this, &DebugProxy::signalSetNeed, agg, &AggregatorDebug::onSetNeed, Qt::QueuedConnection );
	connect( this, &DebugProxy::signalKillGnome, agg, &AggregatorDebug::onKillGnome, Qt::QueuedConnection );
	connect( this, &DebugProxy::signalSpawnItem, agg, &AggregatorDebug::onSpawnItem, Qt::QueuedConnection );
	connect( this, &DebugProxy::signalSpawnCompositeItem, agg, &AggregatorDebug::onSpawnCompositeItem, Qt::QueuedConnection );
	connect( this, &DebugProxy::signalRequestGnomeList, agg, &AggregatorDebug::onRequestGnomeList, Qt::QueuedConnection );
	connect( this, &DebugProxy::signalRequestItemGroups, agg, &AggregatorDebug::onRequestItemGroups, Qt::QueuedConnection );
	connect( this, &DebugProxy::signalRequestItems, agg, &AggregatorDebug::onRequestItems, Qt::QueuedConnection );
	connect( this, &DebugProxy::signalRequestMaterials, agg, &AggregatorDebug::onRequestMaterials, Qt::QueuedConnection );
	connect( this, &DebugProxy::signalSetNeedDecayMultiplier, agg, &AggregatorDebug::onSetNeedDecayMultiplier, Qt::QueuedConnection );
	connect( this, &DebugProxy::signalSetDisableNeedDecay, agg, &AggregatorDebug::onSetDisableNeedDecay, Qt::QueuedConnection );

	connect( agg, &AggregatorDebug::signalGnomeList, this, &DebugProxy::onGnomeList, Qt::QueuedConnection );
	connect( agg, &AggregatorDebug::signalItemGroups, this, &DebugProxy::onItemGroups, Qt::QueuedConnection );
	connect( agg, &AggregatorDebug::signalItems, this, &DebugProxy::onItems, Qt::QueuedConnection );
	connect( agg, &AggregatorDebug::signalMaterials, this, &DebugProxy::onMaterials, Qt::QueuedConnection );
}

/// @brief Binds the proxy to its owning view model.
void DebugProxy::setParent( IngnomiaGUI::DebugModel* parent )
{
	m_parent = parent;
}

/// @brief Forwards a creature-spawn request to the aggregator.
void DebugProxy::spawnCreature( QString type )
{
	emit signalSpawnCreature( type );
}

/// @brief Forwards a window-size change to the aggregator.
void DebugProxy::setWindowSize( int width, int height )
{
    emit signalSetWindowSize( width, height );
}

/// @brief Forwards a "set need value" command to the aggregator.
void DebugProxy::setNeed( unsigned int gnomeID, QString need, float value )
{
	emit signalSetNeed( gnomeID, need, value );
}

/// @brief Forwards a kill-gnome command to the aggregator.
void DebugProxy::killGnome( unsigned int gnomeID )
{
	emit signalKillGnome( gnomeID );
}

/// @brief Forwards a simple item-spawn request to the aggregator.
void DebugProxy::spawnItem( QString itemSID, QString materialSID, int count, int x, int y, int z )
{
	emit signalSpawnItem( itemSID, materialSID, count, x, y, z );
}

/// @brief Forwards a composite item-spawn request (multiple component materials) to the aggregator.
void DebugProxy::spawnCompositeItem( QString itemSID, QStringList materialSIDs, int count, int x, int y, int z )
{
	emit signalSpawnCompositeItem( itemSID, materialSIDs, count, x, y, z );
}

/// @brief Asks the aggregator to refresh the gnome dropdown list.
void DebugProxy::requestGnomeList()
{
	emit signalRequestGnomeList();
}

/// @brief Asks the aggregator for the list of item groups.
void DebugProxy::requestItemGroups()
{
	emit signalRequestItemGroups();
}

/// @brief Asks the aggregator for items belonging to the named group.
void DebugProxy::requestItems( QString group )
{
	emit signalRequestItems( group );
}

/// @brief Asks the aggregator for the materials valid for an item.
void DebugProxy::requestMaterials( QString itemSID )
{
	emit signalRequestMaterials( itemSID );
}

/// @brief Forwards the global need-decay multiplier change to the aggregator.
void DebugProxy::setNeedDecayMultiplier( float value )
{
	emit signalSetNeedDecayMultiplier( value );
}

/// @brief Forwards a per-need disable-decay flag toggle to the aggregator.
void DebugProxy::setDisableNeedDecay( QString need, bool disable )
{
	emit signalSetDisableNeedDecay( need, disable );
}

/// @brief Slot: receives a fresh gnome list and pushes it into the model's dropdown,
///        keeping the synthetic "All Gnomes" entry at index 0.
void DebugProxy::onGnomeList( const QList<QPair<QString, unsigned int>>& gnomes )
{
	if ( m_parent )
	{
		auto list = m_parent->getGnomeList();
		// Keep "All Gnomes" entry, clear the rest
		while ( list->Count() > 1 )
		{
			list->RemoveAt( list->Count() - 1 );
		}
		for ( const auto& g : gnomes )
		{
			list->Add( Noesis::MakePtr<IngnomiaGUI::NameEntry>( g.first, g.second ) );
		}
	}
}

/// @brief Slot: relays an item-groups list update to the model.
void DebugProxy::onItemGroups( const QStringList& groups )
{
	if ( m_parent )
	{
		m_parent->updateItemGroups( groups );
	}
}

/// @brief Slot: relays an items list update to the model.
void DebugProxy::onItems( const QStringList& items )
{
	if ( m_parent )
	{
		m_parent->updateItems( items );
	}
}

/// @brief Slot: relays material list updates (one or two component lists) to the model.
void DebugProxy::onMaterials( int componentCount, const QStringList& mats1, const QStringList& mats2 )
{
	if ( m_parent )
	{
		m_parent->updateMaterials( componentCount, mats1, mats2 );
	}
}
