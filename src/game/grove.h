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
class Game;

struct GroveField
{
	Position pos;
	bool hasJob    = false;
	bool harvested = false;
};

enum GroveJobs : int
{
	PlantTree,
	PickFruit,
	FellTree
};

struct GroveProperties
{
	QString treeType = "";
	QList<qint8> jobPriorities;

	bool plant     = true;
	bool pickFruit = true;
	bool fell      = true;

	bool autoPick = false;
	bool autoFell = false;

	unsigned int autoPickMin = 0;
	unsigned int autoPickMax = 0;
	unsigned int autoFellMin = 0;
	unsigned int autoFellMax = 0;

	void serialize( QVariantMap& out );
	GroveProperties() {};
	GroveProperties( QVariantMap& in );
};

class Grove : public WorldObject
{
	friend class AggregatorAgri;

public:
	Grove() = delete;
	Grove( QList<QPair<Position, bool>> tiles, Game* game );
	Grove( QVariantMap vals, Game* game );
	~Grove();

	QVariant serialize();

	void onTick( quint64 tick );

	unsigned int getJob( unsigned int gnomeID, QString skillID );
	bool finishJob( unsigned int job );
	bool giveBackJob( unsigned int job );
	Job* getJob( unsigned int jobID );
	bool hasJobID( unsigned int jobID );

	bool removeTile( Position& pos );
	void addTile( Position& pos );

	bool hasPlantTreeJob( Position pos );

private:
	GroveProperties m_properties;

	QMap<unsigned int, GroveField*> m_fields;

	void updateAutoForester();

	Job* getPlantJob();
	Job* getPickJob();
	Job* getFellJob();

	QMap<unsigned int, Job*> m_jobsOut;

	QMap<int, int> m_prioValues;

	GroveProperties& properties()
	{
		return m_properties;
	}
};
