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
#include "pathfinder.h"

#include "../base/PathFinderThread.h"
#include "../base/config.h"
#include "../base/global.h"
#include "../game/world.h"

#include <QDebug>
#include <QElapsedTimer>

PathFinder::PathFinder()
{
}

PathFinder::~PathFinder()
{
	m_threadPool->waitForDone();
	m_results.clear();
}

void PathFinder::init()
{
	m_threadPool = new QThreadPool( this );
	m_results.clear();

	int maxThreads = Config::getInstance().get( "maxThreads" ).toInt();
	m_threadPool->setMaxThreadCount( maxThreads );
	qDebug() << "create " << maxThreads << " pathfinding threads";
}

void PathFinder::cancelRequest( unsigned int id )
{
	//TODO implement
}

PathFinderResult PathFinder::getPath( unsigned int id, Position start, Position goal, bool ignoreNoPass, std::vector<Position>& path )
{
	QMutexLocker lock( &m_mutex );
	if ( m_currentRequestsFrom.contains( id ) )
	{
		if ( m_results.contains( id ) )
		{
			path = std::move( m_results[id] );
			m_results.remove( id );
			m_currentRequestsFrom.remove( id );
			if ( path.empty() )
			{
				return PathFinderResult::NoConnection;
			}
			else
			{
				return PathFinderResult::FoundPath;
			}
		}
		path.clear();
		return PathFinderResult::Running;
	}
	else
	{
		// Fast synchronous checks
		if ( !Global::w().isWalkable( goal ) )
		{
			return PathFinderResult::NoConnection;
		}

		if ( !checkConnectedRegions( start, goal ) )
		{
			return PathFinderResult::NoConnection;
		}
		if ( start.distSquare( goal, 2 ) == 1 )
		{
			path = { goal };
			return PathFinderResult::FoundPath;
		}
		if ( start.z == goal.z && start.distSquare( goal ) < 10 )
		{
			auto naivePath = getNaivePath( start, goal );

			if ( !naivePath.empty() )
			{
				path = naivePath;
				return PathFinderResult::FoundPath;
			}
		}

		// If no trivial solution exists, fork to worker
		m_currentRequestsFrom.insert( id );
		PathFinderThread* pft = new PathFinderThread( start, { goal }, ignoreNoPass, [this, id]( Position start, Position goal, PathFinderThread::Path path ) -> void {
			onThreadFinished( id, std::move( path ) );
		} );
		m_threadPool->start( pft );

		return PathFinderResult::Running;
	}
}

void PathFinder::onThreadFinished( unsigned int consumerId, std::vector<Position> path )
{
	QMutexLocker lock( &m_mutex );
	m_results.insert( consumerId, std::move( path ) );
}

std::vector<Position> PathFinder::getNaivePath( Position& start, Position& goal )
{
	std::vector<Position> out;

	Position current = goal;
	out.push_back( goal );
	while ( current != start )
	{
		bool changed = false;
		for ( auto neigh : Global::w().connectedNeighbors( current ) )
		{
			if ( ( neigh.z == current.z ) && ( neigh.distSquare( start ) < current.distSquare( start ) ) )
			{
				current = neigh;
				out.push_back( current );
				changed = true;
				break;
			}
		}
		if ( !changed )
		{
			out.clear();
			return out;
		}
	}
	return out;
}

bool PathFinder::checkConnectedRegions( const Position start, const Position goal )
{
	return Global::w().regionMap().checkConnectedRegions( start, goal );
}