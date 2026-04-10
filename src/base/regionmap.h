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
/** @file regionmap.h
 * @brief World-wide region connectivity map for fast reachability queries.
 */

#pragma once

#include "../base/position.h"

#include <QSet>

#include <vector>

struct Tile;
class Region;
class World;

/**
 * @brief Maps every walkable tile to a region ID and tracks inter-region connectivity.
 *
 * Initialized via flood-fill over all walkable tiles. Dynamically updated when tiles
 * change walkability (construction, mining, etc.). Regions merge when adjacent walkable
 * areas connect and split when connections are broken. Provides O(1) region lookup per
 * tile and cached BFS for cross-region reachability checks.
 */
class RegionMap
{
public:
	RegionMap( World* parent );
	~RegionMap();

	void clear();

	unsigned int regionID( const Position& pos );
	Region& region( const Position& pos );
	Region& region( unsigned int id );

	void initRegions();
	void mergeRegions( const Position& pos, unsigned int region, unsigned int intoRegion );
	void splitRegions( unsigned int region, unsigned int intoRegion );

	unsigned int regionID( unsigned int tileID );

	void updatePosition( const Position& pos );
	void updateConnectedRegions( const Position& pos );

	bool checkConnectedRegions( unsigned int start, unsigned int goal );
	bool checkConnectedRegions( const Position& start, const Position& goal );

private:
	World* m_world = nullptr;

	std::vector<unsigned int> m_regionMap;
	std::vector<Region> m_regions;

	struct mHash
	{
		inline std::size_t operator()( const std::pair<unsigned int, unsigned int>& s ) const noexcept
		{
			return std::hash<std::size_t> {}( static_cast<std::size_t>( s.first ) ^ ( static_cast<std::size_t>( s.second ) << 32 ) );
		}
	};
	std::unordered_map<std::pair<unsigned int, unsigned int>, bool, mHash> m_cachedConnections;

	int m_dimX = 0;
	int m_dimY = 0;
	int m_dimZ = 0;

	bool m_initialized = false;

	unsigned int index( int x, int y, int z );
	unsigned int index( const Position& pos );

	void floodFill( unsigned int oldID, unsigned int newID, int x, int y, int z );

	bool checkSplit( const Position& pos );
	bool checkSplitFlood( const Position& pos, const Position& pos2, unsigned int regionID );

	void updatePositionClearWalkable( const Position& pos );
	void updatePositionSetWalkable( const Position& pos );

	std::vector<Position> connectedNeighborsUp( const Position& pos );
};
