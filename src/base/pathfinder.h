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

class PathFinderThread;

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
	Running
};

class PathFinder : public QObject
{
	Q_OBJECT

private:
	// Private Constructor
	PathFinder();
	// Stop the compiler generating methods of copy the object
	PathFinder( PathFinder const& copy ) = delete;
	PathFinder& operator=( PathFinder const& copy ) = delete;

	QThreadPool* m_threadPool;

	QMap<unsigned int, std::vector<Position>> m_results;
	QSet<unsigned int> m_currentRequestsFrom;

	QMutex m_mutex;

	std::vector<Position> getNaivePath( Position& start, Position& goal );

public:
	~PathFinder();

	static PathFinder& getInstance()
	{
		// The only instance
		// Guaranteed to be lazy initialized
		// Guaranteed that it will be destroyed correctly
		static PathFinder instance;
		return instance;
	}

	void init();

	PathFinderResult getPath( unsigned int id, Position start, Position goal, bool ignoreNoPass, std::vector<Position>& path );

	void cancelRequest( unsigned int id );

	static bool checkConnectedRegions( const Position start, const Position goal );

	void onThreadFinished( unsigned int consumerId, std::vector<Position> path );
};
