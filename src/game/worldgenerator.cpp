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
#include "worldgenerator.h"

#include "../base/config.h"
#include "../base/db.h"
#include "../base/enums.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/position.h"
#include "../game/game.h"
#include "../game/creaturefactory.h"
#include "../game/creaturemanager.h"
#include "../game/gnomemanager.h"
#include "../game/inventory.h"
#include "../game/itemhistory.h"
#include "../game/newgamesettings.h"
#include "../game/plant.h"
#include "../game/world.h"
#include "../gfx/sprite.h"
#include "../gfx/spritefactory.h"

#include <QApplication>
#include <QDebug>
#include <QElapsedTimer>

WorldGenerator::WorldGenerator( NewGameSettings* newGameSettings, Game* parent ) :
	g( parent ),
	ngs( newGameSettings ),
	QObject(parent)
{
}

WorldGenerator::~WorldGenerator()
{
}

World* WorldGenerator::generateTopology()
{
	m_dimX       = ngs->worldSize();
	m_dimY       = ngs->worldSize();
	m_dimZ       = ngs->zLevels();
	Global::dimX = ngs->worldSize();
	Global::dimY = ngs->worldSize();
	Global::dimZ = ngs->zLevels();

	w = new World( m_dimX, m_dimY, m_dimZ, g );

	qDebug() << "creating world with size" << m_dimX << m_dimY << m_dimZ;

	m_groundLevel = ngs->ground();
	m_fow         = Global::cfg->get( "fow" ).toBool();

	auto& world = w->world();
	world.resize( m_dimX * m_dimY * m_dimZ, Tile() );

	for ( auto& tile : world )
	{
		tile.fluidLevel = 0;
		tile.pressure   = 0;
	}

	m_mushroomLevel = m_dimZ - 70;

	m_seed = ngs->seed().toInt();
	m_random.SetSeed( m_seed );
	m_random.SetFrequency( (FN_DECIMAL)0.02 );
	m_random.SetFractalOctaves( 1 );
	m_random.SetFractalLacunarity( 2 );
	m_random.SetFractalGain( 0.5 );
	m_random.SetNoiseType( FastNoise::NoiseType::CubicFractal );
	m_random.SetFractalType( FastNoise::FractalType::Billow );

	emit signalStatus( "Create height map." );
	createHeightMap( m_dimX, m_dimY );
	initMateralVectors();

	// resize world and fill stone layers
	emit signalStatus( "Set stone layers." );
	setStoneLayers();

	if ( ngs->rivers() > 0 )
	{
		createRivers();
	}

	if ( ngs->oceanSize() > 0 )
	{
		emit signalStatus( "Create ocean front." );
		createOceanFront();
	}

	// set metal ores and gems
	emit signalStatus( "Create ore and gem veins." );
	setMetalsAndGems();
	// set water and sand floor at water
	emit signalStatus( "Set water." );
	setWater();

	// set sunlight and grass
	emit signalStatus( "Init sun light." );
	initSunLight();

	emit signalStatus( "Create ramps." );
	createRamps();

	if ( !ngs->isPeaceful() )
	{
		//emit signalStatus( "Place Lairs." );
		//placeLairs();
	}

	emit signalStatus( "Init world." );
	w->init();

	discoverAll();
	qDebug() << "world generator - topology - done";
	return w;
}

void WorldGenerator::addLife()
{
	// add plants and trees
	emit signalStatus( "Add plants and trees." );
	addPlantsAndTrees();
	// add animals
	emit signalStatus( "Add animals." );
	addAnimals();
	// add gnomes and starting items
	emit signalStatus( "Place embark items and gnomes." );
	addGnomesAndStartingItems();

	int maxVeinLength = m_dimX / 2;

	GameState::kingdomName = ngs->kingdomName();

	qDebug() << "world generator - life - done";

	w->regionMap().initRegions();
}

void WorldGenerator::initMateralVectors()
{
	m_mats.clear();
	QList<QString> ids = DB::ids( "TerrainMaterials" );

	for ( auto id : ids )
	{
		QVariantMap row = DB::selectRow( "TerrainMaterials", id );
		TerrainMaterial m;
		m.key       = id;
		m.rowid     = DBH::materialUID( id );
		m.type      = row["Type"].toString();
		m.wall      = row["WallSprite"].toString();
		m.shortwall = row["ShortWallSprite"].toString();
		m.floor     = row["FloorSprite"].toString();
		m.lowest    = row["Lowest"].toInt();
		m.highest   = row["Highest"].toInt();
		m_mats.push_back( m );
	}
	m_matsInLevel.clear();
	m_matsInLevel.resize( m_dimZ );
	TerrainMaterial mat;

	for ( int z = 0; z < m_dimZ; ++z )
	{
		for ( int i = 0; i < m_mats.size(); ++i )
		{
			int levelUnderGround = z - m_groundLevel;
			m_matsInLevel[z]     = i;
			mat                  = m_mats[i];
			if ( levelUnderGround >= mat.lowest && levelUnderGround <= mat.highest )
			{
				break;
			}
		}
	}
}

void WorldGenerator::setStoneLayers()
{
	QElapsedTimer timer;
	timer.start();
	for ( int z = 0; z < m_dimZ; ++z )
	{
		if ( timer.elapsed() > 3000 )
		{
			emit signalStatus( "Set stone layers..." + QString::number( (int)( ( 100. / (float)m_dimZ ) * z ) ) + "%" );
			timer.restart();
		}
		fillFloor( z, m_mats, m_matsInLevel );
		QCoreApplication::processEvents();
	}

	for ( int z = 0; z < 7; ++z )
	{
		if ( timer.elapsed() > 3000 )
		{
			emit signalStatus( "Set stone layers..." + QString::number( (int)( ( 100. / (float)m_dimZ ) * z ) ) + "%" );
			timer.restart();
		}
		fillFloorMushroomBiome( z, m_mats, m_matsInLevel );
		QCoreApplication::processEvents();
	}
}

// set metal ores and gems
void WorldGenerator::setMetalsAndGems()
{
	QMap<QString, EmbeddedMaterial> embeddeds;
	QList<QString> eids = DB::ids( "EmbeddedMaterials" );

	for ( auto id : eids )
	{
		QVariantMap row = DB::selectRow( "EmbeddedMaterials", id );
		EmbeddedMaterial m;
		m.key     = id;
		m.rowid   = DBH::materialUID( id );
		m.type    = row["Type"].toString();
		m.wall    = row["WallSprite"].toString();
		m.lowest  = row["Lowest"].toInt();
		m.highest = row["Highest"].toInt();
		embeddeds.insert( id, m );
	}

	int maxVeinLength = m_dimX / 4;
	auto& world       = w->world();

	for ( int z = 0; z < m_groundLevel; ++z )
	{
		for ( int i = 0; i < 20; ++i )
		{
			std::vector<Position> pw = perlinWorm( z, i, maxVeinLength );

			if ( !pw.empty() )
			{
				Position pos     = pw[0];
				QString embedded = getRandomEmbedded( pos.x, pos.y, pos.z, embeddeds );
				if ( !embedded.isEmpty() )
				{
					int rowid = DBH::materialUID( embedded );
					for ( auto pos : pw )
					{
						unsigned short spriteUID = g->sf()->createSprite( embeddeds[embedded].wall, { embedded } )->uID;

						setEmbedded3x3( world, pos, rowid, spriteUID );
						//clear3x3( world, pos );
					}
				}
			}
		}
	}
}
// set water and sand floor at water
void WorldGenerator::setWater()
{
}
// set sunlight and grass
void WorldGenerator::initSunLight()
{
	auto& world       = w->world();

	int sandRowid = DBH::materialUID( "Sand" );
	int dirtRowid = DBH::materialUID( "Dirt" );

	for ( int y = 1; y < m_dimY - 1; ++y )
	{
		for ( int x = 1; x < m_dimX - 1; ++x )
		{
			Position pos( x, y, m_dimZ - 1 );
			w->getFloorLevelBelow( pos, true );

			Tile& tile = world[x + y * m_dimX + pos.z * m_dimX * m_dimY];

			if ( ( tile.flags & TileFlag::TF_WATER ) && tile.floorType == FloorType::FT_SOLIDFLOOR )
			{
				tile.floorMaterial = sandRowid;
				tile.wallMaterial  = sandRowid;
				tile.flags += TileFlag::TF_WALKABLE;
				tile.flags += TileFlag::TF_SUNLIGHT;
				tile.floorSpriteUID = g->sf()->createSprite( "RoughFloor", { "Sand" } )->uID;
			}
			else
			{
				tile.floorType     = FloorType::FT_SOLIDFLOOR;
				tile.floorMaterial = dirtRowid;
				tile.wallType      = WallType::WT_NOWALL;
				tile.wallMaterial  = 0;
				tile.flags += TileFlag::TF_WALKABLE;

				//tile.flags |= TileFlag::TF_GRASS;
				tile.flags += TileFlag::TF_SUNLIGHT;

				w->createGrass( pos );
			}
		}
	}

	w->initGrassUpdateList();
}
// add plants and trees
void WorldGenerator::addPlantsAndTrees()
{
	QStringList allTrees     = DB::ids( "Plants", "Type", "Tree" );
	QStringList allMushrooms = DB::ids( "Plants", "Type", "Mushroom" );
	QStringList largeShrooms;
	QStringList smallShrooms;

	for ( auto shroom : allMushrooms )
	{
		if ( DB::select( "IsLarge", "Plants", shroom ).toBool() )
		{
			largeShrooms.append( shroom );
		}
		else
		{
			smallShrooms.append( shroom );
		}
	}

	QStringList trees;

	for ( auto tree : allTrees )
	{
		if ( ngs->isChecked( tree ) )
		{
			trees.push_back( tree );
		}
	}

	QStringList allplants = DB::ids( "Plants", "Type", "Plant" );
	QStringList plants;
	for ( auto plant : allplants )
	{
		if ( DB::select( "AllowInWild", "Plants", plant ).toBool() && ngs->isChecked( plant ) )
		{
			plants.push_back( plant );
		}
	}

	int x = m_dimX / 2;
	int y = m_dimY / 2;

	int treeDensity      = ngs->treeDensity();
	int plantDensity     = ngs->plantDensity();
	int startingZoneSize = ngs->startZone();

	bool shroomRejected = false;

	if ( treeDensity > 0 )
	{
		treeDensity = 101 - treeDensity;
		for ( int x_ = 3; x_ < m_dimX - 4; ++x_ )
		{
			for ( int y_ = 3; y_ < m_dimY - 4; ++y_ )
			{
				Position pos( x_, y_, m_mushroomLevel + 5 );
				w->getFloorLevelBelow( pos, false );

				int random = rand();
				if ( random % treeDensity == 0 || shroomRejected )
				{
					int ra = rand() % largeShrooms.size();
					if ( w->getTile( pos ).wallType == WallType::WT_NOWALL && Plant::testLayoutMulti( largeShrooms[ra], pos, g ) && !( w->getTileFlag( pos ) & TileFlag::TF_WATER ) )
					{
						w->plantMushroom( pos, largeShrooms[ra], true );
						shroomRejected = false;
					}
					else
					{
						shroomRejected = true;
					}
				}
			}
		}
		for ( int x_ = 3; x_ < m_dimX - 4; ++x_ )
		{
			for ( int y_ = 3; y_ < m_dimY - 4; ++y_ )
			{
				Position pos( x_, y_, m_mushroomLevel + 5 );
				w->getFloorLevelBelow( pos, false );

				int random = rand();
				if ( random % treeDensity == 0 || shroomRejected )
				{
					int ra = rand() % smallShrooms.size();
					if ( w->getTile( pos ).wallType == WallType::WT_NOWALL && w->noShroom( pos, 2, 2 ) && !( w->getTileFlag( pos ) & TileFlag::TF_WATER ) )
					{
						w->plantMushroom( pos, smallShrooms[ra], true );
						shroomRejected = false;
					}
					else
					{
						shroomRejected = true;
					}
				}
			}
		}
	}

	bool treeRejected = false;

	if ( treeDensity > 0 )
	{
		qDebug() << "tree density " << treeDensity;
		//treeDensity = 101 - treeDensity;

		for ( int x_ = 2; x_ < m_dimX - 2; ++x_ )
		{
			for ( int y_ = 2; y_ < m_dimY - 2; ++y_ )
			{
				if ( sqrt( ( x_ - x ) * ( x_ - x ) + ( y_ - y ) * ( y_ - y ) ) < startingZoneSize )
				{
					continue;
				}

				Position pos( x_, y_, m_dimZ - 2 );
				w->getFloorLevelBelow( pos, false );

				int random = rand();
				if ( random % treeDensity == 0 || treeRejected )
				{
					if ( trees.size() )
					{
						int ra = rand() % trees.size();
						if ( w->getTile( pos ).wallType == WallType::WT_NOWALL && Plant::testLayoutMulti( trees[ra], pos, g ) && !( w->getTileFlag( pos ) & TileFlag::TF_WATER ) )
						{
							w->plantTree( pos, trees[ra], true );
							treeRejected = false;
						}
						else
						{
							treeRejected = true;
						}
					}
				}
			}
		}
	}
	if ( plantDensity > 0 )
	{
		plantDensity = 101 - plantDensity;
		for ( int x_ = 1; x_ < m_dimX - 1; ++x_ )
		{
			for ( int y_ = 1; y_ < m_dimY - 1; ++y_ )
			{
				if ( sqrt( ( x_ - x ) * ( x_ - x ) + ( y_ - y ) * ( y_ - y ) ) < startingZoneSize )
				{
					continue;
				}

				Position pos( x_, y_, m_dimZ - 2 );
				w->getFloorLevelBelow( pos, false );

				int random = rand();
				if ( random % plantDensity == 0 )
				{
					if ( plants.size() )
					{
						int ra = rand() % plants.size();
						if ( w->getTile( pos ).wallType == WallType::WT_NOWALL && w->noTree( pos, 0, 0 ) && !( w->getTileFlag( pos ) & TileFlag::TF_WATER ) )
						{
							Plant plant( pos, plants[ra], true, g );
							w->plants().insert( plant.getPos().toInt(), plant );
						}
					}
				}
			}
		}
	}
}
// add animals
void WorldGenerator::addAnimals()
{
	int numAnimals = ngs->numWildAnimals();
	QStringList keys;
	QStringList waterKeys;
	QStringList mushroomKeys;
	auto ids = DB::ids( "Animals" );
	for ( auto id : ids )
	{
		if ( ngs->isChecked( id ) )
		{
			if ( DB::select( "Aquatic", "Animals", id ).toBool() )
			{
				waterKeys.push_back( id );
			}
			/*
			else if( ta.select( key, "Biome" ).toString() == "Mushroom" )
			{
				mushroomKeys.push_back( key );
			}
			*/
			else
			{
				if ( DB::select( "Biome", "Animals", id ).toString() != "Mushroom" )
				{
					keys.push_back( id );
				}
			}
		}
	}
	for ( auto id : ids )
	{
		if ( DB::select( "Biome", "Animals", id ).toString() == "Mushroom" )
		{
			mushroomKeys.push_back( id );
		}
	}

	int numTypes      = keys.size();
	int numWaterTypes = waterKeys.size();

	int x_               = m_dimX / 2;
	int y_               = m_dimY / 2;
	int startingZoneSize = ngs->startZone();

	QMap<QString, int> countPerType;

	if ( numTypes > 0 )
	{
		int i         = 0;
		int numWolves = 0;
		int numFoxes  = 0;
		while ( i < numAnimals )
		{
			int x = qMax( 2, ( rand() % m_dimX ) - 2 );
			int y = qMax( 2, ( rand() % m_dimY ) - 2 );

			Position pos( x, y, m_dimZ - 2 );
			w->getFloorLevelBelow( pos, false );

			if ( !w->isWalkable( pos ) || ( sqrt( ( x_ - x ) * ( x_ - x ) + ( y_ - y ) * ( y_ - y ) ) < startingZoneSize ) )
			{
				continue;
			}
			else
			{
				if ( ( w->getTileFlag( pos ) & TileFlag::TF_WATER ) && w->fluidLevel( pos ) > 6 )
				{
					if ( numWaterTypes > 0 )
					{
						int randomType = rand() % ( numWaterTypes );
						QString type   = waterKeys[randomType];

						int count = countPerType.value( type );
						if ( count < ngs->globalMaxPerType() && count < ngs->maxAnimalsPerType( type ) )
						{
							g->cm()->addCreature( CreatureType::ANIMAL, type, pos, rand() % 2 == 0 ? Gender::MALE : Gender::FEMALE, true, false );
							countPerType.insert( type, count + 1 );
						}
					}
				}
				else
				{
					int randomType = rand() % ( numTypes );
					QString type   = keys[randomType];

					int count = countPerType.value( type );

					if ( count < ngs->globalMaxPerType() && count < ngs->maxAnimalsPerType( type ) )
					{
						g->cm()->addCreature( CreatureType::ANIMAL, type, pos, rand() % 2 == 0 ? Gender::MALE : Gender::FEMALE, true, false );
						countPerType.insert( type, count + 1 );
					}
				}
			}

			++i;
		}
	}

	for ( int x_ = 3; x_ < m_dimX - 3; ++x_ )
	{
		for ( int y_ = 3; y_ < m_dimY - 3; ++y_ )
		{
			Position pos( x_, y_, m_mushroomLevel + 5 );
			w->getFloorLevelBelow( pos, false );

			int random = rand();
			if ( random % 100 > 96 )
			{
				if ( w->isWalkable( pos ) )
				{
					int randomType = rand() % ( mushroomKeys.size() );
					QString type   = mushroomKeys[randomType];

					g->cm()->addCreature( CreatureType::ANIMAL, type, pos, rand() % 2 == 0 ? Gender::MALE : Gender::FEMALE, true, false );
				}
			}
		}
	}
	/*
	for( int i = 0; i < 35000; ++i )
	{
		g->cm()->addAnimal( "Badger", Position( 20, 20, 100), Gender::MALE, true, false );
	}
	*/
}
// add gnomes and starting items
void WorldGenerator::addGnomesAndStartingItems()
{
	int num = ngs->numGnomes();

	QVector<Position> offsets;
	offsets.push_back( Position( 0, 0, 0 ) );
	offsets.push_back( Position( -1, 0, 0 ) );
	offsets.push_back( Position( 1, 0, 0 ) );
	offsets.push_back( Position( -1, -1, 0 ) );
	offsets.push_back( Position( 0, -1, 0 ) );
	offsets.push_back( Position( 1, -1, 0 ) );
	offsets.push_back( Position( -1, 1, 0 ) );
	offsets.push_back( Position( 0, 1, 0 ) );
	offsets.push_back( Position( 1, 1, 0 ) );

	int x = m_dimX / 2;
	int y = m_dimY / 2;
	Position pos( x, y, m_dimZ - 2 );

	bool found = false;
	int size   = 0;
	while ( !found && size < m_dimX / 2 - 5 )
	{
		for ( int x = -size; x < size; ++x )
		{
			for ( int y = -size; y < size; ++y )
			{
				found             = true;
				Position checkPos = pos;
				checkPos.x        = pos.x + x;
				checkPos.y        = pos.y + y;
				for ( int i = 0; i < 9; ++i )
				{
					Position thisPos( checkPos + offsets[i % 9] );
					w->getFloorLevelBelow( thisPos, false );
					Tile& tile = w->getTile( thisPos );
					if ( tile.fluidLevel > 0 )
					{
						found = false;
					}
				}
				if ( found )
				{
					pos = checkPos;
					break;
				}
			}
			if ( found )
			{
				break;
			}
		}
		size += 1;
	}

	pos.z = m_dimZ - 2;
	//w->getFloorLevelBelow( pos, false );
	GameState::origin = pos;

	for ( int i = 0; i < num; ++i )
	{
		Position thisPos( pos + offsets[i % 6] );
		w->getFloorLevelBelow( thisPos, false );
		g->gm()->addGnome( thisPos );
		GameState::viewLevel = thisPos.z + 5;
	}

	int id   = 0;
	auto sal = ngs->startingAnimals();

	for ( auto sa : sal )
	{
		int amount = sa.amount;
		Position thisPos( pos + offsets[( id % 3 ) + 6] );
		w->getFloorLevelBelow( thisPos, false );

		for ( int i = 0; i < amount; ++i )
		{
			g->cm()->addCreature( CreatureType::ANIMAL, sa.type, thisPos, sa.gender == "Male" ? Gender::MALE : Gender::FEMALE, true, true );
		}
	}

	auto sil = ngs->startingItems();
	for ( auto si : sil )
	{
		if ( !si.mat2.isEmpty() ) //( type == "CombinedItem" )
		{
			//qDebug() << "create combined item " << entry.value( "ItemID" ).toString() << entry.value( "Components" );
			int amount = si.amount;
			Position thisPos( pos + offsets[( id % 3 ) + 6] );
			w->getFloorLevelBelow( thisPos, false );

			QVariantMap row = DB::selectRow( "Items", si.itemSID );

			if ( DB::numRows( "Items_Components", si.itemSID ) > 1 )
			{
				auto comps = DB::selectRows( "Items_Components", si.itemSID );

				QString itemSID1 = comps.first().value( "ItemID" ).toString();
				QString itemSID2 = comps.last().value( "ItemID" ).toString();

				for ( int i = 0; i < amount; ++i )
				{
					QList<unsigned int> newComponents;

					newComponents.append( g->inv()->createItem( pos, itemSID1, si.mat1 ) );
					newComponents.append( g->inv()->createItem( pos, itemSID2, si.mat2 ) );

					g->inv()->createItem( thisPos, si.itemSID, newComponents );

					for ( auto vItem : newComponents )
					{
						g->inv()->destroyObject( vItem );
					}
				}
			}
			//qDebug() << "create combined item " << entry.value( "ItemID" ).toString() << " done";
		}

		else
		{
			int amount        = si.amount;
			QString container = DB::select( "AllowedContainers", "Items", si.itemSID ).toString();
			Position thisPos( pos + offsets[( id % 3 ) + 6] );
			w->getFloorLevelBelow( thisPos, false );
			/*
			if( !container.isEmpty() )
			{
				int capacity = DB::select( "Capacity", "Containers", container ).toInt();

				while( amount > 0 )
				{
					unsigned int containerUID = g->inv()->createItem( thisPos, container, getRandomMaterial( container ) );
					//unsigned int containerUID = g->inv()->createItem( thisPos, container, "Birch" );
					for( int i = 0; i < qMin( amount, capacity ); ++i )
					{
						
						unsigned int itemInContainer = g->inv()->createItem( thisPos, entry.value( "ItemID" ).toString(), entry.value( "MaterialID" ).toString() );
						g->inv()->putItemInContainer( itemInContainer, containerUID );
					}
					amount -= capacity;
				}
			}
			else*/
			{
				for ( int i = 0; i < amount; ++i )
				{
					g->inv()->createItem( thisPos, si.itemSID, si.mat1 );
				}
			}
		}
		++id;
	}
	g->inv()->itemHistory()->finishStart();
}

QString WorldGenerator::getRandomMaterial( QString itemSID )
{
	QVariantMap row = DB::selectRow( "Items", itemSID );

	QStringList out;

	if ( !row.value( "AllowedMaterials" ).toString().isEmpty() )
	{
		for ( auto mat : row.value( "AllowedMaterials" ).toString().split( "|" ) )
		{
			out.append( mat );
		}
	}
	if ( !row.value( "AllowedMaterialTypes" ).toString().isEmpty() )
	{
		for ( auto type : row.value( "AllowedMaterialTypes" ).toString().split( "|" ) )
		{
			for ( auto mat : DB::ids( "Materials", "Type", type ) )
			{
				out.append( mat );
			}
		}
	}
	if ( out.size() )
	{
		return out[rand() % out.size()];
	}
	else
	{
		return "None";
	}
}

// heightmap values = -1 to 1
void WorldGenerator::createHeightMap( int dimX, int dimY )
{
	m_heightMap.resize( dimX * dimY * 4 );
	m_heightMap2.resize( dimX * dimY * 4 );

	float flatness = ngs->flatness();

	for ( int x = 0; x < dimX * 2; ++x )
	{
		for ( int y = 0; y < dimY * 2; ++y )
		{
			float value = m_random.GetPerlin( x, y );
			//m_min = qMin( value, m_min );
			//m_max = qMax( value, m_max );
			m_heightMap[x + y * dimX]  = value * flatness;
			m_heightMap2[x + y * dimX] = value;
		}
	}
	//qDebug() << "heightMap min: " << m_min << "heightMap max: " << m_max;
}

void WorldGenerator::fillFloorMushroomBiome( int zz, QVector<TerrainMaterial>& mats, QVector<int>& matsinLevel )
{
	int baseLevel = m_mushroomLevel;

	int z = zz + baseLevel;

	auto& world = w->world();

	TerrainMaterial mat;

	unsigned short dirtMat = DBH::materialUID( "Dirt" );
	auto dirtSprite        = g->sf()->createSprite( "MushroomGrassWithDetail", { "Grass", "None" } )->uID;

	for ( int y = 3; y < m_dimY - 3; ++y )
	{
		for ( int x = 3; x < m_dimX - 3; ++x )
		{
			Tile& tile          = world[x + y * m_dimX + z * m_dimX * m_dimY];
			tile.floorType      = FloorType::FT_NOFLOOR;
			tile.floorMaterial  = 0;
			tile.wallType       = WallType::WT_NOWALL;
			tile.wallMaterial   = 0;
			tile.floorSpriteUID = 0;
			tile.wallSpriteUID  = 0;
			tile.flags          = TileFlag::TF_NONE;

			if ( x == 0 || y == 0 || x == m_dimX - 1 || y == m_dimY - 1 )
			{
			}
			else
			{
				//if( m_random.GetPerlinFractal( x, y, z ) > 0.00001 )
				//if( fBm( x, y, z ) )
				if ( zz - ( m_heightMap[x + m_dimX + y * m_dimX * 2] * 3. ) < 0 )
				{
					mat                = mats[matsinLevel[qMin( m_dimZ - 1, qMax( 0, z - m_heightMap[x + m_dimX + y * m_dimX * 2] ) )]];
					unsigned short key = mat.rowid;

					tile.floorType     = FloorType::FT_SOLIDFLOOR;
					tile.floorMaterial = key;
					if ( mat.floorSprite )
					{
						tile.floorSpriteUID = mat.floorSprite;
					}
					else
					{
						tile.floorSpriteUID                                                               = g->sf()->createSprite( mat.floor, { mat.key } )->uID;
						mat.floorSprite                                                                   = tile.floorSpriteUID;
						mats[matsinLevel[qMin( m_dimZ - 1, qMax( 0, z - m_heightMap[x + y * m_dimX] ) )]] = mat;
					}
					tile.wallType     = ( WallType )( WallType::WT_SOLIDWALL | WallType::WT_ROUGH | WallType::WT_VIEWBLOCKING | WallType::WT_MOVEBLOCKING );
					tile.wallMaterial = key;
					if ( mat.wallSprite )
					{
						tile.wallSpriteUID = mat.wallSprite;
					}
					else
					{
						tile.wallSpriteUID                                                                = g->sf()->createSprite( mat.wall, { mat.key } )->uID;
						mat.wallSprite                                                                    = tile.wallSpriteUID;
						mats[matsinLevel[qMin( m_dimZ - 1, qMax( 0, z - m_heightMap[x + y * m_dimX] ) )]] = mat;
						g->sf()->createSprite( mat.shortwall, { mat.key } )->uID;
					}
					if ( m_fow )
					{
						tile.flags += TileFlag::TF_UNDISCOVERED;
					}
				}
				else
				{
					Tile& tileBelow = world[x + y * m_dimX + ( z - 1 ) * m_dimX * m_dimY];
					if ( tileBelow.wallType & WallType::WT_ROUGH )
					{
						tile.floorType      = FloorType::FT_SOLIDFLOOR;
						tile.floorMaterial  = dirtMat;
						tile.floorSpriteUID = g->sf()->createSprite( "MushroomGrassWithDetail", { "Grass", "None" } )->uID;
						tile.flags += TileFlag::TF_WALKABLE;
						//tile.flags |= TileFlag::TF_GRASS;
						tile.flags += TileFlag::TF_BIOME_MUSHROOM;
					}
				}
			}
		}
	}
}

void WorldGenerator::fillFloor( int z, QVector<TerrainMaterial>& mats, QVector<int>& matsinLevel )
{
	auto& world = w->world();

	TerrainMaterial mat;

	for ( int y = 0; y < m_dimY; ++y )
	{
		for ( int x = 0; x < m_dimX; ++x )
		{
			mat                = mats[matsinLevel[qMin( m_dimZ - 1, qMax( 0, z - m_heightMap[x + y * m_dimX] ) )]];
			unsigned short key = mat.rowid;

			Tile& tile          = world[x + y * m_dimX + z * m_dimX * m_dimY];
			tile.floorType      = FloorType::FT_NOFLOOR;
			tile.floorMaterial  = 0;
			tile.wallType       = WallType::WT_NOWALL;
			tile.wallMaterial   = 0;
			tile.floorSpriteUID = 0;
			tile.wallSpriteUID  = 0;
			tile.flags          = TileFlag::TF_NONE;

			if ( x == 0 || y == 0 || x == m_dimX - 1 || y == m_dimY - 1 )
			{
			}
			else
			{
				//if( m_random.GetPerlinFractal( x, y, z ) > 0.00001 )
				//if( fBm( x, y, z ) )
				if ( z - m_heightMap[x + y * m_dimX] > 0 )
				{
					if ( mat.key != "Air" )
					{
						tile.floorType     = FloorType::FT_SOLIDFLOOR;
						tile.floorMaterial = key;
						if ( mat.floorSprite )
						{
							tile.floorSpriteUID = mat.floorSprite;
						}
						else
						{
							tile.floorSpriteUID                                                               = g->sf()->createSprite( mat.floor, { mat.key } )->uID;
							mat.floorSprite                                                                   = tile.floorSpriteUID;
							mats[matsinLevel[qMin( m_dimZ - 1, qMax( 0, z - m_heightMap[x + y * m_dimX] ) )]] = mat;
						}
						tile.wallType     = ( WallType )( WallType::WT_SOLIDWALL | WallType::WT_ROUGH | WallType::WT_VIEWBLOCKING | WallType::WT_MOVEBLOCKING );
						tile.wallMaterial = key;
						if ( mat.wallSprite )
						{
							tile.wallSpriteUID = mat.wallSprite;
						}
						else
						{
							tile.wallSpriteUID                                                                = g->sf()->createSprite( mat.wall, { mat.key } )->uID;
							mat.wallSprite                                                                    = tile.wallSpriteUID;
							mats[matsinLevel[qMin( m_dimZ - 1, qMax( 0, z - m_heightMap[x + y * m_dimX] ) )]] = mat;
							g->sf()->createSprite( mat.shortwall, { mat.key } )->uID;
						}
						if ( m_fow )
						{
							tile.flags += TileFlag::TF_UNDISCOVERED;
						}
					}
					else
					{
						Tile& tileBelow = world[x + y * m_dimX + ( z - 1 ) * m_dimX * m_dimY];
						if ( tileBelow.wallType & WallType::WT_ROUGH )
						{
							tile.floorType      = FloorType::FT_SOLIDFLOOR;
							tile.floorMaterial  = tileBelow.floorMaterial;
							tile.floorSpriteUID = tileBelow.floorSpriteUID;
							tile.flags += TileFlag::TF_WALKABLE;
						}
					}
				}
			}
		}
	}
}

void WorldGenerator::discoverAll()
{
	for ( int z = m_dimZ - 2; z > 0; --z )
	{
		for ( int y = 1; y < m_dimY - 1; ++y )
		{
			for ( int x = 1; x < m_dimX - 1; ++x )
			{
				if ( !( w->wallType( Position( x, y, z ) ) & WT_SOLIDWALL ) )
				{
					w->discover( x, y, z );
				}
			}
		}
	}
}

void WorldGenerator::createRamps()
{
	for ( int z = m_dimZ - 2; z > 0; --z )
	{
		createRamp( z );
	}
}

void WorldGenerator::createRamp( int z )
{
	for ( int y = 1; y < m_dimY - 1; ++y )
	{
		for ( int x = 1; x < m_dimX - 1; ++x )
		{
			w->createRamp( x, y, z );
		}
	}
}

bool WorldGenerator::fBm( int x, int y, int z )
{
	int fixedZ = qMin( m_dimZ - 1, qMax( 0, (int)z - m_heightMap[x + y * m_dimX] ) );

	if ( z > m_groundLevel - 6 && z < m_groundLevel )
		return true;

	if ( z < m_groundLevel - 5 )
	{
		return true;

		//float noise = abs( m_random.GetCubic( x, y, z ) ) * 5 ;
		//float compare = 0.1;
		//return ( noise > compare );
	}
	else if ( z < m_dimZ - 10 )
	{
		return m_heightMap[x + y * m_dimX] > ( z - 98 );
	}

	return false;
}

std::vector<Position> WorldGenerator::perlinWorm( int z, int num, int maxLength )
{
	std::vector<Position> out;

	float noiseX = perlinRandWhiteNoise( z, num );
	float noiseY = perlinRandWhiteNoise( num, z );

	int x = ( m_dimX * noiseX ) - 1;
	int y = ( m_dimY * noiseY ) - 1;

	if ( x > 1 && x < m_dimX - 2 && y > 1 && y < m_dimY - 2 )
	{
		out.push_back( Position( x, y, z ) );
	}

	float noiseLength = perlinRandWhiteNoise( x, y );
	int length        = noiseLength * maxLength;

	// init direction
	int dir = qMax( 0, qMin( int( perlinRandWhiteNoise( x * 10, y * 10 ) * 9 ), 8 ) );

	for ( int i = 0; i < length; ++i )
	{
		// do we change dir?
		if ( perlinRandWhiteNoise( x * i, y * i ) > 0.95 )
		{
			//if yes
			float rand    = perlinRandWhiteNoise( x * i * 2, y * i * 2 );
			int dirChange = round( rand );
			dir += dirChange;
		}

		if ( dir < 0 )
			dir = 7;
		if ( dir > 7 )
			dir = 0;
		switch ( dir )
		{
			case 0:
				x -= 1;
				break;
			case 1:
				x -= 1;
				y -= 1;
				break;
			case 2:
				y -= 1;
				break;
			case 3:
				x += 1;
				y -= 1;
				break;
			case 4:
				x += 1;
				break;
			case 5:
				x += 1;
				y += 1;
				break;
			case 6:
				y += 1;
				break;
			case 7:
				x -= 1;
				y += 1;
				break;
		}

		if ( x > 1 && x < m_dimX - 2 && y > 1 && y < m_dimY - 2 )
		{
			out.push_back( Position( x, y, z ) );
		}
	}

	return out;
}

float WorldGenerator::perlinRandWhiteNoise( int x, int y, int z )
{
	FastNoise fn;
	fn.SetSeed( m_seed );
	fn.SetFrequency( (FN_DECIMAL)0.02 );
	fn.SetNoiseType( FastNoise::NoiseType::WhiteNoise );

	if ( z == -1 )
	{
		return ( fn.GetNoise( x, y ) + 1.0 ) / 2.;
	}
	return ( fn.GetNoise( x, y, z ) + 1.0 ) / 2.;
}

void WorldGenerator::clear3x3( std::vector<Tile>& world, Position& pos )
{
	for ( int x = pos.x - 1; x < pos.x + 2; ++x )
	{
		for ( int y = pos.y - 1; y < pos.y + 2; ++y )
		{
			Tile& tile         = world[x + y * m_dimX + pos.z * m_dimX * m_dimY];
			tile.wallSpriteUID = 0;
			tile.flags         = TileFlag::TF_WALKABLE;
			tile.wallType      = WallType::WT_NOWALL;
			tile.wallMaterial  = 0;
		}
	}
}

void WorldGenerator::setEmbedded3x3( std::vector<Tile>& world, Position& pos, unsigned short embeddedMaterial, unsigned short spriteID )
{
	// TODO changed it to 2x2
	for ( int x = pos.x - 1; x < pos.x + 1; ++x )
	{
		for ( int y = pos.y - 1; y < pos.y + 1; ++y )
		{
			Tile& tile = world[x + y * m_dimX + pos.z * m_dimX * m_dimY];

			if ( tile.wallType & WallType::WT_SOLIDWALL )
			{
				tile.embeddedMaterial = embeddedMaterial;
				tile.itemSpriteUID    = spriteID;
			}
		}
	}
}

QString WorldGenerator::getRandomEmbedded( int x, int y, int z, QMap<QString, EmbeddedMaterial>& em )
{
	QStringList possibles;
	for ( auto key : em.keys() )
	{
		auto e = em.value( key );
		int zz = z - ngs->ground() + m_heightMap[x + y * m_dimX];
		if ( zz >= e.lowest && zz <= e.highest )
		{
			possibles.push_back( e.key );
		}
	}
	if ( possibles.isEmpty() )
	{
		return "";
	}

	int index = qMax( 0, qMin( (int)( perlinRandWhiteNoise( x, y ) * ( possibles.size() + 1 ) ), possibles.size() - 1 ) );
	return possibles[index];
}

void WorldGenerator::createOceanFront()
{
	srand( std::chrono::system_clock::now().time_since_epoch().count() );

	auto& world = w->world();

	int size = ngs->oceanSize();

	int edge = rand() % 4;

	int xStart = 1;
	int yStart = 1;
	int maxX   = m_dimX - 2;
	int maxY   = m_dimY - 2;
	int xAdd   = 1;
	int yAdd   = 1;

	int sandRowID = DBH::materialUID( "Sand" );

	QVector<int> sizes;
	for ( int i = 0; i < m_dimX; ++i )
	{
		sizes.push_back( size + ( m_heightMap2[i + 20 * m_dimX] * size ) );
	}

	switch ( edge )
	{
		case 0:
		{
			for ( int y = 1; y < m_dimY - 1; ++y )
			{
				int ySize = sizes[y];
				for ( int i = 0; i < ySize; ++i )
				{
					int x = xStart + i;
					decreaseHeight( x, y, qMin( 10, ySize - i ) );
					setSandFloor( x, y, sandRowID );
				}
			}

			int z = 99; //getLowestZonXLine( xStart + size );

			for ( int y = 1; y < m_dimY - 1; ++y )
			{
				int ySize = sizes[y];
				for ( int i = 0; i < ySize; ++i )
				{
					int x = xStart + i;
					fillWater( x, y, z );
				}
			}
			break;
		}
		case 1:
		{
			for ( int x = 1; x < m_dimX - 1; ++x )
			{
				size = sizes[x];
				for ( int i = 0; i < size; ++i )
				{
					int y = yStart + i;
					decreaseHeight( x, y, qMin( 10, size - i ) );
					setSandFloor( x, y, sandRowID );
				}
			}
			int z = 99; //getLowestZonYLine( yStart + size );

			for ( int x = 1; x < m_dimX - 1; ++x )
			{
				size = sizes[x];
				for ( int i = 0; i < size; ++i )
				{
					int y = yStart + i;
					fillWater( x, y, z );
				}
			}
			break;
		}
		case 2:
		{
			for ( int y = 1; y < m_dimY - 1; ++y )
			{
				size = sizes[y];
				for ( int i = 0; i < size; ++i )
				{
					int x = maxX - i;
					decreaseHeight( x, y, qMin( 10, size - i ) );
					setSandFloor( x, y, sandRowID );
				}
			}
			int z = 99; // getLowestZonXLine( maxX - size );

			for ( int y = 1; y < m_dimY - 1; ++y )
			{
				size = sizes[y];
				for ( int i = 0; i < size; ++i )
				{
					int x = maxX - i;
					fillWater( x, y, z );
				}
			}
			break;
		}
		case 3:
		{

			for ( int x = 1; x < m_dimX - 1; ++x )
			{
				size = sizes[x];
				for ( int i = 0; i < size; ++i )
				{
					int y = maxY - i;
					decreaseHeight( x, y, qMin( 10, size - i ) );
					setSandFloor( x, y, sandRowID );
				}
			}
			int z = 99; //getLowestZonYLine( maxY - size );

			for ( int x = 1; x < m_dimX - 1; ++x )
			{
				size = sizes[x];
				for ( int i = 0; i < size; ++i )
				{
					int y = maxY - i;
					fillWater( x, y, z );
				}
			}
			break;
		}
	}
}

void WorldGenerator::decreaseHeight( int x, int y, int diff )
{
	Position pos( x, y, m_dimZ - 1 );
	w->getFloorLevelBelow( pos, true );

	auto& world = w->world();

	for ( int i = 0; i < diff; ++i )
	{
		if ( pos.z - i > 92 )
		{
			Tile& tile = world[x + y * m_dimX + ( pos.z - i ) * m_dimX * m_dimY];

			tile.wallType      = WT_NOWALL;
			tile.wallMaterial  = 0;
			tile.flags         = TileFlag::TF_NONE;
			tile.wallSpriteUID = 0;

			tile.floorType      = FT_NOFLOOR;
			tile.floorMaterial  = 0;
			tile.floorSpriteUID = 0;

			tile.flags += TileFlag::TF_SUNLIGHT;
			//w->addWater( Position( x, y, pos.z - i ), 8 );
		}
	}
}

void WorldGenerator::setSandFloor( int x, int y, int sandRowID )
{
	Position pos( x, y, m_dimZ - 1 );
	w->getFloorLevelBelow( pos, true );

	Tile& tile = w->world()[x + y * m_dimX + ( pos.z + 1 ) * m_dimX * m_dimY];

	tile.wallType      = WT_NOWALL;
	tile.wallMaterial  = 0;
	tile.flags         = TileFlag::TF_WATER;
	tile.wallSpriteUID = 0;

	tile.floorMaterial  = sandRowID;
	tile.floorType      = FT_SOLIDFLOOR;
	tile.floorSpriteUID = g->sf()->createSprite( "RoughFloor", { "Sand" } )->uID;

	Tile& tile2          = w->world()[x + y * m_dimX + ( pos.z ) * m_dimX * m_dimY];
	tile2.wallType       = ( WallType )( WallType::WT_SOLIDWALL | WallType::WT_ROUGH | WallType::WT_VIEWBLOCKING | WallType::WT_MOVEBLOCKING );
	tile2.floorMaterial  = sandRowID;
	tile2.wallMaterial   = sandRowID;
	tile2.floorType      = FT_SOLIDFLOOR;
	tile2.wallSpriteUID  = g->sf()->createSprite( "RoughWall", { "Sand" } )->uID;
	tile2.floorSpriteUID = g->sf()->createSprite( "RoughFloor", { "Sand" } )->uID;
}

int WorldGenerator::getLowestZonXLine( int x )
{
	short maxZ = m_dimZ - 1;
	for ( int y = 1; y < m_dimY - 1; ++y )
	{
		Position pos( x, y, m_dimZ - 1 );
		w->getFloorLevelBelow( pos, false );
		maxZ = qMin( maxZ, pos.z );
	}
	return maxZ;
}

int WorldGenerator::getLowestZonYLine( int y )
{
	short maxZ = m_dimZ - 1;
	for ( int x = 1; x < m_dimX - 1; ++x )
	{
		Position pos( x, y, m_dimZ - 1 );
		w->getFloorLevelBelow( pos, false );
		maxZ = qMin( maxZ, pos.z );
	}
	return maxZ;
}

void WorldGenerator::fillWater( int x, int y, int z )
{
	while ( z > 1 )
	{
		Tile& tile = w->world()[x + y * m_dimX + z * m_dimX * m_dimY];
		if ( !(bool)( tile.wallType & WT_SOLIDWALL ) )
		{
			w->addWater( Position( x, y, z ), 10 );
		}

		if ( (bool)( tile.floorType & FT_SOLIDFLOOR ) )
		{
			break;
		}
		--z;
	}
}

void WorldGenerator::createRivers()
{
	QList<std::vector<Position>> worms;
	auto& world = w->world();

	for ( int i = 1; i < ngs->rivers() + 1; ++i )
	{
		int border = qMax( 0, qMin( int( perlinRandWhiteNoise( i * 10, i * 20 ) * 4 ), 3 ) );
		int offset = qMax( 10, qMin( int( perlinRandWhiteNoise( i * 15, i * 17 ) * m_dimX ), m_dimX - 10 ) );

		qDebug() << i << ": " << border << offset;

		int dir = 0;
		Position pos;
		switch ( border )
		{
			case 0: //west border
				dir = 4;
				pos = Position( 1, offset, m_dimZ - 1 );
				break;
			case 1: // north border
				pos = Position( offset, 1, m_dimZ - 1 );
				dir = 6;
				break;
			case 2: // east border
				pos = Position( m_dimX - 2, offset, m_dimZ - 1 );
				dir = 0;
				break;
			case 3: // sout border
				pos = Position( offset, m_dimY - 2, m_dimZ - 1 );
				dir = 2;
				break;
		}

		worms.append( riverWorm( pos, dir, i, m_dimX * 3 ) );
	}
	QList<std::vector<Position>> worms2;
	for ( auto worm : worms )
	{
		int prevZ = m_dimZ - 1;

		for ( int i = 0; i < worm.size(); ++i )
		{
			auto pos = worm[i];
			w->getFloorLevelBelow( pos, false );

			if ( pos.z > prevZ )
			{
				pos.z = prevZ;
			}
			prevZ   = pos.z;
			worm[i] = pos;
		}
		worms2.append( worm );
	}
	for ( auto worm : worms2 )
	{
		for ( int i = 0; i < worm.size(); ++i )
		{
			auto pos = worm[i];
			carveRiver( world, pos );
		}

		if ( worm.size() > 1 )
		{
			auto apos = worm[0];
			w->addAquifier( apos.belowOf() );
			auto dpos = worm[worm.size() - 1];
			w->addDeaquifier( dpos.belowOf() );
		}
	}
}

std::vector<Position> WorldGenerator::riverWorm( Position pos, int dir, int num, int maxLength )
{
	std::vector<Position> out;

	if ( pos.x > 1 && pos.x < m_dimX - 2 && pos.y > 1 && pos.y < m_dimY - 2 )
	{
		out.push_back( pos );
	}

	int origDir = dir;

	int x = pos.x;
	int y = pos.y;
	int z = pos.z;

	for ( int i = 0; i < maxLength; ++i )
	{

		// do we change dir?
		if ( perlinRandWhiteNoise( x * i * num, y * i * num ) > 0.8 && out.size() > m_dimX / 10 )
		{
			//if yes
			float rand    = perlinRandWhiteNoise( x * i * 2 * num, y * i * 2 * num );
			int dirChange = rand > 0.5 ? -1 : 1;
			dir += dirChange;

			switch ( origDir )
			{
				case 0:
					if ( dir == 4 )
					{
						dir -= dirChange;
						dir -= dirChange;
					}
					break;
				case 2:
					if ( dir == 6 )
					{
						dir -= dirChange;
						dir -= dirChange;
					}
					break;
				case 4:
					if ( dir == 0 || dir == 8 )
					{
						dir -= dirChange;
						dir -= dirChange;
					}
					break;
				case 6:
					if ( dir == 2 )
					{
						dir -= dirChange;
						dir -= dirChange;
					}
					break;
			}
		}

		if ( dir < 0 )
			dir = 7;
		if ( dir > 7 )
			dir = 0;
		switch ( dir )
		{
			case 0:
				x -= 1;
				break;
			case 1:
				x -= 1;
				y -= 1;
				break;
			case 2:
				y -= 1;
				break;
			case 3:
				x += 1;
				y -= 1;
				break;
			case 4:
				x += 1;
				break;
			case 5:
				x += 1;
				y += 1;
				break;
			case 6:
				y += 1;
				break;
			case 7:
				x -= 1;
				y += 1;
				break;
		}

		if ( x > 1 && x < m_dimX - 2 && y > 1 && y < m_dimY - 2 )
		{
			out.push_back( Position( x, y, z ) );
		}
		else
		{
			break;
		}
	}

	return out;
}

void WorldGenerator::carveRiver( std::vector<Tile>& world, Position& pos )
{
	int size = ngs->riverSize();
	int sandRowID = DBH::materialUID( "Sand" );
	for ( int x = pos.x - size; x < pos.x + size + 1; ++x )
	{
		for ( int y = pos.y - size; y < pos.y + size + 1; ++y )
		{
			if ( x > 0 && x < m_dimX - 1 && y > 0 && y < m_dimY - 1 )
			{
				Tile& tile         = world[x + y * m_dimX + pos.z * m_dimX * m_dimY];
				tile.wallSpriteUID = 0;
				tile.flags         = TileFlag::TF_NONE;
				tile.wallType      = WallType::WT_NOWALL;
				tile.wallMaterial  = 0;

				tile.floorType      = FT_NOFLOOR;
				tile.floorMaterial  = 0;
				tile.floorSpriteUID = 0;

				Tile& tile2         = world[x + y * m_dimX + ( pos.z - 1 ) * m_dimX * m_dimY];
				tile2.wallSpriteUID = 0;
				tile2.flags         = TileFlag::TF_NONE;
				tile2.wallType      = WallType::WT_NOWALL;
				tile2.wallMaterial  = 0;

				tile2.floorType      = FT_NOFLOOR;
				tile2.floorMaterial  = 0;
				tile2.floorSpriteUID = 0;

				Tile& tile3         = world[x + y * m_dimX + ( pos.z - 2 ) * m_dimX * m_dimY];
				tile3.wallSpriteUID = 0;
				tile3.flags         = TileFlag::TF_NONE;
				tile3.wallType      = WallType::WT_NOWALL;
				tile3.wallMaterial  = 0;

				w->addWater( Position( x, y, pos.z - 1 ), 10 );
				w->addWater( Position( x, y, pos.z - 2 ), 10 );

				for ( int z = pos.z; z < m_dimZ - 1; ++z )
				{
					Tile& tile         = world[x + y * m_dimX + z * m_dimX * m_dimY];
					tile.wallSpriteUID = 0;
					tile.flags         = TileFlag::TF_NONE;
					tile.wallType      = WallType::WT_NOWALL;
					tile.wallMaterial  = 0;

					tile.floorType      = FT_NOFLOOR;
					tile.floorMaterial  = 0;
					tile.floorSpriteUID = 0;
				}
				setSandFloor( x, y, sandRowID );

			}
		}
	}
}

void WorldGenerator::placeLairs()
{
	// find suitable location
	QString lairType   = "Mant";
	auto row           = DB::selectRow( "Lairs", "MantLair1" );
	QString sizeString = row.value( "Size" ).toString();
	auto sl            = sizeString.split( " " );
	int xs             = 0;
	int ys             = 0;
	int zs             = 0;
	if ( sl.size() == 3 )
	{
		xs = sl[0].toInt();
		ys = sl[1].toInt();
		zs = sl[2].toInt();
	}
	qDebug() << "lair size " << xs << ys << zs;
	int x = 5;
	int y = 5;
	int z = 100 - zs;
	Position posZero( x, y, z );
	bool found = true;
	while ( ( x + xs ) < m_dimX - 5 && ( y + ys ) < m_dimY - 5 )
	{
		found = checkPlacement( x, y, z, xs, ys, zs );
		if ( found )
		{
			break;
		}
		x += 5;
		y += 5;
	}
	qDebug() << "lair location " << x << y << z;
	if ( found )
	{
		auto& world = w->world();

		QString def  = row.value( "Layout" ).toString();
		auto defList = def.split( " " );

		for ( int zz = 0; zz < zs; ++zz )
		{
			for ( int yy = 0; yy < ys; ++yy )
			{
				for ( int xx = 0; xx < xs; ++xx )
				{
					QString tileDef = defList[xx + yy * xs + zz * xs * ys];
					Tile& tile      = world[x + xx + ( y + yy ) * m_dimX + ( z + zz ) * m_dimX * m_dimY];
					switch ( tileDef.toInt() )
					{
						case 0:
							tile.floorType      = FT_NOFLOOR;
							tile.floorMaterial  = 0;
							tile.floorSpriteUID = 0;

							tile.wallType         = WT_NOWALL;
							tile.wallSpriteUID    = 0;
							tile.wallMaterial     = 0;
							tile.embeddedMaterial = 0;
							tile.itemSpriteUID    = 0;

							tile.flags = TileFlag::TF_NONE;
							break;
						case 1:
							tile.wallType         = WT_NOWALL;
							tile.wallSpriteUID    = 0;
							tile.wallMaterial     = 0;
							tile.embeddedMaterial = 0;
							tile.itemSpriteUID    = 0;
							tile.flags += TileFlag::TF_WALKABLE;
							break;
						case 2:
							break;
						case 3:
							tile.wallType      = WT_NOWALL;
							tile.wallSpriteUID = 0;
							break;
					}
				}
			}
		}
		for ( int zz = 0; zz < zs; ++zz )
		{
			for ( int yy = 0; yy < ys; ++yy )
			{
				for ( int xx = 0; xx < xs; ++xx )
				{
					QString tileDef = defList[xx + yy * xs + zz * xs * ys];
					Tile& tile      = world[x + xx + ( y + yy ) * m_dimX + ( z + zz ) * m_dimX * m_dimY];
					switch ( tileDef.toInt() )
					{
						case 0:
						case 1:
						case 2:
							break;
						case 3:
						{
							Tile& tileAbove     = world[x + xx + ( y + yy ) * m_dimX + ( z + zz + 1 ) * m_dimX * m_dimY];
							tileAbove.floorType = FT_NOFLOOR;
							tileAbove.flags     = TileFlag::TF_NONE;
							//tileAbove.floorMaterial = 0;
							tileAbove.floorSpriteUID = 0;

							tile.wallType      = WT_NOWALL;
							tile.flags         = TileFlag::TF_NONE;
							tile.wallSpriteUID = 0;
							//tile.wallMaterial = 0;
							w->createRamp( x + xx, y + yy, z + zz, DBH::materialSID( tile.wallMaterial ) );
						}
						break;
					}
				}
			}
		}

		for ( auto row : DB::selectRows( "Lairs_Spawns" ) )
		{
			int rot       = row.value( "Rotation" ).toInt();
			Gender gender = (Gender)row.value( "Gender" ).toInt();
			int level     = row.value( "Level" ).toInt();

			g->cm()->addCreature( CreatureType::MONSTER, lairType + row.value( "Type" ).toString(), posZero + Position( row.value( "Offset" ) ), gender, true, false );
		}
	}
}

bool WorldGenerator::checkPlacement( int xLoc, int yLoc, int zLoc, int xSize, int ySize, int zSize )
{
	auto& world = w->world();
	for ( int z = zLoc; z < zSize; ++z )
	{
		for ( int x = xLoc; x < xSize; ++x )
		{
			for ( int y = yLoc; y < ySize; ++y )
			{
				Tile& tile = world[x + y * m_dimX + z * m_dimX * m_dimY];
				if ( !(bool)( tile.wallType & WT_SOLIDWALL ) )
				{
					return false;
				}
			}
		}
	}
	emit signalStatus( "Found lairs location." );
	return true;
}