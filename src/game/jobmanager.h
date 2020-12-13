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


#include "../base/priorityqueue.h"
#include "../game/job.h"

#include <QColor>
#include <QHash>
#include <QList>
#include <QMap>
#include <QQueue>
#include <QString>

class Game;

class JobManager : public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY_MOVE( JobManager )
private:
	QPointer<Game> g;

	QMap<unsigned int, Job> m_jobList;
	QMap<QString, QMultiMap<int, unsigned int>> m_jobsPerType;

	QMap<QString, int> m_skillToInt;

	QHash<unsigned int, unsigned int> m_jobPositions;

	QQueue<unsigned int> m_returnedJobQueue;

	QMap<QString, QStringList> m_jobIDs;

	int m_startIndex;

	bool workPositionWalkable( unsigned int jobID );
	bool isReachable( unsigned int jobID, unsigned int regionID );

	bool isEnclosedBySameType( unsigned int jobID );

	bool requiredToolExists( unsigned int jobID );
	bool requiredItemsAvail( unsigned int jobID );

	bool insertIntoPositionHash( unsigned int jobID );
	void removeFromPositionHash( unsigned int jobID );

public:
	JobManager() = delete;
	JobManager( Game* parent );
	~JobManager();

	void onTick();

	QString jobManagerInfo();

	//Job* getJob( QVariantList& profs, Position& pos );
	//Job* getJob( unsigned int jobID );

	unsigned int getJob( QStringList profs, unsigned int gnomeID, Position& pos );
	Job* getJob( unsigned int jobID );
	Job* getJobAtPos( Position pos );

	void giveBackJob( unsigned int jobID );

	unsigned int addJob( QString type, Position pos, int rotation, bool noJobSprite = false );
	unsigned int addJob( QString type, Position pos, QString item, QList<QString> materials, int rotation, bool noJobSprite = false );

	void setJobAvailable( unsigned int jobID );
	void setJobBeingWorked( unsigned int jobID, bool hasNeededTool = false );
	void finishJob( unsigned int jobID );

	void setJobSprites( unsigned int jobID, bool busy, bool remove );

	void addLoadedJob( QVariant vals );

	QMap<unsigned int, Job>& allJobs()
	{
		return m_jobList;
	}

	void cancelJob( const Position& pos );
	void raisePrio( Position& pos );
	void lowerPrio( Position& pos );
};
