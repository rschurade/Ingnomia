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
/** @file roommanager.cpp
 *  @brief Room manager implementation: room creation, gnome/bed assignment, door management,
 *         alarm job creation, and room type queries.
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

/// @brief Constructs the room manager and initialises the sentinel error-door entry.
/// @param parent Owning Game instance.
RoomManager::RoomManager( Game* parent ) :
	g( parent ),
	QObject( parent )
{
	m_errorDoor.name = "Error Door";
}

/// @brief Destructor — deletes all managed Room objects.
RoomManager::~RoomManager()
{
	for ( const auto& room : m_rooms )
	{
		delete room;
	}
}

/// @brief Propagates the per-tick update to all managed rooms.
/// @param tick Current game tick.
void RoomManager::onTick( quint64 tick )
{
	for ( const auto& room : m_rooms )
	{
		room->onTick( tick );
	}
}

/// @brief Creates a new room or expands an existing one if the first-click tile is already in a room.
///        Sets the room type and assigns a default name based on type.
/// @param firstClick Position used to detect an existing room to expand.
/// @param fields     List of (position, included) tile pairs.
/// @param type       RoomType (PersonalRoom, Dorm, Hospital, Dining).
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

/// @brief Marks the given tiles as non-passable (TF_NOPASS) and triggers region-map updates.
/// @param firstClick Unused; present for API consistency with addRoom.
/// @param fields     List of (position, included) pairs to mark.
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

/// @brief Deserialises and registers a room from a saved variant map.
///        Also sets the alarm-bell flag if any field previously held an AlarmBell item.
/// @param vals Map produced by Room::serialize().
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

/// @brief Removes an entire room by ID, clearing all its tiles from the world and the tile index.
/// @param id UID of the room to remove.
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

/// @brief Removes a single tile from whatever room contains it.
///        If the tile was the last in its room, the room is deleted.
/// @param pos Position of the tile to remove.
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

/// @brief Returns a pointer to the room that contains @p pos, or nullptr if none.
/// @param pos World position to query.
/// @return Pointer to the owning Room, or nullptr.
Room* RoomManager::getRoomAtPos( Position pos )
{
	if ( m_allRoomTiles.contains( pos ) )
	{
		return getRoom( m_allRoomTiles[pos] );
	}
	return nullptr;
}

/// @brief Returns a pointer to the room with the given UID, or nullptr if not found.
/// @param id Room UID.
/// @return Pointer to the Room, or nullptr.
Room* RoomManager::getRoom( unsigned int id )
{
	if ( m_rooms.contains( id ) )
	{
		return m_rooms[id];
	}
	return nullptr;
}

/// @brief Returns a const reference to the full room map (id → Room*).
/// @return All managed rooms.
const QHash<unsigned int, Room*>& RoomManager::allRooms()
{
	return m_rooms;
}

/// @brief Returns a const reference to the full door map (position → Door).
/// @return All managed doors.
const QHash<Position, Door>& RoomManager::allDoors()
{
	return m_doors;
}

/// @brief Forwards a furniture installation to the room that contains @p pos.
/// @param source SourceMaterial of the installed item.
/// @param value  Pre-computed furniture value.
/// @param pos    World position of the furniture.
void RoomManager::addFurniture( const SourceMaterial& source, unsigned short value, Position pos )
{
	Room* room = getRoomAtPos( pos );
	if ( room )
	{
		room->addFurniture( source, value, pos );
	}
}

/// @brief Forwards a furniture removal to the room that contains @p pos.
/// @param pos World position of the furniture to remove.
void RoomManager::removeFurniture( Position pos )
{
	Room* room = getRoomAtPos( pos );
	if ( room )
	{
		room->removeFurniture( pos );
	}
}

/// @brief Registers a new door at @p pos with the given sprite and material source.
/// @param pos      World position of the door tile.
/// @param source   SourceMaterial of the door item.
/// @param spriteID Sprite UID for rendering.
void RoomManager::addDoor( Position pos, const SourceMaterial& source, unsigned int spriteID )
{
	Door door;
	door.pos      = pos;
	door.source   = source;
	door.spriteID = spriteID;
	m_doors.insert( pos, door );
}

/// @brief Finds a free bed in a specific room for @p creatureID.
/// @param roomID     UID of the room to search.
/// @param creatureID UID of the creature looking for a bed.
/// @return Position of a free bed, or an invalid Position if none is available.
Position RoomManager::findFreeBed( unsigned int roomID, unsigned int creatureID )
{
	Room* room = getRoom( roomID );
	if ( room )
	{
		return room->findFreeBed( creatureID );
	}
	return Position();
}

/// @brief Searches all dormitories for a free bed for @p creatureID.
/// @param creatureID UID of the creature looking for a bed.
/// @return Position of the first available dorm bed, or an invalid Position if none exists.
Position RoomManager::findFreeDormBed( unsigned int creatureID )
{
	for ( auto dormID : getDorms() )
	{
		Room* room = getRoom( dormID );
		if ( room )
		{
			Position pos = room->findFreeBed( creatureID );
			if ( !pos.isZero() )
			{
				return pos;
			}
		}
	}
	return Position();
}

/// @brief Claims the bed at @p pos for @p creatureID through the owning room.
/// @param pos        Position of the bed tile.
/// @param creatureID UID of the creature claiming it.
/// @return true if the claim succeeded, false otherwise.
bool RoomManager::claimBed( Position pos, unsigned int creatureID )
{
	Room* room = getRoomAtPos( pos );
	if ( room )
	{
		return room->claimBed( pos, creatureID );
	}
	return false;
}

/// @brief Releases all bed claims held by @p creatureID across every room.
/// @param creatureID UID of the creature whose bed claims should be cleared.
void RoomManager::releaseBed( unsigned int creatureID )
{
	for ( auto room : m_rooms )
	{
		room->releaseAllBeds( creatureID );
	}
}

/// @brief Removes the door entry at @p pos from the door registry.
/// @param pos World position of the door to remove.
void RoomManager::removeDoor( Position pos )
{
	m_doors.remove( pos );
}

/// @brief Deserialises a door from a saved variant map and registers it; also restores the tile's wall sprite.
/// @param vm Map produced during serialisation (Pos, Source, SpriteID, Name, block flags).
void RoomManager::loadDoor( QVariantMap vm )
{
	Door door;
	door.pos           = Position( vm.value( "Pos" ).toString() );
	door.source        = SourceMaterial::deserialize( vm.value( "Source" ).toMap() );
	door.spriteID      = vm.value( "SpriteID" ).toUInt();
	door.name          = vm.value( "Name" ).toString();
	door.blockGnomes   = vm.value( "BlockGnomes" ).toBool();
	door.blockAnimals  = vm.value( "BlockAnimals" ).toBool();
	door.blockMonsters = vm.value( "BlockMonsters" ).toBool();
	m_doors.insert( door.pos, door );

	g->m_world->getTile( door.pos ).wallSpriteUID = door.spriteID;
}

/// @brief Returns a pointer to the Door at the given tile UID position, or nullptr if none.
/// @param tileUID Packed position UID of the door tile.
/// @return Pointer to the Door, or nullptr.
Door* RoomManager::getDoor( unsigned int tileUID )
{
	if ( m_doors.contains( tileUID ) )
	{
		return &m_doors[tileUID];
	}
	return nullptr;
}

/// @brief Returns the UIDs of all dormitory rooms.
/// @return List of dormitory room IDs.
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

/// @brief Returns the UIDs of all dining rooms.
/// @return List of dining room IDs.
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

/// @brief Returns true if @p pos is registered as a room tile.
/// @param pos World position to test.
/// @return true if the position belongs to a room.
bool RoomManager::isRoom( Position pos ) const
{
	return m_allRoomTiles.contains( pos );
}

/// @brief Returns true if @p pos belongs to a Dining room.
/// @param pos World position to test.
/// @return true if the tile is inside a dining room.
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

/// @brief Returns true if an alarm bell may be placed at @p pos (only allowed in Dining rooms).
/// @param pos World position to test.
/// @return true if bell placement is permitted.
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

/// @brief Returns true if a door is registered at the given tile UID.
/// @param tileUID Packed position UID to test.
/// @return true if a door exists at that tile.
bool RoomManager::isDoor( const unsigned int tileUID )
{
	return m_doors.contains( tileUID );
}

/// @brief Returns whether the door at @p tileUID is set to block gnome passage.
/// @param tileUID Packed position UID of the door.
/// @return true if the door blocks gnomes, false if absent or permissive.
bool RoomManager::blockGnomes( const unsigned int tileUID )
{
	if ( m_doors.contains( tileUID ) )
	{
		return m_doors[tileUID].blockGnomes;
	}
	return false;
}

/// @brief Returns whether the door at @p tileUID is set to block animal passage.
/// @param tileUID Packed position UID of the door.
/// @return true if the door blocks animals, false if absent or permissive.
bool RoomManager::blockAnimals( const unsigned int tileUID )
{
	if ( m_doors.contains( tileUID ) )
	{
		return m_doors[tileUID].blockAnimals;
	}
	return false;
}

/// @brief Returns whether the door at @p tileUID is set to block monster passage.
/// @param tileUID Packed position UID of the door.
/// @return true if the door blocks monsters, false if absent or permissive.
bool RoomManager::blockMonsters( const unsigned int tileUID )
{
	if ( m_doors.contains( tileUID ) )
	{
		return m_doors[tileUID].blockMonsters;
	}
	return false;
}

/// @brief Creates a "SoundAlarm" job at the first alarm bell in the specified dining room.
/// @param roomID UID of the room containing the alarm bell.
/// @return true if the job was created successfully, false if the room is absent, not a dining room, or has no bell.
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

/// @brief Cancels any pending "SoundAlarm" jobs for all alarm bells in the specified room.
/// @param roomID UID of the room whose alarm jobs should be cancelled.
/// @return Always returns false (return value unused by callers).
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