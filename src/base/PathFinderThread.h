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

#include <QRunnable>
#include <QThread>

#include <functional>
#include <vector>

class PathFinderThread : public QRunnable
{
public:
	using Path               = std::vector<Position>;
	using CompletionCallback = std::function<void( Path )>;
	PathFinderThread()       = delete;
	PathFinderThread( Position start, Position goal, bool ignoreNoPass, CompletionCallback callback );
	~PathFinderThread();

	virtual void run() override;

private:
	Path findPath();

	const Position m_start;
	const Position m_goal;
	const bool m_ignoreNoPass = false;

	CompletionCallback m_callback;

	static inline double heuristic( const Position& a, const Position& b )
	{
		return abs( a.x - b.x ) + abs( a.y - b.y ) + abs( a.z - b.z );
	}

	static inline double cost( const Position& lhs, const Position& rhs )
	{
		return sqrt( abs( lhs.x - rhs.x ) + abs( lhs.y - rhs.y ) + 2 * abs( lhs.z - rhs.z ) );
	}

	struct PathElement
	{
		double cost;
		Position previous;
	};

	bool evalPos( const Position& current, const Position& next, std::unordered_map<Position, PathElement>& path, PriorityQueue<Position, double>& frontier );
	bool evalRampPos( const Position& current, const Position& next, std::unordered_map<Position, PathElement>& path, PriorityQueue<Position, double>& frontier );
};
