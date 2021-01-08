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
#include "../game/inventory.h"
#include "../game/job.h"
#include "../game/jobmanager.h"
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
		if ( sp->countFields() == 0 )
		{
			removeStockpile( sp->id() );
			break;
		}
	}


	if( m_currentTickStockpile < m_stockpilesOrdered.size() )
	{
		auto sp = m_stockpiles.value( m_stockpilesOrdered[m_currentTickStockpile++] );
		if( sp )
		{
			sp->onTick( tick );
		}
	}
	else
	{
		m_currentTickStockpile = 0;
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
		removeStockpile( id );
		emit signalStockpileDeleted( id );
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

unsigned int StockpileManager::getJob( const Position& gnomePos )
{
	unsigned int jobID =  createStandardHaulingJob( gnomePos );
	if( jobID )
	{
		return jobID;
	}

	return 0;
}

unsigned int StockpileManager::createStandardHaulingJob( const Position& gnomePos )
{
	// get closest reachable item which is not in a stockpile for which an accepting
	// stockpile exists and is reachable
	QSet<QPair<QString, QString>> acceptedItems;
	for( const auto& sp : m_stockpiles )
	{
		acceptedItems.unite( sp->canAcceptSet() );
	}
	unsigned int itemID = g->inv()->getClosestUnstockedItem( gnomePos, acceptedItems );
	if( itemID )
	{
		// get the closest accepting stockpile for that item
		auto sp = getClosestAcceptingStockpile( itemID );
		if( sp )
		{
			// check multi: the order of these 3 checks has to be determined so it could be 
			// aborted as soon as possible
			// carry container exists
			// more of that item/material combo in range of that item
			// stockpile accepts more than one of that item
			bool multiPossible = false;
			if( multiPossible )
			{
				return createMultiHaulingJob( gnomePos, 0 );
			}
			// create single hauling job
			return createSingleHaulingJob( gnomePos, itemID, sp );
		}
	}

	return 0;
}

unsigned int StockpileManager::createSingleHaulingJob( const Position& gnomePos, unsigned int itemID, Stockpile* sp )
{
	QSharedPointer<Job> job( new Job );
	job->setType( "HauleItem" );
	job->setRequiredSkill( "Hauling" );

	job->setStockpile( sp->id() );

	job->addItemToHaul( itemID );
	g->inv()->setInJob( itemID, job->id() );

	Position targetPos;
	if( sp->reserveItem( itemID, targetPos ) )
	{
		job->setPos( targetPos );
		job->setPosItemInput( targetPos );
		job->addPossibleWorkPosition( targetPos );
		job->setWorkPos( targetPos );
		job->setDestroyOnAbort( true );
		job->setNoJobSprite( true );
		return g->jm()->addHaulingJob( job );
	}
	return 0;
}

unsigned int StockpileManager::createMultiHaulingJob( const Position& gnomePos, unsigned int itemID )
{
	return 0;
}

Stockpile* StockpileManager::getClosestAcceptingStockpile( unsigned int itemID )
{
	Stockpile* out = nullptr;
	int maxDist = 999999999;
	auto itemPos = g->inv()->getItemPos( itemID );
	auto pairSID = g->inv()->getPairSID( itemID );
	for( const auto& sp : m_stockpiles )
	{
		if( sp->canAccept( pairSID ) )
		{
			int dist = 0;
			if( sp->isConnectedTo( itemPos, dist ) )
			{
				if( dist < maxDist )
				{
					out = sp;
					maxDist = dist;
				}
			}
		}
	}
	return out;
}

void StockpileManager::unreserveItem( unsigned int stockpileID, unsigned int itemID )
{
	if( m_stockpiles.contains( stockpileID ) )
	{
		m_stockpiles.value( stockpileID )->unreserveItem( itemID );
	}
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

QSet<QPair<QString, QString>> StockpileManager::allAcceptedItems()
{
	QSet<QPair<QString, QString>> acceptedItems;
	for( const auto& sp : m_stockpiles )
	{
		acceptedItems.unite( sp->canAcceptSet() );
	}
	return acceptedItems;
}