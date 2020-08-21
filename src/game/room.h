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

#include <QHash>
#include <QList>
#include <QMap>
#include <QPair>
#include <QtGlobal>

enum class RoomType : unsigned char
{
	NotSet,
	PersonalRoom,
	Dorm,
	Dining,
	Hospital
};

struct RoomTile
{
	Position pos;
	unsigned int furnitureID = 0;
};

class Job;

class Room
{
public:
	Room();
	Room( QList<QPair<Position, bool>> tiles );
	Room( QVariantMap vals );
	~Room();

	QVariant serialize();

	QMap<unsigned int, RoomTile*>& getFields()
	{
		return m_fields;
	}

	void onTick( quint64 tick );

	QString name()
	{
		return m_name;
	}
	void setName( QString name )
	{
		m_name = name;
	}

	unsigned int id()
	{
		return m_id;
	}
	void setId( unsigned int id )
	{
		m_id = id;
	}

	// return true if last tile was removed
	bool removeTile( Position& pos );
	void addTile( Position& pos );

	void setType( RoomType type )
	{
		m_type = type;
	};
	RoomType type()
	{
		return m_type;
	}

	void addFurniture( unsigned int itemUID, Position pos );
	void removeFurniture( const Position& pos );

	bool checkRoofed();
	bool checkEnclosed();

	bool roofed()
	{
		return m_roofed;
	}
	bool enclosed()
	{
		return m_enclosed;
	}

	void setOwner( unsigned int id )
	{
		m_owner = id;
	}
	unsigned int owner()
	{
		return m_owner;
	}

	QList<unsigned int> beds();
	QList<unsigned int> chairs();

	bool hasAlarmBell();
	void setHasAlarmBell( bool value );

	Position firstBellPos();
	QList<Position> allBellPos();

	Position randomTilePos();

private:
	quint64 m_lastUpdateTick = 0;

	unsigned int m_id    = 0;
	unsigned int m_owner = 0;
	bool m_active        = true;
	bool m_roofed        = false;
	bool m_enclosed      = false;
	bool m_hasAlarmBell  = false;

	QMap<unsigned int, RoomTile*> m_fields;

	QString m_name = "Room";

	RoomType m_type = RoomType::NotSet;
};
