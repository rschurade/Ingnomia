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
/** @file region.cpp
 *  @brief Region connection management.
 *
 *  A Region represents a contiguous set of walkable tiles on a single Z-level.
 *  Regions track directional connections (to/from) to other regions, representing
 *  vertical traversal points such as stairs, scaffolds, and ramps.
 */
#include "region.h"

#include "../base/global.h"
#include "../game/world.h"

#include <QDebug>

/** @brief Constructs a Region with the given identifier.
 *  @param id Unique region identifier.
 */
Region::Region( unsigned int id ) :
	m_id( id )
{
}

/** @brief Destructor. */
Region::~Region()
{
}

/** @brief Adds an inbound connection from another region at the given position.
 *  @param toRegion The source region ID that connects into this region.
 *  @param pos      The tile position where the connection exists.
 */
void Region::addConnectionFrom( unsigned int toRegion, const Position& pos )
{
	m_connectionsFrom[toRegion].insert( pos.toString() );
}

/** @brief Adds an outbound connection to another region at the given position.
 *  @param toRegion The destination region ID that this region connects to.
 *  @param pos      The tile position where the connection exists.
 */
void Region::addConnectionTo( unsigned int toRegion, const Position& pos )
{
	m_connectionsTo[toRegion].insert( pos.toString() );
}

/** @brief Removes an inbound connection from another region at the given position.
 *
 *  If no connections remain from @p toRegion, the entry is removed entirely.
 *
 *  @param toRegion The source region ID.
 *  @param pos      The tile position of the connection to remove.
 */
void Region::removeConnectionFrom( unsigned int toRegion, const Position& pos )
{
	m_connectionsFrom[toRegion].remove( pos.toString() );
	if ( m_connectionsFrom[toRegion].empty() )
	{
		m_connectionsFrom.remove( toRegion );
	}
}

/** @brief Removes an outbound connection to another region at the given position.
 *
 *  If no connections remain to @p toRegion, the entry is removed entirely.
 *
 *  @param toRegion The destination region ID.
 *  @param pos      The tile position of the connection to remove.
 */
void Region::removeConnectionTo( unsigned int toRegion, const Position& pos )
{
	m_connectionsTo[toRegion].remove( pos.toString() );
	if ( m_connectionsTo[toRegion].empty() )
	{
		m_connectionsTo.remove( toRegion );
	}
}

/** @brief Adds an inbound connection from another region (string position variant).
 *  @param toRegion The source region ID.
 *  @param pos      String representation of the tile position.
 */
void Region::addConnectionFrom( unsigned int toRegion, QString pos )
{
	m_connectionsFrom[toRegion].insert( pos );
}

/** @brief Adds an outbound connection to another region (string position variant).
 *  @param toRegion The destination region ID.
 *  @param pos      String representation of the tile position.
 */
void Region::addConnectionTo( unsigned int toRegion, QString pos )
{
	m_connectionsTo[toRegion].insert( pos );
}

/** @brief Removes an inbound connection from another region (string position variant).
 *
 *  If no connections remain from @p toRegion, the entry is removed entirely.
 *
 *  @param toRegion The source region ID.
 *  @param pos      String representation of the tile position to remove.
 */
void Region::removeConnectionFrom( unsigned int toRegion, QString pos )
{
	m_connectionsFrom[toRegion].remove( pos );
	if ( m_connectionsFrom[toRegion].empty() )
	{
		m_connectionsFrom.remove( toRegion );
	}
}

/** @brief Removes an outbound connection to another region (string position variant).
 *
 *  If no connections remain to @p toRegion, the entry is removed entirely.
 *
 *  @param toRegion The destination region ID.
 *  @param pos      String representation of the tile position to remove.
 */
void Region::removeConnectionTo( unsigned int toRegion, QString pos )
{
	m_connectionsTo[toRegion].remove( pos );
	if ( m_connectionsTo[toRegion].empty() )
	{
		m_connectionsTo.remove( toRegion );
	}
}

/** @brief Removes all inbound connections from the specified region.
 *  @param id The source region ID whose connections to remove.
 */
void Region::removeAllConnectionsFrom( unsigned int id )
{
	m_connectionsFrom.remove( id );
}

/** @brief Removes all outbound connections to the specified region.
 *  @param id The destination region ID whose connections to remove.
 */
void Region::removeAllConnectionsTo( unsigned int id )
{
	m_connectionsTo.remove( id );
}
