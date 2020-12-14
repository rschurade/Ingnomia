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
#include "aggregatorselection.h"
#include "eventconnector.h"

#include "../base/global.h"
#include "../base/gamestate.h"
#include "../base/selection.h"

#include "../game/game.h"
#include "../game/world.h"

#include <QDebug>

AggregatorSelection::AggregatorSelection( QObject* parent ) :
	QObject(parent)
{
	
}

AggregatorSelection::~AggregatorSelection()
{
}

void AggregatorSelection::onActionChanged( const QString action )
{
    emit signalAction( action );
}

void AggregatorSelection::onUpdateCursorPos( const QString pos )
{
    emit signalCursorPos( pos );
}

void AggregatorSelection::onUpdateFirstClick( const QString pos )
{
    emit signalFirstClick( pos );
}

void AggregatorSelection::onUpdateSize( const QString size )
{
    emit signalSize( size );
}

void AggregatorSelection::onRenderParams( int width, int height, int moveX, int moveY, float scale, int rotation )
{
	m_width = width;
    m_height = height;
    m_moveX = moveX;
    m_moveY = moveY;
    m_scale = scale;
    m_rotation = rotation;
}

void AggregatorSelection::onMouse( int mouseX, int mouseY, bool shift, bool ctrl )
{
	if( Global::sel )
	{
		m_cursorPos = calcCursor( mouseX, mouseY, Global::sel->isFloor(), shift );
		Global::sel->updateSelection( m_cursorPos, shift, ctrl );
		Global::sel->setControlActive( ctrl );

		onUpdateCursorPos( m_cursorPos.toString() );
	}
}

void AggregatorSelection::onLeftClick( bool shift, bool ctrl )
{
	if( Global::sel )
	{
		if ( Global::sel->hasAction() )
		{
			Global::sel->leftClick( m_cursorPos, shift, ctrl );										
		}
		else
		{
			unsigned int tileID = m_cursorPos.toInt();
			emit signalSelectTile( tileID );
		}
	}
}
    
void AggregatorSelection::onRightClick()
{
	if( Global::sel )
	{
		Global::sel->rightClick( m_cursorPos );
	}
}

void AggregatorSelection::onRotateSelection()
{
	if( Global::sel )
	{
		Global::sel->rotate();
	}
}

Position AggregatorSelection::calcCursor( int mouseX, int mouseY, bool isFloor, bool useViewLevel ) const
{
	if( !Global::sel || !Global::eventConnector || !Global::eventConnector->game() || !Global::eventConnector->game()->w() )
	{
		return Position( 0, 0, 0 );
	}

	Position cursorPos;
	int dim = Global::dimX;
	if ( dim == 0 )
	{
		cursorPos = Position( 0, 0, 0 );
		return cursorPos;
	}
	int viewLevel = GameState::viewLevel;
	int w2        = ( m_width / m_scale ) / 2;
	int h2        = ( m_height / m_scale ) / 2;

	int x0 = m_moveX + w2;
	int y0 = m_moveY + h2 + 360;

	int rot = m_rotation;

	int dimZ = Global::dimZ;

	bool zFloorFound = false;

	int origViewLevel = viewLevel;
	int zDiff         = 0;
	while ( !zFloorFound && zDiff < 20 )
	{
		zDiff  = origViewLevel - viewLevel;
		int z0 = qMax( 0, viewLevel - ( viewLevel - 20 ) );

		int mouseXScaled = (double)( mouseX ) / m_scale;
		int mouseYScaled = (double)( mouseY ) / m_scale - ( zDiff * 4 ) - 3;

		int column = ( mouseXScaled - x0 ) / 32;
		if ( mouseXScaled < x0 )
			column -= 1;
		int row = ( mouseYScaled - y0 - 8 + z0 * 20 ) / 16;

		float quadX = ( mouseXScaled - x0 ) % 32;
		float quadY = ( ( mouseYScaled - y0 - 8 + z0 * 20 ) % 16 ) * 2;

		if ( quadX < 0 )
		{
			if ( quadX == -32 )
				quadX = 0;
			quadX = 31 + quadX;
		}
		if ( quadY < 0 )
			quadY = 31 + quadY;

		bool lower  = ( quadX / quadY ) >= 1.0;
		bool lower2 = ( ( 32. - quadX ) / quadY ) > 1.0;

		bool north = lower && lower2;
		bool south = !lower && !lower2;
		bool east  = lower && !lower2;
		bool west  = !lower && lower2;

		int selX = 0;
		int selY = 0;

		if ( south )
		{
			//qDebug() << "south" << quadX << " " << quadY << " " << row << " " <<  column;
			selX = row + 1 + column;
			selY = row + 1 - column;
		}
		if ( west )
		{
			//qDebug() << "west" << quadX << " " << quadY << " " << row << " " <<  column;
			selX = row + column;
			selY = row + 1 - column;
		}
		if ( north )
		{
			//qDebug() << "north" << quadX << " " << quadY << " " << row << " " <<  column;
			selX = row + column;
			selY = row - column;
		}
		if ( east )
		{
			//qDebug() << "east" << quadX << " " << quadY << " " << row << " " <<  column;
			selX = row + 1 + column;
			selY = row - column;
		}

		switch ( rot )
		{
			case 0:
				cursorPos.x = qMin( qMax( 0, selX - zDiff - 1 ), dim - 1 );
				cursorPos.y = qMin( qMax( 0, selY - zDiff - 1 ), dim - 1 );
				cursorPos.z = qMin( qMax( 0, viewLevel ), dimZ - 1 );

				if ( !Global::wallsLowered && cursorPos.valid() && Global::eventConnector->game()->w()->getTile( cursorPos.seOf() ).wallType & WallType::WT_SOLIDWALL )
				{
					cursorPos.x += 1;
					cursorPos.y += 1;
				}

				break;
			case 1:
				cursorPos.x = qMin( qMax( 0, selY - zDiff - 1 ), dim - 1 );
				cursorPos.y = qMin( qMax( 0, dim - selX + zDiff ), dim - 1 );
				cursorPos.z = qMin( qMax( 0, viewLevel ), dimZ - 1 );
				if ( !Global::wallsLowered && cursorPos.valid() && Global::eventConnector->game()->w()->getTile( cursorPos.neOf() ).wallType & WallType::WT_SOLIDWALL )
				{
					cursorPos.x += 1;
					cursorPos.y -= 1;
				}
				break;
			case 2:
				cursorPos.x = qMin( qMax( 0, dim - selX + zDiff ), dim - 1 );
				cursorPos.y = qMin( qMax( 0, dim - selY + zDiff ), dim - 1 );
				cursorPos.z = qMin( qMax( 0, viewLevel ), dimZ - 1 );
				if ( !Global::wallsLowered && cursorPos.valid() && Global::eventConnector->game()->w()->getTile( cursorPos.nwOf() ).wallType & WallType::WT_SOLIDWALL )
				{
					cursorPos.x -= 1;
					cursorPos.y -= 1;
				}
				break;
			case 3:
				cursorPos.x = qMin( qMax( 0, dim - selY + zDiff ), dim - 1 );
				cursorPos.y = qMin( qMax( 0, selX - zDiff - 1 ), dim - 1 );
				cursorPos.z = qMin( qMax( 0, viewLevel ), dimZ - 1 );
				if ( !Global::wallsLowered && cursorPos.valid() && Global::eventConnector->game()->w()->getTile( cursorPos.swOf() ).wallType & WallType::WT_SOLIDWALL )
				{
					cursorPos.x -= 1;
					cursorPos.y += 1;
				}
				break;
		}

		const Tile& tile = Global::eventConnector->game()->w()->getTile( cursorPos );
		if ( cursorPos.z > 0 )
		{
			Tile& tileBelow = Global::eventConnector->game()->w()->getTile( cursorPos.x, cursorPos.y, cursorPos.z - 1 );
			if( isFloor && tile.floorType == FloorType::FT_NOFLOOR && tileBelow.wallType != WallType::WT_NOWALL )
			{
				zFloorFound = true;
			}
			else if ( tile.floorType != FloorType::FT_NOFLOOR || tileBelow.wallType == WallType::WT_SOLIDWALL || useViewLevel )
			{
				zFloorFound = true;
			}
			else
			{
				--viewLevel;
				if ( viewLevel == 1 )
				{
					zFloorFound = true;
				}
			}
		}
		else
		{
			zFloorFound = true;
		}
	}

	if ( useViewLevel )
	{
		cursorPos.z = viewLevel;
	}
	return cursorPos;
}