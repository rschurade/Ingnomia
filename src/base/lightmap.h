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

#include "../base/position.h"
#include "../base/tile.h"

#include <vector>

struct Light
{
	unsigned int id;
	Position pos;
	int intensity;
	QList<unsigned int> effectTiles;
};

class LightMap
{
public:
	LightMap();
	~LightMap();

	void init();

	void addLight( QSet<unsigned int>& updateList, std::vector<Tile>& world, unsigned int id, Position pos, int intensity );
	void removeLight( QSet<unsigned int>& updateList, std::vector<Tile>& world, unsigned int id );
	void updateLight( QSet<unsigned int>& updateList, std::vector<Tile>& world, Position pos );

private:
	QMap<unsigned int, QMap<unsigned int, unsigned char>> m_lightMap;
	QMap<unsigned int, Light> m_lights;

	unsigned char calcIntensity( unsigned int posID );

	int m_dimX = 0;
	int m_dimY = 0;
	int m_dimZ = 0;

	Tile& getTile( std::vector<Tile>& world, unsigned short x, unsigned short y, unsigned short z )
	{
		return world[x + y * m_dimX + z * m_dimX * m_dimY];
	}
	Tile& getTile( std::vector<Tile>& world, Position pos )
	{
		return world[pos.x + pos.y * m_dimX + pos.z * m_dimX * m_dimY];
	}
};
