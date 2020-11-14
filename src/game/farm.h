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
#include "../game/worldobject.h"

#include <QHash>
#include <QList>
#include <QMap>
#include <QPair>
#include <QVariantMap>

class Job;

struct FarmField
{
	Position pos;
	bool hasJob = false;
};

enum FarmJobs : quint8
{
	Till,
	PlantPlant,
	Harvest
};

struct FarmProperties
{
	QString plantType = "";
	QString seedItem  = "";

	Position firstPos;

	bool onlySeed = true;
	bool harvest  = true;

	QList<qint8> jobPriorities;

	bool autoHarvestSeed  = false;
	bool autoHarvestItem1 = false;
	bool autoHarvestItem2 = false;

	unsigned int autoHarvestSeedMin  = 0;
	unsigned int autoHarvestSeedMax  = 0;
	unsigned int autoHarvestItem1Min = 0;
	unsigned int autoHarvestItem1Max = 0;
	unsigned int autoHarvestItem2Min = 0;
	unsigned int autoHarvestItem2Max = 0;

	void serialize( QVariantMap& out );
	FarmProperties() {};
	FarmProperties( QVariantMap& in );
};

class Farm : public WorldObject
{
	friend class AggregatorAgri;

public:
	Farm();
	Farm( QList<QPair<Position, bool>> tiles );
	Farm( QVariantMap vals );
	~Farm();

	QVariant serialize();

	QString plantType()
	{
		return m_properties.plantType;
	}
	bool harvest()
	{
		return m_properties.harvest;
	}

	void onTick( quint64 tick );

	unsigned int getJob( unsigned int gnomeID, QString skillID );
	bool finishJob( unsigned int jobID );
	bool giveBackJob( unsigned int jobID );
	Job* getJob( unsigned int jobID );
	bool hasJobID( unsigned int jobID );

	bool removeTile( Position& pos );
	void addTile( Position& pos );

	void getInfo( int& numPlots, int& tilled, int& planted, int& ready );

	void setPlantType( QString plantID );
	void setHarvest( bool harvest );

	bool canDelete();
	int countTiles();

private:
	FarmProperties m_properties;

	QMap<unsigned int, FarmField*> m_fields;

	QMap<unsigned int, Job*> m_jobsOut;

	void updateAutoFarmer();

	Job* getPlantJob();
	Job* getTillJob();
	Job* getHarvestJob();

	FarmProperties& properties()
	{
		return m_properties;
	}
};
