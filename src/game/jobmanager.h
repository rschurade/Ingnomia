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
/** @file jobmanager.h
 *  @brief Job manager: creates, assigns, and manages all jobs. Handles phase transitions
 *         (PENDING -> HAULING -> READY), item claiming, haul sub-job creation, priority
 *         queuing, and job sprite visualization.
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

/** @brief Manages all jobs in the game: creation, assignment, phase transitions,
 *         item claiming, haul sub-jobs, priority queuing, and sprite visualization. */
class JobManager : public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY_MOVE( JobManager )
private:
	QPointer<Game> g;

	QHash<unsigned int, QSharedPointer<Job>> m_jobList;
	QHash<QString, QMultiHash<int, unsigned int>> m_jobsPerType;

	QHash<QString, int> m_skillToInt;

	QHash<Position, unsigned int> m_jobPositions;

	QQueue<unsigned int> m_returnedJobQueue;

	QHash<QString, QStringList> m_jobIDs;

	QSet<QString> m_workshopSkills;

	int m_startIndex;

	bool workPositionWalkable( unsigned int jobID );
	bool isReachable( unsigned int jobID, unsigned int regionID );

	bool isEnclosedBySameType( unsigned int jobID );

	bool requiredToolExists( unsigned int jobID );
	bool requiredItemsAvail( unsigned int jobID );

	bool insertIntoPositionHash( unsigned int jobID );
	void removeFromPositionHash( unsigned int jobID );

	void processJobPhases();
	bool tryAtomicClaimItems( QSharedPointer<Job> job, QList<unsigned int>& claimedItemIDs );
	void createHaulSubJobs( QSharedPointer<Job> parentJob, const QList<unsigned int>& itemIDs );

public:
	JobManager() = delete;
	JobManager( Game* parent );
	~JobManager();

	void onTick();

	QString jobManagerInfo();

	//QSharedPointer<Job> getJob( QVariantList& profs, Position& pos );
	//QSharedPointer<Job> getJob( unsigned int jobID );

	unsigned int getJob( QStringList profs, unsigned int gnomeID, Position& pos );
	QSharedPointer<Job> getJob( unsigned int jobID );
	QSharedPointer<Job> getJobAtPos( Position pos );

	void giveBackJob( unsigned int jobID );

	unsigned int addJob( QString type, Position pos, int rotation, bool noJobSprite = false );
	unsigned int addJob( QString type, Position pos, QString item, QList<QString> materials, int rotation, bool noJobSprite = false );

	void setJobAvailable( unsigned int jobID );
	void setJobBeingWorked( unsigned int jobID, bool hasNeededTool = false );
	void finishJob( unsigned int jobID );

	void setJobSprites( unsigned int jobID, bool busy, bool remove );

	void addLoadedJob( QVariant vals );

	const QHash<unsigned int, QSharedPointer<Job>>& allJobs()
	{
		return m_jobList;
	}

	void cancelJob( const Position& pos );
	void deleteJob( unsigned int jobID );
	void deleteJobAt( const Position& pos );
	void raisePrio( Position& pos );
	void lowerPrio( Position& pos );

	void onHaulSubJobComplete( unsigned int haulJobID );
};
