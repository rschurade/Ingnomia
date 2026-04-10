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
/** @file pathfinder.cpp
 *  @brief Asynchronous A* pathfinding request manager.
 *
 *  Dispatches pathfinding requests to worker threads, caches results,
 *  and provides fast synchronous fallbacks for trivial cases.
 */
#include "pathfinder.h"

#include "../base/PathFinderThread.h"
#include "../base/config.h"
#include "../base/global.h"
#include "../game/world.h"

#include <QDebug>
#include <QElapsedTimer>

/** @brief Constructs the PathFinder.
 *  @param world Pointer to the game World used for tile and region queries.
 *  @param parent Optional QObject parent for Qt ownership.
 */
PathFinder::PathFinder( World* world, QObject* parent) :
	m_world( world ),
	QObject(parent)
{
}

/** @brief Destructor. Clears all pending pathfinding jobs. */
PathFinder::~PathFinder()
{
	m_jobs.clear();
}

/** @brief Cancels a pending pathfinding request (not yet implemented).
 *  @param id The unique identifier of the request to cancel.
 */
void PathFinder::cancelRequest( unsigned int id )
{
	//TODO implement
}

/** @brief Requests a path between two positions.
 *
 *  If a result for the given @p id already exists, returns it immediately.
 *  If a job is still running, returns PathFinderResult::Running.
 *  Otherwise performs fast synchronous checks (walkability, region connectivity,
 *  adjacency, naive same-level path) before queuing an async A* job.
 *
 *  @param id           Unique identifier for this pathfinding request.
 *  @param start        The starting position.
 *  @param goal         The target position.
 *  @param ignoreNoPass If true, tiles marked TF_NOPASS are treated as passable.
 *  @param[out] path    Populated with the computed path on success, cleared otherwise.
 *  @return The current state of the request (FoundPath, NoConnection, Running, or Pending).
 */
PathFinderResult PathFinder::getPath( unsigned int id, Position start, Position goal, bool ignoreNoPass, std::vector<Position>& path )
{
	QMutexLocker lock( &m_mutex );
	auto it = m_jobs.find( id );
	if ( it != m_jobs.end() )
	{
		if ( it->state != PathFinderResult::Running && it->state != PathFinderResult::Pending )
		{
			path = std::move( it->path );
			const auto result = it->state;
			m_jobs.erase( it );
			return result;
		}
		path.clear();
		return PathFinderResult::Running;
	}
	else
	{
		// Fast synchronous checks
		if ( !m_world->isWalkableGnome( goal ) )
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
		auto it = m_jobs.insert(
			id,
			{
				start,
				goal,
				ignoreNoPass,
				PathFinderResult::Pending,
				{}
			}
		);

		return it->state;
	}
}

/** @brief Launches async A* worker threads for all pending pathfinding jobs.
 *
 *  Collects pending jobs, marks them as running, then batches requests that share
 *  the same start or goal position into a single worker (with inverted direction,
 *  since many creatures often target the same goal). Blocks until all workers complete.
 */
void PathFinder::findPaths()
{
	using namespace std::placeholders;

	std::vector<std::future<void>> tasks;
	decltype( m_jobs ) jobs;

	{
		QMutexLocker lock( &m_mutex );

		// Filter jobs which are not pending yet
		for ( auto it = m_jobs.begin(); it != m_jobs.end(); ++it )
		{
			if ( it.value().state == PathFinderResult::Pending )
			{
				it.value().state = PathFinderResult::Running;
				jobs.insert( it.key(), it.value() );
			}
		}
	}

	for ( auto it = jobs.begin(); it != jobs.end(); ++it)
	{
		// Common case is for many creatures to have the same goal, so invert path finding direction by default
		const Position start = it->goal;
		std::unordered_set<Position> goals = { it->start };
		const bool ignoreNoPass = it->ignoreNoPass;
		// Fold all other requests which share either of goal or start into this one
		for (auto it2 = it + 1; it2 != jobs.end();)
		{
			if (it2->ignoreNoPass == ignoreNoPass)
			{
				if (it2->goal == start)
				{
					goals.emplace( it2->start );
					it2 = jobs.erase( it2 );
				}
				else if (it2->start == start)
				{
					goals.emplace( it2->goal );
					it2 = jobs.erase( it2 );
				}
				else
				{
					++it2;
				}
			}
			else
			{
				++it2;
			}
		}
		tasks.emplace_back(std::async(
			std::launch::async,
			PathFinderThread( m_world, start, std::move( goals ), ignoreNoPass, std::bind( &PathFinder::onResult, this, _1, _2, _3, _4 ) )
		));
	}
	for ( auto& task : tasks )
	{
		task.get();
	}
}


/** @brief Callback invoked by worker threads when a path search completes.
 *
 *  Matches the result back to the original job(s). If the path was computed in
 *  reverse direction (goal-to-start optimization), reverses the path before storing.
 *
 *  @param start        The search origin used by the worker (may be the original goal).
 *  @param goal         The search target used by the worker (may be the original start).
 *  @param ignoreNoPass Whether TF_NOPASS tiles were ignored during the search.
 *  @param path         The computed path from start to goal, empty if no path found.
 */
void PathFinder::onResult( Position start, Position goal, bool ignoreNoPass, std::vector<Position> path )
{
	QMutexLocker lock( &m_mutex );
	for ( auto& job : m_jobs )
	{
		const bool forwardPath = ( job.goal == goal && job.start == start );
		const bool reversePath = ( job.goal == start && job.start == goal );
		if ( ( forwardPath || reversePath ) && job.ignoreNoPass == ignoreNoPass )
		{
			if ( path.empty() )
			{
				job.state = PathFinderResult::NoConnection;
			}
			else
			{
				job.state = PathFinderResult::FoundPath;
				if ( forwardPath )
				{
					job.path = std::move( path );
				}
				else
				{
					// Copy in reverse order, minus element [0] (our current position), and prepend our real goal instead
					job.path.reserve( path.size() );
					job.path.emplace_back( start );
					for ( auto it = path.crbegin(); it != path.crend() - 1; ++it )
					{
						job.path.emplace_back( *it );
					}
				}
			}
		}
	}
}


/** @brief Attempts a simple greedy walk from goal to start on the same Z-level.
 *
 *  At each step, picks the connected neighbor closest to start. Only considers
 *  same-level neighbors. Returns an empty vector if the greedy walk gets stuck.
 *
 *  @param start The starting position (walk destination).
 *  @param goal  The goal position (walk origin — path is built goal-to-start).
 *  @return The path from goal to start, or empty if the greedy approach fails.
 */
std::vector<Position> PathFinder::getNaivePath( Position& start, Position& goal )
{
	std::vector<Position> out;

	Position current = goal;
	out.push_back( goal );
	while ( current != start )
	{
		bool changed = false;
		for ( auto neigh : m_world->connectedNeighbors( current ) )
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

/** @brief Checks whether two positions are in connected regions.
 *
 *  Delegates to RegionMap::checkConnectedRegions to perform a BFS over the
 *  region connectivity graph.
 *
 *  @param start The first position.
 *  @param goal  The second position.
 *  @return True if the regions containing start and goal are connected.
 */
bool PathFinder::checkConnectedRegions( const Position start, const Position goal )
{
	return m_world->regionMap().checkConnectedRegions( start, goal );
}
