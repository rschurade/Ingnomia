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
#pragma once

#include "../base/position.h"
#include "../base/priorityqueue.h"

#include <functional>
#include <vector>
#include <unordered_set>

class World;

class PathFinderThread
{
public:
	using Path               = std::vector<Position>;
	using CompletionCallback = std::function<void(Position, Position, bool ignoreNoPass, Path )>;
	PathFinderThread()       = delete;
	PathFinderThread( World* world, Position start, const std::unordered_set<Position>& goals, bool ignoreNoPass, CompletionCallback callback );

	void operator()();
private:
	void findPath();

	World* m_world = nullptr;
	const Position m_start;
	const std::unordered_set<Position> m_goals;
	const bool m_ignoreNoPass = false;

	CompletionCallback m_callback;

	static inline double heuristic( const Position& a, const Position& b )
	{
		// Heuristic needs to represent the lowest, possible cost from a to b
		// Going even lower increases the search space, but still yields correct path
		// Going too high will result in fiding sub-optimal paths as the search space is reduced too much
		return cost(a, b);
	}

	static inline double cost( const Position& lhs, const Position& rhs )
	{
		return std::sqrt( abs( lhs.x - rhs.x ) + abs( lhs.y - rhs.y ) + 2 * abs( lhs.z - rhs.z ) );
	}

	struct PathElement
	{
		double cost;
		Position previous;
	};

	bool evalPos( const Position& current, const Position& next, std::unordered_map<Position, PathElement>& path, PriorityQueue<Position, double>& frontier, const Position& goal ) const;
	bool evalRampPos( const Position& current, const Position& next, std::unordered_map<Position, PathElement>& path, PriorityQueue<Position, double>& frontier, const Position& goal ) const;
};
