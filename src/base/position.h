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

#include "../base/global.h"

#include <QString>
#include <QStringList>
#include <QVariant>

#include <tuple>

class Sprite;

struct Position
{
	constexpr Position() = default;
	Position& operator=( const Position& other ) = default;

	constexpr Position( int _x, int _y, int _z ) :
		x( _x ), y( _y ), z( _z )
	{
	}

	Position( QString text ) :
		x( 0 ), y( 0 ), z( 0 )
	{
		QStringList sl = text.split( " " );
		if ( sl.size() == 3 )
		{
			x = sl[0].toInt();
			y = sl[1].toInt();
			z = sl[2].toInt();
		}
	}

	Position( QVariant val ) :
		x( 0 ), y( 0 ), z( 0 )
	{
		QStringList sl = val.toString().split( " " );
		if ( sl.size() == 3 )
		{
			x = sl[0].toInt();
			y = sl[1].toInt();
			z = sl[2].toInt();
		}
	}

	Position( unsigned int tileID ) :
		x( 0 ), y( 0 ), z( 0 )
	{
		z = tileID / ( Global::dimX * Global::dimY );
		if ( z > 0 )
		{
			int rest = tileID % ( z * Global::dimX * Global::dimY );
			y        = rest / Global::dimX;
			x        = rest;
			if ( y > 0 )
			{
				x = rest % ( y * Global::dimX );
			}
		}
		else
		{
			y = tileID / Global::dimX;
			x = tileID;
			if ( y > 0 )
			{
				x = tileID % ( y * Global::dimX );
			}
		}
	}

	constexpr bool operator==( const Position& other ) const
	{
		return std::tie( x, y, z ) == std::tie( other.x, other.y, other.z );
	}
	constexpr bool operator!=( const Position& other ) const
	{
		return std::tie( x, y, z ) != std::tie( other.x, other.y, other.z );
	}
	constexpr bool operator<( const Position& other ) const
	{
		return std::tie( x, y, z ) < std::tie( other.x, other.y, other.z );
	}
	constexpr Position operator-( const Position& other ) const
	{
		return Position( x - other.x, y - other.y, z - other.z );
	}
	constexpr Position operator+( const Position& other ) const
	{
		return Position( x + other.x, y + other.y, z + other.z );
	}
	constexpr Position operator/( const int& other ) const
	{
		return Position( x / other, y / other, z / other );
	}

	QString toString() const
	{
		return QString::number( x ) + " " + QString::number( y ) + " " + QString::number( z );
	}

	constexpr int distSquare( const Position& other, int zWeight = 1 ) const
	{
		return ( ( x - other.x ) * ( x - other.x ) ) + ( ( y - other.y ) * ( y - other.y ) ) + ( ( z - other.z ) * ( z - other.z ) * zWeight );
	}
	constexpr int distSquare( int otherX, int otherY, int otherZ, int zWeight = 1 ) const
	{
		return ( ( x - otherX ) * ( x - otherX ) ) + ( ( y - otherY ) * ( y - otherY ) ) + ( ( z - otherZ ) * ( z - otherZ ) * zWeight );
	}

	unsigned int toInt() const
	{
		assert( x < Global::dimX );
		assert( y < Global::dimY );
		assert( z <= Global::dimZ );
		return x + Global::dimX * y + Global::dimX * Global::dimX * z;
		//return x + 4096 * y + 16777216 * z;
	}

	constexpr unsigned int toHashBase() const
	{
		return (z << 20) + (y << 10) + x;
	}

	bool valid() const
	{
		// Usuable volume excludes 1 row/col in each X and Y, so neighbours are addressable for read-only
		return x >= 1 && x <= Global::dimX - 2 && y >= 1 && y <= Global::dimY - 2 && z >= 0 && z <= Global::dimZ - 1;
	}

	constexpr Position northOf() const
	{
		return Position( x, y - 1, z );
	}
	constexpr Position eastOf() const
	{
		return Position( x + 1, y, z );
	}
	constexpr Position southOf() const
	{
		return Position( x, y + 1, z );
	}
	constexpr Position westOf() const
	{
		return Position( x - 1, y, z );
	}
	constexpr Position aboveOf() const
	{
		return Position( x, y, z + 1 );
	}
	constexpr Position belowOf() const
	{
		return Position( x, y, z - 1 );
	}

	constexpr Position neOf() const
	{
		return Position( x + 1, y - 1, z );
	}
	constexpr Position seOf() const
	{
		return Position( x + 1, y + 1, z );
	}
	constexpr Position nwOf() const
	{
		return Position( x - 1, y - 1, z );
	}
	constexpr Position swOf() const
	{
		return Position( x - 1, y + 1, z );
	}

	void setToBounds()
	{
		x = qMax( 0, qMin( Global::dimX - 1, (int)x ) );
		y = qMax( 0, qMin( Global::dimX - 1, (int)y ) );
		z = qMax( 0, qMin( Global::dimZ - 1, (int)z ) );
	}

	constexpr bool isZero() const
	{
		return x == 0 && y == 0 && z == 0;
	}

	constexpr bool is( short xx, short yy, short zz ) const
	{
		return x == xx && y == yy && z == zz;
	}

	short x = 0;
	short y = 0;
	short z = 0;
};
Q_DECLARE_TYPEINFO( Position, Q_PRIMITIVE_TYPE );

constexpr uint qHash( const Position& key, uint seed )
{
	return qHash( key.toHashBase(), seed );
}
namespace std
{
template <>
struct hash<Position>
{
	inline std::size_t operator()( const Position& k ) const
	{
		return std::hash<unsigned int>()( k.toHashBase() );
	}
};

} // namespace std
