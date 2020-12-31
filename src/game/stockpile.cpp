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

		QVariantList vjl = vals.value( "Jobs" ).toList();
		for ( auto vj : vjl )
		{
			QSharedPointer<Job> job( new Job( vj.toMap() ) );
			m_jobsOut.insert( job->id(), job );
		}
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

	QVariantList jobs;
	for ( auto job : m_jobsOut )
	{
		jobs.append( job->serialize() );
	}
	out.insert( "Jobs", jobs );

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
	bool lastUpdateLongAgo = ( tick - m_lastUpdateTick ) > 100;
	if ( ( m_filterChanged || m_possibleItems.empty() ) && lastUpdateLongAgo )
	{
		m_filterChanged = false;
		if ( !m_fields.empty() )
		{
			auto possibleSloits     = freeSlots();
			const auto activeFilter = m_filter.getActive();
			QSet<QPair<QString, QString>> effectiveFilter;
			if ( possibleSloits.contains( QPair<QString, QString> { "Any", "Any" } ) )
			{
				effectiveFilter = activeFilter;
			}
			else
			{
				for ( const auto& filter : activeFilter )
				{
					if ( possibleSloits.contains( filter ) || possibleSloits.contains( QPair<QString, QString> { filter.first, "Any" } ) )
					{
						effectiveFilter.insert( filter );
					}
				}
			}

			m_possibleItems = g->inv()->getClosestItemsForStockpile( m_id, m_fields.first()->pos, m_pullOthers, effectiveFilter );
			if ( m_possibleItems.size() > 5000 )
			{
				qWarning() << "Excessive number of candidates for stockpile";
			}
		}
		m_lastUpdateTick = tick;

		return true;
	}
	return false;
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

unsigned int Stockpile::getJob()
{
	if ( m_isFull || !m_active || m_filter.getActive().isEmpty() || m_fields.empty() )
		return 0;

	int itemsChecked = 0;
	while ( !m_possibleItems.empty() && itemsChecked < 50 )
	{
		++itemsChecked;
		unsigned int item   = m_possibleItems.takeFirst();
		QString itemSID     = g->inv()->itemSID( item );
		QString materialSID = g->inv()->materialSID( item );
		bool suspended      = false;

		if ( m_limitWithmaterial )
		{
			if ( m_limits.contains( itemSID + materialSID ) )
			{
				suspended = m_limits[itemSID + materialSID].suspended;
			}
		}
		else
		{
			if ( m_limits.contains( itemSID ) )
			{
				suspended = m_limits[itemSID].suspended;
			}
		}

		if ( !suspended && !g->inv()->isInJob( item ) && g->inv()->isInStockpile( item ) != m_id && g->w()->regionMap().checkConnectedRegions( m_fields.first()->pos, g->inv()->getItemPos( item ) ) )
		{
			m_isFull = true;
			//loop over all stockpile fields
			for ( auto infi : m_fields )
			{
				if ( !infi->isFull )
				{
					m_isFull = false;
					if ( infi->reservedItems.size() > 0 )
					{
						unsigned int reservedItemID = *infi->reservedItems.begin();
						if ( !g->inv()->itemExists( reservedItemID ) || g->inv()->isInJob( reservedItemID ) == 0 )
						{
							infi->reservedItems.remove( reservedItemID );
						}
					}

					// start with tiles that have a container
					if ( infi->containerID )
					{
						if ( infi->items.size() + infi->reservedItems.size() == 0 )
						{
							//container is empty
							if ( Global::util->itemAllowedInContainer( item, infi->containerID ) )
							{
								return createJob( item, infi );
							}
						}
						else // container not empty
						{
							if ( infi->requireSame )
							{
								if ( infi->capacity > ( g->inv()->itemsInContainer( infi->containerID ).size() + infi->reservedItems.size() ) && infi->capacity > infi->items.size() + infi->reservedItems.size() )
								{
									unsigned int firstItem = 0;
									if ( g->inv()->itemsInContainer( infi->containerID ).size() )
									{
										firstItem = *g->inv()->itemsInContainer( infi->containerID ).begin();
									}
									else if ( infi->reservedItems.size() )
									{
										firstItem = *infi->reservedItems.begin();
									}
									if ( ( firstItem != 0 && g->inv()->isSameTypeAndMaterial( item, firstItem ) ) )
									{
										return createJob( item, infi );
									}
								}
								else
								{
									infi->isFull = true;
								}
							}
							else
							{
								if ( infi->capacity > ( g->inv()->itemsInContainer( infi->containerID ).size() + infi->reservedItems.size() ) &&
									 infi->capacity > infi->items.size() + infi->reservedItems.size() )
								{
									if ( Global::util->itemAllowedInContainer( item, infi->containerID ) )
									{
										return createJob( item, infi );
									}
								}
								else
								{
									infi->isFull = true;
								}
							}
						}
					}
				}
			}
			//loop over all stockpile fields
			for ( auto infi : m_fields )
			{
				if ( !infi->isFull )
				{
					// now tiles without container
					if ( !infi->containerID ) // no container
					{
						if ( infi->items.size() + infi->reservedItems.size() == 0 )
						{
							// field is empty
							if ( !g->inv()->isInJob( item ) && g->inv()->isInStockpile( item ) != m_id )
							{
								// create hauling job
								return createJob( item, infi );
							}
						}
						else
						{
							// field isn't empty, is the item stackable and same type and material as the one on the field
							unsigned char newStackSize = infi->stackSize;
							if ( infi->reservedItems.size() > 0 )
							{
								newStackSize = qMax( newStackSize, g->inv()->stackSize( *infi->reservedItems.begin() ) );
							}

							if ( newStackSize > 1 && ( ( infi->items.size() + infi->reservedItems.size() ) < newStackSize ) )
							{
								// get the stack defining item
								// look if there are any items of that type that aren't in stockpiles yet
								unsigned int itemID = 0;
								if ( !infi->items.empty() )
								{
									itemID = *infi->items.begin();
								}
								else
								{
									itemID = *infi->reservedItems.begin();
								}
								if ( g->inv()->isSameTypeAndMaterial( itemID, item ) )
								{
									return createJob( item, infi );
								}
							}
							else
							{
								infi->isFull = true;
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

unsigned int Stockpile::createJob( unsigned int itemID, InventoryField* infi )
{
	QString itemSID        = g->inv()->itemSID( itemID );
	QString materialSID    = g->inv()->materialSID( itemID );
	int freeSpace          = 1;
	int countItems         = infi->items.size();
	int countReservedItems = infi->reservedItems.size();
	if ( infi->containerID )
	{
		freeSpace = infi->capacity - ( countItems + countReservedItems );
	}
	else
	{
		if ( infi->items.size() == 0 && infi->reservedItems.size() == 0 )
		{
			freeSpace = g->inv()->stackSize( itemID );
		}
		else if ( infi->items.size() == 0 && infi->reservedItems.size() > 0 )
		{
			freeSpace = g->inv()->stackSize( *infi->reservedItems.begin() ) - ( countItems + countReservedItems );
		}
		else
		{
			freeSpace = infi->stackSize - ( countItems + countReservedItems );
		}
	}

	// space for more than one item?
	if ( freeSpace > 1 )
	{
		// there is more than one item of this type not in stockpiles already
		int itemCount = g->inv()->itemCountNotInStockpile( itemSID, materialSID );
		if ( itemCount > 1 )
		{
			// carry container exists?
			QString carryContainer = Global::util->carryContainerForItem( itemSID );
			freeSpace              = qMin( freeSpace, Global::util->capacity( carryContainer ) );

			if ( !carryContainer.isEmpty() )
			{
				if ( g->inv()->itemCount( carryContainer, "any" ) > 0 )
				{
					QSharedPointer<Job> job( new Job );
					job->setType( "HauleMultipleItems" );
					job->setRequiredSkill( "Hauling" );
					job->setRequiredTool( carryContainer, 0 );
					job->setStockpile( m_id );
					job->setPos( infi->pos );
					job->setPosItemInput( infi->pos );
					job->addPossibleWorkPosition( infi->pos );

					job->addItemToHaul( itemID );
					infi->reservedItems.insert( itemID );
					g->inv()->setInJob( itemID, job->id() );
					--freeSpace;

					//auto possibleItems = g->inv()->getClosestItems( infi->pos, false, itemSID, materialSID );
					// m_possible items are now sorted by item and material
					//
					while ( !m_possibleItems.empty() && freeSpace > 0 )
					{
						auto nextItem = m_possibleItems.takeFirst();
						if ( materialSID != g->inv()->materialSID( nextItem ) || itemSID != g->inv()->itemSID( nextItem ) )
						{
							break;
						}

						if ( !g->inv()->isInJob( nextItem ) && g->inv()->isInStockpile( nextItem ) != m_id && g->w()->regionMap().checkConnectedRegions( infi->pos, g->inv()->getItemPos( nextItem ) ) )
						{
							job->addItemToHaul( nextItem );
							infi->reservedItems.insert( nextItem );
							g->inv()->setInJob( nextItem, job->id() );
							--freeSpace;
						}
					}
					m_jobsOut.insert( job->id(), job );
					return job->id();
				}
			}
		}
	}

	QSharedPointer<Job> job( new Job );
	job->setType( "HauleItem" );
	job->setRequiredSkill( "Hauling" );

	job->setStockpile( m_id );

	job->addItemToHaul( itemID );
	g->inv()->setInJob( itemID, job->id() );

	job->setPos( infi->pos );
	job->setPosItemInput( infi->pos );
	job->addPossibleWorkPosition( infi->pos );
	job->setWorkPos( infi->pos );

	infi->reservedItems.insert( itemID );

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

	m_jobsOut.insert( job->id(), job );
	return job->id();
}

unsigned int Stockpile::getCleanUpJob()
{
	if ( !m_active || m_filter.getActive().isEmpty() )
		return 0;

	// cleaning up
	//loop over all stockpile fields
	for ( auto i = m_fields.begin(); i != m_fields.end(); ++i )
	{
		InventoryField* infi = i.value();
		//first tiles with container
		if ( infi->containerID )
		{
			if ( infi->requireSame )
			{
				if ( infi->capacity > ( g->inv()->itemsInContainer( infi->containerID ).size() + infi->reservedItems.size() ) )
				{
					unsigned int firstItem = 0;
					if ( g->inv()->itemsInContainer( infi->containerID ).size() )
					{
						firstItem = *g->inv()->itemsInContainer( infi->containerID ).begin();
					}
					else if ( infi->reservedItems.size() )
					{
						firstItem = *infi->reservedItems.begin();
					}
					if ( firstItem != 0 )
					{
						if ( g->inv()->isInJob( firstItem ) )
						{
							return 0;
						}

						for ( auto j = m_fields.begin(); j != m_fields.end(); ++j )
						{
							InventoryField* infi2 = j.value();
							if ( !infi2->items.empty() && infi2->reservedItems.empty() && !infi2->containerID )
							{
								for ( auto oi : infi2->items )
								{
									if ( g->inv()->itemSID( oi ) == g->inv()->itemSID( firstItem ) &&
										 g->inv()->materialSID( oi ) == g->inv()->materialSID( firstItem ) &&
										 !g->inv()->isInJob( oi ) )
									{
										return createJob( oi, infi );
									}
								}
							}
						}
					}
				}
			}
			else
			{
				if ( infi->capacity > ( g->inv()->itemsInContainer( infi->containerID ).size() + infi->reservedItems.size() ) )
				{
					for ( auto j = m_fields.begin(); j != m_fields.end(); ++j )
					{
						InventoryField* infi2 = j.value();
						if ( !infi2->items.empty() && infi2->reservedItems.empty() && !infi2->containerID )
						{
							for ( auto oi : infi2->items )
							{
								if ( Global::util->itemAllowedInContainer( oi, infi->containerID ) && !g->inv()->isInJob( oi ) )
								{
									return createJob( oi, infi );
								}
							}
						}
					}
				}
			}
		}
	}

	for ( auto i = m_fields.begin(); i != m_fields.end(); ++i )
	{
		InventoryField* infi = i.value();
		if ( !infi->containerID )
		{
			if ( infi->stackSize > 1 && ( infi->items.size() + infi->reservedItems.size() < infi->stackSize ) )
			{
				unsigned int firstItem = *infi->items.begin();
				if ( g->inv()->isInJob( firstItem ) )
				{
					return 0;
				}

				auto iter = m_fields.end() - 1;
				while ( iter != i )
				{
					InventoryField* infi2 = iter.value();
					if ( !infi2->items.empty() && infi2->reservedItems.empty() && !infi2->containerID )
					{
						for ( auto oi : infi2->items )
						{
							if ( g->inv()->itemSID( oi ) == g->inv()->itemSID( firstItem ) &&
								 g->inv()->materialSID( oi ) == g->inv()->materialSID( firstItem ) &&
								 !g->inv()->isInJob( oi ) &&
								 infi2->items.size() < infi2->stackSize )
							{
								return createJob( oi, infi );
							}
						}
					}
					--iter;
				}
			}
		}
	}
	return 0;
}

bool Stockpile::finishJob( unsigned int jobID )
{
	return giveBackJob( jobID );
}

bool Stockpile::giveBackJob( unsigned int jobID )
{
	if ( m_jobsOut.contains( jobID ) )
	{
		QSharedPointer<Job> job = m_jobsOut[jobID];
		for ( auto itemID : job->itemsToHaul() )
		{
			g->inv()->setInJob( itemID, 0 );
			Position pos = job->pos();
			if ( m_fields.contains( pos.toInt() ) )
			{
				m_fields[pos.toInt()]->reservedItems.remove( itemID );
				m_fields[pos.toInt()]->isFull = false;
				m_isFull                      = false;
			}
			QString itemSID     = g->inv()->itemSID( itemID );
			QString materialSID = g->inv()->materialSID( itemID );
			if ( m_limitWithmaterial )
			{
				if ( countPlusReserved( itemSID, materialSID ) <= m_limits[itemSID + materialSID].activateThreshold )
				{
					m_limits[itemSID + materialSID].suspended = false;
					m_suspendStatusChanged                    = true;
				}
			}
			else
			{
				if ( countPlusReserved( itemSID ) <= m_limits[itemSID].activateThreshold )
				{
					m_limits[itemSID].suspended = false;
					m_suspendStatusChanged      = true;
				}
			}
		}
		m_jobsOut.remove( jobID );
		return true;
	}
	return false;
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

QSharedPointer<Job> Stockpile::getJob( unsigned int jobID )
{
	return m_jobsOut[jobID];
}

bool Stockpile::hasJobID( unsigned int jobID ) const
{
	return m_jobsOut.contains( jobID );
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
	if ( m_jobsOut.contains( pos.toInt() ) )
	{
		m_jobsOut[pos.toInt()]->setCanceled();
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

bool Stockpile::stillHasJobs()
{
	return !m_jobsOut.isEmpty();
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