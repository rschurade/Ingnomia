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
/** @file regionmap.cpp
 *  @brief Region map management for tile-level connectivity.
 *
 *  Assigns every walkable tile to a region via flood-fill, tracks inter-region
 *  connections (stairs, scaffolds, ramps), and supports dynamic updates when
 *  tiles become walkable or unwalkable. Provides BFS-based reachability queries
 *  over the region graph with result caching.
 */
#include "regionmap.h"

#include "../base/config.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/region.h"
#include "../game/world.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QQueue>
/** @brief Constructs the RegionMap.
 *  @param parent Pointer to the owning World instance.
 */
RegionMap::RegionMap( World* parent ) :
	m_world( parent )
{
}

/** @brief Destructor. */
RegionMap::~RegionMap()
{
}

/** @brief Resets all region data and dimensions, marking the map as uninitialized. */
void RegionMap::clear()
{
	m_regionMap.clear();
	m_regions.clear();

	m_dimX = 0;
	m_dimY = 0;
	m_dimZ = 0;

	m_initialized = false;
}

/** @brief Returns the region ID for a tile given its linear index.
 *  @param tileID Linear index into the world tile array.
 *  @return The region ID that the tile belongs to.
 */
unsigned int RegionMap::regionID( unsigned int tileID )
{
	return m_regions[m_regionMap[tileID]].id();
}

/** @brief Returns the region ID for a tile at the given position.
 *  @param pos The 3D tile position.
 *  @return The region ID that the tile belongs to.
 */
unsigned int RegionMap::regionID( const Position& pos )
{
	return m_regions[m_regionMap[index( pos )]].id();
}

/** @brief Returns a reference to the Region containing the given position.
 *  @param pos The 3D tile position.
 *  @return Reference to the Region object.
 */
Region& RegionMap::region( const Position& pos )
{
	return m_regions[m_regionMap[index( pos )]];
}

/** @brief Returns a reference to the Region with the given ID.
 *  @param id The region identifier.
 *  @return Reference to the Region object.
 */
Region& RegionMap::region( unsigned int id )
{
	return m_regions[id];
}

/** @brief Initializes regions by flood-filling all walkable, passable tiles.
 *
 *  Iterates over every tile in the world. When an unassigned walkable tile is found,
 *  creates a new region and flood-fills all contiguous walkable tiles into it.
 *  After all regions are created, scans for stairs, scaffolds, and ramps to
 *  establish inter-region vertical connections (to/from).
 */
void RegionMap::initRegions()
{
	m_dimX = Global::dimX;
	m_dimY = Global::dimY;
	m_dimZ = Global::dimZ;

	m_regions.clear();
	m_regions.emplace_back( 0 );
	m_regionMap.clear();
	m_regionMap.resize( m_world->world().size() );
	for ( auto i = 0; i < m_regionMap.size(); ++i )
		m_regionMap[i] = 0;

	QElapsedTimer timer;
	timer.start();

	for ( int z = 1; z < m_dimZ - 1; ++z )
	{
		for ( int y = 1; y < m_dimY - 1; ++y )
		{
			for ( int x = 1; x < m_dimX - 1; ++x )
			{
				Tile& tile = m_world->getTile( x, y, z );
				if ( ( tile.flags & TileFlag::TF_WALKABLE && !( tile.flags & TileFlag::TF_NOPASS ) ) && ( m_regionMap[index( x, y, z )] == 0 ) )
				{
					////qDebug() << "found new tile without region";
					// assign new region numer
					unsigned int id               = static_cast<unsigned int>( m_regions.size() );
					m_regionMap[index( x, y, z )] = id;
					m_regions.emplace_back( id );

					floodFill( 0, id, x, y, z );
				}
			}
		}
	}
	unsigned int currentIndex = 0;
	for ( int z = 1; z < m_dimZ - 1; ++z )
	{
		for ( int y = 1; y < m_dimY - 1; ++y )
		{
			for ( int x = 1; x < m_dimX - 1; ++x )
			{
				Tile& tile = m_world->getTile( x, y, z );

				if ( tile.wallType & WallType::WT_STAIR || tile.wallType & WallType::WT_SCAFFOLD || tile.wallType & WallType::WT_RAMP )
				{
					currentIndex = index( x, y, z );

					std::vector<Position> cons = connectedNeighborsUp( Position( x, y, z ) );
					Position curPos( x, y, z );
					for ( auto con : cons )
					{
						// insert into this region the region above
						m_regions[m_regionMap[currentIndex]].addConnectionTo( regionID( con ), curPos );
						// insert into the region above this region
						m_regions[m_regionMap[index( con )]].addConnectionFrom( m_regions[m_regionMap[currentIndex]].id(), curPos );
					}
				}
			}
		}
	}

	qDebug() << "initialized " << m_regions.size() << " regions in " + QString::number( timer.elapsed() ) + " ms";
	m_initialized = true;
}

/** @brief Merges one region into another after a tile becomes walkable.
 *
 *  Flood-fills tiles from @p oldRegionID into @p newRegionID starting at @p pos,
 *  then migrates all vertical connections (to/from) from the old region to the new one,
 *  updating the connected regions' reciprocal connection sets.
 *
 *  @param pos          The position that triggered the merge.
 *  @param oldRegionID  The region being absorbed.
 *  @param newRegionID  The region absorbing the old one.
 */
void RegionMap::mergeRegions( const Position& pos, unsigned int oldRegionID, unsigned int newRegionID )
{
	floodFill( oldRegionID, newRegionID, pos.x, pos.y, pos.z );

	Region& oldRegion = m_regions[oldRegionID];
	Region& newRegion = m_regions[newRegionID];

	for ( auto con : oldRegion.connectionSetTo().keys() )
	{
		for ( auto conPos : oldRegion.connectionSetTo().value( con ) )
		{
			newRegion.addConnectionTo( con, conPos );
			m_regions[con].addConnectionFrom( newRegionID, conPos );
		}
		m_regions[con].removeAllConnectionsFrom( oldRegionID );
	}

	for ( auto con : oldRegion.connectionSetFrom().keys() )
	{
		for ( auto conPos : oldRegion.connectionSetFrom().value( con ) )
		{
			newRegion.addConnectionFrom( con, conPos );
			m_regions[con].addConnectionTo( newRegionID, Position( conPos ) );
		}
		m_regions[con].removeAllConnectionsTo( oldRegionID );
	}
}

/** @brief Redistributes vertical connections after a region has been split.
 *
 *  After flood-fill has reassigned some tiles from @p fromRegionID to @p intoRegionID,
 *  this method examines all existing connections (to and from) and reassigns them
 *  based on whether the connection position now belongs to the old or new region.
 *  Clears the cached reachability results since connectivity may have changed.
 *
 *  @param fromRegionID The original region that was split.
 *  @param intoRegionID The newly created region from the split.
 */
void RegionMap::splitRegions( unsigned int fromRegionID, unsigned int intoRegionID )
{
	// Purge cache in case of split
	m_cachedConnections.clear();

	Region& fromRegion = m_regions[fromRegionID];
	Region& intoRegion = m_regions[intoRegionID];

	auto oldConSetTo = fromRegion.connectionSetTo();

	fromRegion.clearConnectionsTo();
	for ( auto con : oldConSetTo.keys() )
	{
		for ( auto conPosString : oldConSetTo.value( con ) )
		{
			//check if location is in new region or still in old region
			Position conPos( conPosString );
			if ( m_regionMap[index( conPos )] == fromRegionID )
			{
				fromRegion.addConnectionTo( con, conPos );
				//m_regions[con].addConnectionFrom( fromRegionID, conPos );
			}
			else
			{
				intoRegion.addConnectionTo( con, conPos );
				m_regions[con].removeConnectionFrom( fromRegionID, conPos );
				m_regions[con].addConnectionFrom( intoRegionID, conPos );
			}
		}
	}

	auto oldConSet = fromRegion.connectionSetFrom();
	fromRegion.clearConnectionsFrom();
	for ( auto downRegionID : oldConSet.keys() )
	{
		Region& downRegion = m_regions[downRegionID];
		// get list of position to the old region that is split
		auto connectionsTo = downRegion.connectionsToRegion( fromRegionID );
		// delete the connection set down there
		downRegion.removeAllConnectionsTo( fromRegionID );
		// iterate over the connections and set them again

		for ( auto connectionPos : connectionsTo )
		{
			Position curPos( connectionPos );
			Tile& tile = m_world->getTile( curPos );

			if ( tile.wallType & WallType::WT_STAIR || tile.wallType & WallType::WT_SCAFFOLD || tile.wallType & WallType::WT_RAMP )
			{
				std::vector<Position> cons = connectedNeighborsUp( curPos );

				for ( auto con : cons )
				{
					// insert into this region the region above
					downRegion.addConnectionTo( regionID( con ), curPos );
					// insert into the region above this region
					m_regions[m_regionMap[index( con )]].addConnectionFrom( downRegionID, curPos );
				}
			}
		}
	}
}

/** @brief Computes the linear index for a 3D tile coordinate.
 *  @param x X coordinate.
 *  @param y Y coordinate.
 *  @param z Z coordinate.
 *  @return Linear index into the region map array.
 */
unsigned int RegionMap::index( int x, int y, int z )
{
	return x + y * m_dimX + z * m_dimX * m_dimY;
}

/** @brief Computes the linear index for a Position.
 *  @param pos The 3D tile position.
 *  @return Linear index into the region map array.
 */
unsigned int RegionMap::index( const Position& pos )
{
	return pos.x + pos.y * m_dimX + pos.z * m_dimX * m_dimY;
}

/** @brief Flood-fills contiguous walkable tiles from one region ID to another.
 *
 *  Uses a scanline flood-fill algorithm: for each queued position, scans east and
 *  west along the X axis, reassigning tiles from @p oldID to @p newID. Adjacent
 *  rows (Y-1, Y+1) are enqueued when a new span of matching tiles is found.
 *  Also marks affected tiles for rendering update.
 *
 *  @param oldID The region ID to replace (0 for initial assignment).
 *  @param newID The region ID to assign.
 *  @param x_    Starting X coordinate.
 *  @param y_    Starting Y coordinate.
 *  @param z_    Starting Z coordinate (flood-fill stays on a single Z-level).
 */
void RegionMap::floodFill( unsigned int oldID, unsigned int newID, int x_, int y_, int z_ )
{
	QElapsedTimer timer;
	timer.start();
	QQueue<Position> floodQueue;
	Position startPos( x_, y_, z_ );
	floodQueue.enqueue( startPos );
	//qDebug() << "start flood fill:" << id << startPos.toString();
	while ( !floodQueue.isEmpty() )
	{
		Position p0 = floodQueue.dequeue();
		m_world->addToUpdateList( p0 );
		//qDebug() << "continue with " << p0.toString();
		unsigned int currentIndex = index( p0.x, p0.y, p0.z );

		bool prevLineAdded = false;
		bool nextLineAdded = false;
		for ( int x = p0.x; x < m_dimX - 1; ++x )
		{
			if ( m_world->getTile( currentIndex ).flags & TileFlag::TF_WALKABLE && !( m_world->getTile( currentIndex ).flags & TileFlag::TF_NOPASS ) )
			{
				m_regionMap[currentIndex] = newID;
				m_world->addToUpdateList( Position( x, p0.y, p0.z ) );

				if ( ( m_world->getTile( currentIndex - m_dimX ).flags & TileFlag::TF_WALKABLE && !( m_world->getTile( currentIndex - m_dimX ).flags & TileFlag::TF_NOPASS ) ) && ( m_regionMap[currentIndex - m_dimX] == oldID ) )
				{
					if ( !prevLineAdded )
					{
						m_regionMap[currentIndex - m_dimX] = newID;
						Position pos( x, p0.y - 1, p0.z );
						floodQueue.enqueue( pos );
						prevLineAdded = true;
						//qDebug() << "add to queue: " << pos.toString();
					}
				}
				else
				{
					prevLineAdded = false;
				}
				if ( ( m_world->getTile( currentIndex + m_dimX ).flags & TileFlag::TF_WALKABLE && !( m_world->getTile( currentIndex + m_dimX ).flags & TileFlag::TF_NOPASS ) ) && ( m_regionMap[currentIndex + m_dimX] == oldID ) )
				{
					if ( !nextLineAdded )
					{
						m_regionMap[currentIndex + m_dimX] = newID;
						Position pos( x, p0.y + 1, p0.z );
						floodQueue.enqueue( pos );
						nextLineAdded = true;
						//qDebug() << "add to queue: " << pos.toString();
					}
				}
				else
				{
					nextLineAdded = false;
				}
			}
			else
			{
				break;
			}

			++currentIndex;
		}

		currentIndex  = index( p0.x, p0.y, p0.z );
		prevLineAdded = false;
		nextLineAdded = false;
		for ( int x = p0.x; x > 0; --x )
		{
			if ( m_world->getTile( currentIndex ).flags & TileFlag::TF_WALKABLE && !( m_world->getTile( currentIndex ).flags & TileFlag::TF_NOPASS ) )
			{
				m_regionMap[currentIndex] = newID;
				m_world->addToUpdateList( Position( x, p0.y, p0.z ) );

				if ( ( m_world->getTile( currentIndex - m_dimX ).flags & TileFlag::TF_WALKABLE && !( m_world->getTile( currentIndex - m_dimX ).flags & TileFlag::TF_NOPASS ) ) && ( m_regionMap[currentIndex - m_dimX] == oldID ) )
				{
					if ( !prevLineAdded )
					{
						m_regionMap[currentIndex - m_dimX] = newID;
						Position pos( x, p0.y - 1, p0.z );
						floodQueue.enqueue( pos );
						prevLineAdded = true;
					}
				}
				else
				{
					prevLineAdded = false;
				}
				if ( ( m_world->getTile( currentIndex + m_dimX ).flags & TileFlag::TF_WALKABLE && !( m_world->getTile( currentIndex + m_dimX ).flags & TileFlag::TF_NOPASS ) ) && ( m_regionMap[currentIndex + m_dimX] == oldID ) )
				{
					if ( !nextLineAdded )
					{
						m_regionMap[currentIndex + m_dimX] = newID;
						Position pos( x, p0.y + 1, p0.z );
						floodQueue.enqueue( pos );
						nextLineAdded = true;
					}
				}
				else
				{
					nextLineAdded = false;
				}
			}
			else
			{
				break;
			}

			--currentIndex;
		}
	}
	//if( z_ == 100 ) qDebug() << "initialized region 100 in " +  QString::number( timer.elapsed() ) + " ms";
}

/** @brief Handles a tile walkability change at the given position.
 *
 *  If the tile is now walkable, assigns it to an existing or new region and
 *  merges adjacent regions if needed. If the tile is no longer walkable, removes
 *  it from its region and checks for region splits. In both cases, updates
 *  vertical connections (stairs/ramps/scaffolds) at the position.
 *
 *  @param pos The position of the tile that changed.
 */
void RegionMap::updatePosition( const Position& pos )
{
	if ( m_initialized )
	{
		//qDebug() << "update position " << pos.toString();
		if ( m_world->isWalkableGnome( pos ) )
		{
			updatePositionSetWalkable( pos );
		}
		else
		{
			updatePositionClearWalkable( pos );
		}
		updateConnectedRegions( pos );
	}
}

/** @brief Removes a tile from its region when it becomes unwalkable.
 *
 *  Sets the tile's region to 0 (unassigned) and checks whether removing this
 *  tile causes the region to split into two disconnected parts.
 *
 *  @param pos The position of the tile that became unwalkable.
 */
void RegionMap::updatePositionClearWalkable( const Position& pos )
{
	if ( m_initialized )
	{
		// remove tile from region
		if ( m_regionMap[index( pos )] != 0 )
		{
			// check for possible split of regions
			m_regionMap[index( pos )] = 0;

			bool isSplit = checkSplit( pos );
		}
	}
}

/** @brief Assigns a region to a tile that has become walkable.
 *
 *  Picks the lowest-numbered neighboring region, or creates a new one if no
 *  neighbors have a region. If the tile bridges two or more different regions,
 *  merges them into the lowest-numbered region.
 *
 *  @param pos The position of the tile that became walkable.
 */
void RegionMap::updatePositionSetWalkable( const Position& pos )
{
	if ( m_initialized )
	{
		if ( m_regionMap[index( pos )] == 0 )
		{
			// check if neighboring tiles are walkable, take their region
			unsigned int region1 = m_regionMap[index( pos.x - 1, pos.y, pos.z )];
			unsigned int region2 = m_regionMap[index( pos.x + 1, pos.y, pos.z )];
			unsigned int region3 = m_regionMap[index( pos.x, pos.y - 1, pos.z )];
			unsigned int region4 = m_regionMap[index( pos.x, pos.y + 1, pos.z )];

			unsigned int region = region1;
			if ( ( region == 0 ) || ( ( region2 != 0 ) && ( region2 < region ) ) )
				region = region2;
			if ( ( region == 0 ) || ( ( region3 != 0 ) && ( region3 < region ) ) )
				region = region3;
			if ( ( region == 0 ) || ( ( region4 != 0 ) && ( region4 < region ) ) )
				region = region4;

			if ( region == 0 )
			{
				// if no neighbor with region, start new region
				unsigned int id           = static_cast<unsigned int>( m_regions.size() );
				m_regionMap[index( pos )] = id;
				m_regions.emplace_back( id );
			}
			else
			{
				m_regionMap[index( pos )] = region;

				// if neighbors have different regions -> merge

				if ( region1 && ( region1 != region ) )
				{
					mergeRegions( pos, region1, region );
				}
				if ( region2 && ( region2 != region ) )
				{
					mergeRegions( pos, region2, region );
				}
				if ( region3 && ( region3 != region ) )
				{
					mergeRegions( pos, region3, region );
				}
				if ( region4 && ( region4 != region ) )
				{
					mergeRegions( pos, region4, region );
				}
			}
		}
	}
}

/** @brief Updates vertical region connections at the given position.
 *
 *  If the tile at @p pos contains stairs, scaffolds, or ramps, finds the
 *  connected neighbors above and establishes bidirectional connections
 *  (to/from) between the current region and the regions above.
 *
 *  @param pos The position to update connections for.
 */
void RegionMap::updateConnectedRegions( const Position& pos )
{
	if ( m_initialized )
	{
		//qDebug() << "update connected regions " << pos.toString();

		unsigned int currentIndex = index( pos );

		Tile& tile = m_world->getTile( pos );

		if ( (bool)( tile.wallType & ( WallType::WT_STAIR | WallType::WT_SCAFFOLD | WallType::WT_RAMP ) ) )
		{
			std::vector<Position> cons = connectedNeighborsUp( pos );
			for ( auto con : cons )
			{
				//qDebug() << "set connection at pos " << con.toString() << " regions " << m_regionMap[ currentIndex] << " to " << m_regionMap[ index(con) ];
				// insert into this region the region above
				m_regions[m_regionMap[currentIndex]].addConnectionTo( m_regions[m_regionMap[index( con )]].id(), pos );
				// insert into the region above this region
				m_regions[m_regionMap[index( con )]].addConnectionFrom( m_regions[m_regionMap[currentIndex]].id(), pos );
			}
		}
	}
}

/** @brief Checks whether removing a tile caused its region to split.
 *
 *  Examines the four cardinal neighbors. When two neighbors share the same region
 *  but may no longer be connected (no direct path via diagonal corners), performs
 *  a flood-based connectivity check. If a split is detected, creates a new region
 *  and flood-fills one side, then redistributes connections via splitRegions().
 *  Rechecks after each split since region IDs may change.
 *
 *  @param pos The position of the removed tile.
 *  @return Always returns false (splits are handled in-place).
 */
bool RegionMap::checkSplit( const Position& pos )
{
	unsigned int region1 = m_regionMap[index( pos.x - 1, pos.y, pos.z )];
	unsigned int region2 = m_regionMap[index( pos.x + 1, pos.y, pos.z )];
	unsigned int region3 = m_regionMap[index( pos.x, pos.y - 1, pos.z )];
	unsigned int region4 = m_regionMap[index( pos.x, pos.y + 1, pos.z )];

	unsigned int between13 = m_regionMap[index( pos.x - 1, pos.y - 1, pos.z )];
	unsigned int between14 = m_regionMap[index( pos.x - 1, pos.y + 1, pos.z )];

	unsigned int between23 = m_regionMap[index( pos.x + 1, pos.y - 1, pos.z )];
	unsigned int between24 = m_regionMap[index( pos.x + 1, pos.y + 1, pos.z )];

	if ( region1 )
	{
		bool split = false;
		if ( region1 == region2 )
		{
			//oposite
			if ( ( region1 == between13 ) && ( region1 == region3 ) && ( region1 == between23 ) ||
				 ( region1 == between14 ) && ( region1 == region4 ) && ( region1 == between24 ) )
			{
				//still connected
			}
			else
			{
				//qDebug() << "check split flood";
				split = checkSplitFlood( Position( pos.x - 1, pos.y, pos.z ), Position( pos.x + 1, pos.y, pos.z ), region1 );
			}
		}
		if ( region1 == region3 )
		{
			//quick check is the corner tile between them still connected?
			if ( between13 == region1 )
			{
				//still connected
			}
			else
			{
				//qDebug() << "check split flood";
				split = checkSplitFlood( Position( pos.x - 1, pos.y, pos.z ), Position( pos.x, pos.y - 1, pos.z ), region1 );
			}
		}
		if ( region1 == region4 )
		{
			//quick check is the corner tile between them still connected?
			if ( between23 == region1 )
			{
				//still connected
			}
			else
			{
				//qDebug() << "check split flood";
				split = checkSplitFlood( Position( pos.x - 1, pos.y, pos.z ), Position( pos.x, pos.y + 1, pos.z ), region1 );
			}
		}
		if ( split )
		{
			unsigned int id                                           = static_cast<unsigned int>( m_regions.size() );
			m_regionMap[index( Position( pos.x - 1, pos.y, pos.z ) )] = id;
			m_regions.emplace_back( id );
			//qDebug() << "flood fill 2 : " << Position( pos.x-1, pos.y, pos.z ).toString();
			floodFill( region1, id, pos.x - 1, pos.y, pos.z );
			splitRegions( region1, id );
		}
	}
	region2 = m_regionMap[index( pos.x + 1, pos.y, pos.z )];
	region3 = m_regionMap[index( pos.x, pos.y - 1, pos.z )];
	region4 = m_regionMap[index( pos.x, pos.y + 1, pos.z )];
	if ( region2 )
	{
		bool split = false;
		if ( region2 == region3 )
		{
			//quick check is the corner tile between them still connected?
			if ( between23 == region2 )
			{
				//still connected
			}
			else
			{
				//qDebug() << "check split flood";
				split = checkSplitFlood( Position( pos.x + 1, pos.y, pos.z ), Position( pos.x, pos.y - 1, pos.z ), region2 );
			}
		}
		if ( region2 == region4 )
		{
			//quick check is the corner tile between them still connected?
			if ( between24 == region2 )
			{
				//still connected
			}
			else
			{
				//qDebug() << "check split flood";
				split = checkSplitFlood( Position( pos.x + 1, pos.y, pos.z ), Position( pos.x, pos.y + 1, pos.z ), region2 );
			}
		}
		if ( split )
		{
			unsigned int id                                           = static_cast<unsigned int>( m_regions.size() );
			m_regionMap[index( Position( pos.x + 1, pos.y, pos.z ) )] = id;
			Region r( id );
			m_regions.emplace_back( id );
			//qDebug() << "flood fill 2 : " << Position( pos.x+1, pos.y, pos.z ).toString();
			floodFill( region2, id, pos.x + 1, pos.y, pos.z );
			splitRegions( region2, id );
		}
	}
	region1 = m_regionMap[index( pos.x - 1, pos.y, pos.z )];
	region2 = m_regionMap[index( pos.x + 1, pos.y, pos.z )];
	region3 = m_regionMap[index( pos.x, pos.y - 1, pos.z )];
	region4 = m_regionMap[index( pos.x, pos.y + 1, pos.z )];

	between13 = m_regionMap[index( pos.x - 1, pos.y - 1, pos.z )];
	between14 = m_regionMap[index( pos.x - 1, pos.y + 1, pos.z )];

	between23 = m_regionMap[index( pos.x + 1, pos.y - 1, pos.z )];
	between24 = m_regionMap[index( pos.x + 1, pos.y + 1, pos.z )];
	if ( region3 )
	{
		if ( region3 == region4 )
		{
			//oposite
			if ( ( region3 == between13 ) && ( region3 == region1 ) && ( region3 == between14 ) ||
				 ( region3 == between23 ) && ( region3 == region2 ) && ( region3 == between24 ) )
			{
				//still connected
			}
			else
			{
				//qDebug() << "check split flood";
				if ( checkSplitFlood( Position( pos.x, pos.y - 1, pos.z ), Position( pos.x, pos.y + 1, pos.z ), region3 ) )
				{
					unsigned int id                                           = static_cast<unsigned int>( m_regions.size() );
					m_regionMap[index( Position( pos.x, pos.y - 1, pos.z ) )] = id;
					m_regions.emplace_back( id );
					//qDebug() << "flood fill 2 : " << Position( pos.x, pos.y-1, pos.z ).toString();
					floodFill( region3, id, pos.x, pos.y - 1, pos.z );
					splitRegions( region3, id );
				}
			}
		}
	}
	return false;
}

/** @brief Determines whether two positions in the same region are still connected.
 *
 *  Performs a scanline flood-fill from @p pos, limited to tiles matching @p regionID.
 *  If @p pos2 is reached, the tiles are still connected and no split occurred.
 *  Uses a visited array to avoid revisiting tiles.
 *
 *  @param pos      First position to check connectivity from.
 *  @param pos2     Second position to check connectivity to.
 *  @param regionID The region both positions belong to.
 *  @return True if @p pos2 was NOT reached (a split occurred), false if still connected.
 */
bool RegionMap::checkSplitFlood( const Position& pos, const Position& pos2, unsigned int regionID )
{
	QElapsedTimer timer;
	timer.start();
	QQueue<Position> floodQueue;

	QVector<bool> visited( m_dimX * m_dimX, false );

	floodQueue.enqueue( pos );
	//qDebug() << "start flood fill:" << id << startPos.toString();
	while ( !floodQueue.isEmpty() )
	{
		Position p0 = floodQueue.dequeue();
		if ( p0 == pos2 )
		{
			//found target pos, no split happened;
			//qDebug() << "found target in " +  QString::number( timer.elapsed() ) + " ms, no split";
			return false;
		}

		//qDebug() << "continue with " << p0.toString();
		unsigned int currentIndex     = index( p0.x, p0.y, p0.z );
		visited[p0.x + p0.y * m_dimX] = true;

		bool prevLineAdded = false;
		bool nextLineAdded = false;
		for ( int x = p0.x; x < m_dimX - 1; ++x )
		{
			if ( m_regionMap[currentIndex] == regionID )
			{
				if ( m_regionMap[currentIndex - m_dimX] == regionID && !visited[x + ( p0.y - 1 ) * m_dimX] )
				{
					if ( !prevLineAdded )
					{
						visited[x + ( p0.y - 1 ) * m_dimX] = true;
						Position pos( x, p0.y - 1, p0.z );
						floodQueue.enqueue( pos );
						prevLineAdded = true;
						//qDebug() << "add to queue: " << pos.toString();
					}
				}
				else
				{
					prevLineAdded = false;
				}
				if ( m_regionMap[currentIndex + m_dimX] == regionID && !visited[x + ( p0.y + 1 ) * m_dimX] )
				{
					if ( !nextLineAdded )
					{
						visited[x + ( p0.y + 1 ) * m_dimX] = true;
						Position pos( x, p0.y + 1, p0.z );
						floodQueue.enqueue( pos );
						nextLineAdded = true;
						//qDebug() << "add to queue: " << pos.toString();
					}
				}
				else
				{
					nextLineAdded = false;
				}
			}
			else
			{
				break;
			}

			++currentIndex;
		}

		currentIndex  = index( p0.x, p0.y, p0.z );
		prevLineAdded = false;
		nextLineAdded = false;
		for ( int x = p0.x; x > 0; --x )
		{
			if ( m_regionMap[currentIndex] == regionID )
			{
				if ( m_regionMap[currentIndex - m_dimX] == regionID && !visited[x + ( p0.y - 1 ) * m_dimX] )
				{
					if ( !prevLineAdded )
					{
						visited[x + ( p0.y - 1 ) * m_dimX] = true;
						Position pos( x, p0.y - 1, p0.z );
						floodQueue.enqueue( pos );
						prevLineAdded = true;
					}
				}
				else
				{
					prevLineAdded = false;
				}
				if ( m_regionMap[currentIndex + m_dimX] == regionID && !visited[x + ( p0.y + 1 ) * m_dimX] )
				{
					if ( !nextLineAdded )
					{
						visited[x + ( p0.y + 1 ) * m_dimX] = true;
						Position pos( x, p0.y + 1, p0.z );
						floodQueue.enqueue( pos );
						nextLineAdded = true;
					}
				}
				else
				{
					nextLineAdded = false;
				}
			}
			else
			{
				break;
			}

			--currentIndex;
		}
	}
	//qDebug() << "check took " +  QString::number( timer.elapsed() ) + " ms, split happened";
	return true;
}

/** @brief Checks reachability between two regions via BFS over the region graph.
 *
 *  Performs a breadth-first search over inter-region connections (both to and from)
 *  starting from @p start's connections. Results are cached in m_cachedConnections
 *  to speed up repeated queries. The pair is normalized (smaller ID first) for
 *  cache consistency.
 *
 *  @param start The starting region ID.
 *  @param goal  The target region ID.
 *  @return True if the two regions are connected, false otherwise.
 */
bool RegionMap::checkConnectedRegions( unsigned int start, unsigned int goal )
{
	if ( start == goal )
	{
		return true;
	}
	if ( start > goal )
	{
		std::swap( start, goal );
	}
	const auto it = m_cachedConnections.find( std::make_pair( start, goal ) );
	if ( it != m_cachedConnections.end() )
	{
		return it->second;
	}
	//qDebug() << "check connection between " << start << goal;
	QQueue<unsigned int> floodQueue;
	QSet<unsigned int> visited;
	for ( auto con : region( start ).connectionsTo() )
	{
		floodQueue.enqueue( con );
		visited.insert( con );
	}
	for ( auto con : region( start ).connectionsFrom() )
	{
		floodQueue.enqueue( con );
		visited.insert( con );
	}
	visited.insert( start );

	bool found = false;
	while ( !floodQueue.isEmpty() && !found )
	{
		unsigned int currentRegionID = floodQueue.dequeue();
		if ( currentRegionID == goal )
		{
			m_cachedConnections.insert( { std::make_pair( start, goal ), true } );
			return true;
		}
		for ( auto con : region( currentRegionID ).connectionsTo() )
		{
			if ( con == goal )
			{
				m_cachedConnections.insert( { std::make_pair( start, goal ), true } );
				return true;
			}
			if ( !visited.contains( con ) )
			{
				floodQueue.enqueue( con );
				visited.insert( con );
			}
		}
		for ( auto con : region( currentRegionID ).connectionsFrom() )
		{
			if ( con == goal )
			{
				m_cachedConnections.insert( { std::make_pair( start, goal ), true } );
				return true;
			}
			if ( !visited.contains( con ) )
			{
				floodQueue.enqueue( con );
				visited.insert( con );
			}
		}
	}
	m_cachedConnections.insert( { std::make_pair( start, goal ), false } );
	//qDebug() << "no connection";
	return false;
}

/** @brief Checks reachability between two positions by looking up their regions.
 *  @param start The starting position.
 *  @param goal  The target position.
 *  @return True if the regions containing start and goal are connected.
 */
bool RegionMap::checkConnectedRegions( const Position& start, const Position& goal )
{
	/*
	QElapsedTimer et;
	et.start();
	bool connected = checkConnectedRegions( regionID( start), regionID( goal ) );
	qDebug() << "check connected regions took " << et.elapsed() << " ms";
	return connected;
	*/
	return checkConnectedRegions( regionID( start ), regionID( goal ) );
}


/** @brief Finds walkable positions reachable by going up from the given position.
 *
 *  For stairs and scaffolds, the tile directly above is reachable. For ramps,
 *  the four cardinal neighbors one level above are checked for walkability.
 *
 *  @param pos The position to check for upward connections.
 *  @return Vector of walkable positions reachable by vertical traversal from @p pos.
 */
std::vector<Position> RegionMap::connectedNeighborsUp( const Position& pos )
{
	std::vector<Position> out;

	Tile& tile = m_world->getTile( pos );

	if ( (bool)( tile.wallType & ( WT_STAIR | WT_SCAFFOLD ) ) )
	{
		out.push_back( pos.aboveOf() );
	}
	if ( (bool)( tile.wallType & ( WT_RAMP ) ) )
	{
		auto above = pos.aboveOf();

		Tile& nt = m_world->getTile( above.northOf() );
		if ( nt.flags & TileFlag::TF_WALKABLE )
		{
			out.push_back( above.northOf() );
		}
		Tile& et = m_world->getTile( above.eastOf() );
		if ( et.flags & TileFlag::TF_WALKABLE )
		{
			out.push_back( above.eastOf() );
		}
		Tile& st = m_world->getTile( above.southOf() );
		if ( st.flags & TileFlag::TF_WALKABLE )
		{
			out.push_back( above.southOf() );
		}
		Tile& wt = m_world->getTile( above.westOf() );
		if ( wt.flags & TileFlag::TF_WALKABLE )
		{
			out.push_back( above.westOf() );
		}
	}
	return out;
}