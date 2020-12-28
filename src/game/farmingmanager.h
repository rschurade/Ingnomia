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


#include "farm.h"
#include "grove.h"
#include "pasture.h"

#include <QHash>

class Game;
class Inventory;
class Job;
class JobManager;
class World;

struct Beehive
{
	unsigned int id = 0;
	Position pos;
	float honey  = 0;
	bool harvest = true;
	bool hasJob  = false;

	void serialize( QVariantMap& out ) const;
	Beehive();
	Beehive( QVariantMap& in );
	~Beehive();
private:
	Q_DISABLE_COPY_MOVE( Beehive )
};

class FarmingManager : public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY_MOVE( FarmingManager )
public:
	FarmingManager( Game* parent );
	~FarmingManager();

	bool load( QVariantMap vm );

	void onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged );

	void addGrove( Position firstClick, QList<QPair<Position, bool>> fields );
	void removeGrove( unsigned int id );
	Grove* getGroveAtPos( Position pos );
	Grove* getGrove( unsigned int id );
	bool isGrove( Position pos ) const;
	int grovePriority( unsigned int id );
	int countGroves();
	void setGrovePriority( unsigned int id, int prio );

	void addFarm( Position firstClick, QList<QPair<Position, bool>> fields );
	void removeFarm( unsigned int id );
	Farm* getFarmAtPos( Position pos );
	Farm* getFarm( unsigned int id );
	bool isFarm( Position pos ) const;
	int farmPriority( unsigned int id );
	int countFarms();
	void setFarmPriority( unsigned int id, int prio );

	void addPasture( Position firstClick, QList<QPair<Position, bool>> fields );
	void removePasture( unsigned int id );
	Pasture* getPastureAtPos( Position pos );
	Pasture* getPasture( unsigned int id );
	bool isPasture( Position pos ) const;
	int pasturePriority( unsigned int id );
	int countPastures();
	void setPasturePriority( unsigned int id, int prio );

	const QHash<unsigned int, Grove*>& allGroves();
	const QHash<unsigned int, Farm*>& allFarms();
	const QHash<unsigned int, Pasture*>& allPastures();
	const QHash<unsigned int, Beehive*>& allBeeHives();

	void removeTile( Position pos, bool includeFarm, bool includePasture, bool includeGrove );

	bool addUtil( Position pos, unsigned int itemID );
	bool removeUtil( Position pos );

	bool addUtilToPasture( Position pos, unsigned int itemID );
	bool removeUtilFromPasture( Position pos, unsigned int itemID );
	unsigned int util( Position pos );

	bool isBeehive( Position pos );
	Beehive* getBeehiveAtPos( Position pos );
	Beehive* getBeehive( unsigned int id );
	void setBeehiveHarvest( unsigned int id, bool harvest );
	bool harvestBeehive( Position pos );

	void emitUpdateSignalFarm( unsigned int id );
	void emitUpdateSignalPasture( unsigned int id );
	void emitUpdateSignalGrove( unsigned int id );

	int countAnimals() { return m_totalCountAnimals; }

private:
	QPointer<Game> g;

	void onTickGrove( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged );
	void onTickFarm( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged );
	void onTickPasture( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged );
	void onTickBeeHive( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged );

	QHash<unsigned int, Grove*> m_groves;
	QHash<Position, unsigned int> m_allGroveTiles;

	QHash<unsigned int, Farm*> m_farms;
	QHash<Position, unsigned int> m_allFarmTiles;

	QHash<unsigned int, Pasture*> m_pastures;
	QHash<Position, unsigned int> m_allPastureTiles;

	QHash<unsigned int, Beehive*> m_beehives;
	QHash<Position, unsigned int> m_allBeehiveTiles;

	int m_totalCountAnimals = 0;

signals:
	void signalFarmChanged( unsigned int id );
	void signalPastureChanged( unsigned int id );
	void signalGroveChanged( unsigned int id );
};
