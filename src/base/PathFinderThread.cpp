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
/** @file PathFinderThread.cpp
 *  @brief A* pathfinding worker thread implementation.
 *
 *  Runs the A* search algorithm on a separate thread, supporting 8-directional
 *  movement, stairs, scaffolds, and ramps. Multiple goals can be searched in a
 *  single invocation by reusing the explored path field.
 */
#include "PathFinderThread.h"

#include "../base/global.h"
#include "../game/world.h"

#include <QDebug>

#include <map>

#define DEADLYFLUIDLEVEL 4

/** @brief Constructs a PathFinderThread worker.
 *  @param world        Pointer to the game World for tile lookups.
 *  @param start        The search origin (typically the shared goal of multiple creatures).
 *  @param goals        Set of target positions to find paths to.
 *  @param ignoreNoPass If true, tiles marked TF_NOPASS are treated as passable.
 *  @param callback     Callback invoked for each goal once its path is found (or not).
 */
PathFinderThread::PathFinderThread( World* world, Position start, const std::unordered_set<Position>& goals, bool ignoreNoPass, PathFinderThread::CompletionCallback callback ) :
	m_world( world ),
	m_start( start ),
	m_goals( goals ),
	m_ignoreNoPass( ignoreNoPass ),
	m_callback( callback )
{
}

/** @brief Entry point for the worker thread. Invokes findPath(). */
void PathFinderThread::operator()()
{
	findPath();
}

/** @brief Evaluates a neighboring tile for A* expansion.
 *
 *  Checks whether @p next is walkable and passable (respecting ignoreNoPass),
 *  and that fluid level is below the lethal threshold. If so, computes the
 *  new cost and updates the path field and priority queue if this is a cheaper route.
 *
 *  @param current  The position currently being expanded.
 *  @param next     The neighbor position to evaluate.
 *  @param path     The path field mapping positions to costs and predecessors.
 *  @param frontier The A* priority queue.
 *  @param goal     The current target position (used for heuristic calculation).
 *  @return True if the neighbor is walkable and was considered, false otherwise.
 */
bool PathFinderThread::evalPos( const Position& current, const Position& next, std::unordered_map<Position, PathElement>& path, PriorityQueue<Position, double>& frontier, const Position& goal ) const
{
	const Tile& tile = m_world->getTile( next );

	if ( tile.flags & TileFlag::TF_WALKABLE && tile.fluidLevel < DEADLYFLUIDLEVEL )
	{
		if ( m_ignoreNoPass || !( tile.flags & TileFlag::TF_NOPASS ) )
		{
			double new_cost = path[current].cost + cost( current, next );
			// Try insert, doesn't overwrite if already visited
			auto nextIt   = path.insert( { next, { new_cost, current } } );
			auto& oldPath = nextIt.first->second;
			// If actually new or at least cheaper
			if ( nextIt.second || new_cost < oldPath.cost )
			{
				// then update costs and priority
				oldPath.cost     = new_cost;
				oldPath.previous = current;
				double priority  = new_cost + heuristic( next, goal );
				frontier.put( next, priority );
			}
			return true;
		}
	}
	return false;
}

/** @brief Evaluates a ramp-top neighbor for A* expansion.
 *
 *  Checks whether @p rampPos has a ramp-top floor type, then evaluates the tile
 *  directly below the ramp top for walkability. This handles downward movement
 *  via ramps from the current level to the level below a neighboring ramp top.
 *
 *  @param current  The position currently being expanded.
 *  @param rampPos  The adjacent position to check for a ramp top.
 *  @param path     The path field mapping positions to costs and predecessors.
 *  @param frontier The A* priority queue.
 *  @param goal     The current target position (used for heuristic calculation).
 *  @return True if the ramp-bottom tile is walkable and was considered, false otherwise.
 */
bool PathFinderThread::evalRampPos( const Position& current, const Position& rampPos, std::unordered_map<Position, PathElement>& path, PriorityQueue<Position, double>& frontier, const Position& goal ) const
{
	const Tile& rampTopTile = m_world->getTile( rampPos );

	if ( !(bool)( rampTopTile.floorType & FT_RAMPTOP ) )
		return false;

	auto next = rampPos.belowOf();

	const Tile& tile = m_world->getTile( next );

	if ( tile.flags & TileFlag::TF_WALKABLE && tile.fluidLevel < DEADLYFLUIDLEVEL )
	{
		if ( m_ignoreNoPass || !( tile.flags & TileFlag::TF_NOPASS ) )
		{
			double new_cost = path[current].cost + cost( current, next );
			// Try insert, doesn't overwrite if already visited
			auto nextIt   = path.insert( { next, { new_cost, current } } );
			auto& oldPath = nextIt.first->second;
			// If actually new or at least cheaper
			if ( nextIt.second || new_cost < oldPath.cost )
			{
				// then update costs and priority
				oldPath.cost     = new_cost;
				oldPath.previous = current;
				double priority  = new_cost + heuristic( next, goal );
				frontier.put( next, priority );
			}

			return true;
		}
	}
	return false;
}

/** @brief Runs the A* pathfinding algorithm for all goals.
 *
 *  Initializes the path field and priority queue from m_start, then iterates
 *  over each goal. For each goal, re-weights the frontier heuristic and continues
 *  expanding nodes. Explores 4 cardinal directions (plus ramp variants), 4 diagonal
 *  directions (only if both adjacent cardinal neighbors are walkable), and vertical
 *  movement via stairs, scaffolds, and ramps. The explored path field is reused
 *  across goals for efficiency. Invokes the completion callback for each goal
 *  with the reconstructed path (or empty if unreachable).
 */
void PathFinderThread::findPath()
{
	//qDebug() << "find path " << m_start.toString() << " " << m_goal.toString();
	std::unordered_map<Position, PathElement> pathField;
	PriorityQueue<Position, double> frontier;

	// Take a pessimistic guess for how many positions we need to visit
	int expectedPositions = 0;
	for(const auto& goal : m_goals)
	{
		expectedPositions = std::max( m_start.distSquare( goal, 3 ) / 8, expectedPositions );
	}
	pathField.reserve( expectedPositions );

	frontier.put( m_start, 0 );
	pathField.insert( { m_start, PathElement { 0.0, m_start } } );

	for ( const auto& goal : m_goals )
	{
		bool found = false;
		// Chance we already passed the current goal while searching for a previous one
		if ( pathField.count(goal) > 0 )
		{
			found = true;
		}
		else
		{
			// Re-weight frontier acording to current goal
			{
				auto oldFrontier = std::move( frontier );
				// Don't care about order
				for ( const auto& pos : oldFrontier.raw() )
				{
					const auto& field = pathField[pos.second];
					frontier.put( pos.second, field.cost + heuristic( pos.second, goal ) );
				}
			}
		}

		while ( !frontier.empty() && !found )
		{
			const auto current = frontier.top().second;

			// Either shortcut or fully process
			if ( current == goal )
			{
				found = true;
				break;
			}
			else
			{
				frontier.pop();
			}

			bool north = false;
			bool east  = false;
			bool south = false;
			bool west  = false;

			auto next = current.northOf();
			north     = evalPos( current, next, pathField, frontier, goal );
			evalRampPos( current, next, pathField, frontier, goal );

			next = current.eastOf();
			east = evalPos( current, next, pathField, frontier, goal );
			evalRampPos( current, next, pathField, frontier, goal );

			next  = current.southOf();
			south = evalPos( current, next, pathField, frontier, goal );
			evalRampPos( current, next, pathField, frontier, goal );

			next = current.westOf();
			west = evalPos( current, next, pathField, frontier, goal );
			evalRampPos( current, next, pathField, frontier, goal );

			if ( north && east )
			{
				next = current.neOf();
				evalPos( current, next, pathField, frontier, goal );
			}
			if ( east && south )
			{
				next = current.seOf();
				evalPos( current, next, pathField, frontier, goal );
			}
			if ( south && west )
			{
				next = current.swOf();
				evalPos( current, next, pathField, frontier, goal );
			}
			if ( west && north )
			{
				next = current.nwOf();
				evalPos( current, next, pathField, frontier, goal );
			}

			Tile& curTile = m_world->getTile( current );

			if ( (bool)( curTile.wallType & ( WT_STAIR | WT_SCAFFOLD ) ) )
			{
				next = current.aboveOf();
				evalPos( current, next, pathField, frontier, goal );
			}
			if ( (bool)( curTile.floorType & ( FT_STAIRTOP | FT_SCAFFOLD ) ) )
			{
				next = current.belowOf();
				evalPos( current, next, pathField, frontier, goal );
			}
			if ( (bool)( curTile.wallType & ( WT_RAMP ) ) )
			{
				auto above = current.aboveOf();
				next       = above.northOf();
				evalPos( current, next, pathField, frontier, goal );
				next = above.eastOf();
				evalPos( current, next, pathField, frontier, goal );
				next = above.southOf();
				evalPos( current, next, pathField, frontier, goal );
				next = above.westOf();
				evalPos( current, next, pathField, frontier, goal );
			}
		}

		Path path;
		if ( found )
		{
			Position next = goal;
			while ( next != m_start )
			{
				path.push_back( next );
				next = pathField[next].previous;
			}
		}
		else
		{
			/*
		qDebug() << "##################################################################################################";
		qDebug() << "no path found";
		qDebug() << "start: " << m_start.toString() << " goal: " << m_goal.toString();
		qDebug() << "##################################################################################################";
		*/
		}
		m_callback( m_start, goal, m_ignoreNoPass, std::move(path) );
	}
}
