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

#include "../../3rdparty/fastnoise/FastNoise.h"

#include <QMap>
#include <QObject>
#include <QVector>

#include <vector>

class Grass;
struct Tile;
struct EmbeddedMaterial;
struct TerrainMaterial;
struct Position;
class NewGameSettings;
class World;
class Game;

class WorldGenerator : public QObject
{
	Q_OBJECT

public:
	WorldGenerator( NewGameSettings* ngs, Game* parent );
	~WorldGenerator();

	World* generateTopology();
	void addLife();

private:
	QPointer<Game> g;
	// resize world and fill stone layers
	void setStoneLayers();
	// set metal ores and gems
	void setMetalsAndGems();
	// set water and sand floor at water
	void setWater();
	// set sunlight and grass
	void initSunLight();
	// add plants and trees
	void addPlantsAndTrees();
	// add animals
	void addAnimals();
	// add gnomes and starting items
	void addGnomesAndStartingItems();
	QString getRandomMaterial( QString itemSID );

	void initMateralVectors();

	void fillFloor( int z, QVector<TerrainMaterial>& mats, QVector<int>& matsinLevel );

	void fillFloorMushroomBiome( int z, QVector<TerrainMaterial>& mats, QVector<int>& matsinLevel );

	void discoverAll();

	void createRamps();
	void createRamp( int z );

	void createOceanFront();
	void decreaseHeight( int x, int y, int diff );
	void setSandFloor( int x, int y, int sandRowID );
	int getLowestZonXLine( int x );
	int getLowestZonYLine( int y );
	void fillWater( int x, int y, int z );

	void createRivers();
	std::vector<Position> riverWorm( Position pos, int dir, int num, int maxLength );
	void carveRiver( std::vector<Tile>& world, Position& pos );

	void placeLairs();
	bool checkPlacement( int xLoc, int yLoc, int zLoc, int xSize, int ySize, int zSize );

	bool fBm( int x, int y, int z );

	std::vector<Position> perlinWorm( int z, int num, int maxLength );
	void clear3x3( std::vector<Tile>& world, Position& pos );
	void setEmbedded3x3( std::vector<Tile>& world, Position& pos, unsigned short embeddedMaterial, unsigned short spriteID );

	float perlinRandWhiteNoise( int x, int y, int z = -1 );
	QString getRandomEmbedded( int x, int y, int z, QMap<QString, EmbeddedMaterial>& em );

	void createHeightMap( int dimX, int dimY );

	QVector<int> m_heightMap;
	QVector<float> m_heightMap2;

	int m_seed = 1337;

	float m_min;
	float m_max;

	int m_dimX;
	int m_dimY;
	int m_dimZ;

	int m_groundLevel;

	int m_mushroomLevel = 0;

	FastNoise m_random;

	QVector<TerrainMaterial> m_mats;
	QVector<int> m_matsInLevel;

	bool m_fow = true;

	World* w = nullptr;
	NewGameSettings* ngs = nullptr;
signals:
	void signalStatus( QString msg );
};
