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

struct TerrainMaterial
{
	QString key;
	int rowid;
	QString type;
	QString wall;
	QString shortwall;
	QString floor;
	int lowest;
	int highest;
	unsigned int wallSprite      = 0;
	unsigned int shortWallSprite = 0;
	unsigned int floorSprite     = 0;
};

struct EmbeddedMaterial
{
	QString key;
	int rowid;
	QString type;
	QString wall;
	int lowest;
	int highest;
};

enum WallType : unsigned short
{
	WT_NOWALL       = 0,
	WT_SOLIDWALL    = 0x01,
	WT_ROUGH        = 0x02,
	WT_CONSTRUCTED  = 0x04,
	WT_VIEWBLOCKING = 0x08,
	WT_MOVEBLOCKING = 0x10,
	WT_STAIR        = 0x20,
	WT_RAMP         = 0x40,
	WT_RAMPCORNER   = 0x80,
	WT_SCAFFOLD     = 0x100
};
Q_DECLARE_TYPEINFO( WallType, Q_PRIMITIVE_TYPE );

enum FloorType : unsigned char
{
	FT_NOFLOOR      = 0,
	FT_SOLIDFLOOR   = 0x01,
	FT_STAIRTOP     = 0x02,
	FT_CONSTRUCTION = 0x04,
	FT_RAMPTOP      = 0x08,
	FT_SCAFFOLD     = 0x10
};
Q_DECLARE_TYPEINFO( FloorType, Q_PRIMITIVE_TYPE );

enum class TileFlag : quint64
{
	TF_NONE              = 0,
	TF_WALKABLE          = 0x01,
	TF_UNDISCOVERED      = 0x02,
	TF_SUNLIGHT          = 0x04,
	TF_WET               = 0x08,
	TF_GRASS             = 0x10,
	TF_NOPASS            = 0x20, // no pass designation, gnomes are not allowed to walk here
	TF_BLOCKED           = 0x40, // workshop output tile full
	TF_DOOR              = 0x80,
	TF_STOCKPILE         = 0x0100,
	TF_GROVE             = 0x0200,
	TF_FARM              = 0x0400,
	TF_TILLED            = 0x0800,
	TF_WORKSHOP          = 0x1000,
	TF_ROOM              = 0x2000,
	TF_LAVA              = 0x4000,
	TF_WATER             = 0x8000,
	TF_JOB_FLOOR         = 0x10000,
	TF_JOB_WALL          = 0x20000,
	TF_JOB_BUSY_FLOOR    = 0x40000,
	TF_JOB_BUSY_WALL     = 0x80000,
	TF_MOUSEOVER         = 0x100000,
	TF_WALKABLEANIMALS   = 0x200000,
	TF_WALKABLEMONSTERS  = 0x400000,
	TF_PASTURE           = 0x800000,
	TF_INDIRECT_SUNLIGHT = 0x01000000,
	TF_BIOME_MUSHROOM    = 0x02000000,
	TF_OCCUPIED          = 0x04000000,
	TF_PIPE              = 0x08000000,
	TF_AQUIFIER          = 0x10000000,
	TF_DEAQUIFIER        = 0x20000000,
	TF_TRANSPARENT       = 0x40000000,
	TF_OVERSIZE          = 0x80000000,
};

bool operator|( const TileFlag& a, const TileFlag& b ) = delete;
constexpr inline bool operator&( const TileFlag& a, const TileFlag& b )
{
	return static_cast<quint64>( a ) & static_cast<quint64>( b );
}

constexpr inline TileFlag operator+( const TileFlag& a, const TileFlag& b )
{
	return static_cast<TileFlag>( static_cast<quint64>( a ) | static_cast<quint64>( b ) );
}

constexpr inline TileFlag operator-( const TileFlag& a, const TileFlag& b )
{
	return static_cast<TileFlag>( static_cast<quint64>( a ) & ~static_cast<quint64>( b ) );
}

constexpr inline TileFlag operator~( const TileFlag& a )
{
	return static_cast<TileFlag>( ~static_cast<quint64>( a ) );
}

constexpr inline void operator+=( TileFlag& a, const TileFlag& b )
{
	a = a + b;
}
constexpr inline void operator-=( TileFlag& a, const TileFlag& b )
{
	a = a - b;
}

enum WaterFlow : unsigned char
{
	WF_NOFLOW = 0x00,
	WF_NORTH  = 0x01,
	WF_EAST   = 0x02,
	WF_SOUTH  = 0x04,
	WF_WEST   = 0x08,
	WF_UP     = 0x10,
	WF_DOWN   = 0x20,
	WF_EVAP   = 0x40
};
constexpr inline WaterFlow operator+( const WaterFlow& a, const WaterFlow& b )
{
	return static_cast<WaterFlow>( static_cast<unsigned char>( a ) | static_cast<unsigned char>( b ) );
}
constexpr inline void operator+=( WaterFlow& a, const WaterFlow& b )
{
	a = a + b;
}

Q_DECLARE_TYPEINFO( WaterFlow, Q_PRIMITIVE_TYPE );

struct Tile
{
	TileFlag flags = TileFlag::TF_NONE;

	FloorType floorType          = FT_NOFLOOR;
	unsigned short floorMaterial = 0;

	WallType wallType               = WT_NOWALL;
	unsigned short wallMaterial     = 0;
	unsigned short embeddedMaterial = 0;

	unsigned char floorRotation = 0;
	unsigned char wallRotation  = 0;

	unsigned char fluidLevel = 0;
	unsigned char pressure   = 0;
	WaterFlow flow           = WF_NOFLOW;

	unsigned char lightLevel      = 0;
	unsigned char vegetationLevel = 0;

	unsigned int floorSpriteUID = 0;
	unsigned int wallSpriteUID  = 0;
	unsigned int itemSpriteUID  = 0;
};
Q_DECLARE_TYPEINFO( Tile, Q_PRIMITIVE_TYPE );

struct AxleData
{
	unsigned int itemID = 0;
	Position pos;
	unsigned short spriteID = 0;
	quint8 localRot         = 0;
	bool anim               = false;
	bool isVertical         = false;
};
Q_DECLARE_TYPEINFO( AxleData, Q_PRIMITIVE_TYPE );
