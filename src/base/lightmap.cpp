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
#include "lightmap.h"

#include "../base/config.h"
#include "../base/gamestate.h"

#include <QDebug>
#include <QList>
#include <QPair>
#include <QQueue>
#include <QVector3D>

LightMap::LightMap()
{
}

LightMap::~LightMap()
{
}

void LightMap::init()
{
	m_lightMap.clear();
	m_lights.clear();

	m_dimX = Global::dimX;
	m_dimY = Global::dimY;
	m_dimZ = Global::dimZ;
}

void LightMap::addLight( QSet<unsigned int>& updateList, std::vector<Tile>& world, unsigned int id, Position pos, int intensity )
{
	Light light;
	light.id        = id;
	light.pos       = pos;
	light.intensity = intensity;

	QQueue<QPair<Position, int>> wq;
	QSet<unsigned int> visited;
	wq.enqueue( QPair<Position, int>( pos, 0 ) );
	int decay = Global::cfg->get( "lightDecay" ).toInt();

	int range = intensity / decay;

	visited.reserve( range * range * range );

	while ( !wq.empty() )
	{
		QPair<Position, int> current = wq.dequeue();
		Position curPos              = current.first;
		unsigned int curPosID        = curPos.toInt();
		if ( !visited.contains( curPosID ) )
		{
			visited.insert( curPosID );
			// check line of sight to source
			QVector3D dir( curPos.x - pos.x, curPos.y - pos.y, curPos.z - pos.z );
			int curRadius = current.second;
			bool los      = true;
			for ( int i = 1; i <= curRadius - 1; ++i )
			{
				QVector3D offset = dir * ( (float)i / (float)curRadius );

				Tile& tile = getTile( world, pos.x + (int)( offset.x() ), pos.y + (int)( offset.y() ), pos.z + (int)( offset.z() ) );
				if ( tile.wallType & WallType::WT_VIEWBLOCKING )
				{
					los = false;
					break;
				}
			}
			if ( los )
			{
				int dist         = sqrt( pos.distSquare( curPos ) );
				int curIntensity = 0;
				if ( curPos.z == pos.z )
				{
					curIntensity = qMax( 0, ( intensity - decay * dist ) );
				}
				else
				{
					curIntensity = qMax( 0, ( intensity - decay * curRadius ) );
				}

				if ( curIntensity > 0 )
				{
					m_lightMap[curPosID].insert( id, curIntensity );

					Tile& tile      = getTile( world, curPos );
					tile.lightLevel = calcIntensity( curPosID );

					light.effectTiles.append( curPosID );
					updateList.insert( curPosID );

					if ( !( tile.wallType & WallType::WT_VIEWBLOCKING ) )
					{
						Position north( curPos.x, ( curPos.y == 0 ) ? 0 : curPos.y - 1, curPos.z );
						wq.enqueue( QPair<Position, int>( north, current.second + 1 ) );
						Position south( curPos.x, ( curPos.y == m_dimY - 1 ) ? m_dimY - 1 : curPos.y + 1, curPos.z );
						wq.enqueue( QPair<Position, int>( south, current.second + 1 ) );
						Position east( ( curPos.x == m_dimX - 1 ) ? m_dimX - 1 : curPos.x + 1, curPos.y, curPos.z );
						wq.enqueue( QPair<Position, int>( east, current.second + 1 ) );
						Position west( ( curPos.x == 0 ) ? 0 : curPos.x - 1, curPos.y, curPos.z );
						wq.enqueue( QPair<Position, int>( west, current.second + 1 ) );
						if ( !( tile.floorType & FloorType::FT_SOLIDFLOOR ) )
						{
							Position down( curPos.x, curPos.y, ( curPos.z == 0 ) ? 0 : curPos.z - 1 );
							wq.enqueue( QPair<Position, int>( down, current.second + 1 ) );
						}
						if ( !( getTile( world, pos.aboveOf() ).floorType & FloorType::FT_SOLIDFLOOR ) )
						{
							Position up( curPos.x, curPos.y, ( curPos.z == m_dimZ - 1 ) ? m_dimZ - 1 : curPos.z + 1 );
							wq.enqueue( QPair<Position, int>( up, current.second + 1 ) );
						}
					}
				}
			}
		}
	}
	m_lights.insert( id, light );
}

unsigned char LightMap::calcIntensity( unsigned int posID )
{
	if ( m_lightMap.contains( posID ) )
	{
		const auto& maps = m_lightMap[posID];
		int light = 0;
		for ( const auto& value : maps )
		{
			light += value;
		}
		return qMin( 255, light );
	}
	else
	{
		return 0;
	}
}

void LightMap::removeLight( QSet<unsigned int>& updateList, std::vector<Tile>& world, unsigned int id )
{
	if ( m_lights.contains( id ) )
	{
		Light light = m_lights[id];
		m_lights.remove( id );

		for ( auto posID : light.effectTiles )
		{
			m_lightMap[posID].remove( id );

			Tile& tile      = world[posID];
			tile.lightLevel = calcIntensity( posID );
			updateList.insert( posID );
		}
	}
}

void LightMap::updateLight( QSet<unsigned int>& updateList, std::vector<Tile>& world, Position pos )
{
	unsigned int posID = pos.toInt();
	if ( m_lightMap.contains( posID ) )
	{
		auto maps = m_lightMap[posID];

		for ( auto key : maps.keys() )
		{
			Light light = m_lights[key];

			removeLight( updateList, world, key );
			addLight( updateList, world, key, light.pos, light.intensity );
		}
	}
}