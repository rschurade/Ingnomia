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

#include "../base/filter.h"
#include "../base/position.h"
#include "../base/priorityqueue.h"
#include "../game/worldobject.h"

#include <QHash>
#include <QList>
#include <QMap>
#include <QPair>
#include <QSize>
#include <QThread>
#include <QtGlobal>

class Game;

struct InventoryField
{
	Position pos;
	bool isFull              = false;
	unsigned char capacity   = 1;
	unsigned char stackSize  = 1;
	unsigned int containerID = 0;
	QSet<unsigned int> items;
	QSet<unsigned int> reservedItems;

	bool requireSame = false;
};

struct StockpileItemLimit
{
	StockpileItemLimit() {};
	StockpileItemLimit( int max_, int at, int st, bool susp )
	{
		max               = max_;
		activateThreshold = at;
		suspendThreshold  = st;
		suspended         = susp;
	}
	int max               = 100;
	int activateThreshold = 0;
	int suspendThreshold  = 0;
	bool suspended        = false;
};

class Job;

class Stockpile : public WorldObject
{
	Q_DISABLE_COPY_MOVE( Stockpile )
public:
	Stockpile( Game* game );
	Stockpile( QList<QPair<Position, bool>> tiles, Game* game );
	Stockpile( QVariantMap vals, Game* game );
	~Stockpile();

	QVariant serialize();
	void pasteSettings( QVariantMap vals );

	QMap<unsigned int, InventoryField*>& getFields()
	{
		return m_fields;
	}

	// returns true if the possible item list was updated
	bool onTick( quint64 tick );

	QSet<QPair<QString, QString>> freeSlots() const;

	Filter filter()
	{
		return m_filter;
	}
	Filter* pFilter()
	{
		return &m_filter;
	}

	bool insertItem( Position pos, unsigned int item );
	bool removeItem( Position pos, unsigned int item );

	void addContainer( unsigned int containerID, Position& pos );
	void removeContainer( unsigned int containerID, Position& pos );

	void setCheckState( bool state, QString category, QString group = "", QString item = "", QString material = "" );

	unsigned int getJob();
	unsigned int getCleanUpJob();
	bool finishJob( unsigned int jobID );
	bool giveBackJob( unsigned int jobID );

	QSharedPointer<Job> getJob( unsigned int jobID );
	bool hasJobID( unsigned int jobID ) const;

	// return true if last tile was removed
	bool removeTile( Position& pos );
	void addTile( Position& pos );

	void linkWorkshop( unsigned int workshopID );
	void unlinkWorkshop( unsigned int workshopID );

	int count( QString itemSID, QString materialSID );
	int count( QString itemSID );
	int countPlusReserved( QString itemSID );
	int countPlusReserved( QString itemSID, QString materialSID );

	void setPullOthers( bool value );
	bool pullsOthers();
	void setAllowPull( bool value );
	bool allowsPull();

	void setPriority( int p );
	int priority();

	void resetLimits();
	StockpileItemLimit limit( QString key );
	void setLimit( QString key, StockpileItemLimit limit );
	QMap<QString, StockpileItemLimit> limits()
	{
		return m_limits;
	}

	void setLimitWithMaterial( bool value );
	bool limitWithMaterial();

	bool suspendChanged();

	bool stillHasJobs();

	int countFields();

	void setInfiNotFull( Position pos );

	int capacity( unsigned int tileID );
	int itemCount( unsigned int tileID );
	int reserved( unsigned int tileID );


private:
	bool m_pullOthers = false;
	bool m_allowPull  = false;

	int m_priority = 0;

	bool m_isFull = false;

	QMap<unsigned int, InventoryField*> m_fields;

	QList<unsigned int> m_possibleItems;
	bool m_filterChanged = true;

	Filter m_filter;

	QMap<unsigned int, QSharedPointer<Job>> m_jobsOut;

	QList<unsigned int> m_linkedWorkshops;

	QMap<QString, StockpileItemLimit> m_limits;

	bool m_limitWithmaterial = false;

	unsigned int createJob( unsigned int itemID, InventoryField* infi );

	bool allowedInStockpile( unsigned int itemID );

	bool m_suspendStatusChanged = false;
};
