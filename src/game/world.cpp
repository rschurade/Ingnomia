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
#include "world.h"

#include "../base/config.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/position.h"
#include "../base/util.h"
#include "../base/vptr.h"
#include "../game/creaturemanager.h"
#include "../game/farmingmanager.h"
#include "../game/gnomemanager.h"
#include "../game/inventory.h"
#include "../game/mechanismmanager.h"
#include "../game/plant.h"
#include "../game/roommanager.h"
#include "../game/stockpilemanager.h"
#include "../game/workshopmanager.h"
#include "../gfx/sprite.h"
#include "../gfx/spritefactory.h"
#include "../gui/eventconnector.h"

#include <QDebug>
#include <QJsonDocument>
#include <QVector3D>

#include <random>
#include <time.h>

World::World()
{
	m_constructionSID2ENUM.insert( "Wall", CID_WALL );
	m_constructionSID2ENUM.insert( "FancyWall", CID_FANCYWALL );
	m_constructionSID2ENUM.insert( "Fence", CID_FENCE );
	m_constructionSID2ENUM.insert( "Floor", CID_FLOOR );
	m_constructionSID2ENUM.insert( "FancyFloor", CID_FANCYFLOOR );
	m_constructionSID2ENUM.insert( "WallFloor", CID_WALLFLOOR );
	m_constructionSID2ENUM.insert( "Stairs", CID_STAIRS );
	m_constructionSID2ENUM.insert( "Ramp", CID_RAMP );
	m_constructionSID2ENUM.insert( "RampCorner", CID_RAMPCORNER );
	m_constructionSID2ENUM.insert( "Scaffold", CID_SCAFFOLD );
	m_constructionSID2ENUM.insert( "Item", CID_ITEM );
	m_constructionSID2ENUM.insert( "Workshop", CID_WORKSHOP );

	m_constrItemSID2ENUM.insert( "Containers", CI_STORAGE );
	m_constrItemSID2ENUM.insert( "Furniture", CI_FURNITURE );
	m_constrItemSID2ENUM.insert( "Lights", CI_LIGHT );
	m_constrItemSID2ENUM.insert( "Doors", CI_DOOR );
	m_constrItemSID2ENUM.insert( "AlarmBell", CI_ALARMBELL );
	m_constrItemSID2ENUM.insert( "Farm", CI_FARMUTIL );
	m_constrItemSID2ENUM.insert( "Mechanism", CI_MECHANISM );
	m_constrItemSID2ENUM.insert( "Hydraulics", CI_HYDRAULICS );
}

World::~World()
{
}

void World::init()
{
	qDebug() << "World::init";

	m_dimX = Global::dimX;
	m_dimY = Global::dimY;
	m_dimZ = Global::dimZ;

	m_lightMap.init();
	initWater();
	initGrassUpdateList();
}

void World::initLite()
{
	m_dimX = Global::dimX;
	m_dimY = Global::dimY;
	m_dimZ = Global::dimZ;
}

void World::initWater()
{
	m_water.clear();

	for ( int z = m_dimZ - 2; z >= 0; --z )
	{
		for ( int y = 0; y < m_dimY; ++y )
		{
			for ( int x = 0; x < m_dimX; ++x )
			{
				Tile& here = getTile( x, y, z );

				if ( here.flags & TileFlag::TF_AQUIFIER )
				{
					addAquifier( Position( x, y, z ) );
				}
				if ( here.flags & TileFlag::TF_DEAQUIFIER )
				{
					addDeaquifier( Position( x, y, z ) );
				}

				here.pressure = 0;
				if ( here.fluidLevel > 0 )
				{
					if ( (bool)( here.wallType & WT_SOLIDWALL ) )
					{
						here.fluidLevel = 0;
					}
					else
					{
						Position pos( x, y, z );
						m_water.insert( pos.toInt() );

						Tile& above = getTile( pos.aboveOf() );
						if (
							above.fluidLevel > 0 && here.fluidLevel == 10 && !(bool)( above.floorType & FloorType::FT_SOLIDFLOOR ) )
						{
							here.pressure = above.pressure + 1;
						}
					}
				}
			}
		}
	}
}

void World::reset()
{
	qDebug() << "World::reset";
	m_world.clear();
	m_plants.clear();
	m_creaturePositions.clear();
	m_floorConstructions.clear();
	m_wallConstructions.clear();
	m_grass.clear();
	m_grassCandidatePositions.clear();
	m_regionMap.clear();
	m_lightMap.init();
	m_jobSprites.clear();
	m_water.clear();
	m_aquifiers.clear();
	m_deaquifiers.clear();
}

void World::afterLoad()
{
	qDebug() << "World::afterLoad";
	m_dimX = Global::dimX;
	m_dimY = Global::dimY;
	m_dimZ = Global::dimZ;

	m_regionMap.initRegions();
	m_lightMap.init();
	initGrassUpdateList();
	initWater();
}

void World::setFloorSprite( unsigned short x, unsigned short y, unsigned short z, const unsigned int spriteUID )
{
	getTile( x, y, z ).floorSpriteUID = spriteUID;
	addToUpdateList( x, y, z );
	GameState::addChange( NetworkCommand::SETFLOORSPRITE, { QString::number( Position( x, y, z ).toInt() ), QString::number( spriteUID ) } );
}

void World::setFloorSprite( Position pos, unsigned int spriteUID )
{
	getTile( pos ).floorSpriteUID = spriteUID;
	addToUpdateList( pos );
	GameState::addChange( NetworkCommand::SETFLOORSPRITE, { QString::number( pos.toInt() ), QString::number( spriteUID ) } );
}

void World::setWallSprite( unsigned short x, unsigned short y, unsigned short z, unsigned int spriteUID, unsigned char rotation )
{
	QMutexLocker lock( &m_mutex );
	unsigned int UID           = Position( x, y, z ).toInt();
	m_world[UID].wallSpriteUID = spriteUID;
	m_world[UID].wallRotation  = rotation;
	GameState::addChange( NetworkCommand::SETWALLSPRITE, { QString::number( UID ), QString::number( spriteUID ) } );
	addToUpdateList( UID );
}

void World::setWallSprite( Position pos, unsigned int spriteUID, unsigned char rotation )
{
	QMutexLocker lock( &m_mutex );
	unsigned int UID           = pos.toInt();
	m_world[UID].wallSpriteUID = spriteUID;
	m_world[UID].wallRotation  = rotation;
	GameState::addChange( NetworkCommand::SETWALLSPRITE, { QString::number( UID ), QString::number( spriteUID ) } );
	addToUpdateList( UID );
}

void World::setWallSprite( unsigned int tileID, unsigned int spriteUID )
{
	m_world[tileID].wallSpriteUID = spriteUID;
}

void World::setItemSprite( unsigned short x, unsigned short y, unsigned short z, unsigned int spriteUID, unsigned char rotation )
{
	QMutexLocker lock( &m_mutex );
	unsigned int UID           = Position( x, y, z ).toInt();
	m_world[UID].itemSpriteUID = spriteUID;
	//m_world[UID].wallRotation = rotation;
	//GameState::addChange( NetworkCommand::SETITEMSPRITE, { QString::number( UID ), QString::number( spriteUID ) } );
	addToUpdateList( UID );
}

void World::setItemSprite( Position pos, unsigned int spriteUID, unsigned char rotation )
{
	QMutexLocker lock( &m_mutex );
	unsigned int UID           = pos.toInt();
	m_world[UID].itemSpriteUID = spriteUID;
	//m_world[UID].wallRotation = rotation;
	GameState::addChange( NetworkCommand::SETWALLSPRITE, { QString::number( UID ), QString::number( spriteUID ) } );
	addToUpdateList( UID );
}

void World::setFloorSprite( unsigned int tileID, unsigned int spriteUID )
{
	m_world[tileID].wallSpriteUID = spriteUID;
	addToUpdateList( tileID );
}

void World::setJobSprite( unsigned int tileID, unsigned int spriteUID, unsigned char rotation, bool floor, unsigned int jobID, bool busy )
{
	Position pos( tileID );
	setJobSprite( pos, spriteUID, rotation, floor, jobID, busy );
}

void World::setJobSprite( Position pos, unsigned int spriteUID, unsigned char rotation, bool floor, unsigned int jobID, bool busy )
{
	Tile& tile = getTile( pos );
	setTileFlag( pos, floor ? TileFlag::TF_JOB_FLOOR : TileFlag::TF_JOB_WALL );
	if ( busy )
	{
		if ( floor )
			setTileFlag( pos, TileFlag::TF_JOB_BUSY_FLOOR );
		else
			setTileFlag( pos, TileFlag::TF_JOB_BUSY_WALL );
	}
	else
	{
		if ( floor )
			clearTileFlag( pos, TileFlag::TF_JOB_BUSY_FLOOR );
		else
			clearTileFlag( pos, TileFlag::TF_JOB_BUSY_WALL );
	}

	QVariantMap s;
	s.insert( "Pos", pos.toString() );
	s.insert( "Rot", rotation );
	s.insert( "JobID", jobID );
	s.insert( "SpriteUID", spriteUID );

	QMutexLocker lock( &m_mutex );
	if ( !m_jobSprites.contains( pos.toInt() ) )
	{
		QVariantMap entry;
		m_jobSprites.insert( pos.toInt(), entry );
	}
	if ( floor )
	{
		m_jobSprites[pos.toInt()].insert( "Floor", s );
	}
	else
	{
		m_jobSprites[pos.toInt()].insert( "Wall", s );
	}

	addToUpdateList( pos.toInt() );
	GameState::addChange( NetworkCommand::SETJOBSPRITE, { QString::number( pos.toInt() ), QString::number( spriteUID ), QString::number( rotation ), ( floor ) ? QString::number( 1 ) : QString::number( 0 ), QString::number( jobID ) } );
}

void World::clearJobSprite( Position pos, bool floor )
{
	QMutexLocker lock( &m_mutex );

	if ( m_jobSprites.contains( pos.toInt() ) )
	{
		if ( floor )
		{
			m_jobSprites[pos.toInt()].remove( "Floor" );
		}
		else
		{
			m_jobSprites[pos.toInt()].remove( "Wall" );
		}
	}
	if ( m_jobSprites[pos.toInt()].isEmpty() )
	{
		m_jobSprites.remove( pos.toInt() );
	}
	clearTileFlag( pos, TileFlag::TF_JOB_FLOOR + TileFlag::TF_JOB_WALL + TileFlag::TF_JOB_BUSY_FLOOR + TileFlag::TF_JOB_BUSY_WALL );
	GameState::addChange2( NetworkCommand::CLEARJOBSPRITE, pos.toString() );
	addToUpdateList( pos.toInt() );
}

void World::setTileFlag( unsigned short x, unsigned short y, unsigned short z, TileFlag flag )
{
	Position pos( x, y, z );
	setTileFlag( pos, flag );
}

void World::setTileFlag( Position pos, TileFlag flag )
{
	unsigned int tid = pos.toInt();
	Tile& tile       = getTile( tid );
	tile.flags += flag;

	if ( flag & TileFlag::TF_WALKABLE )
	{
		m_regionMap.updatePosition( pos );
	}
	GameState::addChange( NetworkCommand::SETTILEFLAGS, { QString::number( tid ), Util::tile2String( tile ) } );
	addToUpdateList( pos.toInt() );
}

void World::clearTileFlag( Position pos, TileFlag flag )
{
	unsigned int tid = pos.toInt();
	Tile& tile       = getTile( tid );
	tile.flags -= flag;

	if ( flag & TileFlag::TF_WALKABLE )
	{
		m_regionMap.updatePosition( pos );

		Global::fm().removeTile( pos, true, true, false );
		Global::spm().removeTile( pos );
		Global::rm().removeTile( pos );
	}

	GameState::addChange( NetworkCommand::SETTILEFLAGS, { QString::number( tid ), Util::tile2String( tile ) } );
	addToUpdateList( pos.toInt() );
}

void World::updateFenceSprite( Position pos )
{
	QVariantMap constr;
	if ( m_wallConstructions.contains( pos.toInt() ) )
	{
		constr = m_wallConstructions.value( pos.toInt() );
	}
	else
	{
		return;
	}
	QString suffix          = "Rot";
	QString constructionSID = constr.value( "ConstructionID" ).toString();

	if ( m_wallConstructions.contains( pos.northOf().toInt() ) )
	{
		QVariantMap nc = m_wallConstructions.value( pos.northOf().toInt() );
		if ( nc.value( "ConstructionID" ).toString() == constructionSID )
		{
			suffix += "N";
		}
	}

	if ( m_wallConstructions.contains( pos.eastOf().toInt() ) )
	{
		QVariantMap nc = m_wallConstructions.value( pos.eastOf().toInt() );
		if ( nc.value( "ConstructionID" ).toString() == constructionSID )
		{
			suffix += "E";
		}
	}
	if ( m_wallConstructions.contains( pos.southOf().toInt() ) )
	{
		QVariantMap nc = m_wallConstructions.value( pos.southOf().toInt() );
		if ( nc.value( "ConstructionID" ).toString() == constructionSID )
		{
			suffix += "S";
		}
	}
	if ( m_wallConstructions.contains( pos.westOf().toInt() ) )
	{
		QVariantMap nc = m_wallConstructions.value( pos.westOf().toInt() );
		if ( nc.value( "ConstructionID" ).toString() == constructionSID )
		{
			suffix += "W";
		}
	}

	auto spl = DB::selectRows( "Constructions_Sprites", "ID", constructionSID );
	for ( auto sp : spl )
	{
		Position offset;
		offset = Position( sp.value( "Offset" ).toString() );
		Position constrPos( pos + offset );
		unsigned int tid       = constrPos.toInt();
		Tile& tile             = getTile( tid );
		QString materialID     = DBH::materialSID( tile.wallMaterial );
		QString spriteSID      = sp.value( "SpriteID" ).toString() + suffix;
		unsigned int spriteUID = Global::sf().createSprite( spriteSID, { materialID } )->uID;

		tile.wallSpriteUID = spriteUID;
	}
	addToUpdateList( pos );
}

void World::updatePipeSprite( Position pos )
{
	QVariantMap constr;
	if ( m_wallConstructions.contains( pos.toInt() ) )
	{
		constr = m_wallConstructions.value( pos.toInt() );
	}
	else
	{
		return;
	}
	QString suffix = "Rot";

	unsigned int itemUID = constr.value( "Item" ).toUInt();
	QString itemSID      = Global::inv().itemSID( itemUID );

	if ( itemSID.startsWith( "Pump" ) )
	{
		return;
	}

	if ( getTileFlag( pos.northOf() ) & TileFlag::TF_PIPE )
	{
		suffix += "N";
	}
	if ( getTileFlag( pos.eastOf() ) & TileFlag::TF_PIPE )
	{
		suffix += "E";
	}
	if ( getTileFlag( pos.southOf() ) & TileFlag::TF_PIPE )
	{
		suffix += "S";
	}
	if ( getTileFlag( pos.westOf() ) & TileFlag::TF_PIPE )
	{
		suffix += "W";
	}

	auto spl = DB::selectRows( "Constructions_Sprites", "ID", itemSID );
	for ( auto sp : spl )
	{
		Position offset;
		offset = Position( sp.value( "Offset" ).toString() );
		Position constrPos( pos + offset );
		unsigned int tid       = constrPos.toInt();
		Tile& tile             = getTile( tid );
		QString materialID     = DBH::materialSID( tile.wallMaterial );
		QString spriteSID      = sp.value( "SpriteID" ).toString() + suffix;
		unsigned int spriteUID = Global::sf().createSprite( spriteSID, { materialID } )->uID;

		tile.wallSpriteUID = spriteUID;
	}
	addToUpdateList( pos );
}

void World::addLoadedSprites( QVariantMap vals )
{
	Tile& tile = getTile( Position( vals.value( "Pos" ) ) );

	if ( vals.value( "IsFloor" ).toBool() )
	{
		tile.floorSpriteUID = vals.value( "UID" ).toUInt();
	}
	else
	{
		tile.wallSpriteUID = vals.value( "UID" ).toUInt();
	}
	//Util::string2Tile( tile, vals.value( "OldTileVals" ).toString() );
}

void World::expelTileItems( Position pos, Position& to )
{
	Tile& tile = getTile( pos );
	//do we even have to move anything?
	PositionEntry pe;
	if ( Global::inv().getObjectsAtPosition( pos, pe ) )
	{
		//check if tile is now blocked for items and creatures
		if ( tile.wallType & WallType::WT_MOVEBLOCKING || !isWalkable( pos ) )
		{
			for ( auto i : pe )
			{
				Global::inv().moveItemToPos( i, to );
			}
		}
	}
}

void World::expelTileInhabitants( Position pos, Position& to )
{
	//qDebug() << "expel from " << pos.toString();
	Tile& tile = getTile( pos );
	// check if someone is on the tile
	if ( m_creaturePositions.contains( pos.toInt() ) )
	{
		//check if tile is now blocked for items and creatures
		if ( (bool)( tile.wallType & WallType::WT_MOVEBLOCKING ) || !isWalkable( pos ) )
		{
			Global::gm().forceMoveGnomes( pos, to );
			Global::cm().forceMoveAnimals( pos, to );
		}
	}
}

void World::plantTree( Position pos, QString type, bool fullyGrown )
{
	Plant plant_( pos, type, fullyGrown );
	m_plants.insert( pos.toInt(), plant_ );

	QMutexLocker lock( &m_mutex );
	getTile( pos ).wallSpriteUID = m_plants[pos.toInt()].getSprite()->uID;
	addToUpdateList( pos );
}

void World::plantMushroom( Position pos, QString type, bool fullyGrown )
{
	Plant plant_( pos, type, fullyGrown );
	m_plants.insert( pos.toInt(), plant_ );

	QMutexLocker lock( &m_mutex );
	getTile( pos ).wallSpriteUID = m_plants[pos.toInt()].getSprite()->uID;
	addToUpdateList( pos );
}

void World::plant( Position pos, unsigned int baseItem )
{
	QStringList plants = DB::ids( "Plants", "Type", "Plant" );
	for ( auto plant : plants )
	{
		if ( DB::select( "Material", "Plants", plant ).toString() == Global::inv().materialSID( baseItem ) )
		{
			Plant plant_( pos, plant );
			m_plants.insert( pos.toInt(), plant_ );
			QMutexLocker lock( &m_mutex );
			getTile( pos ).wallSpriteUID = m_plants[pos.toInt()].getSprite()->uID;
			return;
		}
	}
}

void World::addPlant( Plant plant )
{
	Position pos = plant.getPos();
	m_plants.insert( pos.toInt(), plant );
	//getTile( pos ).wallSpriteUID = m_plants[pos.toInt()].getSprite();
}

void World::removePlant( Position pos )
{
	QMutexLocker lock( &m_mutex );
	if ( m_plants.contains( pos.toInt() ) )
	{
		getTile( pos ).wallType      = WallType::WT_NOWALL;
		getTile( pos ).wallSpriteUID = 0;
		m_plants.remove( pos.toInt() );
		getTile( pos ).wallSpriteUID = 0;
		addToUpdateList( pos );
	}
}

void World::removePlant( Plant plant )
{
	m_plants.remove( plant.getPos().toInt() );
}

bool World::reduceOneGrowLevel( Position pos )
{
	if ( m_plants.contains( pos.toInt() ) )
	{
		if ( m_plants[pos.toInt()].reduceOneGrowLevel() )
		{
			removePlant( pos );
			return true;
		}
	}
	return false;
}

void World::removeCreatureFromPosition( Position pos, unsigned int creatureID )
{
	QMutexLocker lock( &m_mutex );
	if ( m_creaturePositions.contains( pos.toInt() ) )
	{
		if ( m_creaturePositions[pos.toInt()].size() == 1 )
		{
			m_creaturePositions.remove( pos.toInt() );
			Global::mcm().updateCreaturesAtPos( pos, 0 );
		}
		else
		{
			QList<unsigned int>& cl = m_creaturePositions[pos.toInt()];
			cl.removeAll( creatureID );
			Global::mcm().updateCreaturesAtPos( pos, m_creaturePositions[pos.toInt()].size() );
		}
		addToUpdateList( pos );
	}
}

void World::insertCreatureAtPosition( Position pos, unsigned int creatureID )
{
	QMutexLocker lock( &m_mutex );
	if ( m_creaturePositions.contains( pos.toInt() ) )
	{
		m_creaturePositions[pos.toInt()].push_back( creatureID );
		Global::mcm().updateCreaturesAtPos( pos, m_creaturePositions[pos.toInt()].size() );
	}
	else
	{
		QList<unsigned int> cl( { creatureID } );
		m_creaturePositions.insert( pos.toInt(), cl );
		Global::mcm().updateCreaturesAtPos( pos, 1 );
	}
	addToUpdateList( pos );
}

void World::processGrass()
{
	QList<Position> toRemove;
	unsigned short dirtUID = Global::dirtUID;
	for ( auto pi : m_grassCandidatePositions )
	{
		if ( m_grass.contains( pi ) )
		{
			toRemove.append( pi );
		}
		else
		{
			Position p( pi );

			if ( rand() % 300 > 298 )
			{
				Tile& tile = getTile( p );
				if ( tile.floorMaterial == dirtUID && tile.floorType & FT_SOLIDFLOOR && tile.flags & TileFlag::TF_SUNLIGHT )
				{
					if ( tile.wallType & WT_RAMP && tile.wallMaterial == dirtUID )
					{
						setTileFlag( p, TileFlag::TF_GRASS );
						createRamp( p );
						addToUpdateList( p );

						auto pa = p.aboveOf();
						addToUpdateList( pa );

						isGrassCandidate( pa.northOf() );
						isGrassCandidate( pa.eastOf() );
						isGrassCandidate( pa.southOf() );
						isGrassCandidate( pa.westOf() );
					}
					else
					{
						createGrass( p );
						addToUpdateList( p );
					}
				}
				toRemove.append( p );

				isGrassCandidate( p.northOf() );
				isGrassCandidate( p.eastOf() );
				isGrassCandidate( p.southOf() );
				isGrassCandidate( p.westOf() );
			}
		}
	}
	for ( auto p : toRemove )
	{
		m_grassCandidatePositions.remove( p.toInt() );
	}
}

void World::initGrassUpdateList()
{
	m_grass.clear();
	m_grassCandidatePositions.clear();

	for ( int z = m_dimZ - 2; z >= 0; --z )
	{
		for ( int y = 0; y < m_dimY; ++y )
		{
			for ( int x = 0; x < m_dimX; ++x )
			{
				Tile& here = getTile( x, y, z );
				if ( (bool)( here.flags & TileFlag::TF_GRASS ) )
				{
					Position pos( x, y, z );
					m_grass.insert( pos );
				}
			}
		}
	}

	for ( auto p : m_grass )
	{
		isGrassCandidate( p.northOf() );
		isGrassCandidate( p.eastOf() );
		isGrassCandidate( p.southOf() );
		isGrassCandidate( p.westOf() );
	}
}

void World::isGrassCandidate( Position pos )
{
	Tile& tile = getTile( pos );

	if ( ( tile.floorType & FT_SOLIDFLOOR ) && ( tile.floorMaterial == Global::dirtUID ) && ( ( tile.wallType == WT_NOWALL ) || ( tile.wallType == WT_RAMP ) ) && !m_grass.contains( pos.toInt() ) )
	{
		m_grassCandidatePositions.insert( pos.toInt() );
	}
}

void World::addGrassCandidate( Position pos )
{
	isGrassCandidate( pos );
}

void World::removeGrass( Position pos )
{
	clearTileFlag( pos, TileFlag::TF_GRASS );

	if ( m_grass.contains( pos.toInt() ) )
	{
		m_grass.remove( pos.toInt() );

		Tile& tile           = getTile( pos );
		tile.vegetationLevel = 0;
		QString materialSID  = DBH::materialSID( tile.floorMaterial );

		if ( tile.floorType & FT_SOLIDFLOOR && materialSID == "Dirt" )
		{
			//if( Global::debugMode ) qDebug() << "add grass candidate at " << pos.toString();
			tile.floorSpriteUID = Global::sf().createSprite( "RoughFloor", { "Dirt" } )->uID;
			m_grassCandidatePositions.insert( pos.toInt() );
		}
	}

	addToUpdateList( pos );
}

void World::createGrass( Position pos )
{
	auto tf = getTileFlag( pos );
	if ( tf & TileFlag::TF_TILLED )
	{
		return;
	}

	Tile& tile          = getTile( pos );
	tile.floorSpriteUID = Global::sf().createSprite( "GrassWithDetail", { "Grass", "None" } )->uID;

	m_grass.insert( pos );
	setTileFlag( pos, TileFlag::TF_GRASS );
	tile.vegetationLevel = 100;
}

void World::addWater( Position pos, unsigned char level )
{
	if ( !m_water.count( pos.toInt() ) )
	{
		m_water.insert( pos.toInt() );

		Tile& tile      = getTile( pos );
		tile.fluidLevel = level;
		tile.flags += TileFlag::TF_WATER;
		addToUpdateList( pos );
	}
}

void World::changeFluidLevel( Position pos, int diff )
{
	Tile& tile         = getTile( pos );
	int effectiveLevel = tile.fluidLevel + tile.pressure + diff;
	tile.pressure      = qMax( 0, effectiveLevel - 10 );
	tile.fluidLevel    = qMin( 10, effectiveLevel );
	m_water.insert( pos.toInt() );
	tile.flags += TileFlag::TF_WATER;
	addToUpdateList( pos );
}

void World::addAquifier( Position pos )
{
	m_aquifiers.append( pos );
	m_water.insert( pos.toInt() );
	Tile& tile = getTile( pos );
	tile.flags += TileFlag::TF_WATER;
	tile.flags += TileFlag::TF_AQUIFIER;
	addToUpdateList( pos );
}

void World::addDeaquifier( Position pos )
{
	m_deaquifiers.append( pos );
	Tile& tile = getTile( pos );
	tile.flags += TileFlag::TF_DEAQUIFIER;
}

void World::processWater()
{
	// Batch updates
	QVector<unsigned int> waterUpdates;
	// Expecting to see every tile again
	waterUpdates.reserve( m_aquifiers.size() + m_deaquifiers.size() );

	// Add / remove 1 water per tick and aquifier / deaquifier
	for ( const auto& pos : m_aquifiers )
	{
		Tile& tile = getTile( pos );
		if ( tile.pressure < 2 )
		{
			if ( tile.fluidLevel < 10 )
			{
				tile.fluidLevel++;
			}
			else
			{
				tile.pressure++;
			}
			waterUpdates.append( pos.toInt() );
		}
		m_water.insert( pos.toInt() );
	}
	for ( const auto& pos : m_deaquifiers )
	{
		Tile& tile = getTile( pos );
		if ( tile.fluidLevel > 0 )
		{
			if ( tile.pressure > 0 )
			{
				tile.pressure--;
			}
			else
			{
				tile.fluidLevel--;
			}
			if ( tile.fluidLevel == 0 )
			{
				tile.flow = WF_NOFLOW;
				tile.flags -= TileFlag::TF_WATER;
				m_water.erase( pos.toInt() );
			}
			waterUpdates.append( pos.toInt() );
		}
	}

	// Batch submit water tile updates
	addToUpdateList( waterUpdates );
	waterUpdates.clear();

	processWaterFlow();
}

struct Neighbors
{
#ifdef _WIN32
	__forceinline Neighbors( unsigned int pos )
#else
	inline Neighbors( unsigned int pos )
#endif
	{
		const auto pitchY = Global::dimX;
		const auto pitchZ = pitchY * Global::dimY;

		const auto maxZ = pitchZ * ( Global::dimZ - 1 );

		const auto plane = pos % pitchZ;
		const auto row   = pos % pitchY;

		above = pos < maxZ ? pos + pitchZ : 0;
		below = pos > pitchZ ? pos - pitchZ : 0;
		north = plane > pitchY * 2 ? pos - pitchY : 0;
		south = plane < pitchZ - 2 * pitchY ? pos + pitchY : 0;
		east  = row < pitchY - 2 ? pos + 1 : 0;
		west  = row > 1 ? pos - 1 : 0;
	}
	unsigned int above;
	unsigned int below;
	unsigned int north;
	unsigned int south;
	unsigned int east;
	unsigned int west;
};

void World::processWaterFlow()
{
	// Batch newly tracked water tiles
	QSet<unsigned int> newWater;
	QSet<unsigned int> removedWater;

	QVector<unsigned int> drain;
	QVector<unsigned int> flood;

	// Random numbers are expensive, and rand() only delivers 15bit of entropy per call
	auto seedBase = rand() ^ rand() << 10 ^ rand() << 20;
	for ( const auto& currentPos : m_water )
	{
		Tile& here = getTile( currentPos );

		if ( (bool)( here.wallType & WallType::WT_MOVEBLOCKING ) )
		{
			// Bogus, this should not be in water list
			here.flow       = WF_NOFLOW;
			here.pressure   = 0;
			here.fluidLevel = 0;
			here.flags -= TileFlag::TF_WATER;
		}

		if ( here.fluidLevel > 0 )
		{
			// Just barely random enough not to be obvious
			const auto seed = ( currentPos ^ currentPos << 16 ^ seedBase ) % 2147483647;

			if ( here.fluidLevel <= 2 && (bool)( here.floorType & FloorType::FT_SOLIDFLOOR ) )
			{
				// Low fluid level on solid floor never moves, only may evaporate
				if ( ( seed % 1000 ) == 0 )
				{
					here.flow = WF_EVAP;
					drain.append( currentPos );
				}
				else
				{
					here.flow = WF_NOFLOW;
				}
				continue;
			}

			const Neighbors neighbors( currentPos );

			// Flow down if no back-pressure
			const Tile& downTile = getTile( neighbors.below );
			if (
				neighbors.below && !(bool)( here.floorType & FloorType::FT_SOLIDFLOOR ) && !(bool)( downTile.wallType & WallType::WT_MOVEBLOCKING ) && ( downTile.fluidLevel < 10 || here.pressure >= downTile.pressure ) )
			{
				here.flow = WF_DOWN;
				drain.append( currentPos );
				flood.append( neighbors.below );
				continue;
			}

			// Flow sidewards in direction of lowest level
			const unsigned int candidates[5] = {
				neighbors.north,
				neighbors.south,
				neighbors.east,
				neighbors.west,
				currentPos
			};
			constexpr WaterFlow direction[5] = {
				WF_NORTH,
				WF_SOUTH,
				WF_EAST,
				WF_WEST,
				WF_NOFLOW
			};

			unsigned int pressure[5] = { UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX };
			for ( size_t i = 0; i < 5; i++ )
			{
				const Tile& there = getTile( candidates[i] );
				if (
					candidates[i] && !(bool)( there.wallType & WallType::WT_MOVEBLOCKING ) )
				{
					pressure[i] = there.pressure * 10 + there.fluidLevel;
				}
			}

			// First order of sampled directions is randomized ...
			const size_t start = seed % 4;
			size_t minIndex    = 4;
			for ( size_t i = 0; i < 4; ++i )
			{
				const size_t j = ( i + start ) % 4;
				if ( pressure[minIndex] > pressure[j] )
				{
					minIndex = j;
				}
			}
			// ... and chance for flow to happen is then proportional to pressure difference
			if ( minIndex != 4 && ( seed % 41 ) < ( pressure[4] - pressure[minIndex] ) )
			{
				here.flow = direction[minIndex];
				drain.append( currentPos );
				flood.append( candidates[minIndex] );
				continue;
			}

			// Only if nothing else worked, flow upwards
			const Tile& upTile = getTile( neighbors.above );
			if (
				neighbors.above && !(bool)( upTile.floorType & FloorType::FT_SOLIDFLOOR ) && !(bool)( upTile.wallType & WallType::WT_MOVEBLOCKING ) && here.fluidLevel == 10 && here.pressure > upTile.pressure + 1 )
			{
				here.flow = WF_UP;
				drain.append( currentPos );
				flood.append( neighbors.above );
				continue;
			}
		}
		else
		{
			// Collecting tiles which should no longer had been tracked
			removedWater.insert( currentPos );
		}
		here.flow = WF_NOFLOW;
	}

	// Batch updates
	QVector<unsigned int> waterUpdates;
	// Expecting to see every tile again
	waterUpdates.reserve( (int)m_water.size() );

	// Flood first
	for ( const auto& pos : flood )
	{
		Tile& here = getTile( pos );
		if ( here.fluidLevel == 0 )
		{
			// Track it, it's probably new
			newWater.insert( pos );
			here.flags += TileFlag::TF_WATER;
		}
		if ( here.fluidLevel < 10 )
		{
			++here.fluidLevel;
			waterUpdates.append( pos );
		}
		else
		{
			++here.pressure;
		}
	}

	// Then apply drain
	for ( const auto& pos : drain )
	{
		Tile& here = getTile( pos );
		if ( here.pressure > 0 )
		{
			--here.pressure;
		}
		else
		{
			--here.fluidLevel;
			waterUpdates.append( pos );
		}
		if ( here.fluidLevel == 0 )
		{
			here.flow = WF_NOFLOW;
			here.flags -= TileFlag::TF_WATER;
			removedWater.insert( pos );
		}
	}

	// Batch submit water tile updates
	addToUpdateList( waterUpdates );
	waterUpdates.clear();

	// Append tracked new water tiles
	for ( const auto& newPos : newWater )
	{
		m_water.insert( newPos );
	}

	// Remove empty tiles
	for ( const auto& oldPos : removedWater )
	{
		auto it = m_water.find( oldPos );
		m_water.erase( it );
	}
}

void World::removeDesignation( Position pos )
{
	Tile& tile = getTile( pos );
	if ( tile.flags & TileFlag::TF_STOCKPILE )
	{
		Global::spm().removeTile( pos );
		addToUpdateList( pos );
	}
	else if ( tile.flags & ( TileFlag::TF_GROVE + TileFlag::TF_FARM + TileFlag::TF_PASTURE ) )
	{
		Global::fm().removeTile( pos, true, true, true );
		addToUpdateList( pos );
	}
	else if ( tile.flags & TileFlag::TF_ROOM )
	{
		Global::rm().removeTile( pos );
		addToUpdateList( pos );
	}
	else if ( tile.flags & TileFlag::TF_NOPASS )
	{
		clearTileFlag( pos, TileFlag::TF_NOPASS );
		m_regionMap.updatePosition( pos );
		addToUpdateList( pos );
	}
}

QPair<unsigned short, unsigned short> World::mineWall( Position pos, Position& workPosition )
{
	Tile& tile                 = getTile( pos );
	unsigned short materialInt = tile.wallMaterial;
	unsigned short embeddedInt = tile.embeddedMaterial;

	if ( tile.wallType == WallType::WT_RAMP )
	{
		Tile& tileAbove          = getTile( pos.aboveOf() );
		tileAbove.floorType      = FloorType::FT_NOFLOOR;
		tileAbove.floorMaterial  = 0;
		tileAbove.floorSpriteUID = 0;
		clearTileFlag( Position( pos.aboveOf() ), TileFlag::TF_WALKABLE );
		removeGrass( pos );
	}

	tile.wallType         = WallType::WT_NOWALL;
	tile.wallMaterial     = 0;
	tile.embeddedMaterial = 0;
	tile.wallSpriteUID    = 0;
	tile.itemSpriteUID    = 0;
	tile.flags += TileFlag::TF_WALKABLE;

	m_regionMap.updatePosition( pos );
	updateLightsInRange( pos );

	updateRampAtPos( pos.northOf() );
	updateRampAtPos( pos.eastOf() );
	updateRampAtPos( pos.southOf() );
	updateRampAtPos( pos.westOf() );

	discover( pos );

	QString ncd = QString::number( pos.toInt() );
	GameState::addChange2( NetworkCommand::CLEARWALLSPRITE, ncd );
	ncd += ";";
	ncd += Util::tile2String( tile );
	GameState::addChange2( NetworkCommand::SETTILEFLAGS, ncd );

	return { materialInt, embeddedInt };
}

QPair<unsigned short, unsigned short> World::removeWall( Position pos, Position& workPosition )
{
	Tile& tile                 = getTile( pos );
	unsigned short materialInt = tile.wallMaterial;
	unsigned short embeddedInt = tile.embeddedMaterial;

	if ( tile.wallType == WallType::WT_RAMP )
	{
		Tile& tileAbove          = getTile( pos.aboveOf() );
		tileAbove.floorType      = FloorType::FT_NOFLOOR;
		tileAbove.floorMaterial  = 0;
		tileAbove.floorSpriteUID = 0;
		clearTileFlag( Position( pos.aboveOf() ), TileFlag::TF_WALKABLE );
		removeGrass( pos );
	}

	tile.wallType         = WallType::WT_NOWALL;
	tile.wallMaterial     = 0;
	tile.embeddedMaterial = 0;
	tile.wallSpriteUID    = 0;
	tile.itemSpriteUID    = 0;
	tile.flags += TileFlag::TF_WALKABLE;

	m_regionMap.updatePosition( pos );
	updateLightsInRange( pos );

	discover( pos );

	QString ncd = QString::number( pos.toInt() );
	GameState::addChange2( NetworkCommand::CLEARWALLSPRITE, ncd );
	ncd += ";";
	ncd += Util::tile2String( tile );
	GameState::addChange2( NetworkCommand::SETTILEFLAGS, ncd );

	return { materialInt, embeddedInt };
}

void World::discover( Position pos )
{
	discover( pos.x, pos.y, pos.z );
	addToUpdateList( pos );
}

void World::discover( int x, int y, int z )
{
	clearTileFlag( Position( x - 1, y - 1, z ), TileFlag::TF_UNDISCOVERED );
	clearTileFlag( Position( x - 1, y, z ), TileFlag::TF_UNDISCOVERED );
	clearTileFlag( Position( x - 1, y + 1, z ), TileFlag::TF_UNDISCOVERED );
	clearTileFlag( Position( x, y - 1, z ), TileFlag::TF_UNDISCOVERED );
	clearTileFlag( Position( x, y, z ), TileFlag::TF_UNDISCOVERED );
	clearTileFlag( Position( x, y + 1, z ), TileFlag::TF_UNDISCOVERED );
	clearTileFlag( Position( x + 1, y - 1, z ), TileFlag::TF_UNDISCOVERED );
	clearTileFlag( Position( x + 1, y, z ), TileFlag::TF_UNDISCOVERED );
	clearTileFlag( Position( x + 1, y + 1, z ), TileFlag::TF_UNDISCOVERED );
}

unsigned short World::removeRamp( Position pos, Position workPosition )
{
	Tile& tile      = getTile( pos );
	Tile& tileAbove = getTile( pos.aboveOf() );

	unsigned short materialInt = tile.wallMaterial;
	// delete current ramp
	tileAbove.floorType      = FloorType::FT_NOFLOOR;
	tileAbove.floorMaterial  = 0;
	tileAbove.floorSpriteUID = 0;

	tile.wallType      = WallType::WT_NOWALL;
	tile.wallMaterial  = 0;
	tile.wallSpriteUID = 0;

	updateLightsInRange( pos );

	removeGrass( pos );

	return materialInt;
}

void World::updateRampAtPos( Position pos )
{
	Tile& tile = getTile( pos );
	if ( !( tile.wallType & WallType::WT_RAMP ) )
	{
		return;
	}
	//	qDebug() << "ramp at " << pos.toString();
	Tile& tileBelow = getTile( pos.belowOf() );
	Tile& tileAbove = getTile( pos.aboveOf() );

	SpriteFactory& sf = Global::sf();

	bool north = getTile( pos.northOf() ).wallType & WallType::WT_ROUGH;
	bool south = getTile( pos.southOf() ).wallType & WallType::WT_ROUGH;
	bool east  = getTile( pos.eastOf() ).wallType & WallType::WT_ROUGH;
	bool west  = getTile( pos.westOf() ).wallType & WallType::WT_ROUGH;

	int sum = (int)north + (int)south + (int)east + (int)west;
	if ( sum > 2 || sum == 0 )
	{
		// delete current ramp
		tileAbove.floorType      = FloorType::FT_NOFLOOR;
		tileAbove.floorMaterial  = 0;
		tileAbove.floorSpriteUID = 0;

		tile.wallType      = WallType::WT_NOWALL;
		tile.wallMaterial  = 0;
		tile.wallSpriteUID = 0;

		updateWalkable( pos );
		updateWalkable( pos.aboveOf() );

		removeGrass( pos );

		return;
	}

	QString mat = DBH::materialSID( tile.wallMaterial );

	tileAbove.floorType = FloorType::FT_RAMPTOP;

	tile.wallType = WallType::WT_RAMP;
	tile.flags += TileFlag::TF_WALKABLE;

	setRampSprites( tile, tileAbove, sum, north, east, south, west, mat );

	updateWalkable( pos );
	updateWalkable( pos.aboveOf() );
}

unsigned short World::removeFloor( Position pos, Position extractTo )
{
	Tile& tile              = getTile( pos );
	tile.floorType          = FloorType::FT_NOFLOOR;
	tile.floorSpriteUID     = 0;
	unsigned short floorMat = tile.floorMaterial;
	tile.floorMaterial      = 0;
	clearTileFlag( pos, TileFlag::TF_WALKABLE );
	m_regionMap.updatePosition( pos );
	removeGrass( pos );
	//Global::inv().gravity( pos );

	if ( tile.flags & TileFlag::TF_SUNLIGHT )
	{
		updateSunlight( pos );
	}

	PositionEntry pe;
	if ( Global::inv().getObjectsAtPosition( pos, pe ) )
	{
		for ( auto i : pe )
		{
			Global::inv().moveItemToPos( i, extractTo );
		}
	}

	if ( m_creaturePositions.contains( pos.toInt() ) )
	{
		Global::gm().forceMoveGnomes( pos, extractTo );
	}
	discover( pos.belowOf() );

	addToUpdateList( pos );

	return floorMat;
}

void World::createRamp( int x, int y, int z, TerrainMaterial mat )
{
	SpriteFactory& os = Global::sf();
	int offset        = z * m_dimX * m_dimY;
	Tile& tileBelow   = m_world[x + y * m_dimX + ( z - 1 ) * m_dimX * m_dimY];
	Tile& tileAbove   = m_world[x + y * m_dimX + ( z + 1 ) * m_dimX * m_dimY];
	Tile& tile        = m_world[x + y * m_dimX + offset];

	if ( tileAbove.floorType == FloorType::FT_SOLIDFLOOR )
		return;
	if ( tileBelow.wallType == WallType::WT_NOWALL )
		return;
	if ( tile.wallType != WallType::WT_NOWALL )
		return;

	unsigned short key = DBH::materialUID( mat.key );

	if ( ( tileBelow.wallType & WallType::WT_ROUGH ) && ( tile.wallType == WallType::WT_NOWALL ) && ( tile.floorType == FloorType::FT_NOFLOOR ) )
	{
		tile.floorType      = FloorType::FT_SOLIDFLOOR;
		tile.floorMaterial  = key;
		tile.floorSpriteUID = os.createSprite( mat.floor, { mat.key } )->uID;
	}

	bool north = m_world[x + ( y - 1 ) * m_dimX + offset].wallType & WallType::WT_ROUGH;
	bool south = m_world[x + ( y + 1 ) * m_dimX + offset].wallType & WallType::WT_ROUGH;
	bool east  = m_world[( x + 1 ) + y * m_dimX + offset].wallType & WallType::WT_ROUGH;
	bool west  = m_world[( x - 1 ) + y * m_dimX + offset].wallType & WallType::WT_ROUGH;

	int sum = (int)north + (int)south + (int)east + (int)west;
	if ( sum > 2 || sum == 0 )
		return;

	tile.wallType     = ( WallType )( WallType::WT_RAMP );
	tile.wallMaterial = key;
	tile.flags += TileFlag::TF_WALKABLE;
	m_regionMap.updatePosition( Position( x, y, z ) );

	tileAbove.floorType     = FloorType::FT_RAMPTOP;
	tileAbove.floorMaterial = key;

	setRampSprites( tile, tileAbove, sum, north, east, south, west, mat.key );
}

void World::createRamp( Position pos )
{
	createRamp( pos.x, pos.y, pos.z );
}

void World::createRamp( int x, int y, int z )
{
	SpriteFactory& os = Global::sf();

	int offset      = z * m_dimX * m_dimY;
	Tile& tileBelow = m_world[x + y * m_dimX + ( z - 1 ) * m_dimX * m_dimY];
	Tile& tileAbove = m_world[x + y * m_dimX + ( z + 1 ) * m_dimX * m_dimY];
	Tile& tile      = m_world[x + y * m_dimX + offset];
	if ( tileAbove.floorType == FloorType::FT_SOLIDFLOOR )
		return;
	if ( tileBelow.wallType == WallType::WT_NOWALL )
		return;
	if ( tile.floorType != FloorType::FT_SOLIDFLOOR )
		return;
	//if( tile.wallType != WallType::WT_NOWALL ) return;
	if ( ( tileBelow.wallType & WallType::WT_ROUGH ) && ( tile.wallType == WallType::WT_NOWALL ) && ( tile.floorType == FloorType::FT_NOFLOOR ) )
	{
		tile.floorType      = FloorType::FT_SOLIDFLOOR;
		tile.floorMaterial  = tileBelow.wallMaterial;
		tile.floorSpriteUID = os.createSprite( DB::select( "FloorSprite", "TerrainMaterials", tileBelow.wallMaterial ).toString(), { DBH::materialSID( tileBelow.wallMaterial ) } )->uID;
	}

	bool north = m_world[x + ( y - 1 ) * m_dimX + offset].wallType & WallType::WT_ROUGH;
	bool south = m_world[x + ( y + 1 ) * m_dimX + offset].wallType & WallType::WT_ROUGH;
	bool east  = m_world[( x + 1 ) + y * m_dimX + offset].wallType & WallType::WT_ROUGH;
	bool west  = m_world[( x - 1 ) + y * m_dimX + offset].wallType & WallType::WT_ROUGH;

	int sum = (int)north + (int)south + (int)east + (int)west;

	if ( sum > 2 || sum == 0 )
	{
		return;
	}
	unsigned int key  = 0;
	unsigned int nMat = m_world[x + ( y - 1 ) * m_dimX + offset].wallMaterial;
	unsigned int sMat = m_world[x + ( y + 1 ) * m_dimX + offset].wallMaterial;
	unsigned int eMat = m_world[( x + 1 ) + y * m_dimX + offset].wallMaterial;
	unsigned int wMat = m_world[( x - 1 ) + y * m_dimX + offset].wallMaterial;

	if ( sum == 1 )
	{
		key = nMat | sMat | eMat | wMat;
	}
	else
	{
		if ( north )
		{
			if ( nMat == sMat || nMat == eMat || nMat == wMat )
				key = nMat;
		}
		if ( south )
		{
			if ( sMat == eMat || sMat == wMat )
				key = sMat;
		}
		if ( east )
		{
			if ( eMat == wMat )
				key = eMat;
		}
	}

	if ( key == 0 )
	{
		return;
	}
	key            = tile.floorMaterial;
	QString matSID = DBH::materialSID( key );

	tile.wallType     = ( WallType )( WallType::WT_RAMP );
	tile.wallMaterial = key;
	tile.flags += TileFlag::TF_WALKABLE;
	m_regionMap.updatePosition( Position( x, y, z ) );

	tileAbove.floorType     = FloorType::FT_RAMPTOP;
	tileAbove.floorMaterial = key;

	setRampSprites( tile, tileAbove, sum, north, east, south, west, matSID );
}

void World::createRamp( Position pos, QString materialSID )
{
	createRamp( pos.x, pos.y, pos.z, materialSID );
}

void World::createRamp( int x, int y, int z, QString materialSID )
{
	SpriteFactory& os = Global::sf();

	int offset      = z * m_dimX * m_dimY;
	Tile& tileBelow = m_world[x + y * m_dimX + ( z - 1 ) * m_dimX * m_dimY];
	Tile& tileAbove = m_world[x + y * m_dimX + ( z + 1 ) * m_dimX * m_dimY];
	Tile& tile      = m_world[x + y * m_dimX + offset];
	if ( tileAbove.floorType == FloorType::FT_SOLIDFLOOR )
		return;
	//if( tileBelow.wallType == WallType::WT_NOWALL ) return;
	if ( tile.floorType != FloorType::FT_SOLIDFLOOR )
		return;
	if ( tile.wallType != WallType::WT_NOWALL )
		return;
	if ( ( tileBelow.wallType & WallType::WT_ROUGH ) && ( tile.wallType == WallType::WT_NOWALL ) && ( tile.floorType == FloorType::FT_NOFLOOR ) )
	{
		tile.floorType      = FloorType::FT_SOLIDFLOOR;
		tile.floorMaterial  = tileBelow.wallMaterial;
		tile.floorSpriteUID = os.createSprite( DB::select( "FloorSprite", "TerrainMaterials", tileBelow.wallMaterial ).toString(), { DBH::materialSID( tileBelow.wallMaterial ) } )->uID;
	}

	bool north = m_world[x + ( y - 1 ) * m_dimX + offset].wallType & WallType::WT_SOLIDWALL;
	bool south = m_world[x + ( y + 1 ) * m_dimX + offset].wallType & WallType::WT_SOLIDWALL;
	bool east  = m_world[( x + 1 ) + y * m_dimX + offset].wallType & WallType::WT_SOLIDWALL;
	bool west  = m_world[( x - 1 ) + y * m_dimX + offset].wallType & WallType::WT_SOLIDWALL;

	int sum = (int)north + (int)south + (int)east + (int)west;

	if ( sum > 2 || sum == 0 )
	{
		return;
	}
	unsigned int key = DBH::materialUID( materialSID );

	tile.wallType     = ( WallType )( WallType::WT_RAMP );
	tile.wallMaterial = key;
	tile.flags += TileFlag::TF_WALKABLE;
	removeGrass( Position( x, y, z ) );

	m_regionMap.updatePosition( Position( x, y, z ) );

	tileAbove.floorType     = FloorType::FT_RAMPTOP;
	tileAbove.floorMaterial = key;

	setRampSprites( tile, tileAbove, sum, north, east, south, west, materialSID );
}

void World::setRampSprites( Tile& tile, Tile& tileAbove, int sum, bool north, bool east, bool south, bool west, QString materialSID )
{
	SpriteFactory& sf      = Global::sf();
	unsigned int ramp      = sf.createSprite( "Ramp", { materialSID } )->uID;
	unsigned int top       = sf.createSprite( "RampTop", { materialSID } )->uID;
	unsigned int corner    = sf.createSprite( "CornerRamp", { materialSID } )->uID;
	unsigned int cornerTop = sf.createSprite( "CornerRampTop", { materialSID } )->uID;
	unsigned int uRamp     = sf.createSprite( "URamp", { materialSID } )->uID;
	unsigned int uRampTop  = sf.createSprite( "URampTop", { materialSID } )->uID;

	if ( tile.flags & TileFlag::TF_GRASS )
	{
		tile.floorSpriteUID = sf.createSprite( "RoughFloor", { "Dirt" } )->uID;
		ramp                = sf.createSprite( "GrassSoilRamp", { "Dirt", "Grass" } )->uID;
		top                 = sf.createSprite( "GrassSoilRampTop", { "Dirt", "Grass" } )->uID;
		corner              = sf.createSprite( "GrassSoilCornerRamp", { "Dirt", "Grass" } )->uID;
		cornerTop           = sf.createSprite( "GrassSoilCornerRampTop", { "Dirt", "Grass" } )->uID;
		uRamp               = sf.createSprite( "GrassSoilURamp", { "Dirt", "Grass" } )->uID;
		uRampTop            = sf.createSprite( "GrassSoilURampTop", { "Dirt", "Grass" } )->uID;
		if ( tile.flags & TileFlag::TF_SUNLIGHT )
		{
			tileAbove.flags += TileFlag::TF_SUNLIGHT;
		}
	}

	if ( tile.flags & TileFlag::TF_BIOME_MUSHROOM )
	{
		//floorSpriteUID = sf.createSprite( "RoughFloor", { "Dirt" } )->uID;
		ramp      = sf.createSprite( "MushroomGrassSoilRamp", { "Dirt", "Grass" } )->uID;
		top       = sf.createSprite( "MushroomGrassSoilRampTop", { "Dirt", "Grass" } )->uID;
		corner    = sf.createSprite( "MushroomGrassSoilCornerRamp", { "Dirt", "Grass" } )->uID;
		cornerTop = sf.createSprite( "MushroomGrassSoilCornerRampTop", { "Dirt", "Grass" } )->uID;
		uRamp     = sf.createSprite( "MushroomGrassSoilURamp", { "Dirt", "Grass" } )->uID;
		uRampTop  = sf.createSprite( "MushroomGrassSoilURampTop", { "Dirt", "Grass" } )->uID;
	}

	if ( sum == 1 )
	{
		if ( north )
		{
			tile.wallSpriteUID       = ramp;
			tileAbove.floorSpriteUID = top;
			tile.wallRotation        = 1;
			tileAbove.floorRotation  = 1;
		}
		else if ( west )
		{
			tile.wallSpriteUID       = ramp;
			tileAbove.floorSpriteUID = top;
			tile.wallRotation        = 0;
			tileAbove.floorRotation  = 0;
		}
		else if ( east )
		{
			tile.wallSpriteUID       = ramp;
			tileAbove.floorSpriteUID = top;
			tile.wallRotation        = 2;
			tileAbove.floorRotation  = 2;
		}
		else // south
		{
			tile.wallSpriteUID       = ramp;
			tileAbove.floorSpriteUID = top;
			tile.wallRotation        = 3;
			tileAbove.floorRotation  = 3;
		}
	}
	else
	{
		if ( north && west )
		{
			tile.wallSpriteUID       = corner;
			tileAbove.floorSpriteUID = cornerTop;
			tile.wallRotation        = 0;
			tileAbove.floorRotation  = 0;
		}
		else if ( north && east )
		{
			tile.wallSpriteUID       = corner;
			tileAbove.floorSpriteUID = cornerTop;
			tile.wallRotation        = 1;
			tileAbove.floorRotation  = 1;
		}
		else if ( south && west )
		{
			tile.wallSpriteUID       = corner;
			tileAbove.floorSpriteUID = cornerTop;
			tile.wallRotation        = 3;
			tileAbove.floorRotation  = 3;
		}
		else if ( south && east )
		{
			tile.wallSpriteUID       = corner;
			tileAbove.floorSpriteUID = cornerTop;
			tile.wallRotation        = 2;
			tileAbove.floorRotation  = 2;
		}
		else if ( north && south )
		{
			tile.wallSpriteUID       = uRamp;
			tileAbove.floorSpriteUID = uRampTop;
			tile.wallRotation        = 0;
			tileAbove.floorRotation  = 0;
		}
		else if ( west && east )
		{
			tile.wallSpriteUID       = uRamp;
			tileAbove.floorSpriteUID = uRampTop;
			tile.wallRotation        = 1;
			tileAbove.floorRotation  = 1;
		}
	}
}

void World::createRampOuterCorners( int x, int y, int z )
{
	int offset = z * m_dimX * m_dimY;
	Tile& tile = m_world[x + y * m_dimX + offset];

	if ( tile.wallType != WallType::WT_NOWALL )
		return;

	bool north = m_world[x + ( y - 1 ) * m_dimX + offset].wallType & WallType::WT_RAMP;
	bool south = m_world[x + ( y + 1 ) * m_dimX + offset].wallType & WallType::WT_RAMP;
	bool east  = m_world[( x + 1 ) + y * m_dimX + offset].wallType & WallType::WT_RAMP;
	bool west  = m_world[( x - 1 ) + y * m_dimX + offset].wallType & WallType::WT_RAMP;

	int sum = (int)north + (int)south + (int)east + (int)west;

	if ( sum > 2 || sum == 0 )
	{
		return;
	}

	unsigned int key = tile.floorMaterial;

	QString matSID = DBH::materialSID( key );

	unsigned int corner = Global::sf().createSprite( "OuterCornerRamp", { matSID } )->uID;

	if ( tile.flags & TileFlag::TF_GRASS )
	{
		matSID = "Grass";
		corner = Global::sf().createSprite( "GrassOuterCornerRamp", { matSID } )->uID;
	}

	tile.wallType     = ( WallType )( WallType::WT_RAMPCORNER );
	tile.wallMaterial = tile.floorMaterial;

	if ( north && west )
	{
		tile.wallSpriteUID = corner;
		tile.wallRotation  = 0;
	}
	if ( north && east )
	{
		tile.wallSpriteUID = corner;

		tile.wallRotation = 1;
	}
	if ( south && west )
	{
		tile.wallSpriteUID = corner;
		tile.wallRotation  = 3;
	}
	if ( south && east )
	{
		tile.wallSpriteUID = corner;
		tile.wallRotation  = 2;
	}
}

bool World::setNetworkMove( unsigned int id, Position newPos, int facing )
{
	/* TODO
	if( Global::cm().animals().contains( id ) )
	{
		Global::cm().animals()[id].setNetworkMove( newPos, facing );
		return true;
	}
	*/
	return false;
}

void World::addToUpdateList( const unsigned int uID )
{
	QMutexLocker lock( &m_updateMutex );
	m_updatedTiles.insert( uID );
}

void World::addToUpdateList( Position pos )
{
	auto tileID = pos.toInt();

	QMutexLocker lock( &m_updateMutex );
	m_updatedTiles.insert( tileID );
}

void World::addToUpdateList( const unsigned short x, const unsigned short y, const unsigned short z )
{
	auto tileID = Position( x, y, z ).toInt();

	QMutexLocker lock( &m_updateMutex );
	m_updatedTiles.insert( tileID );
}

void World::addToUpdateList( const QVector<unsigned int>& ul )
{
	QMutexLocker lock( &m_updateMutex );
	for ( auto tileID : ul )
	{
		m_updatedTiles.insert( tileID );
	}
}

void World::addToUpdateList( const QSet<unsigned int>& ul )
{
	QMutexLocker lock( &m_updateMutex );
	m_updatedTiles += ul;
}

void World::setDoorLocked( unsigned int tileUID, bool lockGnome, bool lockMonster, bool lockAnimal )
{
	Position pos( tileUID );

	if ( lockGnome )
	{
		clearTileFlag( pos, TileFlag::TF_WALKABLE );
	}
	else
	{
		setTileFlag( pos, TileFlag::TF_WALKABLE );
	}
	if ( lockMonster )
	{
		clearTileFlag( pos, TileFlag::TF_WALKABLEMONSTERS );
	}
	else
	{
		setTileFlag( pos, TileFlag::TF_WALKABLEMONSTERS );
	}
	if ( lockAnimal )
	{
		clearTileFlag( pos, TileFlag::TF_WALKABLEANIMALS );
	}
	else
	{
		setTileFlag( pos, TileFlag::TF_WALKABLEANIMALS );
	}
	m_regionMap.updateConnectedRegions( pos );
}

void World::putLight( Position pos, int light )
{
	Tile& tile = getTile( pos );
	tile.lightLevel += light;
	addToUpdateList( pos );
}

void World::putLight( const unsigned short x, const unsigned short y, const unsigned short z, int light )
{
	Tile& tile = getTile( x, y, z );
	tile.lightLevel += light;
	addToUpdateList( x, y, z );
}

void World::addLight( unsigned int id, Position pos, int intensity )
{
	QSet<unsigned int> ul;
	m_lightMap.addLight( ul, m_world, id, pos, intensity );
	addToUpdateList( ul );
}

void World::removeLight( unsigned int id )
{
	QSet<unsigned int> ul;
	m_lightMap.removeLight( ul, m_world, id );
	addToUpdateList( ul );
}

void World::moveLight( unsigned int id, Position pos, int intensity )
{
	QSet<unsigned int> ul;
	m_lightMap.removeLight( ul, m_world, id );
	m_lightMap.addLight( ul, m_world, id, pos, intensity );
	addToUpdateList( ul );
}

void World::updateLightsInRange( Position pos )
{
	QSet<unsigned int> ul;
	m_lightMap.updateLight( ul, m_world, pos );
	addToUpdateList( ul );
}

void World::updateSunlight( Position pos )
{
	Tile& tile = getTile( pos );
	tile.flags += TileFlag::TF_SUNLIGHT;
	addToUpdateList( pos );
	if ( pos.z > 0 && tile.floorType == FT_NOFLOOR )
	{
		updateSunlight( pos.belowOf() );
	}
}

bool World::hasSunlight( Position pos )
{
	Tile& tile = getTile( pos );
	return tile.flags & TileFlag::TF_SUNLIGHT;
}

bool World::hasGrass( Position pos )
{
	Tile& tile = getTile( pos );
	return tile.flags & TileFlag::TF_GRASS;
}

bool World::hasMaxGrass( Position pos )
{
	Tile& tile = getTile( pos );
	return tile.flags & TileFlag::TF_GRASS && tile.flags & TileFlag::TF_WALKABLE && tile.vegetationLevel == 100; //TODO bad fix, making tiles not walkable should remove designations
}

bool World::checkTrapGnomeFloor( Position pos, Position workPos )
{
	// TODO just realized this function is garbage, needs to be fixed
	// check other 3 neighbors of pos
	Position checkPos = pos.northOf();
	if ( checkPos != workPos )
	{
		// checkPos only has one walkable neighbor which is the tile we want to remove
		if ( walkableNeighbors( checkPos ) == 1 )
		{
			// checkIf Gnome is on tile
			if ( isWalkable( pos.eastOf() ) && Global::gm().gnomesAtPosition( pos.eastOf() ).size() > 0 )
				return true;
			if ( isWalkable( pos.westOf() ) && Global::gm().gnomesAtPosition( pos.eastOf() ).size() > 0 )
				return true;
			if ( isWalkable( pos.southOf() ) && Global::gm().gnomesAtPosition( pos.eastOf() ).size() > 0 )
				return true;
			if ( isWalkable( pos.northOf() ) && Global::gm().gnomesAtPosition( pos.eastOf() ).size() > 0 )
				return true;
		}
	}
	checkPos = pos.eastOf();
	if ( checkPos != workPos )
	{
		// checkPos only has one walkable neighbor which is the tile we want to remove
		if ( walkableNeighbors( checkPos ) == 1 )
		{
			// checkIf Gnome is on tile
			if ( isWalkable( pos.eastOf() ) && Global::gm().gnomesAtPosition( pos.eastOf() ).size() > 0 )
				return true;
			if ( isWalkable( pos.westOf() ) && Global::gm().gnomesAtPosition( pos.eastOf() ).size() > 0 )
				return true;
			if ( isWalkable( pos.southOf() ) && Global::gm().gnomesAtPosition( pos.eastOf() ).size() > 0 )
				return true;
			if ( isWalkable( pos.northOf() ) && Global::gm().gnomesAtPosition( pos.eastOf() ).size() > 0 )
				return true;
		}
	}
	checkPos = pos.southOf();
	if ( checkPos != workPos )
	{
		// checkPos only has one walkable neighbor which is the tile we want to remove
		if ( walkableNeighbors( checkPos ) == 1 )
		{
			// checkIf Gnome is on tile
			if ( isWalkable( pos.eastOf() ) && Global::gm().gnomesAtPosition( pos.eastOf() ).size() > 0 )
				return true;
			if ( isWalkable( pos.westOf() ) && Global::gm().gnomesAtPosition( pos.eastOf() ).size() > 0 )
				return true;
			if ( isWalkable( pos.southOf() ) && Global::gm().gnomesAtPosition( pos.eastOf() ).size() > 0 )
				return true;
			if ( isWalkable( pos.northOf() ) && Global::gm().gnomesAtPosition( pos.eastOf() ).size() > 0 )
				return true;
		}
	}
	checkPos = pos.westOf();
	if ( checkPos != workPos )
	{
		// checkPos only has one walkable neighbor which is the tile we want to remove
		if ( walkableNeighbors( checkPos ) == 1 )
		{
			// checkIf Gnome is on tile
			if ( isWalkable( pos.eastOf() ) && Global::gm().gnomesAtPosition( pos.eastOf() ).size() > 0 )
				return true;
			if ( isWalkable( pos.westOf() ) && Global::gm().gnomesAtPosition( pos.eastOf() ).size() > 0 )
				return true;
			if ( isWalkable( pos.southOf() ) && Global::gm().gnomesAtPosition( pos.eastOf() ).size() > 0 )
				return true;
			if ( isWalkable( pos.northOf() ) && Global::gm().gnomesAtPosition( pos.eastOf() ).size() > 0 )
				return true;
		}
	}

	return false;
}

void World::setWallSpriteAnim( Position pos, bool anim )
{
	Tile& tile = getTile( pos );
	if ( anim )
	{
		tile.wallSpriteUID |= 0x00040000;
	}
	else
	{
		tile.wallSpriteUID &= 0xFFFBFFFF;
	}
	addToUpdateList( pos );
}
