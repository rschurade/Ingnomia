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
/** @file stockpile.cpp
 *  @brief Stockpile storage management: item filtering, hauling job creation, container support,
 *         stack merging, item limits with suspend/activate thresholds, and tile lifecycle.
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

/// @brief Constructs an empty stockpile with no tiles.
/// @param game Owning Game instance.
Stockpile::Stockpile( Game* game ) :
	WorldObject( game )
{
	m_name = "Stockpile";
}

/// @brief Constructs a stockpile from a list of (position, included) tile pairs.
///        Only tiles with the bool flag set are added.
/// @param tiles List of (Position, bool) pairs; true means include the tile.
/// @param game  Owning Game instance.
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

/// @brief Destructor. Frees all InventoryField allocations.
Stockpile::~Stockpile()
{
	for ( const auto& field : m_fields )
	{
		delete field;
	}
}

/// @brief Adds a single tile to this stockpile, creating an InventoryField and setting the TF_STOCKPILE flag.
/// @param pos World position of the tile to add.
void Stockpile::addTile( Position& pos )
{
	InventoryField* infi = new InventoryField;
	infi->pos            = pos;
	infi->capacity       = 1;
	infi->stackSize      = 1;
	m_fields.insert( pos.toInt(), infi );

	g->w()->setTileFlag( infi->pos, TileFlag::TF_STOCKPILE );
}

/// @brief Deserialising constructor. Restores a stockpile from a previously serialised QVariantMap.
/// @param vals QVariantMap produced by Stockpile::serialize().
/// @param game  Owning Game instance.
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
		infi->capacity       = vf.toMap().value( "Capacity" ).toUInt();
		infi->stackSize      = vf.toMap().value( "StackSize" ).toUInt();
		if ( vf.toMap().contains( "Container" ) )
		{
			infi->container   = SourceMaterial::deserialize( vf.toMap().value( "Container" ).toMap() );
			infi->requireSame = vf.toMap().value( "RequireSame" ).toBool();
			infi->allowedItems = vf.toMap().value( "AllowedItems" ).toStringList();
		}

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

/// @brief Serialises this stockpile's full state (fields, filter, limits, pending jobs) to a QVariant.
/// @return QVariant map with all persistent stockpile data.
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
		if ( field->hasContainer() )
		{
			fima.insert( "Container", field->container.serialize() );
			fima.insert( "RequireSame", field->requireSame );
			QVariantList allowedItems;
			for ( const auto& ai : field->allowedItems ) allowedItems.append( ai );
			fima.insert( "AllowedItems", allowedItems );
		}

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

/// @brief Applies filter, pull-flags, and limits from @p vals (e.g. copied from another stockpile).
///        Does not modify tile layout or items.
/// @param vals QVariantMap with Active, PullOthers, AllowPull, Filter, Limits keys.
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

/// @brief Per-tick update. Refreshes the candidate item list when the filter has changed
///        or when the possible-item list is empty and hasn't been updated recently.
/// @param tick Current game tick.
/// @return true if a filter update was performed.
bool Stockpile::onTick( quint64 tick )
{
	if ( !m_active )
		return false;
	bool lastUpdateLongAgo = ( tick - m_lastUpdateTick ) > 100;
	if ( m_filterChanged || ( m_possibleItems.empty() && lastUpdateLongAgo ) )
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

/// @brief Returns the set of (itemSID, materialSID) pairs that could still be accepted
///        by at least one non-full field. "Any" wildcards are used for empty plain fields.
/// @return Set of (itemSID, materialSID) pairs with available capacity.
QSet<QPair<QString, QString>> Stockpile::freeSlots() const
{
	QSet<QPair<QString, QString>> freeSlots;
	for ( auto infi : m_fields )
	{
		if ( !infi->isFull )
		{
			if ( infi->hasContainer() )
			{
				if ( infi->items.size() == 0 || !infi->requireSame )
				{
					for ( const auto& itemSID : infi->allowedItems )
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

/// @brief Tries to create a hauling job for the next candidate item from m_possibleItems.
///        Checks item limits, region connectivity, container rules, and stack eligibility.
///        Creates a multi-item haul job (HauleMultipleItems) when a carry container is available.
/// @return The job ID of the created job, or 0 if no suitable item/field pair was found.
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
					if ( infi->hasContainer() )
					{
						if ( infi->items.size() + infi->reservedItems.size() == 0 )
						{
							//container is empty
							if ( infi->allowedItems.contains( g->inv()->itemSID( item ) ) )
							{
								return createJob( item, infi );
							}
						}
						else // container not empty
						{
							if ( infi->capacity > infi->items.size() + infi->reservedItems.size() )
							{
								if ( infi->requireSame )
								{
									unsigned int firstItem = 0;
									if ( !infi->items.empty() )
									{
										firstItem = *infi->items.begin();
									}
									else if ( !infi->reservedItems.empty() )
									{
										firstItem = *infi->reservedItems.begin();
									}
									if ( firstItem != 0 && g->inv()->isSameTypeAndMaterial( item, firstItem ) )
									{
										return createJob( item, infi );
									}
								}
								else
								{
									if ( infi->allowedItems.contains( g->inv()->itemSID( item ) ) )
									{
										return createJob( item, infi );
									}
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
			//loop over all stockpile fields
			for ( auto infi : m_fields )
			{
				if ( !infi->isFull )
				{
					// now tiles without container
					if ( !infi->hasContainer() ) // no container
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

/// @brief Creates a HauleItem (or HauleMultipleItems) job targeting @p infi for item @p itemID.
///        Updates item limits, reserves the item slot, and registers the job.
/// @param itemID UID of the item to haul.
/// @param infi   Target inventory field in this stockpile.
/// @return The job ID of the created job.
unsigned int Stockpile::createJob( unsigned int itemID, InventoryField* infi )
{
	QString itemSID        = g->inv()->itemSID( itemID );
	QString materialSID    = g->inv()->materialSID( itemID );
	int freeSpace          = 1;
	int countItems         = infi->items.size();
	int countReservedItems = infi->reservedItems.size();
	if ( infi->hasContainer() )
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

/// @brief Tries to create a consolidation job: moves items from plain fields into containers,
///        or merges partially-filled stacks within the stockpile.
/// @return The job ID of the created consolidation job, or 0 if nothing to consolidate.
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
		if ( infi->hasContainer() )
		{
			if ( infi->capacity > infi->items.size() + infi->reservedItems.size() )
			{
				if ( infi->requireSame )
				{
					unsigned int firstItem = 0;
					if ( !infi->items.empty() )
					{
						firstItem = *infi->items.begin();
					}
					else if ( !infi->reservedItems.empty() )
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
							if ( !infi2->items.empty() && infi2->reservedItems.empty() && !infi2->hasContainer() )
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
				else
				{
					for ( auto j = m_fields.begin(); j != m_fields.end(); ++j )
					{
						InventoryField* infi2 = j.value();
						if ( !infi2->items.empty() && infi2->reservedItems.empty() && !infi2->hasContainer() )
						{
							for ( auto oi : infi2->items )
							{
								if ( infi->allowedItems.contains( g->inv()->itemSID( oi ) ) && !g->inv()->isInJob( oi ) )
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
		if ( !infi->hasContainer() )
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
					if ( !infi2->items.empty() && infi2->reservedItems.empty() && !infi2->hasContainer() )
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

/// @brief Called when a hauling job completes successfully. Delegates to giveBackJob().
/// @param jobID ID of the completed job.
/// @return true if the job was found and removed.
bool Stockpile::finishJob( unsigned int jobID )
{
	return giveBackJob( jobID );
}

/// @brief Cancels or returns a hauling job: releases item reservations, resets field isFull flags,
///        and reactivates item limits if count drops below the activate threshold.
/// @param jobID ID of the job to return.
/// @return true if the job was found and removed.
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

/// @brief Clears the isFull flag on the field at @p pos and on the stockpile as a whole.
/// @param pos World position of the field to mark as not full.
void Stockpile::setInfiNotFull( Position pos )
{
	if ( m_fields.contains( pos.toInt() ) )
	{
		InventoryField* field = m_fields.value( pos.toInt() );
		field->isFull         = false;
		m_isFull              = false;
	}
}

/// @brief Records a delivered item into the field at @p pos. Updates reservation tracking,
///        container sprite visibility, stack size, item-limit suspend thresholds, and inventory flags.
/// @param pos  World position of the destination field.
/// @param item UID of the item being placed.
/// @return true on success; false if @p pos is not a field of this stockpile.
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

		if ( field->hasContainer() )
		{
			// Hide item sprite for items inside the container
			// Show sprite only for items at this position NOT in the container's item set
			unsigned int visibleItem = 0;
			PositionEntry pe;
			if ( g->inv()->getObjectsAtPosition( pos, pe ) )
			{
				for ( auto id : pe )
				{
					if ( !field->items.contains( id ) && !field->reservedItems.contains( id ) )
					{
						visibleItem = id;
						break;
					}
				}
			}
			g->w()->setItemSprite( pos, visibleItem ? g->inv()->spriteID( visibleItem ) : 0 );
		}
		else
		{
			field->stackSize = g->inv()->stackSize( *field->items.begin() );
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

/// @brief Removes an item from the field at @p pos, resets isFull, clears the inventory
///        stockpile flag, and reactivates item limits if count drops below the activate threshold.
/// @param pos  World position of the field.
/// @param item UID of the item to remove.
/// @return true if the item was found and removed.
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
			if ( field->items.empty() && !field->hasContainer() )
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

/// @brief Updates the item filter for this stockpile at the given specificity level
///        (category / group / item / material). If items are unchecked, expels any already-stored
///        items that no longer match the new filter.
/// @param state    New checked state.
/// @param category Filter category key.
/// @param group    Filter group key (empty to apply to whole category).
/// @param item     Filter item key (empty to apply to whole group).
/// @param material Filter material key (empty to apply to all materials of the item).
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

/// @brief Places a container on the field at @p pos, setting its source, capacity, requireSame flag,
///        and allowed-item list. Items already on the field that aren't allowed are removed.
/// @param source       Container item/material source descriptor.
/// @param capacity     Maximum number of items the container can hold.
/// @param requireSame  If true, all items in the container must be the same type and material.
/// @param allowedItems List of itemSIDs permitted in this container.
/// @param pos          World position of the field.
void Stockpile::addContainer( const SourceMaterial& source, unsigned char capacity, bool requireSame, const QStringList& allowedItems, Position& pos )
{
	if ( m_fields.contains( pos.toInt() ) )
	{
		InventoryField* field = m_fields.value( pos.toInt() );

		field->container    = source;
		field->requireSame  = requireSame;
		field->capacity     = capacity;
		field->allowedItems = allowedItems;

		// Check existing items on this tile — remove any that aren't allowed
		auto items = field->items;
		for ( auto item : items )
		{
			QString iSID = g->inv()->itemSID( item );
			if ( !allowedItems.contains( iSID ) )
			{
				removeItem( pos, item );
			}
		}
	}
}

/// @brief Removes the container from the field at @p pos. Retains up to stackSize items of the
///        first allowed type; removes any overflow or disallowed items from the stockpile.
///        Forces a filter refresh so overflow items can be redistributed.
/// @param pos World position of the field.
void Stockpile::removeContainer( Position& pos )
{
	if ( m_fields.contains( pos.toInt() ) )
	{
		InventoryField* field = m_fields.value( pos.toInt() );

		// Clear container properties
		field->container = SourceMaterial();
		field->allowedItems.clear();
		field->requireSame = false;

		// Without a container, capacity is 1 (stackSize for the item type on the field)
		// Keep up to stackSize items, remove the rest
		auto items = field->items;
		field->items.clear();

		bool first = true;
		for ( auto item : items )
		{
			if ( first && allowedInStockpile( item ) )
			{
				// Keep one item (or stackSize items) on the now-containerless field
				field->items.insert( item );
				field->stackSize = g->inv()->stackSize( item );
				first = false;
			}
			else if ( !first && field->items.size() < field->stackSize && allowedInStockpile( item ) )
			{
				// Stack more if same type and stackable
				unsigned int firstItem = *field->items.begin();
				if ( g->inv()->isSameTypeAndMaterial( item, firstItem ) )
				{
					field->items.insert( item );
				}
				else
				{
					// Different type — remove from stockpile
					g->inv()->setInStockpile( item, 0 );
				}
			}
			else
			{
				// Overflow — remove from stockpile
				g->inv()->setInStockpile( item, 0 );
			}
		}

		field->capacity = field->stackSize;
		field->isFull = ( field->items.size() >= field->stackSize );

		// Force possibleItems refresh so overflow items are redistributed
		m_possibleItems.clear();
		m_filterChanged = true;
	}
}

/// @brief Returns whether the given item matches the stockpile's active filter.
/// @param itemID UID of the item to check.
/// @return true if the item's combinedID is in the active simple filter set.
bool Stockpile::allowedInStockpile( unsigned int itemID )
{
	return m_filter.getActiveSimple().contains( g->inv()->combinedID( itemID ) );
}

/// @brief Returns the pending job with the given ID.
/// @param jobID ID of the job to look up.
/// @return Shared pointer to the job, or null if not found.
QSharedPointer<Job> Stockpile::getJob( unsigned int jobID )
{
	return m_jobsOut[jobID];
}

/// @brief Returns whether this stockpile owns the job with the given ID.
/// @param jobID ID to check.
/// @return true if the job is in m_jobsOut.
bool Stockpile::hasJobID( unsigned int jobID ) const
{
	return m_jobsOut.contains( jobID );
}

/// @brief Removes a tile from this stockpile. Deconstructs any container, frees all items,
///        cancels pending jobs for that position, and clears the TF_STOCKPILE flag.
/// @param pos World position of the tile to remove.
/// @return true if this was the last tile (stockpile is now empty).
bool Stockpile::removeTile( Position& pos )
{
	InventoryField* infi = m_fields.value( pos.toInt() );
	// deconstruct container on tile if present
	if ( infi->hasContainer() )
	{
		g->w()->deconstruct( infi->pos, infi->pos, false );
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

/// @brief Links a workshop to this stockpile so it can pull items directly.
/// @param workshopID UID of the workshop to link.
void Stockpile::linkWorkshop( unsigned int workshopID )
{
	if ( !m_linkedWorkshops.contains( workshopID ) )
	{
		m_linkedWorkshops.push_back( workshopID );
	}
}

/// @brief Removes the link between this stockpile and the given workshop.
/// @param workshopID UID of the workshop to unlink.
void Stockpile::unlinkWorkshop( unsigned int workshopID )
{
	if ( m_linkedWorkshops.contains( workshopID ) )
	{
		m_linkedWorkshops.removeAll( workshopID );
	}
}

/// @brief Counts stored items matching both itemSID and materialSID (committed items only).
/// @param itemSID     Item type string ID.
/// @param materialSID Material string ID.
/// @return Number of matching items across all fields.
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

/// @brief Counts stored items matching itemSID (any material, committed items only).
/// @param itemSID Item type string ID.
/// @return Number of matching items across all fields.
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

/// @brief Counts committed + reserved items matching itemSID (any material).
///        Used for limit suspend/activate threshold checks.
/// @param itemSID Item type string ID.
/// @return Total number of matching items (stored + reserved) across all fields.
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

/// @brief Counts committed + reserved items matching both itemSID and materialSID.
/// @param itemSID     Item type string ID.
/// @param materialSID Material string ID.
/// @return Total number of matching items (stored + reserved) across all fields.
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

/// @brief Sets whether this stockpile actively pulls items from other stockpiles.
/// @param value New value for m_pullOthers.
void Stockpile::setPullOthers( bool value )
{
	m_pullOthers = value;
}

/// @brief Returns whether this stockpile pulls items from other stockpiles.
/// @return m_pullOthers.
bool Stockpile::pullsOthers()
{
	return m_pullOthers;
}

/// @brief Sets whether other stockpiles are allowed to pull items from this one.
/// @param value New value for m_allowPull.
void Stockpile::setAllowPull( bool value )
{
	m_allowPull = value;
}

/// @brief Returns whether other stockpiles may pull items from this one.
/// @return m_allowPull.
bool Stockpile::allowsPull()
{
	return m_allowPull;
}

/// @brief Sets the hauling priority for this stockpile.
/// @param p New priority value (higher = preferred).
void Stockpile::setPriority( int p )
{
	m_priority = p;
}

/// @brief Returns the hauling priority of this stockpile.
/// @return m_priority.
int Stockpile::priority()
{
	return m_priority;
}

/// @brief Clears all item limits for this stockpile.
void Stockpile::resetLimits()
{
	m_limits.clear();
}

/// @brief Returns the item limit record for the given key (itemSID or itemSID+materialSID).
/// @param key Limit key string.
/// @return Copy of the StockpileItemLimit for that key.
StockpileItemLimit Stockpile::limit( QString key )
{
	return m_limits[key];
}

/// @brief Inserts or replaces the item limit record for the given key.
/// @param key   Limit key string (itemSID or itemSID+materialSID).
/// @param limit New limit configuration.
void Stockpile::setLimit( QString key, StockpileItemLimit limit )
{
	m_limits.insert( key, limit );
}

/// @brief Returns and clears the suspend-status-changed flag.
///        Used by the stockpile manager to detect when limit suspensions have changed.
/// @return true if any limit suspension changed since the last call.
bool Stockpile::suspendChanged()
{
	bool out               = m_suspendStatusChanged;
	m_suspendStatusChanged = false;
	return out;
}

/// @brief Returns whether this stockpile has any pending hauling jobs not yet completed.
/// @return true if m_jobsOut is non-empty.
bool Stockpile::stillHasJobs()
{
	return !m_jobsOut.isEmpty();
}

/// @brief Returns the total number of tile fields in this stockpile.
/// @return m_fields.size().
int Stockpile::countFields()
{
	return m_fields.size();
}

/// @brief Sets whether item limits are keyed by itemSID+materialSID (true) or itemSID only (false).
/// @param value New value for m_limitWithmaterial.
void Stockpile::setLimitWithMaterial( bool value )
{
	m_limitWithmaterial = value;
}

/// @brief Returns whether item limits include material in the key.
/// @return m_limitWithmaterial.
bool Stockpile::limitWithMaterial()
{
	return m_limitWithmaterial;
}

/// @brief Returns the item capacity of the field identified by tileID.
/// @param tileID Integer key (Position::toInt()) of the field.
/// @return Capacity of the field, or 0 if the tileID is not part of this stockpile.
int Stockpile::capacity( unsigned int tileID )
{
	if ( m_fields.contains( tileID ) )
	{

		InventoryField* field = m_fields.value( tileID );
		return field->capacity;
		
	}
	return 0;
}

/// @brief Returns the number of committed items currently on the field identified by tileID.
/// @param tileID Integer key (Position::toInt()) of the field.
/// @return Item count on the field, or 0 if the tileID is not part of this stockpile.
int Stockpile::itemCount( unsigned int tileID )
{
	if ( m_fields.contains( tileID ) )
	{

		InventoryField* field = m_fields.value( tileID );
		return field->items.size();
		
	}
	return 0;
}

/// @brief Returns the number of reserved (in-transit) items for the field identified by tileID.
/// @param tileID Integer key (Position::toInt()) of the field.
/// @return Reserved item count, or 0 if the tileID is not part of this stockpile.
int Stockpile::reserved( unsigned int tileID )
{
	if ( m_fields.contains( tileID ) )
	{

		InventoryField* field = m_fields.value( tileID );
		return field->reservedItems.size();
		
	}
	return 0;
}