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
#include "room.h"

#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/util.h"
#include "../game/inventory.h"
#include "../game/jobmanager.h"
#include "../game/world.h"
#include "../game/game.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QString>

Room::Room() :
	WorldObject( nullptr )
{
}

Room::Room( QList<QPair<Position, bool>> tiles, Game* game ) :
	WorldObject( game )
{
	m_id = GameState::createID();

	m_name = "Room "; // + QString::number( m_id );

	int i = 1;
	for ( auto p : tiles )
	{
		if ( p.second )
		{
			addTile( p.first );
		}
	}

	checkRoofed();
	checkEnclosed();
}

Room::~Room()
{
	for ( const auto& field : m_fields )
	{
		delete field;
	}
}

void Room::addTile( const Position & pos )
{
	RoomTile* rt = new RoomTile;
	rt->pos      = pos;

	// Check if there's already installed furniture at this position (from construction record)
	auto& wallConstrs = g->w()->wallConstructions();
	auto& floorConstrs = g->w()->floorConstructions();
	QVariantMap constr;
	if ( wallConstrs.contains( pos.toInt() ) )
	{
		constr = wallConstrs.value( pos.toInt() );
	}
	else if ( floorConstrs.contains( pos.toInt() ) )
	{
		constr = floorConstrs.value( pos.toInt() );
	}

	if ( constr.contains( "Source" ) && constr.value( "ConstructionID" ).toString() == "Item" )
	{
		SourceMaterial sm = SourceMaterial::deserialize( constr.value( "Source" ).toMap() );
		QString category = DB::select( "Category", "Items", sm.itemSID ).toString();
		if ( category == "Furniture" )
		{
			rt->furniture = sm;
			rt->furnitureValue = DB::select( "Value", "Items", sm.itemSID ).toFloat() * DB::select( "Value", "Materials", sm.materialSID ).toFloat();
			rt->isAlarmBell = ( sm.itemSID == "AlarmBell" );
			QString itemGroup = DB::select( "ItemGroup", "Items", sm.itemSID ).toString();
			rt->isBed = ( itemGroup == "Beds" );
			rt->isChair = ( itemGroup == "Chairs" );
			if ( rt->isAlarmBell )
			{
				m_hasAlarmBell = true;
			}
		}
	}

	m_fields.insert( pos.toInt(), rt );

	g->w()->setTileFlag( rt->pos, TileFlag::TF_ROOM );
}

Room::Room( QVariantMap vals, Game* game ) :
	WorldObject( game )
{
	m_id             = vals.value( "ID" ).toUInt();
	m_owner          = vals.value( "Owner" ).toUInt();
	m_lastUpdateTick = vals.value( "LastUpdate" ).value<quint64>();
	m_name           = vals.value( "Name" ).toString();
	m_active         = vals.value( "Active" ).toBool();
	m_type           = (RoomType)vals.value( "Type" ).toInt();

	QVariantList vfl = vals.value( "Fields" ).toList();
	for ( auto vf : vfl )
	{
		RoomTile* rt    = new RoomTile;
		rt->pos         = Position( vf.toMap().value( "Pos" ).toString() );
		// furnitureID no longer stored — furniture items are destroyed on install
		if ( vf.toMap().contains( "FurSource" ) )
		{
			rt->furniture      = SourceMaterial::deserialize( vf.toMap().value( "FurSource" ).toMap() );
			rt->furnitureValue = vf.toMap().value( "FurValue" ).toUInt();
			rt->isAlarmBell    = ( rt->furniture.itemSID == "AlarmBell" );
			QString itemGroup  = DB::select( "ItemGroup", "Items", rt->furniture.itemSID ).toString();
			rt->isBed          = ( itemGroup == "Beds" );
			rt->isChair        = ( itemGroup == "Chairs" );
			if ( rt->isAlarmBell )
			{
				m_hasAlarmBell = true;
			}
		}
		m_fields.insert( rt->pos.toInt(), rt );
	}
}

QVariant Room::serialize() const
{
	QVariantMap out;

	out.insert( "ID", m_id );
	out.insert( "Owner", m_owner );
	out.insert( "LastUpdate", m_lastUpdateTick );
	out.insert( "Active", m_active );
	out.insert( "Type", (unsigned char)m_type );

	QVariantList fields;
	for ( auto field : m_fields )
	{
		QVariantMap fima;
		fima.insert( "Pos", field->pos.toString() );
		// furnitureID no longer stored — furniture items are destroyed on install
		if ( field->hasFurniture() )
		{
			fima.insert( "FurSource", field->furniture.serialize() );
			fima.insert( "FurValue", field->furnitureValue );
		}
		fields.append( fima );
	}
	out.insert( "Fields", fields );
	out.insert( "Name", m_name );

	return out;
}

void Room::onTick( quint64 tick )
{
	if ( !m_active )
		return;
	bool lastUpdateLongAgo = ( tick - m_lastUpdateTick ) > 200;
	if ( lastUpdateLongAgo )
	{
		checkEnclosed();
		checkRoofed();
		m_lastUpdateTick = tick;
	}
}

bool Room::removeTile( const Position & pos )
{
	RoomTile* rt   = m_fields.value( pos.toInt() );
	// remove tile and remove tile flag
	m_fields.remove( pos.toInt() );
	g->w()->clearTileFlag( pos, TileFlag::TF_ROOM );
	delete rt;

	if ( rt->isAlarmBell )
	{
		bool found = false;
		for ( auto& f : m_fields )
		{
			if ( f->isAlarmBell )
			{
				found = true;
				break;
			}
		}
		m_hasAlarmBell = found;
	}

	// if last tile deleted return true
	return m_fields.empty();
}

void Room::addFurniture( const SourceMaterial& source, unsigned short value, Position pos )
{
	if ( m_fields.contains( pos.toInt() ) )
	{
		auto field = m_fields[pos.toInt()];
		field->furniture      = source;
		field->furnitureValue = value;
		field->isAlarmBell    = ( source.itemSID == "AlarmBell" );

		QString itemGroup = DB::select( "ItemGroup", "Items", source.itemSID ).toString();
		field->isBed  = ( itemGroup == "Beds" );
		field->isChair = ( itemGroup == "Chairs" );

		if ( field->isAlarmBell )
		{
			m_hasAlarmBell = true;
		}
	}
}

void Room::removeFurniture( const Position& pos )
{
	if ( m_fields.contains( pos.toInt() ) )
	{
		RoomTile* rt = m_fields[pos.toInt()];
		bool wasAlarmBell = rt->isAlarmBell;

		rt->furniture = SourceMaterial();
		rt->isBed = false;
		rt->isChair = false;
		rt->isAlarmBell = false;
		rt->furnitureValue = 0;

		if ( wasAlarmBell )
		{
			m_hasAlarmBell = false;
			for ( auto& f : m_fields )
			{
				if ( f->isAlarmBell )
				{
					m_hasAlarmBell = true;
					break;
				}
			}
		}
	}
}

bool Room::checkRoofed()
{
	bool roofed = true;
	for ( auto tile : m_fields )
	{
		Position pos = tile->pos;
		pos.z        = pos.z + 1;
		Tile& _tile  = g->w()->getTile( pos );
		if ( !( (bool)( _tile.floorType & FloorType::FT_SOLIDFLOOR ) ) )
		{
			roofed = false;
			break;
		}
	}
	m_roofed = roofed;
	return roofed;
}

bool Room::checkEnclosed()
{
	bool enclosed = true;
	for ( auto tile : m_fields )
	{
		Position posi = tile->pos;

		Position pos( posi.x, posi.y - 1, posi.z );
		if ( !m_fields.contains( pos.toInt() ) )
		{
			Tile& tn = g->w()->getTile( pos );
			if ( !( (bool)( tn.wallType & WT_SOLIDWALL ) || (bool)( tn.flags & TileFlag::TF_DOOR ) ) )
			{
				enclosed = false;
				break;
			}
		}
		pos = Position( posi.x, posi.y + 1, posi.z );
		if ( !m_fields.contains( pos.toInt() ) )
		{
			Tile& tn = g->w()->getTile( pos );
			if ( !( (bool)( tn.wallType & WT_SOLIDWALL ) || (bool)( tn.flags & TileFlag::TF_DOOR ) ) )
			{
				enclosed = false;
				break;
			}
		}
		pos = Position( posi.x - 1, posi.y, posi.z );
		if ( !m_fields.contains( pos.toInt() ) )
		{
			Tile& tn = g->w()->getTile( pos );
			if ( !( (bool)( tn.wallType & WT_SOLIDWALL ) || (bool)( tn.flags & TileFlag::TF_DOOR ) ) )
			{
				enclosed = false;
				break;
			}
		}
		pos = Position( posi.x + 1, posi.y, posi.z );
		if ( !m_fields.contains( pos.toInt() ) )
		{
			Tile& tn = g->w()->getTile( pos );
			if ( !( (bool)( tn.wallType & WT_SOLIDWALL ) || (bool)( tn.flags & TileFlag::TF_DOOR ) ) )
			{
				enclosed = false;
				break;
			}
		}
	}

	m_enclosed = enclosed;
	return enclosed;
}

int Room::numBeds()
{
	int count = 0;
	for ( auto field : m_fields )
	{
		if ( field->isBed )
			++count;
	}
	return count;
}

int Room::numChairs()
{
	int count = 0;
	for ( auto field : m_fields )
	{
		if ( field->isChair )
			++count;
	}
	return count;
}

Position Room::findFreeChair( Position nearPos )
{
	int bestDist = INT_MAX;
	Position bestPos;
	for ( auto field : m_fields )
	{
		if ( field->isChair && field->claimedBy == 0 && field->usedBy == 0 )
		{
			int d = field->pos.distSquare( nearPos, 5 );
			if ( d < bestDist )
			{
				bestDist = d;
				bestPos = field->pos;
			}
		}
	}
	return bestPos;
}

Position Room::findFreeBed( unsigned int creatureID )
{
	// Personal rooms: only the owner can use beds here
	if ( m_owner != 0 && m_owner != creatureID )
	{
		return Position();
	}

	// First check if this creature already claimed a bed here
	for ( auto field : m_fields )
	{
		if ( field->isBed && field->claimedBy == creatureID )
		{
			return field->pos;
		}
	}
	// Find an unclaimed bed
	for ( auto field : m_fields )
	{
		if ( field->isBed && field->claimedBy == 0 && field->usedBy == 0 )
		{
			return field->pos;
		}
	}
	return Position();
}

bool Room::claimBed( Position pos, unsigned int creatureID )
{
	if ( m_fields.contains( pos.toInt() ) )
	{
		auto field = m_fields[pos.toInt()];
		if ( field->isBed && ( field->claimedBy == 0 || field->claimedBy == creatureID ) )
		{
			field->claimedBy = creatureID;
			return true;
		}
	}
	return false;
}

void Room::releaseBed( Position pos )
{
	if ( m_fields.contains( pos.toInt() ) )
	{
		auto field = m_fields[pos.toInt()];
		field->claimedBy = 0;
		field->usedBy = 0;
	}
}

void Room::releaseAllBeds( unsigned int creatureID )
{
	for ( auto field : m_fields )
	{
		if ( field->claimedBy == creatureID )
		{
			field->claimedBy = 0;
			field->usedBy = 0;
		}
	}
}

bool Room::hasAlarmBell() const
{
	return m_hasAlarmBell;
}

void Room::setHasAlarmBell( bool value )
{
	m_hasAlarmBell = value;
}

Position Room::firstBellPos() const
{
	for ( const auto& f : m_fields )
	{
		if ( f->isAlarmBell )
		{
			return f->pos;
		}
	}
	return Position();
}

QList<Position> Room::allBellPos() const
{
	QList<Position> out;
	for ( const auto& f : m_fields )
	{
		if ( f->isAlarmBell )
		{
			out.append( f->pos );
		}
	}
	return out;
}

Position Room::randomTilePos() const
{
	if ( m_fields.size() )
	{
		auto id = rand() % m_fields.size();
		for ( auto rt : m_fields )
		{
			if ( id == 0 )
			{
				return rt->pos;
			}
			--id;
		}
	}
	return Position();
}

unsigned int Room::value()
{
	unsigned int out = 0;
	for ( const auto& f : m_fields )
	{
		if ( f->hasFurniture() )
		{
			out += f->furnitureValue;
		}
	}
	return out;
}
