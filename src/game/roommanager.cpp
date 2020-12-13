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
#include "game.h"

#include "../base/config.h"
#include "../base/global.h"
#include "../game/inventory.h"
#include "../game/job.h"
#include "../game/jobmanager.h"
#include "../game/world.h"
#include "../gfx/spritefactory.h"

#include <QDebug>

RoomManager::RoomManager( Game* parent ) :
	g( parent ),
	QObject( parent )
{
	m_errorDoor.name = "Error Door";
}

RoomManager::~RoomManager()
{
	for ( const auto& room : m_rooms )
	{
		delete room;
	}
}

void RoomManager::onTick( quint64 tick )
{
	for ( const auto& room : m_rooms )
	{
		room->onTick( tick );
	}
}

void RoomManager::addRoom( Position firstClick, QList<QPair<Position, bool>> fields, RoomType type )
{
	if ( m_allRoomTiles.contains( firstClick ) )
	{
		unsigned int spID = m_allRoomTiles.value( firstClick );
		Room& sp          = *m_rooms[spID];
		for ( auto p : fields )
		{
			if ( p.second && !m_allRoomTiles.contains( p.first ) )
			{
				m_allRoomTiles.insert( p.first, spID );
				sp.addTile( p.first );
			}
		}
	}
	else
	{
		auto sp = new Room( fields, g );
		for ( auto p : fields )
		{
			if ( p.second )
			{
				m_allRoomTiles.insert( p.first, sp->id() );
			}
		}
		sp->setType( type );

		switch( type )
		{
			case RoomType::PersonalRoom:
				sp->setName( "Personal Room" );
				break;
			case RoomType::Dorm:
				sp->setName( "Dormitory" );
				break;
			case RoomType::Hospital:
				sp->setName( "Hospital" );
				break;
			case RoomType::Dining:
				sp->setName( "Dining Room" );
				break;
		}

		m_rooms.insert( sp->id(), sp );
	}
}

void RoomManager::addNoPass( Position firstClick, QList<QPair<Position, bool>> fields )
{
	for ( auto p : fields )
	{
		if ( p.second )
		{
			g->m_world->setTileFlag( p.first, TileFlag::TF_NOPASS );
			g->m_world->regionMap().updatePosition( p.first );
		}
	}
}

void RoomManager::load( QVariantMap vals )
{
	auto sp = new Room( vals, g );
	for ( auto sf : vals.value( "Fields" ).toList() )
	{
		auto sfm = sf.toMap();
		m_allRoomTiles.insert( Position( sfm.value( "Pos" ) ), sp->id() );
		auto furID = sfm.value( "FurID" ).toUInt();
		if ( g->m_inv->itemSID( furID ) == "AlarmBell" )
		{
			sp->setHasAlarmBell( true );
		}
	}
	m_rooms.insert( sp->id(), sp );
}

void RoomManager::removeRoom( unsigned int id )
{
	auto it = m_rooms.find( id );
	if ( it != m_rooms.end() )
	{
		for ( auto field = m_allRoomTiles.begin(); field != m_allRoomTiles.end(); )
		{
			if ( field.value() == it.value()->id() )
			{
				it.value()->removeTile( field.key() );
				field = m_allRoomTiles.erase( field );
			}
			else
			{
				++field;
			}
		}
		delete it.value();
		m_rooms.erase( it );
	}
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
	m_allRoomTiles.remove( pos );
}

Room* RoomManager::getRoomAtPos( Position pos )
{
	if ( m_allRoomTiles.contains( pos ) )
	{
		return getRoom( m_allRoomTiles[pos] );
	}
	return nullptr;
}

Room* RoomManager::getRoom( unsigned int id )
{
	if ( m_rooms.contains( id ) )
	{
		return m_rooms[id];
	}
	return nullptr;
}

const QHash<unsigned int, Room*>& RoomManager::allRooms()
{
	return m_rooms;
}

const QHash<Position, Door>& RoomManager::allDoors()
{
	return m_doors;
}

void RoomManager::addFurniture( unsigned int itemUID, Position pos )
{
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

void RoomManager::addDoor( Position pos, unsigned int itemUID, unsigned int materialUID )
{
	Door door;
	door.pos         = pos;
	door.itemUID     = itemUID;
	door.materialUID = materialUID;
	m_doors.insert( pos, door );
}

void RoomManager::removeDoor( Position pos )
{
	m_doors.remove( pos );
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
	m_doors.insert( door.pos, door );

	g->m_world->getTile( door.pos ).wallSpriteUID = g->m_inv->spriteID( door.itemUID );
	unsigned int nextItem                         = g->m_inv->getFirstObjectAtPosition( door.pos );
	if ( nextItem )
	{
		g->m_world->setItemSprite( door.pos, g->m_inv->spriteID( nextItem ) );
	}
	else
	{
		g->m_world->setItemSprite( door.pos, 0 );
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

	for ( const auto& room : m_rooms)
	{
		if ( room->type() == RoomType::Dorm )
		{
			out.append( room->id() );
		}
	}

	return out;
}

QList<unsigned int> RoomManager::getDinings()
{
	QList<unsigned int> out;

	for ( const auto& room : m_rooms )
	{
		if ( room->type() == RoomType::Dining )
		{
			out.append( room->id() );
		}
	}

	return out;
}

bool RoomManager::isRoom( Position pos ) const
{
	return m_allRoomTiles.contains( pos );
}

bool RoomManager::isDining( Position pos )
{
	auto room = getRoomAtPos( pos );
	if (room)
	{
		if ( room->type() == RoomType::Dining )
		{
			return true;
		}
	}
	return false;
}

bool RoomManager::allowBell( Position pos )
{
	auto room = getRoomAtPos( pos );
	if ( room )
	{
		if ( room->type() == RoomType::Dining )
		{
			return true;
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
	auto room = getRoom( roomID );
	if ( room )
	{
		if ( room->type() == RoomType::Dining )
		{
			auto pos = room->firstBellPos();

			if ( pos.isZero() )
			{
				//qDebug() << "create alarm job - no bell";
				return false;
			}

			if ( g->m_jobManager->addJob( "SoundAlarm", pos, 0 ) )
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
	auto room = getRoom( roomID );
	if ( room )
	{
		if ( room->type() == RoomType::Dining )
		{
			auto posList = room->allBellPos();

			for ( const auto& pos : posList )
			{
				g->m_jobManager->cancelJob( pos );
			}
		}
	}
	return false;
}