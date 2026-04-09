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

/** @file worldgetters.cpp
 *  @brief Accessor and query functions on World: tile getters, flag queries, material
 *         lookups, walkability checks, creature/job queries, sunlight, vegetation,
 *         construction debug output, line-of-sight, and connected-neighbor enumeration.
 */

#include "../base/global.h"
#include "../game/game.h"
#include "../game/animal.h"
#include "../game/creaturemanager.h"
#include "../game/farmingmanager.h"
#include "../game/gnomemanager.h"
#include "../game/plant.h"
#include "../game/world.h"

#include <QDebug>
#include <QJsonDocument>

/**
 * @brief Returns a reference to the tile at the given x, y, z coordinates.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param z Z coordinate (elevation).
 * @return Mutable reference to the Tile.
 */
Tile& World::getTile( const unsigned short x, const unsigned short y, const unsigned short z )
{
	return getTile( Position( x, y, z ) );
}

/**
 * @brief Returns a mutable reference to the tile at the given position.
 * @param pos World position.
 * @return Mutable reference to the Tile.
 */
Tile& World::getTile( const Position pos )
{
	return m_world[pos.toInt()];
}

/**
 * @brief Returns a const reference to the tile at the given position.
 * @param pos World position.
 * @return Const reference to the Tile.
 */
const Tile& World::getTile( const Position pos ) const
{
	return m_world[pos.toInt()];
}

/**
 * @brief Returns a mutable reference to the tile by flat array index.
 * @param id Flat tile index (Position::toInt()).
 * @return Mutable reference to the Tile.
 */
Tile& World::getTile( const unsigned int id )
{
	return m_world[id];
}

/**
 * @brief Checks whether any job sprite exists at the given position.
 * @param pos World position to check.
 * @return True if a job sprite is registered at this position.
 */
bool World::hasJob( Position pos )
{
	return hasJob( pos.toInt() );
}

/**
 * @brief Checks whether any job sprite exists at the given x, y, z coordinates.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param z Z coordinate.
 * @return True if a job sprite is registered at this position.
 */
bool World::hasJob( int x, int y, int z )
{
	return hasJob( Position( x, y, z ).toInt() );
}

/**
 * @brief Checks whether any job sprite exists at the given tile ID.
 * @param tileID Flat tile index.
 * @return True if a job sprite is registered for this tile.
 */
bool World::hasJob( unsigned int tileID )
{
	return m_jobSprites.contains( tileID );
}

/**
 * @brief Returns the job sprite data map for the given position.
 * @param pos World position.
 * @return QVariantMap with job sprite info, or empty map if none.
 */
QVariantMap World::jobSprite( Position pos )
{
	return m_jobSprites.value( pos.toInt() );
}

/**
 * @brief Returns the job sprite data map for the given x, y, z coordinates.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param z Z coordinate.
 * @return QVariantMap with job sprite info, or empty map if none.
 */
QVariantMap World::jobSprite( int x, int y, int z )
{
	return jobSprite( Position( x, y, z ) );
}

/**
 * @brief Returns the job sprite data map for the given flat tile ID.
 * @param tileID Flat tile index.
 * @return QVariantMap with job sprite info, or empty map if none.
 */
QVariantMap World::jobSprite( unsigned int tileID )
{
	return m_jobSprites.value( tileID );
}

/**
 * @brief Returns the tile flags for the given position.
 * @param pos World position.
 * @return Combined TileFlag bitmask for the tile.
 */
TileFlag World::getTileFlag( Position pos )
{
	return getTile( pos ).flags;
}

/**
 * @brief Returns the wall material UID for the tile at the given position.
 * @param pos World position.
 * @return Material UID of the wall, or 0 if no wall.
 */
unsigned short World::wallMaterial( Position pos )
{
	Tile& tile = getTile( pos );
	return tile.wallMaterial;
}

/**
 * @brief Returns the embedded material UID (ore/gem) for the tile at the given position.
 * @param pos World position.
 * @return Material UID of the embedded vein, or 0 if none.
 */
unsigned short World::embeddedMaterial( Position pos )
{
	Tile& tile = getTile( pos );
	return tile.embeddedMaterial;
}

/**
 * @brief Returns the floor material UID for the tile at the given position.
 * @param pos World position.
 * @return Material UID of the floor, or 0 if no floor.
 */
unsigned short World::floorMaterial( Position pos )
{
	Tile& tile = getTile( pos );
	return tile.floorMaterial;
}

/**
 * @brief Returns the wall type flags for the tile at the given position.
 * @param pos World position.
 * @return WallType bitmask for the tile.
 */
WallType World::wallType( Position pos )
{
	Tile& tile = getTile( pos );
	return tile.wallType;
}

/**
 * @brief Returns the floor type flags for the tile at the given position.
 * @param pos World position.
 * @return FloorType bitmask for the tile.
 */
FloorType World::floorType( Position pos )
{
	Tile& tile = getTile( pos );
	return tile.floorType;
}

/**
 * @brief Checks whether any creature occupies the given position.
 * @param pos World position.
 * @return True if at least one creature is registered at this position.
 */
bool World::creatureAtPos( Position pos )
{
	return m_creaturePositions.contains( pos.toInt() );
}

/**
 * @brief Checks whether any creature occupies the given flat position ID.
 * @param posID Flat tile index.
 * @return True if at least one creature is registered at this position.
 */
bool World::creatureAtPos( unsigned int posID )
{
	bool ret = m_creaturePositions.contains( posID );
	return ret;
}

/**
 * @brief Returns a pointer to the first creature (animal or gnome) at the given position.
 * @param posID Flat tile index.
 * @param rotation Output parameter for the creature's rotation (unused in current implementation).
 * @return Pointer to the first Creature found, or nullptr if none.
 */
Creature* World::firstCreatureAtPos( unsigned int posID, quint8& rotation )
{
	if ( m_creaturePositions.contains( posID ) )
	{
		unsigned int ID = m_creaturePositions[posID].first();
		Animal* a       = g->cm()->animal( ID );
		if ( a )
		{
			return a;
		}
		Gnome* gn = g->gm()->gnome( ID );
		if ( gn )
		{
			return gn;
		}
	}
	return 0;
}

/**
 * @brief Sets or clears the walkable flag on a tile and updates the region map.
 * @param pos World position.
 * @param value True to mark walkable, false to mark unwalkable.
 */
void World::setWalkable( Position pos, bool value )
{
	Tile& tile = getTile( pos );
	if ( value )
	{
		setTileFlag( pos, TileFlag::TF_WALKABLE );
	}
	else
	{
		clearTileFlag( pos, TileFlag::TF_WALKABLE );
	}
	updateWalkable( pos );
}

/**
 * @brief Notifies the region map that the walkability of a tile may have changed.
 * @param pos World position to update.
 */
void World::updateWalkable( Position pos )
{
	m_regionMap.updatePosition( pos );
}

/**
 * @brief Checks whether the tile at the given position has the walkable flag.
 * @param pos World position.
 * @return True if the tile is walkable.
 */
bool World::isWalkable( Position pos )
{
	Tile& tile = getTile( pos );
	return tile.flags & TileFlag::TF_WALKABLE;
}

/**
 * @brief Checks whether a gnome can walk on this tile (walkable, not blocked, low fluid, no lava).
 * @param pos World position.
 * @return True if a gnome can traverse this tile.
 */
bool World::isWalkableGnome( Position pos )
{
	Tile& tile = getTile( pos );
	return tile.flags & TileFlag::TF_WALKABLE && !( tile.flags & TileFlag::TF_NOPASS ) && ( tile.fluidLevel < 4 ) && !( tile.flags & TileFlag::TF_LAVA );
}

/**
 * @brief Checks whether the tile at the given position is a ramp or ramp top.
 * @param pos World position.
 * @return True if the tile contains a ramp wall or ramp-top floor.
 */
bool World::isRamp( Position pos )
{
	Tile& tile = getTile( pos );
	return ( tile.wallType & WallType::WT_RAMP ) || ( tile.floorType & FloorType::FT_RAMPTOP );
}

/**
 * @brief Checks whether the tile at the given position has a solid floor.
 * @param pos World position.
 * @return True if the tile's floor type includes FT_SOLIDFLOOR.
 */
bool World::isSolidFloor( Position pos )
{
	Tile& tile = getTile( pos );
	return ( tile.floorType & FloorType::FT_SOLIDFLOOR );
}

/**
 * @brief Checks that no tree exists within the given range around a position.
 * @param pos Center position to check around.
 * @param xRange Horizontal range in X to scan.
 * @param yRange Horizontal range in Y to scan.
 * @return True if no tree or tree-planting job is found within the range.
 */
bool World::noTree( const Position pos, const int xRange, const int yRange )
{
	if ( pos.x < 3 || pos.x > m_dimX - 4 || pos.y < 3 || pos.y > m_dimY - 4 )
	{
		return false;
	}

	int xStart = qMax( 0, pos.x - xRange );
	int xEnd   = qMin( m_dimX - 1, pos.x + xRange );
	int yStart = qMax( 0, pos.y - yRange );
	int yEnd   = qMin( m_dimY - 1, pos.y + yRange );

	for ( int x = xStart; x <= xEnd; ++x )
	{
		for ( int y = yStart; y <= yEnd; ++y )
		{
			unsigned int testPosID = Position( x, y, pos.z ).toInt();
			if ( m_plants.contains( testPosID ) )
			{
				if ( m_plants[testPosID].isTree() )
				{
					return false;
				}
			}
			if ( auto job = g->jm()->getJobAtPos( Position( x, y, pos.z ) ) )
			{
				if ( job && job->type() == "PlantTree" )
				{
					return false;
				}
			}
		}
	}
	return true;
}

/**
 * @brief Checks that no mushroom exists within the given range around a position.
 * @param pos Center position to check around.
 * @param xRange Horizontal range in X to scan.
 * @param yRange Horizontal range in Y to scan.
 * @return True if no mushroom plant is found within the range.
 */
bool World::noShroom( const Position pos, const int xRange, const int yRange )
{
	int xStart = qMax( 0, pos.x - xRange );
	int xEnd   = qMin( m_dimX - 1, pos.x + xRange );
	int yStart = qMax( 0, pos.y - yRange );
	int yEnd   = qMin( m_dimY - 1, pos.y + yRange );

	for ( int x = xStart; x <= xEnd; ++x )
	{
		for ( int y = yStart; y <= yEnd; ++y )
		{
			unsigned int testPosID = Position( x, y, pos.z ).toInt();
			if ( m_plants.contains( testPosID ) )
			{
				if ( m_plants[testPosID].isMushroom() )
				{
					return false;
				}
			}
		}
	}
	return true;
}

/**
 * @brief Atomically retrieves and clears the set of tile IDs updated since the last call.
 * @return Set of flat tile indices that were modified and need rendering updates.
 */
QSet<unsigned int> World::updatedTiles()
{
	QMutexLocker lock( &m_updateMutex );
	QSet<unsigned int> ret;
	ret.swap( m_updatedTiles );
	// Assume next update batch is going to be similar in size
	m_updatedTiles.reserve( ret.size() );
	return ret;
}

/**
 * @brief Scans downward from pos.z to find the first solid floor, updating pos.z in place.
 * @param pos Position to scan from; pos.z is modified to the floor level found.
 * @param setSunlight If true, sets TF_SUNLIGHT on each tile scanned through and propagates indirect sunlight.
 */
void World::getFloorLevelBelow( Position& pos, bool setSunlight )
{
	int z = pos.z;
	while ( z > 0 )
	{
		Tile& tile = getTile( pos.x, pos.y, z );
		if ( setSunlight )
		{
			tile.flags += TileFlag::TF_SUNLIGHT;
			addToUpdateList( Position( pos.x, pos.y, z ) );
			spreadIndirectSunlight( Position( pos.x, pos.y, z ) );
		}
		if ( !( tile.floorType == FloorType::FT_NOFLOOR || ( tile.floorType & FloorType::FT_RAMPTOP ) ) )
		{
			pos.z = z;
			return;
		}
		--z;
	}
}

/**
 * @brief Marks the four cardinal neighbors of a sunlit tile as indirectly sunlit.
 * @param pos Position of the sunlit tile whose neighbors to update.
 */
void World::spreadIndirectSunlight( Position pos )
{
	Tile& te = getTile( pos.eastOf() );
	te.flags += TileFlag::TF_INDIRECT_SUNLIGHT;
	Tile& ts = getTile( pos.southOf() );
	ts.flags += TileFlag::TF_INDIRECT_SUNLIGHT;
	Tile& tw = getTile( pos.westOf() );
	tw.flags += TileFlag::TF_INDIRECT_SUNLIGHT;
	Tile& tn = getTile( pos.northOf() );
	tn.flags += TileFlag::TF_INDIRECT_SUNLIGHT;
}

/**
 * @brief Re-evaluates indirect sunlight for all four cardinal neighbors of the given position.
 * @param pos Position whose neighbors should be checked.
 */
void World::checkIndirectSunlightForNeighbors( Position pos )
{
	checkIndirectSunlight( pos.eastOf() );
	checkIndirectSunlight( pos.southOf() );
	checkIndirectSunlight( pos.westOf() );
	checkIndirectSunlight( pos.northOf() );
}

/**
 * @brief Sets or clears indirect sunlight on a tile based on whether any cardinal neighbor has direct sunlight.
 * @param pos Position to evaluate.
 */
void World::checkIndirectSunlight( Position pos )
{
	Tile& here = getTile( pos );
	Tile& te   = getTile( pos.eastOf() );
	if ( te.flags & TileFlag::TF_SUNLIGHT )
	{
		here.flags += TileFlag::TF_INDIRECT_SUNLIGHT;
		return;
	}
	Tile& ts = getTile( pos.southOf() );
	if ( ts.flags & TileFlag::TF_SUNLIGHT )
	{
		here.flags += TileFlag::TF_INDIRECT_SUNLIGHT;
		return;
	}
	Tile& tw = getTile( pos.westOf() );
	if ( tw.flags & TileFlag::TF_SUNLIGHT )
	{
		here.flags += TileFlag::TF_INDIRECT_SUNLIGHT;
		return;
	}
	Tile& tn = getTile( pos.northOf() );
	if ( tn.flags & TileFlag::TF_SUNLIGHT )
	{
		here.flags += TileFlag::TF_INDIRECT_SUNLIGHT;
		return;
	}

	clearTileFlag( pos, TileFlag::TF_INDIRECT_SUNLIGHT );
}

/**
 * @brief Returns the fluid (water) level at the given position.
 * @param pos World position.
 * @return Fluid level (0-10).
 */
int World::fluidLevel( Position pos )
{
	Tile& tile = getTile( pos );
	return tile.fluidLevel;
}

/**
 * @brief Returns the vegetation (grass growth) level at the given position.
 * @param pos World position.
 * @return Vegetation level (0-100).
 */
int World::vegetationLevel( Position pos )
{
	Tile& tile = getTile( pos );
	return tile.vegetationLevel;
}

/**
 * @brief Sets the vegetation level on the tile at the given position.
 * @param pos World position.
 * @param level New vegetation level to set (0-100).
 */
void World::setVegetationLevel( Position pos, int level )
{
	Tile& tile           = getTile( pos );
	tile.vegetationLevel = level;
}

/**
 * @brief Loads floor construction records from a saved list into the floor constructions map.
 * @param list List of QVariantMaps, each containing a "Pos" key and construction data.
 */
void World::loadFloorConstructions( QVariantList list )
{
	for ( auto item : list )
	{
		Position pos( item.toMap().value( "Pos" ) );
		auto constr = item.toMap();
		m_floorConstructions.insert( pos.toInt(), constr );
	}
}

/**
 * @brief Loads wall construction records from a saved list into the wall constructions map.
 *
 * Also restores light sources from constructions that have a "Light" value.
 * @param list List of QVariantMaps, each containing a "Pos" key and construction data.
 */
void World::loadWallConstructions( QVariantList list )
{
	for ( auto item : list )
	{
		auto map = item.toMap();

		Position pos( map.value( "Pos" ) );
		auto constr = item.toMap();
		m_wallConstructions.insert( pos.toInt(), constr );

		if ( map.value( "Light" ).toInt() > 0 )
		{
			addLight( pos.toInt(), pos, map.value( "Light" ).toInt() );
		}
	}
}

/**
 * @brief Returns the full map of job sprites, keyed by flat tile index.
 * @return Copy of the job sprites map.
 */
QMap<unsigned int, QVariantMap> World::jobSprites()
{
	return m_jobSprites;
}

/**
 * @brief Inserts a job sprite entry loaded from a save file.
 * @param key Flat tile index.
 * @param entry QVariantMap containing the job sprite data.
 */
void World::insertLoadedJobSprite( unsigned int key, QVariantMap entry )
{
	m_jobSprites.insert( key, entry );
}

/**
 * @brief Counts how many of the four cardinal neighbors are walkable.
 * @param pos World position to check neighbors of.
 * @return Number of walkable cardinal neighbors (0-4).
 */
int World::walkableNeighbors( Position pos )
{
	int out = 0;
	if ( isWalkable( pos.eastOf() ) )
		++out;
	if ( isWalkable( pos.westOf() ) )
		++out;
	if ( isWalkable( pos.southOf() ) )
		++out;
	if ( isWalkable( pos.northOf() ) )
		++out;
	return out;
}

/**
 * @brief Returns a JSON string representation of the wall construction at the given position for debugging.
 * @param pos World position.
 * @return JSON string of the construction data, or "no construction" if none exists.
 */
QString World::getDebugWallConstruction( Position pos )
{
	if ( m_wallConstructions.contains( pos.toInt() ) )
	{
		auto wc = m_wallConstructions.value( pos.toInt() );
		return QString( QJsonDocument::fromVariant( wc ).toJson() );
	}
	return "no construction";
}

/**
 * @brief Returns the item UID of furniture placed on the given tile, if any.
 * @param pos World position.
 * @return Item UID of the furniture, or 0 if no furniture is present.
 */
unsigned int World::getFurnitureOnTile( Position pos )
{
	if ( m_wallConstructions.contains( pos.toInt() ) )
	{
		auto wc = m_wallConstructions.value( pos.toInt() );
		if( wc.value( "Type").toString() == "Furniture" )
		{
			return wc.value( "Item" ).toUInt();
		}
	}
	return 0;
}

/**
 * @brief Tests whether there is an unobstructed line of sight between two positions.
 * @param a Starting position.
 * @param b Target position.
 * @return True if no view-blocking wall or solid floor obstructs the path.
 */
bool World::isLineOfSight( Position a, Position b ) const
{
	return testLine( a, b, [world = this]( const Position &current, const Position& previous ) -> bool {
		const auto& tile = world->getTile( current );
		if ( tile.wallType & WallType::WT_VIEWBLOCKING )
		{
			return false;
		}
		if ( current.z > previous.z && tile.floorType & FloorType::FT_SOLIDFLOOR )
		{
			return false;
		}
		if (current.z < previous.z)
		{
			const auto& tilePrev = world->getTile( current );
			if ( tilePrev.floorType & FloorType::FT_SOLIDFLOOR )
			{
				return false;
			}
		}
		return true;
	} );
}

/**
 * @brief Returns a JSON string representation of the floor construction at the given position for debugging.
 * @param pos World position.
 * @return JSON string of the construction data, or "no construction" if none exists.
 */
QString World::getDebugFloorConstruction( Position pos )
{
	if ( m_floorConstructions.contains( pos.toInt() ) )
	{
		if ( m_floorConstructions.contains( pos.toInt() ) )
		{
			auto fc = m_floorConstructions.value( pos.toInt() );
			return QString( QJsonDocument::fromVariant( fc ).toJson() );
		}
	}
	return "no construction";
}

/**
 * @brief Returns all positions reachable from the given position in a single step.
 *
 * Includes cardinal and diagonal neighbors (diagonal only if both adjacent cardinals
 * are walkable), ramp transitions to adjacent levels, stairs, and scaffolds.
 * @param pos World position to find neighbors for.
 * @return List of reachable neighbor positions.
 */
QList<Position> World::connectedNeighbors( Position pos )
{
	QList<Position> out;

	bool north = false;
	bool east  = false;
	bool south = false;
	bool west  = false;

	auto next = pos.northOf();
	if ( isWalkable( next ) )
	{
		north = true;
		out.append( next );
	}
	{
		Tile& tile = getTile( next );
		if ( tile.floorType & FT_RAMPTOP )
		{
			out.append( next.belowOf() );
		}
	}

	next = pos.eastOf();
	if ( isWalkable( next ) )
	{
		east = true;
		out.append( next );
	}
	{
		Tile& tile = getTile( next );
		if ( tile.floorType & FT_RAMPTOP )
		{
			out.append( next.belowOf() );
		}
	}

	next = pos.southOf();
	if ( isWalkable( next ) )
	{
		south = true;
		out.append( next );
	}
	{
		Tile& tile = getTile( next );
		if ( tile.floorType & FT_RAMPTOP )
		{
			out.append( next.belowOf() );
		}
	}

	next = pos.westOf();
	if ( isWalkable( next ) )
	{
		west = true;
		out.append( next );
	}
	{
		Tile& tile = getTile( next );
		if ( tile.floorType & FT_RAMPTOP )
		{
			out.append( next.belowOf() );
		}
	}

	if ( north && east )
	{
		next = pos.neOf();
		if ( isWalkable( next ) )
		{
			out.append( next );
		}
	}
	if ( east && south )
	{
		next = pos.seOf();
		if ( isWalkable( next ) )
		{
			out.append( next );
		}
	}
	if ( south && west )
	{
		next = pos.swOf();
		if ( isWalkable( next ) )
		{
			out.append( next );
		}
	}
	if ( west && north )
	{
		next = pos.nwOf();
		if ( isWalkable( next ) )
		{
			out.append( next );
		}
	}

	Tile& curTile = g->w()->getTile( pos );

	if ( (bool)( curTile.wallType & ( WT_STAIR | WT_SCAFFOLD ) ) )
	{
		next = pos.aboveOf();
		out.append( next );
	}
	if ( (bool)( curTile.floorType & ( FT_STAIRTOP | FT_SCAFFOLD ) ) )
	{
		next = pos.belowOf();
		out.append( next );
	}
	if ( (bool)( curTile.wallType & ( WT_RAMP ) ) )
	{
		auto above = pos.aboveOf();
		next       = above.northOf();
		if ( isWalkable( next ) )
		{
			out.append( next );
		}
		next = above.eastOf();
		if ( isWalkable( next ) )
		{
			out.append( next );
		}
		next = above.southOf();
		if ( isWalkable( next ) )
		{
			out.append( next );
		}
		next = above.westOf();
		if ( isWalkable( next ) )
		{
			out.append( next );
		}
	}
	return out;
}
