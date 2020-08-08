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

#include <QSet>

class Job;
class Room;

struct Door
{
	Position pos;
	QString name             = "Door";
	unsigned int itemUID     = 0;
	unsigned int materialUID = 0;
	bool blockGnomes         = false;
	bool blockAnimals        = true;
	bool blockMonsters       = true;
};

class RoomManager
{
public:
	RoomManager();
	~RoomManager();

	void reset();

	void onTick( quint64 tick );

	void addRoom( Position firstClick, QList<QPair<Position, bool>> fields );
	void addNoPass( Position firstClick, QList<QPair<Position, bool>> fields );
	void load( QVariantMap vals );

	void addFurniture( unsigned int itemUID, Position pos );
	void removeFurniture( Position pos );

	void removeRoom( unsigned int id );
	void removeTile( Position pos );

	bool isRoom( Position pos )
	{
		return m_allRoomTiles.contains( pos.toInt() );
	}
	bool isDining( Position pos );
	bool allowBell( Position pos );

	Room* getRoomAtPos( Position pos );
	Room* getRoom( unsigned int id );

	QMap<unsigned int, Room>& allRooms()
	{
		return m_rooms;
	}
	QMap<unsigned int, Door>& allDoors()
	{
		return m_doors;
	}

	Room* getLastAddedRoom();

	QList<unsigned int> getDorms();
	QList<unsigned int> getDinings();

	void addDoor( Position pos, unsigned int itemID, unsigned int materialUID );
	void removeDoor( Position pos );
	void loadDoor( QVariantMap vm );

	Door* getDoor( unsigned int tileUID );

	bool isDoor( const unsigned int tileUID );
	bool blockGnomes( const unsigned int tileUID );
	bool blockAnimals( const unsigned int tileUID );
	bool blockMonsters( const unsigned int tileUID );

	bool createAlarmJob( unsigned int roomID );
	bool cancelAlarmJob( unsigned int roomID );

private:
	QMap<unsigned int, Room> m_rooms;
	QHash<unsigned int, unsigned int> m_allRoomTiles;

	QMap<unsigned int, Door> m_doors;

	Door m_errorDoor;

	unsigned int m_lastAdded = 0;
};
