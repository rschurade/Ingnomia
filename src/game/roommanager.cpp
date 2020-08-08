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
#include "roommanager.h"

#include "../base/config.h"
#include "../base/global.h"
#include "../game/inventory.h"
#include "../game/job.h"
#include "../game/jobmanager.h"
#include "../game/room.h"
#include "../game/world.h"
#include "../gfx/spritefactory.h"

#include <QDebug>

RoomManager::RoomManager()
{
	m_errorDoor.name = "Error Door";
}

RoomManager::~RoomManager()
{
}

void RoomManager::reset()
{
	m_rooms.clear();
	m_allRoomTiles.clear();
	m_doors.clear();
}

void RoomManager::onTick( quint64 tick )
{
	for ( auto&& room : m_rooms )
	{
		room.onTick( tick );
	}
}

void RoomManager::addRoom( Position firstClick, QList<QPair<Position, bool>> fields )
{
	if ( m_allRoomTiles.contains( firstClick.toInt() ) )
	{
		unsigned int spID = m_allRoomTiles.value( firstClick.toInt() );
		Room& sp          = m_rooms[spID];
		for ( auto p : fields )
		{
			if ( p.second && !m_allRoomTiles.contains( p.first.toInt() ) )
			{
				m_allRoomTiles.insert( p.first.toInt(), spID );
				sp.addTile( p.first );
			}
		}
		m_lastAdded = spID;
	}
	else
	{
		Room sp( fields );
		for ( auto p : fields )
		{
			if ( p.second )
			{
				m_allRoomTiles.insert( p.first.toInt(), sp.id() );
			}
		}
		m_rooms.insert( sp.id(), sp );
		m_lastAdded = sp.id();
	}
}

void RoomManager::addNoPass( Position firstClick, QList<QPair<Position, bool>> fields )
{
	for ( auto p : fields )
	{
		if ( p.second )
		{
			Global::w().setTileFlag( p.first, TileFlag::TF_NOPASS );
			Global::w().regionMap().updatePosition( p.first );
		}
	}
}

void RoomManager::load( QVariantMap vals )
{
	Room sp( vals );
	for ( auto sf : vals.value( "Fields" ).toList() )
	{
		auto sfm = sf.toMap();
		m_allRoomTiles.insert( Position( sfm.value( "Pos" ) ).toInt(), sp.id() );
		auto furID = sfm.value( "FurID" ).toUInt();
		if ( Global::inv().itemSID( furID ) == "AlarmBell" )
		{
			sp.setHasAlarmBell( true );
		}
	}
	m_rooms.insert( sp.id(), sp );
}

void RoomManager::removeRoom( unsigned int id )
{
}

void RoomManager::removeTile( Position pos )
{
	Room* sp = getRoomAtPos( pos );
	if ( sp )
	{
		if ( sp->removeTile( pos ) )
		{
			m_rooms.remove( sp->id() );
		}
	}
	m_allRoomTiles.remove( pos.toInt() );
}

Room* RoomManager::getRoomAtPos( Position pos )
{
	if ( m_allRoomTiles.contains( pos.toInt() ) )
	{
		if ( m_rooms.contains( m_allRoomTiles[pos.toInt()] ) )
		{
			return &m_rooms[m_allRoomTiles[pos.toInt()]];
		}
	}
	return nullptr;
}

Room* RoomManager::getRoom( unsigned int id )
{
	if ( m_rooms.contains( id ) )
	{
		return &m_rooms[id];
	}
	return nullptr;
}

void RoomManager::addFurniture( unsigned int itemUID, Position pos )
{
	//TODO make safe
	Room* room = getRoomAtPos( pos );
	if ( room )
	{
		room->addFurniture( itemUID, pos );
	}
}

void RoomManager::removeFurniture( Position pos )
{
	Room* room = getRoomAtPos( pos );
	if ( room )
	{
		room->removeFurniture( pos );
	}
}

Room* RoomManager::getLastAddedRoom()
{
	if ( m_lastAdded && m_rooms.contains( m_lastAdded ) )
	{
		return &m_rooms[m_lastAdded];
	}

	if ( m_rooms.size() )
	{
		return &m_rooms.last();
	}
	return nullptr;
}

void RoomManager::addDoor( Position pos, unsigned int itemUID, unsigned int materialUID )
{
	Door door;
	door.pos         = pos;
	door.itemUID     = itemUID;
	door.materialUID = materialUID;
	m_doors.insert( pos.toInt(), door );
}

void RoomManager::removeDoor( Position pos )
{
	m_doors.remove( pos.toInt() );
}

void RoomManager::loadDoor( QVariantMap vm )
{
	Door door;
	door.pos           = Position( vm.value( "Pos" ).toString() );
	door.itemUID       = vm.value( "ItemUID" ).toUInt();
	door.materialUID   = vm.value( "MaterialUID" ).toUInt();
	door.name          = vm.value( "Name" ).toString();
	door.blockGnomes   = vm.value( "BlockGnomes" ).toBool();
	door.blockAnimals  = vm.value( "BlockAnimals" ).toBool();
	door.blockMonsters = vm.value( "BlockMonsters" ).toBool();
	m_doors.insert( door.pos.toInt(), door );

	Global::w().getTile( door.pos ).wallSpriteUID = Global::inv().spriteID( door.itemUID );
	unsigned int nextItem                         = Global::inv().getFirstObjectAtPosition( door.pos );
	if ( nextItem )
	{
		Global::w().setItemSprite( door.pos, Global::inv().spriteID( nextItem ) );
	}
	else
	{
		Global::w().setItemSprite( door.pos, 0 );
	}
}

Door* RoomManager::getDoor( unsigned int tileUID )
{
	if ( m_doors.contains( tileUID ) )
	{
		return &m_doors[tileUID];
	}
	return nullptr;
}

QList<unsigned int> RoomManager::getDorms()
{
	QList<unsigned int> out;

	for ( auto id : m_rooms.keys() )
	{
		if ( m_rooms[id].type() == RoomType::Dorm )
		{
			out.append( id );
		}
	}

	return out;
}

QList<unsigned int> RoomManager::getDinings()
{
	QList<unsigned int> out;

	for ( auto id : m_rooms.keys() )
	{
		if ( m_rooms[id].type() == RoomType::Dining )
		{
			out.append( id );
		}
	}

	return out;
}

bool RoomManager::isDining( Position pos )
{
	if ( m_allRoomTiles.contains( pos.toInt() ) )
	{
		auto roomID = m_allRoomTiles.value( pos.toInt() );
		if ( m_rooms.contains( roomID ) )
		{
			auto room = m_rooms.value( roomID );
			if ( room.type() == Dining )
			{
				return true;
			}
		}
	}
	return false;
}

bool RoomManager::allowBell( Position pos )
{
	if ( m_allRoomTiles.contains( pos.toInt() ) )
	{
		auto roomID = m_allRoomTiles.value( pos.toInt() );
		if ( m_rooms.contains( roomID ) )
		{
			auto room = m_rooms.value( roomID );
			if ( room.type() == Dining )
			{
				return true;
				/*
				if( !room.hasAlarmBell() )
				{
					return true;
				}
				*/
			}
		}
	}
	return false;
}

bool RoomManager::isDoor( const unsigned int tileUID )
{
	return m_doors.contains( tileUID );
}

bool RoomManager::blockGnomes( const unsigned int tileUID )
{
	if ( m_doors.contains( tileUID ) )
	{
		return m_doors[tileUID].blockGnomes;
	}
	return false;
}

bool RoomManager::blockAnimals( const unsigned int tileUID )
{
	if ( m_doors.contains( tileUID ) )
	{
		return m_doors[tileUID].blockAnimals;
	}
	return false;
}

bool RoomManager::blockMonsters( const unsigned int tileUID )
{
	if ( m_doors.contains( tileUID ) )
	{
		return m_doors[tileUID].blockMonsters;
	}
	return false;
}

bool RoomManager::createAlarmJob( unsigned int roomID )
{
	if ( m_rooms.contains( roomID ) )
	{
		auto room = m_rooms.value( roomID );
		if ( room.type() == Dining )
		{
			auto pos = room.firstBellPos();

			if ( pos.isZero() )
			{
				//qDebug() << "create alarm job - no bell";
				return false;
			}

			if ( Global::jm().addJob( "SoundAlarm", pos, 0 ) )
			{
				return true;
			}
			else
			{
				//qDebug() << "create alarm job - create job returned 0";
				return false;
			}
		}
	}
	return false;
}

bool RoomManager::cancelAlarmJob( unsigned int roomID )
{
	if ( m_rooms.contains( roomID ) )
	{
		auto room = m_rooms.value( roomID );
		if ( room.type() == Dining )
		{
			auto posList = room.allBellPos();

			for ( auto pos : posList )
			{
				Global::jm().cancelJob( pos );
			}
		}
	}
	return false;
}