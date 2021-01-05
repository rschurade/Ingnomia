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

#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/selection.h"
#include "../base/util.h"
#include "../game/game.h"
#include "../game/world.h"
#include "../gfx/spritefactory.h"
#include "eventconnector.h"

#include <QDebug>

AggregatorSelection::AggregatorSelection( QObject* parent ) :
	QObject( parent )
{
	qRegisterMetaType<SelectionData>();
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
	m_width    = width;
	m_height   = height;
	m_moveX    = moveX;
	m_moveY    = moveY;
	m_scale    = scale;
	m_rotation = rotation;
}

void AggregatorSelection::onMouse( int mouseX, int mouseY, bool shift, bool ctrl )
{
	if ( Global::sel )
	{
		m_cursorPos = calcCursor( mouseX, mouseY, Global::sel->isFloor(), shift );
		Global::sel->updateSelection( m_cursorPos, shift, ctrl );
		Global::sel->setControlActive( ctrl );

		onUpdateCursorPos( m_cursorPos.toString() );
		updateSelection();
	}
}

void AggregatorSelection::onLeftClick( bool shift, bool ctrl )
{
	if ( Global::sel )
	{
		if ( Global::sel->hasAction() )
		{
			Global::sel->leftClick( m_cursorPos, shift, ctrl );
			Global::sel->updateSelection( m_cursorPos, shift, ctrl );
			updateSelection();
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
	if ( Global::sel )
	{
		Global::sel->rightClick( m_cursorPos );
		updateSelection();
	}
}

void AggregatorSelection::onRotateSelection()
{
	if ( Global::sel )
	{
		Global::sel->rotate();
		updateSelection();
	}
}

static bool isSelectableWall( const Position& pos )
{
	const auto w     = Global::eventConnector->game()->w();
	const auto& tile = w->getTile( pos );
	if ( tile.wallType & WallType::WT_SOLIDWALL )
	{
		return true;
	}
	if ( Global::showJobs && w->jobSprite( pos ).contains( "Wall" ) )
	{
		return true;
	}
	return false;
}

static bool isSelectableFloor( const Position& pos, bool snapToWallBelow )
{
	const auto w     = Global::eventConnector->game()->w();
	const auto& tile = w->getTile( pos );
	if ( tile.floorType != FloorType::FT_NOFLOOR )
	{
		return true;
	}
	const auto& tileBelow = w->getTile( pos.belowOf() );
	if ( snapToWallBelow )
	{
		if ( isSelectableWall( pos.belowOf() ) )
		{
			return true;
		}
	}
	if ( Global::showJobs && w->jobSprite( pos ).contains( "Floor" ) )
	{
		return true;
	}
	return false;
}

Position AggregatorSelection::calcCursor( int mouseX, int mouseY, bool isFloor, bool useViewLevel ) const
{
	if ( !Global::sel || !Global::eventConnector || !Global::eventConnector->game() || !Global::eventConnector->game()->w() )
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

				if ( !Global::wallsLowered && cursorPos.valid() && isSelectableWall( cursorPos.seOf() ) )
				{
					cursorPos.x += 1;
					cursorPos.y += 1;
				}

				break;
			case 1:
				cursorPos.x = qMin( qMax( 0, selY - zDiff - 1 ), dim - 1 );
				cursorPos.y = qMin( qMax( 0, dim - selX + zDiff ), dim - 1 );
				cursorPos.z = qMin( qMax( 0, viewLevel ), dimZ - 1 );
				if ( !Global::wallsLowered && cursorPos.valid() && isSelectableWall( cursorPos.neOf() ) )
				{
					cursorPos.x += 1;
					cursorPos.y -= 1;
				}
				break;
			case 2:
				cursorPos.x = qMin( qMax( 0, dim - selX + zDiff ), dim - 1 );
				cursorPos.y = qMin( qMax( 0, dim - selY + zDiff ), dim - 1 );
				cursorPos.z = qMin( qMax( 0, viewLevel ), dimZ - 1 );
				if ( !Global::wallsLowered && cursorPos.valid() && isSelectableWall( cursorPos.nwOf() ) )
				{
					cursorPos.x -= 1;
					cursorPos.y -= 1;
				}
				break;
			case 3:
				cursorPos.x = qMin( qMax( 0, dim - selY + zDiff ), dim - 1 );
				cursorPos.y = qMin( qMax( 0, selX - zDiff - 1 ), dim - 1 );
				cursorPos.z = qMin( qMax( 0, viewLevel ), dimZ - 1 );
				if ( !Global::wallsLowered && cursorPos.valid() && isSelectableWall( cursorPos.swOf() ) )
				{
					cursorPos.x -= 1;
					cursorPos.y += 1;
				}
				break;
		}

		if ( cursorPos.z > 0 )
		{
			if ( isSelectableFloor(cursorPos, isFloor ) )
			{
				zFloorFound = true;
			}
			else if ( useViewLevel )
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

void AggregatorSelection::updateSelection()
{
	if ( Global::sel->changed() )
	{
		//DebugScope s( "update selection" );

		QString action = Global::sel->action();
		m_selectionData.clear();
		if ( !action.isEmpty() )
		{
			bool isFloor = Global::sel->isFloor();

			QList<QPair<Position, bool>> selection = Global::sel->getSelection();

			QList<QPair<Sprite*, QPair<Position, unsigned char>>> sprites;
			QList<QPair<Sprite*, QPair<Position, unsigned char>>> spritesInv;

			int rotation = Global::sel->rotation();

			QList<QVariantMap> spriteIDs;

			if ( action == "BuildWall" || action == "BuildFancyWall" || action == "BuildFloor" || action == "BuildFancyFloor" || action == "BuildRamp" || action == "BuildRampCorner" || action == "BuildStairs" )
			{
				spriteIDs = DB::selectRows( "Constructions_Sprites", "ID", Global::sel->itemID() );
			}
			else if ( action == "BuildWorkshop" )
			{
				spriteIDs = DB::selectRows( "Workshops_Components", "ID", Global::sel->itemID() );
			}
			else if ( action == "BuildItem" )
			{
				QVariantMap sprite;
				sprite.insert( "SpriteID", DBH::spriteID( Global::sel->itemID() ) );
				sprite.insert( "Offset", "0 0 0" );
				sprite.insert( "Type", "Furniture" );
				sprite.insert( "Material", Global::sel->material() );
				spriteIDs.push_back( sprite );
			}
			else
			{
				spriteIDs = DB::selectRows( "Actions_Tiles", "ID", action );
			}
			for ( auto asi : spriteIDs )
			{
				QVariantMap entry = asi;
				if ( !entry.value( "SpriteID" ).toString().isEmpty() )
				{
					if ( entry.value( "SpriteID" ).toString() == "none" )
					{
						continue;
					}
					if ( !entry.value( "SpriteIDOverride" ).toString().isEmpty() )
					{
						entry.insert( "SpriteID", entry.value( "SpriteIDOverride" ).toString() );
					}

					// TODO repair rot
					unsigned char localRot = Global::util->rotString2Char( entry.value( "WallRotation" ).toString() );

					Sprite* addSpriteValid = nullptr;

					QStringList mats;
					for ( auto mv : entry.value( "Material" ).toList() )
					{
						mats.push_back( mv.toString() );
					}

					if ( entry.contains( "Material" ) )
					{
						addSpriteValid = Global::eventConnector->game()->sf()->createSprite( entry["SpriteID"].toString(), mats );
					}
					else
					{
						addSpriteValid = Global::eventConnector->game()->sf()->createSprite( entry["SpriteID"].toString(), { "None" } );
					}
					Position offset( 0, 0, 0 );
					if ( entry.contains( "Offset" ) )
					{
						QString os      = entry["Offset"].toString();
						QStringList osl = os.split( " " );

						if ( osl.size() == 3 )
						{
							offset.x = osl[0].toInt();
							offset.y = osl[1].toInt();
							offset.z = osl[2].toInt();
						}
						int rotX = offset.x;
						int rotY = offset.y;
						switch ( rotation )
						{
							case 1:
								offset.x = -1 * rotY;
								offset.y = rotX;
								break;
							case 2:
								offset.x = -1 * rotX;
								offset.y = -1 * rotY;
								break;
							case 3:
								offset.x = rotY;
								offset.y = -1 * rotX;
								break;
						}
					}
					sprites.push_back( QPair<Sprite*, QPair<Position, unsigned char>>( addSpriteValid, { offset, localRot } ) );
					spritesInv.push_back( QPair<Sprite*, QPair<Position, unsigned char>>( addSpriteValid, { offset, localRot } ) );
				}
				else
				{
					QString os      = entry["Offset"].toString();
					QStringList osl = os.split( " " );
					Position offset( 0, 0, 0 );
					if ( osl.size() == 3 )
					{
						offset.x = osl[0].toInt();
						offset.y = osl[1].toInt();
						offset.z = osl[2].toInt();
					}
					int rotX = offset.x;
					int rotY = offset.y;
					switch ( rotation )
					{
						case 1:
							offset.x = -1 * rotY;
							offset.y = rotX;
							break;
						case 2:
							offset.x = -1 * rotX;
							offset.y = -1 * rotY;
							break;
						case 3:
							offset.x = rotY;
							offset.y = -1 * rotX;
							break;
					}
					sprites.push_back( QPair<Sprite*, QPair<Position, unsigned char>>( Global::eventConnector->game()->sf()->createSprite( "SolidSelectionFloor", { "None" } ), { offset, 0 } ) );
					spritesInv.push_back( QPair<Sprite*, QPair<Position, unsigned char>>( Global::eventConnector->game()->sf()->createSprite( "SolidSelectionFloor", { "None" } ), { offset, 0 } ) );
				}
			}
			unsigned int tileID = 0;
			for ( auto p : selection )
			{
				if ( p.second )
				{
					for ( auto as : sprites )
					{
						if ( as.first )
						{
							SelectionData sd;
							sd.spriteID = as.first->uID;
							sd.localRot = ( ( rotation + as.second.second ) % 4 );
							sd.pos      = Position( p.first + as.second.first );
							sd.pos.setToBounds();
							sd.isFloor = isFloor;
							sd.valid   = true;
							m_selectionData.insert( posToInt( sd.pos, m_rotation ), sd );
						}
					}
				}
				else
				{
					for ( auto as : spritesInv )
					{
						if ( as.first )
						{
							SelectionData sd;
							sd.spriteID = as.first->uID;
							sd.localRot = ( ( rotation + as.second.second ) % 4 );
							sd.pos      = Position( p.first + as.second.first );
							sd.pos.setToBounds();
							sd.isFloor = isFloor;
							sd.valid   = false;
							m_selectionData.insert( posToInt( sd.pos, m_rotation ), sd );
						}
					}
				}
			}
		}
		bool noDepthTest = ( action == "DigStairsDown" || action == "DigRampDown" );

		emit signalUpdateSelection( m_selectionData, noDepthTest );
	}
}

unsigned int AggregatorSelection::posToInt( Position pos, quint8 rotation )
{
	//return x + Global::dimX * y + Global::dimX * Global::dimX * z;

	switch ( rotation )
	{
		case 0:
			return pos.toInt();
			break;
		case 1:
			return pos.x + Global::dimX * ( Global::dimX - pos.y ) + Global::dimX * Global::dimX * pos.z;
			break;
		case 2:
			return ( Global::dimX - pos.x ) + Global::dimX * ( Global::dimX - pos.y ) + Global::dimX * Global::dimX * pos.z;
			break;
		case 3:
			return ( Global::dimX - pos.x ) + Global::dimX * pos.y + Global::dimX * Global::dimX * pos.z;
			break;
	}
	return 0;
}