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

#include "managerbase.h"
#include "workshop.h"

#include <QList>
#include <QObject>
#include <QQueue>

class Job;

class WorkshopManager : public ManagerBase
{
	Q_OBJECT

public:
	WorkshopManager( QObject* parent = 0 );
	~WorkshopManager();

	void reset();

	void onTick( quint64 tick );
	Workshop* addWorkshop( QString type, Position& pos, int rotation );
	void addWorkshop( QVariantMap vals );

	bool isWorkshop( Position& pos );
	Workshop* workshopAt( const Position& pos );
	Workshop* workshop( unsigned int ID );

	unsigned int getJob( unsigned int gnomeID, QString skillID );
	bool finishJob( unsigned int jobID );
	bool giveBackJob( unsigned jobID );

	Job* getJob( unsigned int jobID );
	bool hasJobID( unsigned int jobID );

	QList<Workshop*>& workshops()
	{
		return m_workshops;
	}

	void deleteWorkshop( unsigned int workshopID );

	//void setJobQueueJson( unsigned int workshopID, QVariantMap vals );

	bool autoGenCraftJob( QString itemSID, QString materialSID, int amount );

	int count()
	{
		return m_workshops.size();
	}

	void setPriority( unsigned int workshopID, int prio );
	int priority( unsigned int workshopID );
	int maxPriority()
	{
		return m_workshops.size();
	}

	QList<Workshop*> getTrainingGrounds();

	bool craftJobExists( const QString& itemSID, const QString& materialSID );

private:
	QList<Workshop*> m_workshops;
	QQueue<unsigned int> m_toDelete;

	QMutex m_mutex;

signals:
	void signalJobListChanged( unsigned int workshopID );
};
