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
#include "../base/priorityqueue.h"

#include "../game/worldobject.h"

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

class Room : public WorldObject
{
	Q_DISABLE_COPY_MOVE( Room )
public:
	Room();
	Room( QList<QPair<Position, bool>> tiles, Game* game );
	Room( QVariantMap vals, Game* game );
	~Room();

	QVariant serialize() const;

	QMap<unsigned int, RoomTile*>& getFields()
	{
		return m_fields;
	}

	void onTick( quint64 tick );

	// return true if last tile was removed
	bool removeTile( const Position & pos );
	void addTile( const Position & pos );

	void setType( RoomType type )
	{
		m_type = type;
	};
	RoomType type() const
	{
		return m_type;
	}

	void addFurniture( unsigned int itemUID, Position pos );
	void removeFurniture( const Position& pos );

	bool checkRoofed();
	bool checkEnclosed();

	bool roofed() const
	{
		return m_roofed;
	}
	bool enclosed() const
	{
		return m_enclosed;
	}

	unsigned int value();

	void setOwner( unsigned int id )
	{
		m_owner = id;
	}
	unsigned int owner() const
	{
		return m_owner;
	}

	QList<unsigned int> beds();
	QList<unsigned int> chairs();

	bool hasAlarmBell() const;
	void setHasAlarmBell( bool value );

	Position firstBellPos() const;
	QList<Position> allBellPos() const;

	Position randomTilePos() const;

private:
	quint64 m_lastUpdateTick = 0;

	unsigned int m_owner = 0;
	bool m_roofed        = false;
	bool m_enclosed      = false;
	bool m_hasAlarmBell  = false;

	QMap<unsigned int, RoomTile*> m_fields;

	RoomType m_type = RoomType::NotSet;
};
