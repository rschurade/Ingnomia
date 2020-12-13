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

#include <QMutex>
#include <QQueue>
#include <QSet>
#include <QThreadPool>
#include <QVector>
#include <optional>

class PathFinderThread;
class World;

struct PathFindingRequest
{
	unsigned int ID;
	Position start;
	Position goal;
	bool ignoreNoPass;
};

enum class PathFinderResult
{
	NoConnection,
	FoundPath,
	Running,
	Pending
};

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
