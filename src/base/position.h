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
		assert( sl.size() == 3 );
		if ( sl.size() == 3 )
		{
			x = sl[0].toInt();
			y = sl[1].toInt();
			z = sl[2].toInt();

			assert( x < Global::dimX );
			assert( y < Global::dimY );
			assert( z <= Global::dimZ );
		}
	}

	Position( QVariant val ) :
		x( 0 ), y( 0 ), z( 0 )
	{
		QStringList sl = val.toString().split( " " );
		assert( sl.size() == 3 );
		if ( sl.size() == 3 )
		{
			x = sl[0].toInt();
			y = sl[1].toInt();
			z = sl[2].toInt();

			assert( x < Global::dimX );
			assert( y < Global::dimY );
			assert( z <= Global::dimZ );
		}
	}

	Position( unsigned int tileID ) :
		x( 0 ), y( 0 ), z( 0 )
	{
		z = tileID / ( Global::dimX * Global::dimY );
		y = ( tileID / Global::dimX ) % Global::dimY;
		x = tileID % Global::dimX;

		assert( x < Global::dimX );
		assert( y < Global::dimY );
		assert( z <= Global::dimZ );
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
		return ( z << 20 ) + ( y << 10 ) + x;
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

inline bool testLine( const Position& a, const Position& b, const std::function<bool( const Position& current, const Position& previous )>& callback )
{
	// Bresenham's Algorithm
	const auto distX = std::abs( a.x - b.x );
	const auto distY = std::abs( a.y - b.y );
	const auto distZ = std::abs( a.z - b.z );

	const auto dirX = a.x < b.x ? 1 : ( a.x > b.x ? -1 : 0 );
	const auto dirY = a.y < b.y ? 1 : ( a.y > b.y ? -1 : 0 );
	const auto dirZ = a.z < b.z ? 1 : ( a.z > b.z ? -1 : 0 );

	const bool xIsMax = distX >= distY && distX >= distZ;
	const bool yIsMax = distY >= distX && distY >= distZ;
	const bool zIsMax = distZ >= distX && distZ >= distY;

	auto current  = a;
	auto previous = a;

	if ( !callback( current, previous ) )
	{
		return false;
	}

	if ( xIsMax )
	{
		auto p1 = 2 * distY - distX;
		auto p2 = 2 * distZ - distX;
		while ( current.x != b.x )
		{
			previous = current;
			current.x += dirX;
			if ( p1 > 0 )
			{
				current.y += dirY;
				p1 -= 2 * dirX;
			}
			if ( p2 > 0 )
			{
				current.z += dirZ;
				p2 -= 2 * dirX;
			}
			p1 += 2 * dirY;
			p2 += 2 * dirZ;
			if ( !callback( current, previous ) )
			{
				return false;
			}
		}
	}
	else if ( yIsMax )
	{
		auto p1 = 2 * distX - distY;
		auto p2 = 2 * distZ - distY;
		while ( current.y != b.y )
		{
			previous = current;
			current.y += dirY;
			if ( p1 > 0 )
			{
				current.x += dirX;
				p1 -= 2 * dirY;
			}
			if ( p2 > 0 )
			{
				current.z += dirZ;
				p2 -= 2 * dirY;
			}
			p1 += 2 * dirX;
			p2 += 2 * dirZ;
			if ( !callback( current, previous ) )
			{
				return false;
			}
		}
	}
	else if ( zIsMax )
	{
		auto p1 = 2 * distX - distZ;
		auto p2 = 2 * distY - distZ;
		while ( current.z != b.z )
		{
			previous = current;
			current.z += dirZ;
			if ( p1 > 0 )
			{
				current.x += dirX;
				p1 -= 2 * dirZ;
			}
			if ( p2 > 0 )
			{
				current.y += dirY;
				p2 -= 2 * dirZ;
			}
			p1 += 2 * dirX;
			p2 += 2 * dirY;
			if ( !callback( current, previous ) )
			{
				return false;
			}
		}
	}
	return true;
}