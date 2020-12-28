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
#include "../game/animal.h"
#include "../game/worldobject.h"
#include "../game/job.h"

#include <QHash>
#include <QList>
#include <QMap>
#include <QPair>
#include <QVariantMap>

class Job;
class Game;

struct PastureField
{
	Position pos;
	QWeakPointer<Job> job;
	unsigned int util = 0;
};

struct PastureProperties
{
	QString animalType = "";

	Position firstPos;

	int max        = 0;
	int maxMale    = 0;
	int maxFemale  = 0;
	int animalSize = 0;

	bool onlyAvailable = false;
	bool tameWild      = false;
	bool collectEggs   = false;
	bool harvest       = true;
	bool harvestHay    = false;
	int maxHay         = 100;

	int maxTroughCapacity = 0;
	int troughContent     = 0;

	QList<qint8> jobPriorities;

	QSet<QString> foodSettings;

	void serialize( QVariantMap& out ) const;
	PastureProperties() {};
	PastureProperties( QVariantMap& in );
};

class Pasture : public WorldObject
{
	friend class AggregatorAgri;
	Q_DISABLE_COPY_MOVE( Pasture )
public:
	Pasture( QList<QPair<Position, bool>> tiles, Game* game );
	Pasture( QVariantMap vals, Game* game );
	//Pasture( const Pasture& other );
	~Pasture();

	QVariant serialize() const;

	void onTick( quint64 tick, int& count );

	void addAnimal( unsigned int id );

	bool removeTile( const Position & pos );
	void addTile( const Position & pos );

	void getInfo( int& numPlots, int& numMale, int& numFemale );

	bool canDelete();
	int countTiles();

	void removeAnimal( unsigned int animalID );
	void removeAllAnimals();

	QList<unsigned int> animals()
	{
		return m_animals;
	}

	bool addUtil( Position pos, unsigned int itemID );
	bool removeUtil( Position pos );
	unsigned int util( Position pos );

	Position randomFieldPos();
	Position findShed();

	QSet<QString>& foodSettings();
	void addFoodSetting( QString itemSID, QString materialSID );
	void removeFoodSetting( QString itemSID, QString materialSID );
	void addFood( unsigned int itemID );
	bool eatFromTrough();

	void setAnimalType( QString type );
	QString animalType();

	bool harvest();
	void setHarvest( bool harvest );

	bool harvestHay();
	void setHarvestHay( bool harvest );
	int maxHay();
	void setMaxHay( int value );
	int foodLevel();
	int maxFoodLevel();

	int maxNumber();
	int animalSize();

	void setMaxMale( int max );
	void setMaxFemale( int max );

	bool tameWild();
	void setTameWild( bool value );

	Position firstPos();

private:
	PastureProperties m_properties;

	QMap<unsigned int, PastureField*> m_fields;

	QList<unsigned int> m_animals;
};
