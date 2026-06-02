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
/** @file pathfinder.h
 * @brief Asynchronous pathfinding manager dispatching A* searches to worker threads.
 */

#pragma once

#include "../base/position.h"

#include <QMutex>
#include <QQueue>
#include <QSet>
#include <QThreadPool>
#include <QVector>
#include <optional>

class PathFinderThread;
class World;

/** @brief A pending pathfinding request with start/goal positions. */
struct PathFindingRequest
{
	unsigned int ID;
	Position start;
	Position goal;
	bool ignoreNoPass;
};

/** @brief Result state of a pathfinding request. */
enum class PathFinderResult
{
	NoConnection,
	FoundPath,
	Running,
	Pending
};

/**
 * @brief Manages asynchronous A* pathfinding requests dispatched to a thread pool.
 *
 * Creatures request paths via getPath(). If the path isn't cached, a PathFinderThread
 * worker is dispatched. Results are collected via onResult() callback. Uses the RegionMap
 * for fast connected-region checks before launching expensive A* searches.
 */
class PathFinder : public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY_MOVE( PathFinder )
public:
	PathFinder( World* world, QObject* parent );
	~PathFinder();

private:
	struct PathFinderJob
	{
		Position start;
		Position goal;
		bool ignoreNoPass      = false;
		PathFinderResult state = PathFinderResult::Pending;
		std::vector<Position> path;
	};

	QMap<unsigned int, PathFinderJob> m_jobs;

	QMutex m_mutex;

	std::vector<Position> getNaivePath( Position& start, Position& goal );

	World* m_world = nullptr;

public:
	PathFinderResult getPath( unsigned int id, Position start, Position goal, bool ignoreNoPass, std::vector<Position>& path );

	void cancelRequest( unsigned int id );

	bool checkConnectedRegions( const Position start, const Position goal );

	void onResult( Position start, Position goal, bool ignoreNoPass, std::vector<Position> path );
	// Dispatch workers for all outstanding pathfinding requests
	void findPaths();
};
