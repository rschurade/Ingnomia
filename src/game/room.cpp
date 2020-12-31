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
	rt->furnitureID = g->w()->getFurnitureOnTile( pos );

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
		rt->furnitureID = vf.toMap().value( "FurID" ).toUInt();
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
		fima.insert( "FurID", field->furnitureID );
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

	unsigned int itemID = rt->furnitureID;
	if ( itemID )
	{
		if ( g->inv()->itemSID( itemID ) == "AlarmBell" )
		{
			bool found = false;
			for ( auto& f : m_fields )
			{
				if ( f->furnitureID )
				{
					if ( g->inv()->itemSID( f->furnitureID ) == "AlarmBell" )
					{
						found = true;
						break;
					}
				}
			}
			m_hasAlarmBell = found;
		}
	}

	// if last tile deleted return true
	return m_fields.empty();
}

void Room::addFurniture( unsigned int itemUID, Position pos )
{
	if ( m_fields.contains( pos.toInt() ) )
	{
		m_fields[pos.toInt()]->furnitureID = itemUID;
		if ( itemUID )
		{
			g->inv()->setConstructed( itemUID, true );
			g->inv()->setItemPos( itemUID, pos );
		}
		if ( g->inv()->itemSID( itemUID ) == "AlarmBell" )
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

		unsigned int itemID = rt->furnitureID;
		rt->furnitureID     = 0;
		if ( itemID )
		{
			if ( g->inv()->itemSID( itemID ) == "AlarmBell" )
			{
				for ( auto& f : m_fields )
				{
					if ( f->furnitureID )
					{
						if ( g->inv()->itemSID( f->furnitureID ) == "AlarmBell" )
						{
							return;
						}
					}
				}
				m_hasAlarmBell = false;
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

QList<unsigned int> Room::beds()
{
	QList<unsigned int> beds;
	for ( auto field : m_fields )
	{
		if ( g->inv()->isInGroup( "Furniture", "Beds", field->furnitureID ) )
		{
			beds.append( field->furnitureID );
		}
	}
	return beds;
}

QList<unsigned int> Room::chairs()
{
	QList<unsigned int> chairs;
	for ( auto field : m_fields )
	{
		if ( g->inv()->isInGroup( "Furniture", "Chairs", field->furnitureID ) )
		{
			chairs.append( field->furnitureID );
		}
	}
	return chairs;
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
		if ( f->furnitureID )
		{
			if ( g->inv()->itemSID( f->furnitureID ) == "AlarmBell" )
			{
				return f->pos;
			}
		}
	}
	return Position();
}

QList<Position> Room::allBellPos() const
{
	QList<Position> out;
	for ( const auto& f : m_fields )
	{
		if ( f->furnitureID )
		{
			if ( g->inv()->itemSID( f->furnitureID ) == "AlarmBell" )
			{
				out.append( f->pos );
			}
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
		if ( f->furnitureID )
		{
			out += g->inv()->value( f->furnitureID );
		}
	}
	return out;
}
