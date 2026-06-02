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
/** @file stockpilemanager.cpp
 *  @brief Manages all stockpiles: creation, removal, tile edits, job dispatch,
 *         priority ordering, container placement, and suspend-status signals.
 */
#include "stockpilemanager.h"
#include "game.h"

#include "../base/config.h"
#include "../base/global.h"
#include "../game/job.h"
#include "../game/stockpile.h"
#include "../game/world.h"

#include <QDebug>

/// @brief Constructs the stockpile manager.
/// @param parent Owning Game instance.
StockpileManager::StockpileManager( Game* parent ) :
	g( parent ),
	QObject( parent )
{
}

/// @brief Destructor. Deletes all Stockpile objects.
StockpileManager::~StockpileManager()
{
	for ( const auto& sp : m_stockpiles )
	{
		delete sp;
	}
}

/// @brief Per-tick update: removes any empty, job-free stockpiles, then ticks each remaining
///        stockpile to refresh candidate item lists (stops after first stockpile that updates).
/// @param tick Current game tick.
void StockpileManager::onTick( quint64 tick )
{
	for ( auto& sp : m_stockpiles )
	{
		if ( sp->countFields() == 0 && !sp->stillHasJobs() )
		{
			removeStockpile( sp->id() );
			break;
		}
	}

	for ( auto& sp : m_stockpiles )
	{
		if ( sp->onTick( tick ) )
		{
			//stockpile updated its possible item list
			break;
		}
	}
}

/// @brief Creates a new stockpile from @p fields, or extends an existing one if @p firstClick
///        already belongs to a stockpile. Emits signalStockpileAdded.
/// @param firstClick Position of the player's first click; used to detect extension.
/// @param fields     List of (Position, included) pairs for all designated tiles.
void StockpileManager::addStockpile( Position& firstClick, QList<QPair<Position, bool>> fields )
{
	if ( m_allStockpileTiles.contains( firstClick.toInt() ) )
	{
		unsigned int spID = m_allStockpileTiles.value( firstClick.toInt() );
		Stockpile* sp     = m_stockpiles[spID];
		for ( auto p : fields )
		{
			if ( p.second && !m_allStockpileTiles.contains( p.first.toInt() ) )
			{
				m_allStockpileTiles.insert( p.first.toInt(), spID );
				sp->addTile( p.first );
			}
		}
		m_lastAdded = spID;
	}
	else
	{
		Stockpile* sp = new Stockpile( fields, g );
		for ( auto p : fields )
		{
			if ( p.second )
			{
				m_allStockpileTiles.insert( p.first.toInt(), sp->id() );
			}
		}
		sp->setPriority( m_stockpilesOrdered.size() );
		m_stockpiles.insert( sp->id(), sp );
		m_stockpilesOrdered.append( sp->id() );
		m_lastAdded = sp->id();
	}
	qDebug() << "StockpileManager::addStockpile";
	emit signalStockpileAdded( m_lastAdded );
}

/// @brief Deserialises and registers a stockpile from a saved QVariantMap.
/// @param vals QVariantMap produced by Stockpile::serialize().
void StockpileManager::load( QVariantMap vals )
{
	Stockpile* sp = new Stockpile( vals, g );
	for ( auto sf : vals.value( "Fields" ).toList() )
	{
		auto sfm = sf.toMap();
		m_allStockpileTiles.insert( Position( sfm.value( "Pos" ) ).toInt(), sp->id() );
	}
	sp->setPriority( m_stockpilesOrdered.size() );
	m_stockpiles.insert( sp->id(), sp );
	m_stockpilesOrdered.append( sp->id() );
}

/// @brief Deletes the stockpile with the given ID, removes it from all tracking structures,
///        and emits signalStockpileDeleted.
/// @param id UID of the stockpile to remove.
void StockpileManager::removeStockpile( unsigned int id )
{
	auto sp = m_stockpiles.value( id );
	if ( sp )
	{
		delete sp;
	}
	m_stockpiles.remove( id );
	m_stockpilesOrdered.removeAll( id );
	emit signalStockpileDeleted( id );
}

/// @brief Removes a single tile from its stockpile. If it was the last tile and no jobs remain,
///        the stockpile is deleted; otherwise signalStockpileDeleted is emitted to notify the GUI.
/// @param pos World position of the tile to remove.
void StockpileManager::removeTile( Position& pos )
{
	Stockpile* sp = getStockpileAtPos( pos );
	if ( sp && sp->removeTile( pos ) )
	{
		unsigned int id = sp->id();
		if ( !sp->stillHasJobs() )
		{
			removeStockpile( id );
		}
		else
		{
			emit signalStockpileDeleted( id );
		}
	}
	m_allStockpileTiles.remove( pos.toInt() );
}

/// @brief Returns the stockpile that owns the tile at @p pos, or nullptr if none.
/// @param pos World position to look up.
/// @return Pointer to the owning Stockpile, or nullptr.
Stockpile* StockpileManager::getStockpileAtPos( Position& pos )
{
	if ( m_allStockpileTiles.contains( pos.toInt() ) )
	{
		return m_stockpiles[m_allStockpileTiles[pos.toInt()]];
	}
	return nullptr;
}

/// @brief Returns the stockpile that owns the tile with integer key @p id, or nullptr if none.
/// @param id Integer tile key (Position::toInt()).
/// @return Pointer to the owning Stockpile, or nullptr.
Stockpile* StockpileManager::getStockpileAtTileID( unsigned int id )
{
	if ( m_allStockpileTiles.contains( id ) )
	{
		return m_stockpiles[m_allStockpileTiles[id]];
	}
	return nullptr;
}

/// @brief Returns the stockpile with the given UID, or nullptr if not found.
/// @param id UID of the stockpile.
/// @return Pointer to the Stockpile, or nullptr.
Stockpile* StockpileManager::getStockpile( unsigned int id )
{
	if ( m_stockpiles.contains( id ) )
	{
		return m_stockpiles[id];
	}
	return nullptr;
}

/// @brief Returns the most recently added stockpile, falling back to the last in the ordered list.
/// @return Pointer to the last-added Stockpile.
Stockpile* StockpileManager::getLastAddedStockpile()
{
	if ( m_lastAdded && m_stockpiles.contains( m_lastAdded ) )
	{
		return m_stockpiles[m_lastAdded];
	}
	return m_stockpiles.last();
}

/// @brief Inserts @p item into the stockpile @p stockpileID at @p pos. Emits content-changed
///        and suspend-status-changed signals as needed.
/// @param stockpileID UID of the target stockpile.
/// @param pos         Destination field position.
/// @param item        UID of the item to insert.
void StockpileManager::insertItem( unsigned int stockpileID, Position pos, unsigned int item )
{
	if ( m_stockpiles.contains( stockpileID ) )
	{
		if ( !m_stockpiles[stockpileID]->insertItem( pos, item ) )
		{
			if ( Global::debugMode )
				qDebug() << "insert into stockpile " << stockpileID << " failed";
		}
		else
		{
			emit signalStockpileContentChanged( stockpileID );
			if ( m_stockpiles[stockpileID]->suspendChanged() )
			{
				emit signalSuspendStatusChanged( stockpileID );
			}
		}
	}
}

/// @brief Removes @p item from the stockpile @p stockpileID at @p pos. Emits content-changed
///        and suspend-status-changed signals as needed.
/// @param stockpileID UID of the stockpile.
/// @param pos         Field position.
/// @param item        UID of the item to remove.
void StockpileManager::removeItem( unsigned int stockpileID, Position pos, unsigned int item )
{
	if ( m_stockpiles.contains( stockpileID ) )
	{
		m_stockpiles[stockpileID]->removeItem( pos, item );
		emit signalStockpileContentChanged( stockpileID );
		if ( m_stockpiles[stockpileID]->suspendChanged() )
		{
			emit signalSuspendStatusChanged( stockpileID );
		}
	}
}

/// @brief Iterates stockpiles in priority order and returns the first available hauling job ID.
///        First tries normal haul jobs, then consolidation (clean-up) jobs.
/// @return Job ID, or 0 if no job is available.
unsigned int StockpileManager::getJob()
{
	for ( auto stockpileID : m_stockpilesOrdered )
	{
		unsigned int job = m_stockpiles[stockpileID]->getJob();
		if ( job )
		{
			if ( m_stockpiles[stockpileID]->suspendChanged() )
			{
				emit signalSuspendStatusChanged( stockpileID );
			}
			return job;
		}
	}

	for ( auto stockpileID : m_stockpilesOrdered )
	{
		unsigned int job = m_stockpiles[stockpileID]->getCleanUpJob();
		if ( job )
		{
			if ( m_stockpiles[stockpileID]->suspendChanged() )
			{
				emit signalSuspendStatusChanged( stockpileID );
			}
			return job;
		}
	}

	return 0;
}

/// @brief Notifies the owning stockpile that job @p jobID has completed successfully.
/// @param jobID ID of the completed job.
/// @return true if the job was found and finished.
bool StockpileManager::finishJob( unsigned int jobID )
{
	for ( auto& sp : m_stockpiles )
	{
		if ( sp->finishJob( jobID ) )
		{
			return true;
		}
	}
	return false;
}

/// @brief Cancels or returns job @p jobID to the owning stockpile (e.g. gnome couldn't complete it).
///        Emits signalSuspendStatusChanged if limits changed as a result.
/// @param jobID ID of the job to return.
/// @return true if the job was found and returned.
bool StockpileManager::giveBackJob( unsigned int jobID )
{
	for ( auto& sp : m_stockpiles )
	{
		if ( sp->giveBackJob( jobID ) )
		{
			if ( sp->suspendChanged() )
			{
				emit signalSuspendStatusChanged( sp->id() );
			}
			return true;
		}
	}
	return false;
}

/// @brief Returns the Job object for @p jobID from whichever stockpile owns it.
/// @param jobID ID of the job to retrieve.
/// @return Shared pointer to the job.
QSharedPointer<Job> StockpileManager::getJob( unsigned int jobID )
{
	for ( const auto& sp : m_stockpiles )
	{
		if ( sp->hasJobID( jobID ) )
		{
			return sp->getJob( jobID );
		}
	}
}

/// @brief Returns whether any stockpile owns the job with the given ID.
/// @param jobID ID to check.
/// @return true if the job is found in any stockpile.
bool StockpileManager::hasJobID( unsigned int jobID ) const
{
	for ( const auto& sp : m_stockpiles )
	{
		if ( sp->hasJobID( jobID ) )
		{
			return true;
		}
	}
	return false;
}

/// @brief Forwards a container placement request to the stockpile at @p pos.
/// @param source       Container source material descriptor.
/// @param capacity     Maximum items the container holds.
/// @param requireSame  Require all container items to be the same type and material.
/// @param allowedItems ItemSIDs permitted in this container.
/// @param pos          World position of the field.
void StockpileManager::addContainer( const SourceMaterial& source, unsigned char capacity, bool requireSame, const QStringList& allowedItems, Position& pos )
{
	Stockpile* sp = getStockpileAtPos( pos );
	if ( sp )
	{
		sp->addContainer( source, capacity, requireSame, allowedItems, pos );
	}
}

/// @brief Forwards a container removal request to the stockpile at @p pos.
/// @param pos World position of the field whose container should be removed.
void StockpileManager::removeContainer( Position& pos )
{
	Stockpile* sp = getStockpileAtPos( pos );
	if ( sp )
	{
		sp->removeContainer( pos );
	}
}

/// @brief Returns the total number of stockpiles (used as the maximum priority index).
/// @return Count of registered stockpiles.
unsigned int StockpileManager::maxPriority()
{
	return m_stockpilesOrdered.size();
}

/// @brief Moves the stockpile one position earlier in the priority order.
/// @param stockpileID UID of the stockpile to promote.
void StockpileManager::movePriorityUp( unsigned int stockpileID )
{
	int index = m_stockpilesOrdered.indexOf( stockpileID );
	if ( index > 0 )
	{
		m_stockpilesOrdered.move( index, index - 1 );
		m_stockpiles[m_stockpilesOrdered[index - 1]]->setPriority( index - 1 );
		m_stockpiles[m_stockpilesOrdered[index]]->setPriority( index );
	}
}

/// @brief Sets the stockpile to an absolute position in the priority order and renumbers all others.
/// @param stockpileID UID of the stockpile to move.
/// @param priority    Target zero-based index in the ordered list.
void StockpileManager::setPriority( unsigned int stockpileID, int priority )
{
	int index = m_stockpilesOrdered.indexOf( stockpileID );

	if ( index != -1 && index < m_stockpilesOrdered.size() )
	{
		m_stockpilesOrdered.takeAt( index );
		m_stockpilesOrdered.insert( priority, stockpileID );

		for ( int i = 0; i < m_stockpilesOrdered.size(); ++i )
		{
			m_stockpiles[m_stockpilesOrdered[i]]->setPriority( i );
		}
	}
}

/// @brief Moves the stockpile one position later in the priority order.
/// @param stockpileID UID of the stockpile to demote.
void StockpileManager::movePriorityDown( unsigned int stockpileID )
{
	int index = m_stockpilesOrdered.indexOf( stockpileID );
	if ( index != -1 && index < m_stockpilesOrdered.size() - 1 )
	{
		m_stockpilesOrdered.move( index, index + 1 );

		m_stockpiles[m_stockpilesOrdered[index + 1]]->setPriority( index + 1 );
		m_stockpiles[m_stockpilesOrdered[index]]->setPriority( index );
	}
}

/// @brief Moves the stockpile to the highest priority (index 0) and renumbers all others.
/// @param stockpileID UID of the stockpile to promote to top.
void StockpileManager::movePriorityTop( unsigned int stockpileID )
{
	int index = m_stockpilesOrdered.indexOf( stockpileID );
	if ( index > 0 )
	{
		m_stockpilesOrdered.takeAt( index );
		m_stockpilesOrdered.insert( 0, stockpileID );
		for ( int i = 0; i < m_stockpilesOrdered.size(); ++i )
		{
			m_stockpiles[m_stockpilesOrdered[i]]->setPriority( i );
		}
	}
}

/// @brief Moves the stockpile to the lowest priority (last index) and renumbers all others.
/// @param stockpileID UID of the stockpile to demote to bottom.
void StockpileManager::movePriorityBottom( unsigned int stockpileID )
{
	int index = m_stockpilesOrdered.indexOf( stockpileID );
	if ( index != -1 && index < m_stockpilesOrdered.size() - 1 )
	{
		m_stockpilesOrdered.takeAt( index );
		m_stockpilesOrdered.append( stockpileID );
		for ( int i = 0; i < m_stockpilesOrdered.size(); ++i )
		{
			m_stockpiles[m_stockpilesOrdered[i]]->setPriority( i );
		}
	}
}

/// @brief Returns whether the stockpile with @p stockpileID allows others to pull items from it.
/// @param stockpileID UID of the stockpile to query.
/// @return true if the stockpile's allowPull flag is set.
bool StockpileManager::allowsPull( unsigned int stockpileID )
{
	return m_stockpiles[stockpileID]->allowsPull();
}

/// @brief Returns whether @p stockpileID has higher priority (lower index) than @p stockpileID2.
/// @param stockpileID  First stockpile UID.
/// @param stockpileID2 Second stockpile UID.
/// @return true if stockpileID's priority < stockpileID2's priority.
bool StockpileManager::hasPriority( unsigned int stockpileID, unsigned int stockpileID2 )
{
	return m_stockpiles[stockpileID]->priority() < m_stockpiles[stockpileID2]->priority();
}

/// @brief Returns the display name of the stockpile with the given ID.
/// @param id UID of the stockpile.
/// @return Name string, or "ERROR" if not found.
QString StockpileManager::name( unsigned int id )
{
	if ( m_stockpiles.contains( id ) )
	{
		return m_stockpiles[id]->name();
	}
	return "ERROR";
}

/// @brief Clears the isFull flag on the field at @p pos in whichever stockpile owns it.
/// @param pos World position of the field.
void StockpileManager::setInfiNotFull( Position pos )
{
	auto sp = getStockpileAtPos( pos );

	if ( sp )
	{
		sp->setInfiNotFull( pos );
	}
}
