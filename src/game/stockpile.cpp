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
#include "stockpile.h"

#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/util.h"
#include "../game/game.h"
#include "../game/inventory.h"
#include "../game/jobmanager.h"
#include "../game/world.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QString>

Stockpile::Stockpile( Game* game ) :
	WorldObject( game )
{
	m_name = "Stockpile";
}

Stockpile::Stockpile( QList<QPair<Position, bool>> tiles, Game* game ) :
	WorldObject( game )
{
	m_name = "Stockpile"; // + QString::number( m_id );

	int i = 1;
	for ( auto p : tiles )
	{
		if ( p.second )
		{
			addTile( p.first );
		}
	}
	updateCanAccept();
}

Stockpile::~Stockpile()
{
	for ( const auto& field : m_fields )
	{
		delete field;
	}
}

void Stockpile::addTile( Position& pos )
{
	InventoryField* infi = new InventoryField;
	infi->pos            = pos;
	infi->capacity       = 1;
	infi->stackSize      = 1;
	infi->containerID    = 0;

	m_fields.insert( pos.toInt(), infi );

	g->w()->setTileFlag( infi->pos, TileFlag::TF_STOCKPILE );
}

Stockpile::Stockpile( QVariantMap vals, Game* game ) :
	WorldObject( vals, game )
{
	m_pullOthers        = vals.value( "PullOthers" ).toBool();
	m_allowPull         = vals.value( "AllowPull" ).toBool();
	m_limitWithmaterial = vals.value( "LimitWithMaterial" ).toBool();

	QVariantList vfl = vals.value( "Fields" ).toList();
	for ( auto vf : vfl )
	{
		InventoryField* infi = new InventoryField;
		infi->pos            = Position( vf.toMap().value( "Pos" ).toString() );
		infi->containerID    = vf.toMap().value( "ContainerID" ).toUInt();
		infi->capacity       = vf.toMap().value( "Capacity" ).toUInt();
		infi->stackSize      = vf.toMap().value( "StackSize" ).toUInt();

		if ( vf.toMap().contains( "Items" ) )
		{
			QVariantList vli = vf.toMap().value( "Items" ).toList();
			for ( auto vi : vli )
			{
				if ( g->inv()->itemExists( vi.toUInt() ) )
				{
					infi->items.insert( vi.toUInt() );
				}
			}
		}
		if ( vf.toMap().contains( "ReservedItems" ) )
		{
			QVariantList vli = vf.toMap().value( "ReservedItems" ).toList();
			for ( auto vi : vli )
			{
				unsigned int itemID = vi.toUInt();
				if ( g->inv()->itemExists( itemID ) && g->inv()->isInJob( itemID ) )
				{
					infi->reservedItems.insert( itemID );
				}
			}
		}

		if ( infi->containerID )
		{
			infi->requireSame = g->inv()->requireSame( infi->containerID );
			//repair capacity
			infi->capacity = DB::select( "Capacity", "Containers", g->inv()->itemSID( infi->containerID ) ).value<unsigned char>(); 

		}

		m_fields.insert( infi->pos.toInt(), infi );
	}

	Filter filter( vals.value( "Filter" ).toMap() );
	m_filter = filter;

	QVariantList vll = vals.value( "Limits" ).toList();
	for ( auto vl : vll )
	{
		auto vm = vl.toMap();
		StockpileItemLimit limit( vm.value( "Max" ).toInt(), vm.value( "Activate" ).toInt(), vm.value( "Suspend" ).toInt(), vm.value( "Suspended" ).toBool() );
		m_limits.insert( vm.value( "Key" ).toString(), limit );
	}
	updateCanAccept();
}

QVariant Stockpile::serialize()
{
	QVariantMap out;

	WorldObject::serialize( out );

	out.insert( "PullOthers", m_pullOthers );
	out.insert( "AllowPull", m_allowPull );
	out.insert( "LimitWithMaterial", m_limitWithmaterial );

	QVariantList fields;
	for ( auto field : m_fields )
	{
		QVariantMap fima;
		fima.insert( "Pos", field->pos.toString() );
		fima.insert( "Capacity", field->capacity );
		fima.insert( "StackSize", field->stackSize );
		fima.insert( "ContainerID", field->containerID );

		if ( field->items.size() > 0 )
		{
			QVariantList items;
			for ( auto item : field->items )
			{
				items.append( item );
			}
			fima.insert( "Items", items );
		}

		if ( field->reservedItems.size() > 0 )
		{
			QVariantList reservedItems;
			for ( auto item : field->reservedItems )
			{
				reservedItems.append( item );
			}
			fima.insert( "ReservedItems", reservedItems );
		}
		fields.append( fima );
	}
	out.insert( "Fields", fields );

	QVariantList limits;
	for ( auto key : m_limits.keys() )
	{
		auto limit = m_limits[key];
		QVariantMap lima;
		lima.insert( "Key", key );
		lima.insert( "Max", limit.max );
		lima.insert( "Activate", limit.activateThreshold );
		lima.insert( "Suspend", limit.suspendThreshold );
		lima.insert( "Suspended", limit.suspended );
		limits.append( lima );
	}
	out.insert( "Limits", limits );
	
	out.insert( "Filter", m_filter.serialize() );

	return out;
}

void Stockpile::pasteSettings( QVariantMap vals )
{
	m_active     = vals.value( "Active" ).toBool();
	m_pullOthers = vals.value( "PullOthers" ).toBool();
	m_allowPull  = vals.value( "AllowPull" ).toBool();

	Filter filter( vals.value( "Filter" ).toMap() );
	m_filter = filter;

	m_limits.clear();
	QVariantList vll = vals.value( "Limits" ).toList();
	for ( auto vl : vll )
	{
		auto vm = vl.toMap();
		StockpileItemLimit limit( vm.value( "Max" ).toInt(), vm.value( "Activate" ).toInt(), vm.value( "Suspend" ).toInt(), false );
		m_limits.insert( vm.value( "Key" ).toString(), limit );
	}
}

bool Stockpile::onTick( quint64 tick )
{
	if ( !m_active )
		return false;

	if( tick - m_lastUpdateTick > 100 )
	{
		m_lastUpdateTick = tick;
		updateCanAccept();
	}
}

QSet<QPair<QString, QString>> Stockpile::freeSlots() const
{
	QSet<QPair<QString, QString>> freeSlots;
	for ( auto infi : m_fields )
	{
		if ( !infi->isFull )
		{
			if ( infi->containerID )
			{
				if ( infi->items.size() == 0 || !infi->requireSame )
				{
					for ( const auto& itemSID : Global::util->itemsAllowedInContainer( infi->containerID ) )
					{
						freeSlots.insert( { itemSID, "Any" } );
					}
				}
				else
				{
					auto item = *infi->items.begin();
					// Same item only
					freeSlots.insert( { g->inv()->itemSID( item ), g->inv()->materialSID( item ) } );
				}
			}
			else
			{
				if ( infi->items.size() == 0 )
				{
					freeSlots.insert( { "Any", "Any" } );
				}
				else if ( infi->items.size() < infi->stackSize )
				{
					auto item = *infi->items.begin();
					// Same item only
					freeSlots.insert( { g->inv()->itemSID( item ), g->inv()->materialSID( item ) } );
				}
			}
		}
	}
	return freeSlots;
}

void Stockpile::setInfiNotFull( Position pos )
{
	if ( m_fields.contains( pos.toInt() ) )
	{
		InventoryField* field = m_fields.value( pos.toInt() );
		field->isFull         = false;
		m_isFull              = false;
	}
}

bool Stockpile::insertItem( Position pos, unsigned int item )
{
	if ( m_fields.contains( pos.toInt() ) )
	{
		InventoryField* field = m_fields.value( pos.toInt() );
		field->reservedItems.remove( item );
		field->items.insert( item );
		field->isFull  = false;
		m_isFull       = false;
		g->inv()->setInStockpile( item, m_id );
		g->inv()->setInJob( item, 0 );

		if ( field->containerID )
		{
			if ( Global::util->itemAllowedInContainer( item, field->containerID ) )
			{
				g->inv()->putItemInContainer( item, field->containerID );
			}
		}
		else
		{
			field->stackSize = g->inv()->stackSize( *field->items.begin() );
			g->inv()->setInContainer( item, 0 );
		}
		// if count of that item type reached suspend threshold, suspend that item
		QString itemSID     = g->inv()->itemSID( item );
		QString materialSID = g->inv()->materialSID( item );
		if ( m_limitWithmaterial )
		{
			int threshold = m_limits[itemSID + materialSID].suspendThreshold;
			if ( threshold > 0 && countPlusReserved( itemSID, materialSID ) >= threshold )
			{
				m_limits[itemSID + materialSID].suspended = true;
				m_suspendStatusChanged                    = true;
			}
		}
		else
		{
			int threshold = m_limits[itemSID].suspendThreshold;
			if ( threshold > 0 && countPlusReserved( itemSID ) >= threshold )
			{
				m_limits[itemSID].suspended = true;
				m_suspendStatusChanged      = true;
			}
		}
		return true;
	}

	g->inv()->setInStockpile( item, 0 );
	g->inv()->setInJob( item, 0 );

	if ( Global::debugMode )
		qDebug() << "item insert failed";
	return false;
}

bool Stockpile::removeItem( Position pos, unsigned int item )
{
	if ( m_fields.contains( pos.toInt() ) )
	{
		InventoryField* field = m_fields[pos.toInt()];
		if ( field->items.remove( item ) )
		{
			field->isFull = false;
			m_isFull      = false;
			g->inv()->setInStockpile( item, 0 );
			if ( field->items.empty() && !field->containerID )
			{
				field->stackSize = 1;
			}
			QString itemSID     = g->inv()->itemSID( item );
			QString materialSID = g->inv()->materialSID( item );

			if ( m_limitWithmaterial )
			{
				if ( countPlusReserved( itemSID, materialSID ) <= m_limits[itemSID + materialSID].activateThreshold )
				{
					m_limits[itemSID + materialSID].suspended = false;
					//qDebug() << m_name << "suspend status changed";
					m_suspendStatusChanged = true;
				}
			}
			else
			{
				if ( countPlusReserved( itemSID ) <= m_limits[itemSID].activateThreshold )
				{
					m_limits[itemSID].suspended = false;
					//qDebug() << m_name << "suspend status changed";
					m_suspendStatusChanged = true;
				}
			}
			return true;
		}
	}
	if ( Global::debugMode )
		qDebug() << "remove item failed";
	return false;
}

void Stockpile::setCheckState( bool state, QString category, QString group, QString item, QString material )
{
	m_filterChanged = true;
	if ( material != "" )
	{
		qDebug() << category << group << item << material << state;
		m_filter.setCheckState( category, group, item, material, state );
	}
	else if ( item != "" )
	{
		m_filter.setCheckState( category, group, item, state );
	}
	else if ( group != "" )
	{
		m_filter.setCheckState( category, group, state );
	}
	else
	{
		m_filter.setCheckState( category, state );
	}
	if ( !state )
	{
		// unchecked some items, need to expel it from the stockpile
		QSet<QString> filter = m_filter.getActiveSimple();

		for ( auto infi : m_fields )
		{
			QList<unsigned int> toRemove;
			for ( auto oi : infi->items )
			{
				if ( !filter.contains( g->inv()->combinedID( oi ) ) )
				{
					g->inv()->setInStockpile( oi, false );
					toRemove.append( oi );
				}
			}
			for ( auto tr : toRemove )
			{
				infi->items.remove( tr );
			}
		}
	}
}

void Stockpile::addContainer( unsigned int containerID, Position& pos )
{
	if ( m_fields.contains( pos.toInt() ) )
	{
		if ( containerID )
		{
			InventoryField* field = m_fields.value( pos.toInt() );

			field->containerID = containerID;
			field->requireSame = g->inv()->requireSame( containerID );

			field->capacity = g->inv()->capacity( containerID );

			g->inv()->setConstructed( containerID, true );

			for ( auto itemID : g->inv()->itemsInContainer( containerID ) )
			{
				g->inv()->setItemPos( itemID, pos );
				if ( allowedInStockpile( itemID ) )
				{
					//insertItem( pos, itemID );
					g->inv()->setInStockpile( itemID, m_id );
				}
			}

			auto items = m_fields[pos.toInt()]->items;
			for ( auto item : items )
			{
				if ( Global::util->itemAllowedInContainer( item, containerID ) && g->inv()->itemsInContainer( containerID ).size() < g->inv()->capacity( containerID ) )
				{
					// if an item is already in the container and the container requires same
					if ( field->requireSame && g->inv()->itemsInContainer( containerID ).size() )
					{
						unsigned int firstItem = *g->inv()->itemsInContainer( containerID ).begin();
						if ( g->inv()->isSameTypeAndMaterial( item, firstItem ) )
						{
							g->inv()->putItemInContainer( item, containerID );
						}
					}
					else
					{
						g->inv()->putItemInContainer( item, containerID );
					}
				}
				else
				{
					removeItem( pos, item );
				}
			}
			field->items.clear();
			for ( auto itemID : g->inv()->itemsInContainer( containerID ) )
			{
				if ( allowedInStockpile( itemID ) )
				{
					field->items.insert( itemID );
				}
			}
		}
	}
}

void Stockpile::removeContainer( unsigned int containerID, Position& pos )
{
	if ( m_fields.contains( pos.toInt() ) )
	{
		InventoryField* field = m_fields.value( pos.toInt() );
		if ( field->containerID == containerID )
		{
			auto items     = g->inv()->itemsInContainer( containerID );
			for ( auto item : items )
			{
				removeItem( pos, item );
			}
			field->capacity    = 1;
			field->stackSize   = 1;
			field->containerID = 0;
		}
	}
}

bool Stockpile::allowedInStockpile( unsigned int itemID )
{
	return m_filter.getActiveSimple().contains( g->inv()->combinedID( itemID ) );
}

bool Stockpile::removeTile( Position& pos )
{
	InventoryField* infi = m_fields.value( pos.toInt() );
	// unconstruct container on tile
	if ( infi->containerID != 0 )
	{
		g->w()->deconstruct( infi->pos, infi->pos, false );
		//g->inv()->setConstructed( infi->containerID, false );
	}

	// unstockpile all items on tile
	for ( auto itemID : infi->items )
	{
		g->inv()->setInContainer( itemID, 0 );
		g->inv()->setInStockpile( itemID, 0 );
	}

	infi->items.clear();
	infi->reservedItems.clear();
	// remove tile and remove tile flag
	m_fields.remove( pos.toInt() );
	g->w()->clearTileFlag( pos, TileFlag::TF_STOCKPILE );
	delete infi;
	// if last tile deleted return true
	if ( m_fields.empty() )
	{
		m_active = false;
	}
	return m_fields.empty();
}

void Stockpile::linkWorkshop( unsigned int workshopID )
{
	if ( !m_linkedWorkshops.contains( workshopID ) )
	{
		m_linkedWorkshops.push_back( workshopID );
	}
}

void Stockpile::unlinkWorkshop( unsigned int workshopID )
{
	if ( m_linkedWorkshops.contains( workshopID ) )
	{
		m_linkedWorkshops.removeAll( workshopID );
	}
}

int Stockpile::count( QString itemSID, QString materialSID )
{
	int count      = 0;
	for ( auto spf : m_fields )
	{
		// if exists get item from that position
		for ( auto itemUID : spf->items )
		{
			if ( itemSID == g->inv()->itemSID( itemUID ) && materialSID == g->inv()->materialSID( itemUID ) )
			{
				++count;
			}
		}
	}
	return count;
}

int Stockpile::count( QString itemSID )
{
	int count      = 0;
	for ( auto spf : m_fields )
	{
		// if exists get item from that position
		for ( auto itemUID : spf->items )
		{
			if ( itemSID == g->inv()->itemSID( itemUID ) )
			{
				++count;
			}
		}
	}
	return count;
}

int Stockpile::countPlusReserved( QString itemSID )
{
	int count      = 0;
	for ( auto spf : m_fields )
	{
		// if exists get item from that position
		for ( auto itemUID : spf->items )
		{
			if ( itemSID == g->inv()->itemSID( itemUID ) )
			{
				++count;
			}
		}
		for ( auto itemUID : spf->reservedItems )
		{
			if ( itemSID == g->inv()->itemSID( itemUID ) )
			{
				++count;
			}
		}
	}
	return count;
}

int Stockpile::countPlusReserved( QString itemSID, QString materialSID )
{
	int count      = 0;
	for ( auto spf : m_fields )
	{
		// if exists get item from that position
		for ( auto itemUID : spf->items )
		{
			if ( itemSID == g->inv()->itemSID( itemUID ) && materialSID == g->inv()->materialSID( itemUID ) )
			{
				++count;
			}
		}
		for ( auto itemUID : spf->reservedItems )
		{
			if ( itemSID == g->inv()->itemSID( itemUID ) && materialSID == g->inv()->materialSID( itemUID ) )
			{
				++count;
			}
		}
	}
	return count;
}

void Stockpile::setPullOthers( bool value )
{
	m_pullOthers = value;
}

bool Stockpile::pullsOthers()
{
	return m_pullOthers;
}

void Stockpile::setAllowPull( bool value )
{
	m_allowPull = value;
}

bool Stockpile::allowsPull()
{
	return m_allowPull;
}

void Stockpile::setPriority( int p )
{
	m_priority = p;
}

int Stockpile::priority()
{
	return m_priority;
}

void Stockpile::resetLimits()
{
	m_limits.clear();
}

StockpileItemLimit Stockpile::limit( QString key )
{
	return m_limits[key];
}

void Stockpile::setLimit( QString key, StockpileItemLimit limit )
{
	m_limits.insert( key, limit );
}

bool Stockpile::suspendChanged()
{
	bool out               = m_suspendStatusChanged;
	m_suspendStatusChanged = false;
	return out;
}

int Stockpile::countFields()
{
	return m_fields.size();
}

void Stockpile::setLimitWithMaterial( bool value )
{
	m_limitWithmaterial = value;
}

bool Stockpile::limitWithMaterial()
{
	return m_limitWithmaterial;
}

int Stockpile::capacity( unsigned int tileID )
{
	if ( m_fields.contains( tileID ) )
	{
		InventoryField* field = m_fields.value( tileID );
		return field->capacity;
	}
	return 0;
}

int Stockpile::itemCount( unsigned int tileID )
{
	if ( m_fields.contains( tileID ) )
	{
		InventoryField* field = m_fields.value( tileID );
		return field->items.size();
	}
	return 0;
}

int Stockpile::reserved( unsigned int tileID )
{
	if ( m_fields.contains( tileID ) )
	{
		InventoryField* field = m_fields.value( tileID );
		return field->reservedItems.size();
	}
	return 0;
}

void Stockpile::updateCanAccept()
{
	m_canAccept.clear();
	if( m_isFull || !m_active )
	{
		return;
	}
	for( const auto& fe : m_filter.getActive() )
	{
		if( hasRoom( fe.first, fe.second ) )
		{
			m_canAccept.insert( fe );
		}
	}
}

bool Stockpile::hasRoom( const QString& itemSID, const QString& materialSID )
{
	for( const auto& infi : m_fields )
	{
		if( !infi->isFull )
		{
			if( infi->containerID == 0 )
			{
				if( infi->items.size() == 0  )
				{
					if( infi->reservedItems.size() == 0 )
						return true;
				}
				else
				{
					if( ( infi->items.size() + infi->reservedItems.size() ) < infi->stackSize )
					{
						auto item = *infi->items.begin();
						if( g->inv()->itemSID( item ) == itemSID && g->inv()->materialSID( item ) == materialSID )
						{
							return true;
						}
					}
				}
			}
			else
			{
				if ( infi->items.size() == 0 )
				{
					if( Global::allowedInContainer.value( g->inv()->itemSID( infi->containerID ) ).contains( itemSID ) )
					{
						return true;
					}
				}
				else
				{
					if ( infi->requireSame )
					{
						auto item = *infi->items.begin();
						if( g->inv()->itemSID( item ) == itemSID && g->inv()->materialSID( item ) == materialSID )
						{
							return true;
						}
					}
					else
					{
						if( Global::allowedInContainer.value( g->inv()->itemSID( infi->containerID ) ).contains( itemSID ) )
						{
							return true;
						}
					}
				}
			}
		}
	}
}

bool Stockpile::canAccept( const QString& itemSID, const QString& materialSID )
{
	return m_canAccept.contains( QPair<QString,QString>( itemSID, materialSID ) );
}

bool Stockpile::canAccept( const QPair<QString, QString>& pair )
{
	return m_canAccept.contains( pair );
}

bool Stockpile::isConnectedTo( const Position& pos, int& dist )
{
	if( m_fields.size() ) 
	{
		if( g->w()->regionMap().checkConnectedRegions( pos, m_fields.first()->pos ) )
		{
			dist = pos.distSquare( m_fields.first()->pos );
			return true;
		}
	}
	return false;
}

bool Stockpile::reserveItem( unsigned int itemID, Position& pos )
{
	auto pairSID = g->inv()->getPairSID( itemID );
	for( const auto& infi : m_fields )
	{
		if( !infi->isFull )
		{
			if( infi->containerID == 0 )
			{
				if( infi->items.size() == 0  )
				{
					if( infi->reservedItems.size() == 0 )
					{
						infi->reservedItems.insert( itemID );
						pos = infi->pos;
						return true;
					}
				}
				else
				{
					if( ( infi->items.size() + infi->reservedItems.size() ) < infi->stackSize )
					{
						auto item = *infi->items.begin();
						if( g->inv()->itemSID( item ) == pairSID.first && g->inv()->materialSID( item ) == pairSID.second )
						{
							infi->reservedItems.insert( itemID );
							pos = infi->pos;
							return true;
						}
					}
				}
			}
			else
			{
				if ( infi->items.size() == 0 )
				{
					if( Global::allowedInContainer.value( g->inv()->itemSID( infi->containerID ) ).contains( pairSID.first ) )
					{
						infi->reservedItems.insert( itemID );
						pos = infi->pos;
						return true;
					}
				}
				else
				{
					if ( infi->requireSame )
					{
						auto item = *infi->items.begin();
						if( g->inv()->itemSID( item ) == pairSID.first && g->inv()->materialSID( item ) == pairSID.second )
						{
							infi->reservedItems.insert( itemID );
							pos = infi->pos;
							return true;
						}
					}
					else
					{
						if( Global::allowedInContainer.value( g->inv()->itemSID( infi->containerID ) ).contains( pairSID.first ) )
						{
							infi->reservedItems.insert( itemID );
							pos = infi->pos;
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

void Stockpile::unreserveItem( unsigned int itemID )
{
	for( auto& infi : m_fields )
	{
		if( infi->reservedItems.remove( itemID ) )
		{
			return;
		}
	}
}