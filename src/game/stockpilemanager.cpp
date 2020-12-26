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
#include "stockpilemanager.h"
#include "game.h"

#include "../base/config.h"
#include "../base/global.h"
#include "../game/job.h"
#include "../game/stockpile.h"
#include "../game/world.h"

#include <QDebug>

StockpileManager::StockpileManager( Game* parent ) :
	g( parent ),
	QObject( parent )
{
}

StockpileManager::~StockpileManager()
{
	for ( const auto& sp : m_stockpiles )
	{
		delete sp;
	}
}

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

Stockpile* StockpileManager::getStockpileAtPos( Position& pos )
{
	if ( m_allStockpileTiles.contains( pos.toInt() ) )
	{
		return m_stockpiles[m_allStockpileTiles[pos.toInt()]];
	}
	return nullptr;
}

Stockpile* StockpileManager::getStockpileAtTileID( unsigned int id )
{
	if ( m_allStockpileTiles.contains( id ) )
	{
		return m_stockpiles[m_allStockpileTiles[id]];
	}
	return nullptr;
}

Stockpile* StockpileManager::getStockpile( unsigned int id )
{
	if ( m_stockpiles.contains( id ) )
	{
		return m_stockpiles[id];
	}
	return nullptr;
}

Stockpile* StockpileManager::getLastAddedStockpile()
{
	if ( m_lastAdded && m_stockpiles.contains( m_lastAdded ) )
	{
		return m_stockpiles[m_lastAdded];
	}
	return m_stockpiles.last();
}

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

void StockpileManager::addContainer( unsigned int containerID, Position& pos )
{
	Stockpile* sp = getStockpileAtPos( pos );
	if ( sp )
	{
		sp->addContainer( containerID, pos );
	}
}

void StockpileManager::removeContainer( unsigned int containerID, Position& pos )
{
	Stockpile* sp = getStockpileAtPos( pos );
	if ( sp )
	{
		sp->removeContainer( containerID, pos );
	}
}

unsigned int StockpileManager::maxPriority()
{
	return m_stockpilesOrdered.size();
}

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

bool StockpileManager::allowsPull( unsigned int stockpileID )
{
	return m_stockpiles[stockpileID]->allowsPull();
}

bool StockpileManager::hasPriority( unsigned int stockpileID, unsigned int stockpileID2 )
{
	return m_stockpiles[stockpileID]->priority() < m_stockpiles[stockpileID2]->priority();
}

QString StockpileManager::name( unsigned int id )
{
	if ( m_stockpiles.contains( id ) )
	{
		return m_stockpiles[id]->name();
	}
	return "ERROR";
}

void StockpileManager::setInfiNotFull( Position pos )
{
	auto sp = getStockpileAtPos( pos );

	if ( sp )
	{
		sp->setInfiNotFull( pos );
	}
}
