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
#include "../game/job.h"

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
	QWeakPointer<Job> job;
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

	void serialize( QVariantMap& out ) const;
	GroveProperties() {};
	GroveProperties( QVariantMap& in );
};

class Grove : public WorldObject
{
	friend class AggregatorAgri;
	Q_DISABLE_COPY_MOVE( Grove )
public:
	Grove() = delete;
	Grove( QList<QPair<Position, bool>> tiles, Game* game );
	Grove( QVariantMap vals, Game* game );
	~Grove();

	QVariant serialize() const;

	void onTick( quint64 tick );

	bool canDelete() const;

	bool removeTile( const Position & pos );
	void addTile( const Position & pos );

	int numTrees();

	int numPlots() { return m_fields.size(); }

private:
	GroveProperties m_properties;

	QMap<unsigned int, GroveField*> m_fields;

	void updateAutoForester();

	QMap<int, int> m_prioValues;

	GroveProperties& properties()
	{
		return m_properties;
	}
};
