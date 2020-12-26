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
#include "../game/stockpile.h"

#include <QSet>

class Job;
class Stockpile;
class Inventory;
class Game;

class StockpileManager : public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY_MOVE( StockpileManager )
public:
	StockpileManager( Game* parent );
	~StockpileManager();

	void onTick( quint64 tick );

	void addStockpile( Position& firstClick, QList<QPair<Position, bool>> fields );
	void load( QVariantMap vals );

	void addContainer( unsigned int containerID, Position& pos );
	void removeContainer( unsigned int containerID, Position& pos );

	void removeStockpile( unsigned int id );
	void removeTile( Position& pos );

	bool isStockPile( Position& pos )
	{
		return m_allStockpileTiles.contains( pos.toInt() );
	}

	Stockpile* getStockpileAtPos( Position& pos );
	Stockpile* getStockpileAtTileID( unsigned int id );
	Stockpile* getStockpile( unsigned int id );
	Stockpile* getLastAddedStockpile();

	void insertItem( unsigned int stockpileID, Position pos, unsigned int item );
	void removeItem( unsigned int stockpileID, Position pos, unsigned int item );

	void setInfiNotFull( Position pos );

	unsigned int getJob();
	bool finishJob( unsigned int jobID );
	bool giveBackJob( unsigned int jobID );
	QSharedPointer<Job> getJob( unsigned int jobID );
	bool hasJobID( unsigned int jobID ) const;

	QList<unsigned int> allStockpiles()
	{
		return m_stockpiles.keys();
	}
	QList<unsigned int>& allStockpilesOrdered()
	{
		return m_stockpilesOrdered;
	}

	unsigned int maxPriority();
	void setPriority( unsigned int stockpileID, int priority );
	void movePriorityUp( unsigned int stockpileID );
	void movePriorityDown( unsigned int stockpileID );
	void movePriorityTop( unsigned int stockpileID );
	void movePriorityBottom( unsigned int stockpileID );

	bool allowsPull( unsigned int stockpileID );

	bool hasPriority( unsigned int stockpileID, unsigned int stockpileID2 );

	QString name( unsigned int id );

private:
	QPointer<Game> g;

	QMap<unsigned int, Stockpile*> m_stockpiles;
	QList<unsigned int> m_stockpilesOrdered;
	QHash<unsigned int, unsigned int> m_allStockpileTiles;

	unsigned int m_lastAdded = 0;

signals:
	void signalSuspendStatusChanged( unsigned int stockpileUID );
	void signalStockpileDeleted( unsigned int stockpileUID );
	void signalStockpileAdded( unsigned int stockpileUID );
	void signalStockpileContentChanged( unsigned int stockpileUID );
};
