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

Tile& World::getTile( const unsigned short x, const unsigned short y, const unsigned short z )
{
	return getTile( Position( x, y, z ) );
}

Tile& World::getTile( const Position pos )
{
	return m_world[pos.toInt()];
}

const Tile& World::getTile( const Position pos ) const
{
	return m_world[pos.toInt()];
}

Tile& World::getTile( const unsigned int id )
{
	return m_world[id];
}

bool World::hasJob( Position pos )
{
	return hasJob( pos.toInt() );
}

bool World::hasJob( int x, int y, int z )
{
	return hasJob( Position( x, y, z ).toInt() );
}

bool World::hasJob( unsigned int tileID )
{
	return m_jobSprites.contains( tileID );
}

QVariantMap World::jobSprite( Position pos )
{
	return m_jobSprites.value( pos.toInt() );
}

QVariantMap World::jobSprite( int x, int y, int z )
{
	return jobSprite( Position( x, y, z ) );
}

QVariantMap World::jobSprite( unsigned int tileID )
{
	return m_jobSprites.value( tileID );
}

TileFlag World::getTileFlag( Position pos )
{
	return getTile( pos ).flags;
}

unsigned short World::wallMaterial( Position pos )
{
	Tile& tile = getTile( pos );
	return tile.wallMaterial;
}

unsigned short World::embeddedMaterial( Position pos )
{
	Tile& tile = getTile( pos );
	return tile.embeddedMaterial;
}

unsigned short World::floorMaterial( Position pos )
{
	Tile& tile = getTile( pos );
	return tile.floorMaterial;
}

WallType World::wallType( Position pos )
{
	Tile& tile = getTile( pos );
	return tile.wallType;
}

FloorType World::floorType( Position pos )
{
	Tile& tile = getTile( pos );
	return tile.floorType;
}

bool World::creatureAtPos( Position pos )
{
	return m_creaturePositions.contains( pos.toInt() );
}

bool World::creatureAtPos( unsigned int posID )
{
	bool ret = m_creaturePositions.contains( posID );
	return ret;
}

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

void World::updateWalkable( Position pos )
{
	m_regionMap.updatePosition( pos );
}

bool World::isWalkable( Position pos )
{
	Tile& tile = getTile( pos );
	return tile.flags & TileFlag::TF_WALKABLE;
}

bool World::isWalkableGnome( Position pos )
{
	Tile& tile = getTile( pos );
	return tile.flags & TileFlag::TF_WALKABLE && !( tile.flags & TileFlag::TF_NOPASS ) && ( tile.fluidLevel < 4 ) && !( tile.flags & TileFlag::TF_LAVA );
}

bool World::isRamp( Position pos )
{
	Tile& tile = getTile( pos );
	return ( tile.wallType & WallType::WT_RAMP ) || ( tile.floorType & FloorType::FT_RAMPTOP );
}

bool World::isSolidFloor( Position pos )
{
	Tile& tile = getTile( pos );
	return ( tile.floorType & FloorType::FT_SOLIDFLOOR );
}

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

QSet<unsigned int> World::updatedTiles()
{
	QMutexLocker lock( &m_updateMutex );
	QSet<unsigned int> ret;
	ret.swap( m_updatedTiles );
	// Assume next update batch is going to be similar in size
	m_updatedTiles.reserve( ret.size() );
	return ret;
}

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

void World::checkIndirectSunlightForNeighbors( Position pos )
{
	checkIndirectSunlight( pos.eastOf() );
	checkIndirectSunlight( pos.southOf() );
	checkIndirectSunlight( pos.westOf() );
	checkIndirectSunlight( pos.northOf() );
}

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

int World::fluidLevel( Position pos )
{
	Tile& tile = getTile( pos );
	return tile.fluidLevel;
}

int World::vegetationLevel( Position pos )
{
	Tile& tile = getTile( pos );
	return tile.vegetationLevel;
}

void World::setVegetationLevel( Position pos, int level )
{
	Tile& tile           = getTile( pos );
	tile.vegetationLevel = level;
}

void World::loadFloorConstructions( QVariantList list )
{
	for ( auto item : list )
	{
		Position pos( item.toMap().value( "Pos" ) );
		auto constr = item.toMap();
		m_floorConstructions.insert( pos.toInt(), constr );
	}
}

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

QMap<unsigned int, QVariantMap> World::jobSprites()
{
	return m_jobSprites;
}

void World::insertLoadedJobSprite( unsigned int key, QVariantMap entry )
{
	m_jobSprites.insert( key, entry );
}

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

QString World::getDebugWallConstruction( Position pos )
{
	if ( m_wallConstructions.contains( pos.toInt() ) )
	{
		auto wc = m_wallConstructions.value( pos.toInt() );
		return QString( QJsonDocument::fromVariant( wc ).toJson() );
	}
	return "no construction";
}

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
