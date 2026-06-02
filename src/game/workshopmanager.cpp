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
/** @file workshopmanager.cpp
 *  @brief Workshop lifecycle management: creation, deletion, per-tick updates,
 *         craft job dispatch, priority ordering, and auto-generated craft jobs.
 */
#include "workshopmanager.h"
#include "game.h"

#include "../base/gamestate.h"

#include <QDebug>
#include <QJsonDocument>

/// @brief Constructs the workshop manager.
/// @param parent Owning Game instance.
WorkshopManager::WorkshopManager( Game* parent ) :
	g( parent ),
	QObject( parent )
{
}

/// @brief Destructor. Deletes all Workshop objects.
WorkshopManager::~WorkshopManager()
{
	for ( const auto& ws : m_workshops )
	{
		delete ws;
	}
}

/// @brief Per-tick update. Processes deferred workshop deletions (retrying until canDelete()
///        returns true), then ticks every active workshop.
/// @param tick Current game tick.
void WorkshopManager::onTick( quint64 tick )
{
	if ( !m_toDelete.isEmpty() )
	{
		int tds = m_toDelete.size();
		for ( int i = 0; i < tds; ++i )
		{
			unsigned int id = m_toDelete.dequeue();
			Workshop* ws    = workshop( id );
			if ( ws->canDelete() )
			{
				for ( int k = 0; k < m_workshops.size(); ++k )
				{
					if ( m_workshops[k]->id() == id )
					{
						m_workshops.removeAt( k );
						break;
					}
				}
			}
			else
			{
				m_toDelete.enqueue( id );
			}
		}
	}

	for ( auto& w : m_workshops )
	{
		w->onTick( tick );
	}
}

/// @brief Creates and registers a new workshop of the given type.
/// @param type     Workshop type string (DB key).
/// @param pos      World position of the workshop's anchor tile.
/// @param rotation Orientation (0–3).
/// @return Pointer to the newly created Workshop.
Workshop* WorkshopManager::addWorkshop( QString type, Position& pos, int rotation )
{
	Workshop* w = new Workshop( type, pos, rotation, g );
	m_workshops.push_back( w );

	return m_workshops.last();
}

/// @brief Deserialises and registers a workshop from a saved QVariantMap.
/// @param vals QVariantMap produced by Workshop::serialize().
void WorkshopManager::addWorkshop( QVariantMap vals )
{
	Workshop* w = new Workshop( vals, g );
	m_workshops.push_back( w );
}

/// @brief Returns whether any workshop occupies the tile at @p pos.
/// @param pos World position to check.
/// @return true if a workshop tile is at @p pos.
bool WorkshopManager::isWorkshop( Position& pos )
{
	for ( const auto& w : m_workshops )
	{
		if ( w->isOnTile( pos ) )
		{
			return true;
		}
	}
	return false;
}

/// @brief Returns the workshop occupying @p pos, or nullptr if none.
/// @param pos World position to look up.
/// @return Pointer to the Workshop at @p pos, or nullptr.
Workshop* WorkshopManager::workshopAt( const Position& pos )
{
	for ( auto& w : m_workshops )
	{
		if ( w->isOnTile( pos ) )
		{
			return w;
		}
	}
	return 0;
}

/// @brief Returns the workshop with the given UID, or nullptr if not found.
/// @param ID UID of the workshop.
/// @return Pointer to the Workshop, or nullptr.
Workshop* WorkshopManager::workshop( unsigned int ID )
{
	for ( auto& w : m_workshops )
	{
		if ( w->id() == ID )
		{
			return w;
		}
	}
	return 0;
}

/// @brief Queues a workshop for deferred deletion. The deletion is retried each tick
///        until Workshop::canDelete() returns true.
/// @param workshopID UID of the workshop to delete.
void WorkshopManager::deleteWorkshop( unsigned int workshopID )
{
	m_toDelete.push_back( workshopID );
}

/// @brief Automatically generates a craft job for @p itemSID/@p materialSID in the workshop
///        with the fewest jobs that can produce it. Only creates the job if none already exists.
///        Prefers workshops that accept generated jobs (isAcceptingGenerated()).
/// @param itemSID     Item type string ID to craft.
/// @param materialSID Material string ID for the craft.
/// @param amount      Number of items to request.
/// @return true if a craft job was successfully created.
bool WorkshopManager::autoGenCraftJob( QString itemSID, QString materialSID, int amount )
{
	if( !craftJobExists( itemSID, materialSID ) )
	{
		QList<Workshop*> possibles;
		for ( auto&& w : m_workshops )
		{
			if ( w->isAcceptingGenerated() )
			{
				if( w->canCraft( itemSID, materialSID ) )
				{
					possibles.append( w );
				}
			}
		}
		auto compare = []( Workshop* a, Workshop* b) { return a->numJobs() < b->numJobs(); };
		std::sort( std::begin( possibles ), std::end( possibles ), compare );

		for ( auto&& w : possibles )
		{
			if ( w->autoCraft( itemSID, materialSID, amount ) )
			{
				emit signalJobListChanged( w->id() );
				return true;
			}
		}
	}
	return false;
}

/// @brief Returns whether any auto-accepting workshop already has a craft job for the given item/material.
/// @param itemSID     Item type string ID.
/// @param materialSID Material string ID.
/// @return true if a matching craft job is already queued.
bool WorkshopManager::craftJobExists( const QString& itemSID, const QString& materialSID )
{
	for ( const auto& w : m_workshops )
	{
		if ( w->isAcceptingGenerated() )
		{
			if ( w->hasCraftJob( itemSID, materialSID ) )
			{
				return true;
			}
		}
	}
	return false;
}

/// @brief Moves the workshop to position @p prio in the ordered workshop list.
/// @param workshopID UID of the workshop to reorder.
/// @param prio       Target zero-based index (clamped to valid range).
void WorkshopManager::setPriority( unsigned int workshopID, int prio )
{
	if( prio > 0 && prio < m_workshops.size() )
	{
		int current = 0;
		for ( const auto& w : m_workshops )
		{
			if ( w->id() == workshopID )
			{
				break;
			}
			++current;
		}
		m_workshops.move( current, prio );
	}
}

/// @brief Returns the current priority index (position in the ordered list) of the workshop.
/// @param workshopID UID of the workshop.
/// @return Zero-based index in m_workshops; returns m_workshops.size() if not found.
int WorkshopManager::priority( unsigned int workshopID )
{
	int current = 0;
	for ( const auto& w : m_workshops )
	{
		if ( w->id() == workshopID )
		{
			break;
		}
		++current;
	}
	return current;
}

/// @brief Returns all workshops of type "MeleeTraining" (used by the military manager to assign squads).
/// @return List of pointers to all melee-training workshops.
QList<Workshop*> WorkshopManager::getTrainingGrounds()
{
	QList<Workshop*> out;
	for ( auto& w : m_workshops )
	{
		if ( w->type() == "MeleeTraining" )
		{
			out.append( w );
		}
	}
	return out;
}

/// @brief Emits signalJobListChanged for the given workshop (used by Workshop to notify the GUI).
/// @param workshopID UID of the workshop whose job list changed.
void WorkshopManager::emitJobListChanged( unsigned int workshopID )
{
	emit signalJobListChanged( workshopID );
}