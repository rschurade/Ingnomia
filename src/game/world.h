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

#ifndef WORLD_H_
#define WORLD_H_

#include "../base/enums.h"
#include "../base/lightmap.h"
#include "../base/regionmap.h"
#include "../base/tile.h"

#include <QMutex>
#include <QPixmap>
#include <QSet>

#include <set>
#include <vector>

class Plant;
class Animal;
class Creature;
class Game;

struct Position;
class Sprite;

enum CONSTRUCTION_ID
{
	CID_WALL = 1,
	CID_FANCYWALL,
	CID_FENCE,
	CID_FLOOR,
	CID_FANCYFLOOR,
	CID_WALLFLOOR,
	CID_STAIRS,
	CID_RAMP,
	CID_RAMPCORNER,
	CID_ITEM,
	CID_WORKSHOP
};

enum CONSTR_ITEM_ID
{
	CI_STORAGE = 1,
	CI_FURNITURE,
	CI_LIGHT,
	CI_LIGHTSHROOM,
	CI_DOOR,
	CI_ALARMBELL,
	CI_FARMUTIL,
	CI_MECHANISM,
	CI_UTILITY,
	CI_HYDRAULICS
};

class World
{
	Q_DISABLE_COPY_MOVE( World )
private:
	QPointer<Game> g;

	LightMap m_lightMap;
	RegionMap m_regionMap;

	QMutex m_updateMutex;

	int m_dimX = 1;
	int m_dimY = 1;
	int m_dimZ = 1;

	bool m_grassChanged = false;

	std::vector<Tile> m_world;

	QMap<unsigned int, Plant> m_plants;
	QMap<unsigned int, QList<unsigned int>> m_creaturePositions;
	QMap<unsigned int, QVariantMap> m_wallConstructions;
	QMap<unsigned int, QVariantMap> m_floorConstructions;
	QSet<Position> m_grass;
	QSet<unsigned int> m_grassCandidatePositions;
	QMap<unsigned int, QVariantMap> m_jobSprites;
	std::set<unsigned int> m_water;
	QList<Position> m_aquifiers;
	QList<Position> m_deaquifiers;

	QSet<unsigned int> m_updatedTiles;

	QMap<QString, CONSTRUCTION_ID> m_constructionSID2ENUM;
	QMap<QString, CONSTR_ITEM_ID> m_constrItemSID2ENUM;

	bool constructWall( QVariantMap& con, Position pos, int rotation, QVariantList itemUIDs, QVariantList materialUIDs, QStringList materialSIDs, Position extractTo );
	bool constructFloor( QVariantMap& con, Position pos, int rotation, QVariantList itemUIDs, QVariantList materialUIDs, QStringList materialSIDs, Position extractTo );
	bool constructFence( QVariantMap& con, Position pos, int rotation, QVariantList itemUIDs, QVariantList materialUIDs, QStringList materialSIDs, Position extractTo );
	bool constructWallFloor( QVariantMap& con, Position pos, int rotation, QVariantList itemUIDs, QVariantList materialUIDs, QStringList materialSIDs, Position extractTo );
	bool constructStairs( QVariantMap& con, Position pos, int rotation, QVariantList itemUIDs, QVariantList materialUIDs, QStringList materialSIDs, Position extractTo );
	bool constructRamp( QVariantMap& con, Position pos, int rotation, QVariantList itemUIDs, QVariantList materialUIDs, QStringList materialSIDs, Position extractTo );
	bool constructRampCorner( QVariantMap& con, Position pos, int rotation, QVariantList itemUIDs, QVariantList materialUIDs, QStringList materialSIDs, Position extractTo );
	bool constructPipe( QString type, Position pos, unsigned int itemUID );
	bool deconstructPipe( QVariantMap constr, Position pos, Position workPos );

	

public:
	World( int dimX, int dimY, int dimZ, Game* game );
	~World();

	void init();
	void initWater();
	void afterLoad();

	void processGrass();
	void removeGrass( Position pos );
	void createGrass( Position pos );
	void initGrassUpdateList();
	void isGrassCandidate( Position pos );
	void addGrassCandidate( Position pos );
	bool hasGrass( Position pos );
	bool hasMaxGrass( Position pos );

	LightMap& lightMap()
	{
		return m_lightMap;
	}
	RegionMap& regionMap()
	{
		return m_regionMap;
	}

	std::vector<Tile>& world()
	{
		return m_world;
	}
	QMap<unsigned int, QVariantMap>& wallConstructions()
	{
		return m_wallConstructions;
	}
	QMap<unsigned int, QVariantMap>& floorConstructions()
	{
		return m_floorConstructions;
	}

	void setWallSprite( unsigned short x, unsigned short y, unsigned short z, unsigned int spriteUID, unsigned char rotation = 0 );
	void setWallSprite( const Position pos, unsigned int spriteUID, unsigned char rotation = 0 );

	void setItemSprite( unsigned short x, unsigned short y, unsigned short z, unsigned int spriteUID, unsigned char rotation = 0 );
	void setItemSprite( const Position pos, unsigned int spriteUID, unsigned char rotation = 0 );

	void setFloorSprite( unsigned short x, unsigned short y, unsigned short z, unsigned int spriteUID );
	void setFloorSprite( const Position pos, unsigned int spriteUID );

	void setJobSprite( Position pos, unsigned int spriteUID, unsigned char rotation, bool floor, unsigned int jobID, bool busy );
	void clearJobSprite( Position pos, bool floor );

	void setWallSprite( unsigned int tileID, unsigned int spriteID );
	void setFloorSprite( unsigned int tileID, unsigned int spriteID );
	void setJobSprite( unsigned int tileID, unsigned int spriteID, unsigned char rotation, bool floor, unsigned int jobID, bool busy );

	QVariantMap jobSprite( Position pos );
	QVariantMap jobSprite( int x, int y, int z );
	QVariantMap jobSprite( unsigned int tileID );
	QMap<unsigned int, QVariantMap> jobSprites();
	void insertLoadedJobSprite( unsigned int key, QVariantMap entry );
	bool hasJob( Position pos );
	bool hasJob( int x, int y, int z );
	bool hasJob( unsigned int tileID );

	void setTileFlag( unsigned short x, unsigned short y, unsigned short z, TileFlag flag );
	void setTileFlag( Position pos, TileFlag flag );

	TileFlag getTileFlag( Position pos );

	void clearTileFlag( Position pos, TileFlag flag );

	void setWalkable( Position pos, bool value );
	void updateWalkable( Position pos );

	unsigned short wallMaterial( Position pos );
	unsigned short embeddedMaterial( Position pos );
	unsigned short floorMaterial( Position pos );
	WallType wallType( Position pos );
	FloorType floorType( Position pos );

	int walkableNeighbors( Position pos );
	QList<Position> connectedNeighbors( Position pos );

	QMap<unsigned int, Plant>& plants()
	{
		return m_plants;
	}

	// returns true when construction succeeded, false otherwise
	bool construct( QString constructionSID, Position pos, int rotation, QList<unsigned int> itemUIDs, Position extractTo );
	bool constructItem( QString itemSID, Position pos, int rotation, QList<unsigned int> itemUIDs, Position extractTo );
	bool constructWorkshop( QString constructionSID, Position pos, int rotation, QList<unsigned int> itemUIDs, Position extractTo );
	// returns true when deconstruction succeeded, false otherwise
	bool deconstruct( Position pos, Position workPos, bool ignoreGravity );
	bool deconstruct2( QVariantMap constr, Position decPos, bool isFloor, Position workPos, bool ignoreGravity );

	void updateNavigation( QList<Position>& coords, Position extractTo );

	void updateFenceSprite( Position pos );
	void updatePipeSprite( Position pos );
	void addLoadedSprites( QVariantMap vals );

	void expelTileInhabitants( Position pos, Position& to );
	void expelTileItems( Position pos, Position& to );

	void plantTree( Position pos, QString type, bool fullyGrown = false );
	void plantMushroom( Position pos, QString type, bool fullyGrown = false );
	void plant( Position pos, unsigned int baseItem );
	void addPlant( Plant plant );
	void removePlant( Position pos );
	void removePlant( Plant plant );
	bool reduceOneGrowLevel( Position pos );

	QMap<unsigned int, QList<unsigned int>>& creaturePositions()
	{
		return m_creaturePositions;
	}
	void removeCreatureFromPosition( Position pos, unsigned int creatureID );
	void insertCreatureAtPosition( Position pos, unsigned int creatureID );
	bool creatureAtPos( Position pos );
	bool creatureAtPos( unsigned int posID );
	Creature* firstCreatureAtPos( unsigned int posID, quint8& rotation );

	void addAquifier( Position pos );
	void addWater( Position pos, unsigned char level );
	void changeFluidLevel( Position pos, int diff );
	void addDeaquifier( Position pos );
	void processWater();
	void processWaterFlow();

	void removeDesignation( Position pos );

	// return materialUID
	QPair<unsigned short, unsigned short> mineWall( Position pos, Position& workPosition );
	QPair<unsigned short, unsigned short> removeWall( Position pos, Position& workPosition );
	unsigned short removeRamp( Position pos, Position workPosition );
	void updateRampAtPos( Position pos );

	void discover( Position pos );
	void discover( int x, int y, int z );

	// return materialUID
	unsigned short removeFloor( Position pos, Position extractTo );

	bool isWalkable( Position pos );
	bool isWalkableGnome( Position pos );
	bool isRamp( Position pos );
	void getFloorLevelBelow( Position& pos, bool setSunlight );
	bool isSolidFloor( Position pos );

	int fluidLevel( Position pos );
	int vegetationLevel( Position pos );
	void setVegetationLevel( Position pos, int level );

	void createRamp( int x, int y, int z, TerrainMaterial mat );
	void createRamp( Position pos );
	void createRamp( int x, int y, int z );
	void createRamp( Position pos, QString material );
	void createRamp( int x, int y, int z, QString material );

	void setRampSprites( Tile& tile, Tile& tileAbove, int sum, bool north, bool east, bool south, bool west, QString materialSID );

	void createRampOuterCorners( int x, int y, int z );

	// TODO make the tile getters all private
	Tile& getTile( const unsigned short x, const unsigned short y, const unsigned short z );
	//Tile& getTile( Position pos );
	Tile& getTile( const Position pos );
	const Tile& getTile( const Position pos ) const;
	Tile& getTile( const unsigned int id );

	bool noTree( const Position pos, const int xRange, const int yRange );
	bool noShroom( const Position pos, const int xRange, const int yRange );

	QSet<unsigned int> updatedTiles();
	void addToUpdateList( const unsigned int uID );
	void addToUpdateList( const Position pos );
	void addToUpdateList( const unsigned short x, const unsigned short y, const unsigned short z );
	void addToUpdateList( const QVector<unsigned int>& ul );
	void addToUpdateList( const QSet<unsigned int>& ul );

	void setDoorLocked( unsigned int tileUID, bool lockGnome, bool lockMonster, bool lockAnimal );

	void putLight( const Position pos, int light );
	void putLight( const unsigned short x, const unsigned short y, const unsigned short z, int light );
	void addLight( unsigned int id, Position pos, int intensity );

	void updateSunlight( Position pos );
	bool hasSunlight( Position pos );
	void updateLightsInRange( Position pos );

	void removeLight( unsigned int id );

	void moveLight( unsigned int id, Position pos, int intensity );

	void loadFloorConstructions( QVariantList list );
	void loadWallConstructions( QVariantList list );

	bool checkTrapGnomeFloor( Position pos, Position workPos );

	void spreadIndirectSunlight( Position pos );
	void checkIndirectSunlightForNeighbors( Position pos );
	void checkIndirectSunlight( Position pos );

	void setWallSpriteAnim( Position pos, bool anim );

	QString getDebugWallConstruction( Position pos );
	QString getDebugFloorConstruction( Position pos );

	unsigned int getFurnitureOnTile( Position pos );

	bool isLineOfSight( Position a, Position b ) const;
};

#endif /* WORLD_H_ */
