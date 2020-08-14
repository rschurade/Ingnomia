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
#include "PathFinderThread.h"

#include "../base/global.h"
#include "../game/world.h"

#include <QDebug>

#include <map>

#define DEADLYFLUIDLEVEL 6

PathFinderThread::PathFinderThread( Position start, Position goal, bool ignoreNoPass, PathFinderThread::CompletionCallback callback ) :
	m_start( start ),
	m_goal( goal ),
	m_ignoreNoPass( ignoreNoPass ),
	m_callback( callback )
{
}

PathFinderThread::~PathFinderThread()
{
}

void PathFinderThread::run()
{
	m_callback( findPath() );
}

bool PathFinderThread::evalPos( const Position& current, const Position& next, std::unordered_map<Position, PathElement>& path, PriorityQueue<Position, double>& frontier )
{
	const Tile& tile = Global::w().getTile( next );

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
				double priority  = new_cost + heuristic( next, m_goal );
				frontier.put( next, priority );
			}
			return true;
		}
	}
	return false;
}

bool PathFinderThread::evalRampPos( const Position& current, const Position& rampPos, std::unordered_map<Position, PathElement>& path, PriorityQueue<Position, double>& frontier )
{
	const Tile& rampTopTile = Global::w().getTile( rampPos );

	if ( !(bool)( rampTopTile.floorType & FT_RAMPTOP ) )
		return false;

	auto next = rampPos.belowOf();

	const Tile& tile = Global::w().getTile( next );

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
				double priority  = new_cost + heuristic( next, m_goal );
				frontier.put( next, priority );
			}

			return true;
		}
	}
	return false;
}

PathFinderThread::Path PathFinderThread::findPath()
{
	//qDebug() << "find path " << m_start.toString() << " " << m_goal.toString();
	std::unordered_map<Position, PathElement> pathField;
	PriorityQueue<Position, double> frontier;

	// Take a pessimistic guess for how many positions we need to visit
	const size_t expectedPositions = m_start.distSquare( m_goal, 3 ) / 8;
	pathField.reserve( expectedPositions );

	frontier.put( m_start, 0 );

	pathField.insert( { m_start, PathElement { 0.0, m_start } } );
	bool found = false;
	while ( !frontier.empty() )
	{
		//qDebug() << frontier.size();
		auto current = frontier.get();

		if ( current == m_goal )
		{
			found = true;
			break;
		}

		bool north = false;
		bool east  = false;
		bool south = false;
		bool west  = false;

		auto next = current.northOf();
		north     = evalPos( current, next, pathField, frontier );
		evalRampPos( current, next, pathField, frontier );

		next = current.eastOf();
		east = evalPos( current, next, pathField, frontier );
		evalRampPos( current, next, pathField, frontier );

		next  = current.southOf();
		south = evalPos( current, next, pathField, frontier );
		evalRampPos( current, next, pathField, frontier );

		next = current.westOf();
		west = evalPos( current, next, pathField, frontier );
		evalRampPos( current, next, pathField, frontier );

		if ( north && east )
		{
			next = current.neOf();
			evalPos( current, next, pathField, frontier );
		}
		if ( east && south )
		{
			next = current.seOf();
			evalPos( current, next, pathField, frontier );
		}
		if ( south && west )
		{
			next = current.swOf();
			evalPos( current, next, pathField, frontier );
		}
		if ( west && north )
		{
			next = current.nwOf();
			evalPos( current, next, pathField, frontier );
		}

		Tile& curTile = Global::w().getTile( current );

		if ( (bool)( curTile.wallType & ( WT_STAIR | WT_SCAFFOLD ) ) )
		{
			next = current.aboveOf();
			evalPos( current, next, pathField, frontier );
		}
		if ( (bool)( curTile.floorType & ( FT_STAIRTOP | FT_SCAFFOLD ) ) )
		{
			next = current.belowOf();
			evalPos( current, next, pathField, frontier );
		}
		if ( (bool)( curTile.wallType & ( WT_RAMP ) ) )
		{
			auto above = current.aboveOf();
			next       = above.northOf();
			evalPos( current, next, pathField, frontier );
			next = above.eastOf();
			evalPos( current, next, pathField, frontier );
			next = above.southOf();
			evalPos( current, next, pathField, frontier );
			next = above.westOf();
			evalPos( current, next, pathField, frontier );
		}
	}

	Path path;
	if ( found )
	{
		Position next = m_goal;
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
	return path;
}
