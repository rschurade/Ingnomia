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

#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/util.h"
#include "../game/creaturemanager.h"
#include "../game/farmingmanager.h"
#include "../game/fluidmanager.h"
#include "../game/gnomemanager.h"
#include "../game/inventory.h"
#include "../game/mechanismmanager.h"
#include "../game/roommanager.h"
#include "../game/stockpilemanager.h"
#include "../game/workshopmanager.h"
#include "../gfx/sprite.h"
#include "../gfx/spritefactory.h"
#include "world.h"

//#include <QDebug>
#include <QJsonDocument>

const int ROT_BIT  = 65536;
const int ANIM_BIT = 262144;
const int WALL_BIT = 524288;

bool World::constructWorkshop( QString constructionSID, Position pos, int rotation, QList<unsigned int> itemUIDs, Position extractTo )
{
	if ( itemUIDs.empty() )
	{
		qDebug() << "World::constructWorkshop() - source item list is empty!";
		return false;
	}
	//qDebug() << "world::constructWorkshop() " << constructionSID << pos.toString() << rotation;
	QVariantMap con = DB::selectRow( "Workshops", constructionSID );

	constructWorkshop( con, pos, rotation, Util::uintList2Variant( itemUIDs ), extractTo );
	return true;
}

bool World::construct( QString constructionSID, Position pos, int rotation, QList<unsigned int> itemIDs, Position extractTo )
{
	if ( itemIDs.empty() )
	{
		qDebug() << "World::construct() - source item list is empty!";
		return false;
	}

	//qDebug() << "world::construct() " << constructionSID << pos.toString() << rotation;
	QVariantMap con = DB::selectRow( "Constructions", constructionSID );
	QString type    = con.value( "Type" ).toString();
	int typeNum     = m_constructionSID2ENUM.value( type );

	QStringList materialSIDs;
	QVariantList materialUIDs;
	QVariantList itemUIDs;
	for ( auto vItem : itemIDs )
	{
		if ( Global::inv().itemExists( vItem ) )
		{
			materialSIDs.append( Global::inv().materialSID( vItem ) );
			materialUIDs.append( Global::inv().materialUID( vItem ) );
			itemUIDs.append( Global::inv().itemUID( vItem ) );
		}
		else
		{
			qDebug() << "Souce item no longer exists!" << vItem;
			return false;
		}
	}

	bool result = false;

	switch ( typeNum )
	{
		case CID_WALL:
			return constructWall( con, pos, rotation, itemUIDs, materialUIDs, materialSIDs, extractTo );
			break;
		case CID_FANCYWALL:
			return constructWall( con, pos, rotation, itemUIDs, materialUIDs, materialSIDs, extractTo );
			break;
		case CID_FENCE:
			return constructFence( con, pos, rotation, itemUIDs, materialUIDs, materialSIDs, extractTo );
			break;
		case CID_FLOOR:
			return constructFloor( con, pos, rotation, itemUIDs, materialUIDs, materialSIDs, extractTo );
			break;
		case CID_FANCYFLOOR:
			return constructFloor( con, pos, rotation, itemUIDs, materialUIDs, materialSIDs, extractTo );
			break;
		case CID_STAIRS:
			return constructStairs( con, pos, rotation, itemUIDs, materialUIDs, materialSIDs, extractTo );
			break;
		case CID_RAMP:
			return constructRamp( con, pos, rotation, itemUIDs, materialUIDs, materialSIDs, extractTo );
			break;
		case CID_RAMPCORNER:
			return constructRampCorner( con, pos, rotation, itemUIDs, materialUIDs, materialSIDs, extractTo );
			break;
		case CID_SCAFFOLD:
			return constructScaffold( con, pos, rotation, itemUIDs, materialUIDs, materialSIDs, extractTo );
			break;
		case CID_WALLFLOOR:
			return constructWallFloor( con, pos, rotation, itemUIDs, materialUIDs, materialSIDs, extractTo );
	}

	return result;
}

bool World::constructWall( QVariantMap& con, Position pos, int rotation, QVariantList itemUIDs, QVariantList materialUIDs, QStringList materialSIDs, Position extractTo )
{
	QList<Position> updateCoords;
	QVariantList positions;

	auto spl = DB::selectRows( "Constructions_Sprites", "ID", con.value( "ID" ).toString() );
	for ( auto sp : spl )
	{
		QVariantMap spMap = sp;
		Position offset( spMap.value( "Offset" ).toString() );
		Position constrPos( pos + offset );

		Tile& tile = getTile( constrPos );

		if ( tile.wallType & WallType::WT_ROUGH )
		{
			auto mats = mineWall( pos, extractTo );
			Util::createRawMaterialItem( extractTo, mats.first );
			Util::createRawMaterialItem( extractTo, mats.second );
		}
		if ( m_wallConstructions.contains( pos.toInt() ) )
		{
			deconstruct( pos, extractTo, true );
		}

		QString spriteSID      = spMap.value( "SpriteID" ).toString();
		unsigned int spriteUID = Global::sf().createSprite( spriteSID, materialSIDs )->uID;
		tile.wallSpriteUID     = spriteUID;

		if ( con.value( "NoConstruction" ).toBool() )
		{
			tile.wallType = ( WallType )( WallType::WT_SOLIDWALL | WallType::WT_ROUGH | WallType::WT_VIEWBLOCKING | WallType::WT_MOVEBLOCKING );
		}
		else
		{
			tile.wallType = ( WallType )( WallType::WT_SOLIDWALL | WallType::WT_VIEWBLOCKING | WallType::WT_MOVEBLOCKING | WallType::WT_CONSTRUCTED );

			positions.append( "W" );
			positions.append( constrPos.toString() );
		}

		tile.wallMaterial     = materialUIDs.first().toUInt();
		tile.embeddedMaterial = 0;
		tile.itemSpriteUID    = 0;
		tile.wallRotation     = rotation;
		clearTileFlag( constrPos, TileFlag::TF_WALKABLE );
		removeGrass( constrPos );
		if ( tile.lightLevel > 0 )
		{
			updateLightsInRange( constrPos );
		}

		//GameState::addChange( NetworkCommand::SETTILEFLAGS, { QString::number( tid ), Util::tile2String( tile ) } );
		updateCoords.push_back( constrPos );
	}

	if ( !con.value( "NoConstruction" ).toBool() )
	{
		QVariantMap constr;
		constr.insert( "Pos", pos.toString() );
		if ( positions.size() > 2 )
		{
			constr.insert( "Positions", positions );
		}
		else
		{
			constr.insert( "T", "W" );
		}
		constr.insert( "Items", itemUIDs );
		constr.insert( "Materials", materialUIDs );

		m_wallConstructions.insert( pos.toInt(), constr );
	}

	updateNavigation( updateCoords, extractTo );

	return true;
}

bool World::constructFloor( QVariantMap& con, Position pos, int rotation, QVariantList itemUIDs, QVariantList materialUIDs, QStringList materialSIDs, Position extractTo )
{
	QList<Position> updateCoords;
	QVariantList positions;

	auto spl = DB::selectRows( "Constructions_Sprites", "ID", con.value( "ID" ).toString() );
	for ( auto sp : spl )
	{
		QVariantMap spMap = sp;
		Position offset( spMap.value( "Offset" ).toString() );

		Position constrPos( pos + offset );

		Tile& tile = getTile( constrPos );

		if ( (bool)( tile.floorType & FloorType::FT_SOLIDFLOOR ) && !(bool)( tile.floorType & FloorType::FT_CONSTRUCTION ) )
		{
			auto mat = floorMaterial( constrPos );
			//auto mats = removeFloor( pos, extractTo );
			Util::createRawMaterialItem( extractTo, mat );
		}
		if ( m_floorConstructions.contains( pos.toInt() ) )
		{
			auto constr = m_floorConstructions.value( pos.toInt() );
			m_floorConstructions.remove( pos.toInt() );
			deconstruct2( constr, pos, true, extractTo, false );
		}

		QString spriteSID      = spMap.value( "SpriteID" ).toString();
		unsigned int spriteUID = Global::sf().createSprite( spriteSID, materialSIDs )->uID;

		tile.floorSpriteUID = spriteUID;
		if ( con.value( "NoConstruction" ).toBool() )
		{
			tile.floorType = ( FloorType )( FloorType::FT_SOLIDFLOOR );
		}
		else
		{
			tile.floorType = ( FloorType )( FloorType::FT_SOLIDFLOOR | FloorType::FT_CONSTRUCTION );

			positions.append( "F" );
			positions.append( constrPos.toString() );
		}

		tile.floorMaterial = materialUIDs.first().toUInt();
		tile.floorRotation = rotation;
		tile.flags += TileFlag::TF_WALKABLE;
		removeGrass( constrPos );

		if ( tile.flags & TileFlag::TF_SUNLIGHT )
		{
			//tile has sunlight, now we need to remove it below
			int z = constrPos.z - 1;
			while ( z > 0 )
			{
				Tile& tileBelow = getTile( constrPos.x, constrPos.y, z );
				if ( tileBelow.flags & TileFlag::TF_SUNLIGHT )
				{
					clearTileFlag( Position( constrPos.x, constrPos.y, z ), TileFlag::TF_SUNLIGHT );
					checkIndirectSunlightForNeighbors( Position( constrPos.x, constrPos.y, z ) );
				}
				else
				{
					break;
				}
				--z;
			}
		}

		//GameState::addChange( NetworkCommand::SETTILEFLAGS, { QString::number( tid ), Util::tile2String( tile ) } );
		updateCoords.push_back( constrPos );
	}

	if ( !con.value( "NoConstruction" ).toBool() )
	{
		QVariantMap constr;
		constr.insert( "Pos", pos.toString() );
		if ( positions.size() > 2 )
		{
			constr.insert( "Positions", positions );
		}
		else
		{
			constr.insert( "T", "F" );
		}
		constr.insert( "Items", itemUIDs );
		constr.insert( "Materials", materialUIDs );
		m_floorConstructions.insert( pos.toInt(), constr );
	}

	isGrassCandidate( pos );

	updateNavigation( updateCoords, extractTo );

	return true;
}

bool World::constructFence( QVariantMap& con, Position pos, int rotation, QVariantList itemUIDs, QVariantList materialUIDs, QStringList materialSIDs, Position extractTo )
{
	QList<Position> updateCoords;
	QVariantList positions;
	QString type;

	auto spl = DB::selectRows( "Constructions_Sprites", "ID", con.value( "ID" ).toString() );
	for ( auto sp : spl )
	{
		Position offset( sp.value( "Offset" ).toString() );
		Position constrPos( pos + offset );

		positions.append( "W" );
		positions.append( constrPos.toString() );

		unsigned int tid = constrPos.toInt();
		Tile& tile       = getTile( tid );

		type = sp.value( "Type" ).toString();

		if ( type == "Wall" )
		{
			tile.wallType     = ( WallType )( WallType::WT_SOLIDWALL | WallType::WT_VIEWBLOCKING | WallType::WT_MOVEBLOCKING | WallType::WT_CONSTRUCTED );
			tile.wallMaterial = materialUIDs.first().toUInt();
			tile.wallRotation = 0;
			clearTileFlag( constrPos, TileFlag::TF_WALKABLE );
			if ( tile.lightLevel > 0 )
			{
				updateLightsInRange( constrPos );
			}
		}
		else if ( type == "WallSeeThrough" )
		{
			tile.wallType     = ( WallType )( WallType::WT_MOVEBLOCKING | WallType::WT_CONSTRUCTED );
			tile.wallMaterial = materialUIDs.first().toUInt();
			tile.wallRotation = 0;
			clearTileFlag( constrPos, TileFlag::TF_WALKABLE );
		}

		GameState::addChange( NetworkCommand::SETTILEFLAGS, { QString::number( tid ), Util::tile2String( tile ) } );
		updateCoords.push_back( constrPos );
	}

	QVariantMap constr;
	constr.insert( "ConstructionID", con.value( "ID" ).toString() );
	constr.insert( "Pos", pos.toString() );
	if ( positions.size() > 2 )
	{
		constr.insert( "Positions", positions );
	}
	else
	{
		constr.insert( "T", "W" );
	}
	constr.insert( "Items", itemUIDs );
	constr.insert( "Materials", materialUIDs );

	m_wallConstructions.insert( pos.toInt(), constr );

	updateFenceSprite( pos );
	updateFenceSprite( pos.northOf() );
	updateFenceSprite( pos.eastOf() );
	updateFenceSprite( pos.southOf() );
	updateFenceSprite( pos.westOf() );

	updateNavigation( updateCoords, extractTo );

	return true;
}

bool World::constructPipe( QString itemSID, Position pos, unsigned int itemUID )
{
	QVariantMap constr;
	constr.insert( "ConstructionID", "Item" );
	constr.insert( "Pos", pos.toString() );
	constr.insert( "Item", itemUID );
	constr.insert( "Type", "Hydraulics" );

	m_wallConstructions.insert( pos.toInt(), constr );

	setTileFlag( pos, TileFlag::TF_PIPE );
	Tile& tile        = getTile( pos );
	tile.wallMaterial = Global::inv().materialUID( itemUID );
	tile.wallType     = WT_CONSTRUCTED;

	updatePipeSprite( pos );
	updatePipeSprite( pos.northOf() );
	updatePipeSprite( pos.eastOf() );
	updatePipeSprite( pos.southOf() );
	updatePipeSprite( pos.westOf() );

	if ( itemSID == "Pipe" )
	{
		Global::flm().addPipe( pos, itemUID );
	}
	else if ( itemSID == "PipeExit" )
	{
		Global::flm().addOutput( pos, itemUID );
	}
	else if ( itemSID == "Pump" )
	{
		Global::flm().addInput( pos, itemUID );
		Global::mcm().installItem( itemUID, pos, 0 );
	}

	return true;
}

bool World::deconstructPipe( QVariantMap constr, Position decPos, Position workPos )
{
	Tile& tile         = getTile( decPos );
	tile.wallType      = ( WallType )( WallType::WT_NOWALL );
	tile.wallSpriteUID = 0;
	clearTileFlag( decPos, TileFlag::TF_PIPE );

	updatePipeSprite( decPos.northOf() );
	updatePipeSprite( decPos.eastOf() );
	updatePipeSprite( decPos.southOf() );
	updatePipeSprite( decPos.westOf() );

	unsigned itemID = constr.value( "Item" ).toUInt();
	Global::inv().setConstructedOrEquipped( itemID, false );
	Global::inv().moveItemToPos( itemID, workPos );

	QString itemSID = Global::inv().itemSID( constr.value( "Item" ).toUInt() );

	Global::flm().removeAt( decPos );

	if ( itemSID == "Pump" )
	{
		Global::mcm().uninstallItem( constr.value( "Item" ).toUInt() );
	}

	return true;
}

bool World::constructWallFloor( QVariantMap& con, Position pos, int rotation, QVariantList itemUIDs, QVariantList materialUIDs, QStringList materialSIDs, Position extractTo )
{
	QList<Position> updateCoords;
	QVariantList positions;

	auto spl = DB::selectRows( "Constructions_Sprites", "ID", con.value( "ID" ).toString() );
	for ( auto sp : spl )
	{
		QVariantMap spMap = sp;
		Position offset( spMap.value( "Offset" ).toString() );

		Position constrPos( pos + offset );

		Tile& tile = getTile( constrPos );

		QString spriteSID      = spMap.value( "SpriteID" ).toString();
		unsigned int spriteUID = Global::sf().createSprite( spriteSID, materialSIDs )->uID;

		if ( spMap.value( "Type" ).toString() == "Wall" )
		{
			if ( con.value( "NoConstruction" ).toBool() )
			{
				tile.wallType = ( WallType )( WallType::WT_SOLIDWALL | WallType::WT_ROUGH | WallType::WT_VIEWBLOCKING | WallType::WT_MOVEBLOCKING );
			}
			else
			{
				tile.wallType = ( WallType )( WallType::WT_SOLIDWALL | WallType::WT_VIEWBLOCKING | WallType::WT_MOVEBLOCKING | WallType::WT_CONSTRUCTED );

				positions.append( "W" );
				positions.append( constrPos.toString() );
			}

			tile.wallSpriteUID = spriteUID;
			tile.wallMaterial  = materialUIDs.first().toUInt();
			tile.wallRotation  = rotation;
			clearTileFlag( constrPos, TileFlag::TF_WALKABLE );
			removeGrass( constrPos );
			if ( tile.lightLevel > 0 )
			{
				updateLightsInRange( constrPos );
			}
		}
		else if ( spMap.value( "Type" ).toString() == "Floor" )
		{
			tile.floorSpriteUID = spriteUID;
			if ( con.value( "NoConstruction" ).toBool() )
			{
				tile.floorType = ( FloorType )( FloorType::FT_SOLIDFLOOR );
			}
			else
			{
				tile.floorType = ( FloorType )( FloorType::FT_SOLIDFLOOR | FloorType::FT_CONSTRUCTION );

				positions.append( "F" );
				positions.append( constrPos.toString() );
			}

			tile.floorSpriteUID = spriteUID;
			tile.floorMaterial  = materialUIDs.first().toUInt();
			tile.floorRotation  = rotation;
			tile.flags += TileFlag::TF_WALKABLE;
			removeGrass( constrPos );
			if ( tile.flags & TileFlag::TF_SUNLIGHT )
			{
				//tile has sunlight, now we need to remove it below
				int z = constrPos.z - 1;
				while ( z > 0 )
				{
					Tile& tileBelow = getTile( constrPos.x, constrPos.y, z );
					if ( tileBelow.flags & TileFlag::TF_SUNLIGHT )
					{
						clearTileFlag( Position( constrPos.x, constrPos.y, z ), TileFlag::TF_SUNLIGHT );
						checkIndirectSunlightForNeighbors( Position( constrPos.x, constrPos.y, z ) );
					}
					else
					{
						break;
					}
					--z;
				}
			}
		}

		//GameState::addChange( NetworkCommand::SETTILEFLAGS, { QString::number( tid ), Util::tile2String( tile ) } );
		updateCoords.push_back( constrPos );
	}

	if ( !con.value( "NoConstruction" ).toBool() )
	{
		QVariantMap constr;
		constr.insert( "Pos", pos.toString() );
		constr.insert( "Positions", positions );
		constr.insert( "Items", itemUIDs );
		constr.insert( "Materials", materialUIDs );

		for ( int i = 0; i < positions.size(); i += 2 )
		{
			bool isWall = ( positions[i].toString() == "W" );
			Position insertPos( positions[i + 1] );
			constr.insert( "Pos", insertPos.toString() );
			if ( isWall )
			{
				m_wallConstructions.insert( insertPos.toInt(), constr );
			}
			else
			{
				m_floorConstructions.insert( insertPos.toInt(), constr );
			}
		}
	}

	updateNavigation( updateCoords, extractTo );

	return true;
}

bool World::constructStairs( QVariantMap& con, Position pos, int rotation, QVariantList itemUIDs, QVariantList materialUIDs, QStringList materialSIDs, Position extractTo )
{
	QList<Position> updateCoords;
	QVariantList positions;
	QString type;

	auto spl = DB::selectRows( "Constructions_Sprites", "ID", con.value( "ID" ).toString() );
	for ( auto sp : spl )
	{
		Position offset( sp.value( "Offset" ).toString() );
		Position constrPos( pos + offset );
		unsigned int tid = constrPos.toInt();
		Tile& tile       = getTile( tid );

		if ( sp.contains( "Type" ) )
		{
			type                   = sp.value( "Type" ).toString();
			QString spriteSID      = sp.value( "SpriteID" ).toString();
			unsigned int spriteUID = Global::sf().createSprite( spriteSID, materialSIDs )->uID;

			if ( type == "StairsBottom" )
			{
				tile.wallSpriteUID = spriteUID;
				tile.wallType      = ( WallType )( WallType::WT_STAIR | WallType::WT_CONSTRUCTED );
				tile.wallMaterial  = materialUIDs.first().toUInt();
				tile.wallRotation  = rotation;
				tile.flags += TileFlag::TF_WALKABLE;
				removeGrass( constrPos );
				positions.append( "W" );
				positions.append( constrPos.toString() );
			}
			else if ( type == "StairsTop" )
			{
				tile.floorSpriteUID = spriteUID;
				tile.floorType      = ( FloorType )( FloorType::FT_STAIRTOP | FloorType::FT_CONSTRUCTION );
				tile.floorMaterial  = materialUIDs.first().toUInt();
				tile.floorRotation  = rotation;
				tile.flags += TileFlag::TF_WALKABLE;
				tile.wallType = WallType::WT_NOWALL;
				removeGrass( constrPos );
				if ( tile.flags & TileFlag::TF_SUNLIGHT )
				{
					Tile& tileBelow = getTile( constrPos.x, constrPos.y, constrPos.z - 1 );
					tileBelow.flags += TileFlag::TF_SUNLIGHT;
				}
				positions.append( "F" );
				positions.append( constrPos.toString() );
			}

			GameState::addChange( NetworkCommand::SETTILEFLAGS, { QString::number( tid ), Util::tile2String( tile ) } );
			updateCoords.push_back( constrPos );
		}
	}

	QVariantMap constr;
	constr.insert( "Pos", pos.toString() );
	constr.insert( "Positions", positions );
	constr.insert( "Items", itemUIDs );
	constr.insert( "Materials", materialUIDs );

	for ( int i = 0; i < positions.size(); i += 2 )
	{
		bool isWall = ( positions[i].toString() == "W" );
		Position insertPos( positions[i + 1] );
		constr.insert( "Pos", insertPos.toString() );
		if ( isWall )
		{
			m_wallConstructions.insert( insertPos.toInt(), constr );
		}
		else
		{
			m_floorConstructions.insert( insertPos.toInt(), constr );
		}
	}

	updateNavigation( updateCoords, extractTo );

	return true;
}

bool World::constructRamp( QVariantMap& con, Position pos, int rotation, QVariantList itemUIDs, QVariantList materialUIDs, QStringList materialSIDs, Position extractTo )
{
	QList<Position> updateCoords;
	QVariantList positions;
	QString type;
	/*
	if( Util::materialType( materialSIDs.first() ) == "Soil" || Util::materialType( materialSIDs.first() ) == "Stone" )
	{
		createRamp( pos, materialSIDs.first() );
		updateCoords.append( pos );
		updateNavigation( updateCoords, extractTo );
		return true;
	}
	*/
	auto spl = DB::selectRows( "Constructions_Sprites", "ID", con.value( "ID" ).toString() );
	for ( auto sp : spl )
	{
		Position offset( sp.value( "Offset" ).toString() );
		Position constrPos( pos + offset );
		unsigned int tid = constrPos.toInt();
		Tile& tile       = getTile( tid );

		if ( sp.contains( "Type" ) )
		{
			type                   = sp.value( "Type" ).toString();
			QString spriteSID      = sp.value( "SpriteID" ).toString();
			unsigned int spriteUID = Global::sf().createSprite( spriteSID, materialSIDs )->uID;
			removeGrass( constrPos );

			if ( type == "RampBottom" )
			{
				tile.wallSpriteUID = spriteUID;
				if ( Util::materialType( materialSIDs.first() ) == "Soil" || Util::materialType( materialSIDs.first() ) == "Stone" )
				{
					tile.wallType = ( WallType )( WallType::WT_RAMP );
				}
				else
				{
					tile.wallType = ( WallType )( WallType::WT_RAMP | WallType::WT_CONSTRUCTED );
				}
				tile.wallMaterial = materialUIDs.first().toUInt();
				tile.wallRotation = rotation;
				tile.flags += TileFlag::TF_WALKABLE;
				positions.append( "W" );
				positions.append( constrPos.toString() );
			}
			if ( type == "RampTop" )
			{
				tile.floorSpriteUID = spriteUID;
				if ( Util::materialType( materialSIDs.first() ) == "Soil" || Util::materialType( materialSIDs.first() ) == "Stone" )
				{
					tile.floorType = ( FloorType )( FloorType::FT_RAMPTOP );
				}
				else
				{
					tile.floorType = ( FloorType )( FloorType::FT_RAMPTOP | FloorType::FT_CONSTRUCTION );
				}

				tile.floorMaterial = materialUIDs.first().toUInt();
				tile.floorRotation = rotation;
				tile.flags += TileFlag::TF_WALKABLE;
				tile.wallType = WallType::WT_NOWALL;
				if ( tile.flags & TileFlag::TF_SUNLIGHT )
				{
					Tile& tileBelow = getTile( constrPos.x, constrPos.y, constrPos.z - 1 );
					tileBelow.flags -= TileFlag::TF_SUNLIGHT;
				}
				positions.append( "F" );
				positions.append( constrPos.toString() );
			}

			GameState::addChange( NetworkCommand::SETTILEFLAGS, { QString::number( tid ), Util::tile2String( tile ) } );
			updateCoords.push_back( constrPos );
		}
	}

	if ( !( Util::materialType( materialSIDs.first() ) == "Soil" || Util::materialType( materialSIDs.first() ) == "Stone" ) )
	{
		QVariantMap constr;
		constr.insert( "Pos", pos.toString() );
		constr.insert( "Positions", positions );
		constr.insert( "Items", itemUIDs );
		constr.insert( "Materials", materialUIDs );

		for ( int i = 0; i < positions.size(); i += 2 )
		{
			bool isWall = ( positions[i].toString() == "W" );
			Position insertPos( positions[i + 1] );
			constr.insert( "Pos", insertPos.toString() );
			if ( isWall )
			{
				m_wallConstructions.insert( insertPos.toInt(), constr );
			}
			else
			{
				m_floorConstructions.insert( insertPos.toInt(), constr );
			}
		}
	}

	updateNavigation( updateCoords, extractTo );

	return true;
}

bool World::constructScaffold( QVariantMap& con, Position pos, int rotation, QVariantList itemUIDs, QVariantList materialUIDs, QStringList materialSIDs, Position extractTo )
{
	QList<Position> updateCoords;
	QVariantList positions;

	QString materialSID = materialSIDs.first();

	QString type;

	// object is rotatable, add rotation suffixes to spriteIds
	auto spl = DB::selectRows( "Constructions_Sprites", "ID", con.value( "ID" ).toString() );

	for ( auto sp : spl )
	{
		Position offset( sp.value( "Offset" ).toString() );
		Position constrPos( pos + offset );

		unsigned int tid = constrPos.toInt();
		Tile& tile       = getTile( tid );

		if ( sp.contains( "Type" ) )
		{
			type                   = sp.value( "Type" ).toString();
			QString spriteSID      = sp.value( "SpriteID" ).toString();
			unsigned int spriteUID = Global::sf().createSprite( spriteSID, { materialSID } )->uID;

			if ( type == "Wall" )
			{
				tile.wallSpriteUID = spriteUID;
				tile.wallType      = ( WallType )( WallType::WT_SCAFFOLD | WallType::WT_CONSTRUCTED );
				tile.wallMaterial  = materialUIDs.first().toUInt();
				tile.wallRotation  = rotation;
				tile.flags += TileFlag::TF_WALKABLE;
				positions.append( "W" );
				positions.append( constrPos.toString() );
			}
			else if ( type == "Floor" )
			{
				tile.floorSpriteUID = spriteUID;
				tile.floorType      = ( FloorType )( FloorType::FT_SCAFFOLD | FloorType::FT_CONSTRUCTION );
				tile.floorMaterial  = materialUIDs.first().toUInt();
				tile.floorRotation  = rotation;
				tile.flags += TileFlag::TF_WALKABLE;
				tile.wallType = WallType::WT_NOWALL;
				positions.append( "F" );
				positions.append( constrPos.toString() );
			}
			else
			{
				continue;
			}
			GameState::addChange( NetworkCommand::SETTILEFLAGS, { QString::number( tid ), Util::tile2String( tile ) } );
			updateCoords.push_back( constrPos );
		}
	}

	QVariantMap constr;
	constr.insert( "ConstructionID", "Scaffold" );
	constr.insert( "Pos", pos.toString() );
	constr.insert( "Positions", positions );
	constr.insert( "Items", itemUIDs );
	constr.insert( "Materials", materialUIDs );

	for ( int i = 0; i < positions.size(); i += 2 )
	{
		bool isWall = ( positions[i].toString() == "W" );
		Position insertPos( positions[i + 1] );
		constr.insert( "Pos", insertPos.toString() );
		if ( isWall )
		{
			m_wallConstructions.insert( insertPos.toInt(), constr );
		}
		else
		{
			m_floorConstructions.insert( insertPos.toInt(), constr );
		}
	}

	updateNavigation( updateCoords, extractTo );

	return true;
}

bool World::constructWorkshop( QVariantMap& con, Position pos, int rotation, QVariantList items, Position extractTo )
{
	QVariantList spriteComposition;

	auto cpl = DB::selectRows( "Workshops_Components", "ID", con.value( "ID" ).toString() );
	for ( auto cp : cpl )
	{
		QVariantMap spm = cp;
		Position offset;

		offset   = Position( spm.value( "Offset" ).toString() );
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

		Position constrPos( pos + offset );

		unsigned int tid = constrPos.toInt();
		Tile& tile       = getTile( tid );
		addToUpdateList( tid );
		QVariantMap scm;
		scm.insert( "Pos", constrPos.toString() );
		scm.insert( "OldTileVals", Util::tile2String( tile ) );

		if ( spm.value( "SpriteID" ).toString().isEmpty() || spm.value( "SpriteID" ).toString() == "WorkshopInputIndicator" )
		{
			tile.wallType = WallType::WT_CONSTRUCTED;
			tile.flags += TileFlag::TF_WORKSHOP;
			spriteComposition.append( scm );
			continue;
		}

		QStringList materialIDs;

		if ( !spm.value( "ItemID" ).toString().isEmpty() )
		{
			QString baseItem = spm.value( "ItemID" ).toString();

			for ( auto item : items )
			{
				if ( Global::inv().itemSID( item.toUInt() ) == baseItem )
				{
					materialIDs.push_back( Global::inv().materialSID( item.toUInt() ) );
					break;
				}
			}
		}
		else if ( !spm.value( "MaterialItem" ).toString().isEmpty() )
		{
			for ( auto matID : spm.value( "MaterialItem" ).toString().split( "|" ) )
			{
				materialIDs.push_back( Global::inv().materialSID( items[matID.toInt()].toUInt() ) );
			}
		}
		else
		{
			materialIDs.push_back( "None" );
		}
		bool floor = spm.value( "IsFloor" ).toBool();
		scm.insert( "IsFloor", floor );

		if ( floor )
		{
			tile.floorSpriteUID = Global::sf().createSprite( spm.value( "SpriteID" ).toString(), materialIDs )->uID;
			scm.insert( "UID", tile.floorSpriteUID );
			spriteComposition.append( scm );
			tile.floorType = FT_SOLIDFLOOR;
			setWalkable( constrPos, true );
		}
		else
		{
			Sprite* sprite     = Global::sf().createSprite( spm.value( "SpriteID" ).toString(), materialIDs );
			tile.wallSpriteUID = sprite->uID;
			Global::w().setWallSpriteAnim( constrPos, sprite->anim );
			scm.insert( "UID", tile.wallSpriteUID );
			spriteComposition.append( scm );

			if ( spm.contains( "WallRotation" ) )
			{
				QString rot = spm.value( "WallRotation" ).toString();
				if ( rot == "FR" )
					tile.wallRotation = 0;
				else if ( rot == "FL" )
					tile.wallRotation = 1;
				else if ( rot == "BL" )
					tile.wallRotation = 2;
				else if ( rot == "BR" )
					tile.wallRotation = 3;
			}
			tile.wallRotation = ( tile.wallRotation + rotation ) % 4;
			tile.wallType     = WallType::WT_CONSTRUCTED;
		}
		tile.flags += TileFlag::TF_WORKSHOP;
		GameState::addChange( NetworkCommand::SETTILEFLAGS, { QString::number( tid ), Util::tile2String( tile ) } );
	}

	Workshop* ws = Global::wsm().addWorkshop( con.value( "ID" ).toString(), pos, rotation );
	ws->setSourceItems( items );
	GameState::addChange2( NetworkCommand::WORKSHOPADD, QJsonDocument::fromVariant( ws->serialize() ).toJson() );
	ws->setSprites( spriteComposition );

	return true;
}

bool World::constructRampCorner( QVariantMap& con, Position pos, int rotation, QVariantList itemUIDs, QVariantList materialUIDs, QStringList materialSIDs, Position extractTo )
{
	QList<Position> updateCoords;
	QVariantList positions;
	auto spl = DB::selectRows( "Constructions_Sprites", "ID", con.value( "ID" ).toString() );
	for ( auto spMap : spl )
	{
		Position offset( spMap.value( "Offset" ).toString() );

		Position constrPos( pos + offset );

		positions.append( "W" );
		positions.append( constrPos.toString() );

		Tile& tile = getTile( constrPos );

		QString spriteSID = spMap.value( "SpriteID" ).toString();

		unsigned int spriteUID = Global::sf().createSprite( spriteSID, materialSIDs )->uID;

		tile.wallSpriteUID = spriteUID;

		if ( spMap.value( "Type" ).toString() == "Wall" )
		{
			if ( con.value( "NoConstruction" ).toBool() )
			{
				tile.wallType = ( WallType )( WallType::WT_RAMPCORNER );
			}
			else
			{
				tile.wallType = ( WallType )( WallType::WT_RAMPCORNER | WallType::WT_CONSTRUCTED );
			}

			tile.wallMaterial = materialUIDs.first().toUInt();
			tile.wallRotation = rotation;
			clearTileFlag( constrPos, TileFlag::TF_WALKABLE );
			removeGrass( constrPos );
			if ( tile.lightLevel > 0 )
			{
				updateLightsInRange( constrPos );
			}
			positions.append( "W" );
			positions.append( constrPos.toString() );
		}
		else if ( spMap.value( "Type" ).toString() == "Floor" )
		{
			tile.floorSpriteUID = spriteUID;
			if ( con.value( "NoConstruction" ).toBool() )
			{
				tile.floorType = ( FloorType )( FloorType::FT_RAMPTOP );
			}
			else
			{
				tile.floorType = ( FloorType )( FloorType::FT_RAMPTOP | FloorType::FT_CONSTRUCTION );
			}

			tile.floorMaterial = materialUIDs.first().toUInt();
			tile.floorRotation = rotation;
			tile.flags += TileFlag::TF_WALKABLE;
			removeGrass( constrPos );
			if ( tile.flags & TileFlag::TF_SUNLIGHT )
			{
				//tile has sunlight, now we need to remove it below
				int z = constrPos.z - 1;
				while ( z > 0 )
				{
					Tile& tileBelow = getTile( constrPos.x, constrPos.y, z );
					if ( tileBelow.flags & TileFlag::TF_SUNLIGHT )
					{
						clearTileFlag( Position( constrPos.x, constrPos.y, z ), TileFlag::TF_SUNLIGHT );
						checkIndirectSunlightForNeighbors( Position( constrPos.x, constrPos.y, z ) );
					}
					else
					{
						break;
					}
					--z;
				}
			}
			positions.append( "F" );
			positions.append( constrPos.toString() );
		}

		//GameState::addChange( NetworkCommand::SETTILEFLAGS, { QString::number( tid ), Util::tile2String( tile ) } );
		updateCoords.push_back( constrPos );
	}

	QVariantMap constr;
	constr.insert( "Pos", pos.toString() );
	constr.insert( "Positions", positions );
	constr.insert( "Items", itemUIDs );
	constr.insert( "Materials", materialUIDs );

	for ( int i = 0; i < positions.size(); i += 2 )
	{
		bool isWall = ( positions[i].toString() == "W" );
		Position insertPos( positions[i + 1] );
		constr.insert( "Pos", insertPos.toString() );
		if ( isWall )
		{
			m_wallConstructions.insert( insertPos.toInt(), constr );
		}
		else
		{
			m_floorConstructions.insert( insertPos.toInt(), constr );
		}
	}

	updateNavigation( updateCoords, extractTo );

	return true;
}

bool World::constructItem( QString itemSID, Position pos, int rotation, QList<unsigned int> items, Position extractTo )
{
	if ( items.empty() )
		return false;

	unsigned int itemID = items.first();

	QVariantMap constr;

	if ( Global::inv().itemSID( itemID ) != itemSID )
	{
		qDebug() << "create item from components before installing";
		auto vItems = Util::uintList2Variant( items );
		itemID      = Global::inv().createItem( pos, itemSID, vItems );
		constr.insert( "Items", vItems );
		constr.insert( "FromParts", true );
		for ( auto item : items )
		{
			Global::inv().setConstructedOrEquipped( item, true );
		}
	}

	QString type  = DB::select( "Category", "Items", Global::inv().itemUID( itemID ) ).toString();
	QString group = DB::select( "ItemGroup", "Items", Global::inv().itemUID( itemID ) ).toString();

	Tile& tile = getTile( pos );

	Sprite* sprite = Global::sf().getSprite( Global::inv().spriteID( itemID ) );

	QString location = DB::select( "Location", "Items_Tiles", itemSID ).toString();

	if ( location == "Wall" )
	{
		tile.wallSpriteUID = sprite->uID;
		tile.wallRotation  = rotation;
		tile.wallType      = WallType::WT_CONSTRUCTED;
		if ( sprite->anim )
		{
			//qDebug() << "set anim";
			tile.wallSpriteUID += ANIM_BIT;
		}
	}
	else if ( location == "Floor" )
	{
		tile.floorSpriteUID = sprite->uID;
		tile.floorRotation  = rotation;
		tile.floorType      = FloorType::FT_CONSTRUCTION;
		if ( sprite->anim )
		{
			tile.floorSpriteUID += ANIM_BIT;
		}
	}
	else if ( location == "HiddenFloor" )
	{
	}

	Global::inv().setConstructedOrEquipped( itemID, true );

	unsigned int nextItem = Global::inv().getFirstObjectAtPosition( pos );
	if ( nextItem )
	{
		Global::w().setItemSprite( pos, Global::inv().spriteID( nextItem ) );
	}
	else
	{
		Global::w().setItemSprite( pos, 0 );
	}
	//qDebug() << "##" << type << group;

	addToUpdateList( pos );
	switch ( m_constrItemSID2ENUM.value( type ) )
	{
		case CI_STORAGE:
			Global::spm().addContainer( itemID, pos );
			break;
		case CI_FURNITURE:
			Global::rm().addFurniture( itemID, pos );
			break;
	}
	switch ( m_constrItemSID2ENUM.value( group ) )
	{
		case CI_DOOR:
			setTileFlag( pos, TileFlag::TF_DOOR );
			Global::rm().addDoor( pos, itemID, Global::inv().materialUID( itemID ) );
			break;
		case CI_LIGHT:
		{
			int intensity = DB::select( "LightIntensity", "Items", Global::inv().itemSID( itemID ) ).toInt();
			constr.insert( "Light", intensity );
			if ( intensity )
			{
				addLight( pos.toInt(), pos, intensity );
			}
		}
		break;
		case CI_ALARMBELL:
			Global::rm().addFurniture( itemID, pos );
			break;
		case CI_MECHANISM:
			Global::mcm().installItem( itemID, pos, rotation );
			break;
		case CI_HYDRAULICS:
			return constructPipe( itemSID, pos, itemID );
			break;
		case CI_FARMUTIL:
			Global::fm().addUtil( pos, itemID );
			break;
	}

	constr.insert( "ConstructionID", "Item" );
	constr.insert( "Pos", pos.toString() );
	constr.insert( "Rot", rotation );
	constr.insert( "Item", itemID );
	constr.insert( "Type", type );
	constr.insert( "Group", group );

	if ( location == "Wall" )
	{
		m_wallConstructions.insert( pos.toInt(), constr );
	}
	else if ( location == "Floor" || location == "FloorHidden" )
	{
		constr.insert( "Hidden", location == "FloorHidden" );
		m_floorConstructions.insert( pos.toInt(), constr );
	}

	return true;
}

void World::updateNavigation( QList<Position>& coords, Position extractTo )
{
	for ( auto p : coords )
	{
		expelTileInhabitants( p, extractTo );
		expelTileItems( p, extractTo );
	}

	for ( auto p : coords )
	{
		m_regionMap.updatePosition( p );
	}
	for ( auto p : coords )
	{
		m_regionMap.updateConnectedRegions( p );
	}
}

bool World::deconstruct( Position decPos, Position workPos, bool ignoreGravity )
{
	QVariantMap constr;
	if ( m_wallConstructions.contains( decPos.toInt() ) )
	{
		constr = m_wallConstructions.value( decPos.toInt() );
		m_wallConstructions.remove( decPos.toInt() );
		return deconstruct2( constr, decPos, false, workPos, ignoreGravity );
	}
	else if ( Global::wsm().isWorkshop( decPos ) )
	{
		Workshop* ws     = Global::wsm().workshopAt( decPos );
		auto sourceItems = ws->sourceItems();
		ws->destroy();

		QVariantList sprites = ws->sprites();
		for ( auto spritevm : sprites )
		{
			Position wsPos( spritevm.toMap().value( "Pos" ).toString() );
			unsigned int tid = wsPos.toInt();
			Tile& tile       = getTile( tid );
			if ( !m_floorConstructions.contains( wsPos.toInt() ) )
			{
				Util::string2Tile( tile, spritevm.toMap().value( "OldTileVals" ).toString() );
			}
			else
			{
				tile.wallSpriteUID = 0;
				clearTileFlag( wsPos, TileFlag::TF_WORKSHOP );
			}

			updateWalkable( wsPos );
			addToUpdateList( wsPos );

			GameState::addChange( NetworkCommand::SETTILEFLAGS, { QString::number( tid ), Util::tile2String( tile ) } );
		}
		for ( auto vItem : sourceItems )
		{
			Global::inv().putDownItem( vItem.toUInt(), workPos );
			Global::inv().setConstructedOrEquipped( vItem.toUInt(), false );
		}

		Global::wsm().deleteWorkshop( ws->id() );
		return true;
	}
	else if ( m_floorConstructions.contains( decPos.toInt() ) )
	{
		constr = m_floorConstructions.value( decPos.toInt() );
		m_floorConstructions.remove( decPos.toInt() );
		return deconstruct2( constr, decPos, true, workPos, ignoreGravity );
	}
	else
	{
		return false;
	}
}

bool World::deconstruct2( QVariantMap constr, Position decPos, bool isFloor, Position workPos, bool ignoreGravity )
{
	if ( constr.value( "ConstructionID" ).toString() == "Item" )
	{
		unsigned itemID = constr.value( "Item" ).toUInt();

		QString type  = constr.value( "Type" ).toString();
		QString group = constr.value( "Group" ).toString();

		switch ( m_constrItemSID2ENUM.value( type ) )
		{
			case CI_STORAGE:
				Global::spm().removeContainer( itemID, decPos );
				break;
			case CI_FURNITURE:
				Global::rm().removeFurniture( decPos );
				break;
			case CI_HYDRAULICS:
				deconstructPipe( constr, decPos, workPos );
		}
		switch ( m_constrItemSID2ENUM.value( group ) )
		{
			case CI_DOOR:
				clearTileFlag( decPos, TileFlag::TF_DOOR );
				Global::rm().removeDoor( decPos );
				break;
			case CI_ALARMBELL:
				Global::rm().removeFurniture( decPos );
				break;
			case CI_LIGHT:
			{
				removeLight( decPos.toInt() );
			}
			break;
			case CI_MECHANISM:
				Global::mcm().uninstallItem( itemID );
				break;
			case CI_FARMUTIL:
				Global::fm().removeUtil( decPos );
				break;
		}

		Tile& tile = getTile( decPos );
		if ( isFloor )
		{
			//if( m_constrItemSID2ENUM.value( group ) != CI_MECHANISM )
			if ( !constr.value( "Hidden" ).toBool() )
			{
				tile.floorType      = FT_NOFLOOR;
				tile.floorSpriteUID = 0;
				setWalkable( decPos, false );

				if ( m_creaturePositions.contains( decPos.toInt() ) )
				{
					Global::gm().forceMoveGnomes( decPos, workPos );
				}
				discover( decPos.belowOf() );
			}
		}
		else
		{
			tile.wallType      = WT_NOWALL;
			tile.wallSpriteUID = 0;
		}

		if ( constr.value( "FromParts" ).toBool() )
		{
			for ( auto vItem : constr.value( "Items" ).toList() )
			{
				Global::inv().setConstructedOrEquipped( vItem.toUInt(), false );
				Global::inv().moveItemToPos( vItem.toUInt(), workPos );
			}
			Global::inv().destroyObject( itemID );
		}
		else
		{
			Global::inv().setConstructedOrEquipped( itemID, false );
			Global::inv().moveItemToPos( itemID, workPos );
		}

		return true;
	}
	else if ( constr.value( "ConstructionID" ).toString() == "Scaffold" )
	{
		//if construction above is scaffold
		if ( m_wallConstructions.contains( decPos.aboveOf().toInt() ) )
		{
			auto aboveConstr = m_wallConstructions.value( decPos.aboveOf().toInt() );
			if ( aboveConstr.value( "ConstructionID" ).toString() == "Scaffold" )
			{
				deconstruct( decPos.aboveOf(), workPos, ignoreGravity );
			}
		}
	}
	else if ( constr.value( "ConstructionID" ).toString().endsWith( "Fence" ) )
	{
		Tile& tile         = getTile( decPos );
		tile.wallType      = ( WallType )( WallType::WT_NOWALL );
		tile.wallSpriteUID = 0;
		if ( tile.floorType != ( FloorType )( FloorType::FT_NOFLOOR ) )
		{
			tile.flags += TileFlag::TF_WALKABLE;
		}
		updateFenceSprite( decPos.northOf() );
		updateFenceSprite( decPos.eastOf() );
		updateFenceSprite( decPos.southOf() );
		updateFenceSprite( decPos.westOf() );
	}

	QList<Position> updateCoords;
	if ( constr.contains( "Positions" ) )
	{
		auto posList = constr.value( "Positions" ).toList();
		for ( int i = 0; i < posList.size(); i += 2 )
		{
			bool isWall = ( posList[i].toString() == "W" );
			Position pos( posList[i + 1] );
			updateCoords.append( pos );

			if ( isWall )
			{
				Tile& tile         = getTile( pos );
				tile.wallType      = ( WallType )( WallType::WT_NOWALL );
				tile.wallSpriteUID = 0;
				if ( tile.floorType != ( FloorType )( FloorType::FT_NOFLOOR ) )
				{
					tile.flags += TileFlag::TF_WALKABLE;
				}

				m_wallConstructions.remove( pos.toInt() );
			}
			else
			{
				Tile& tile          = getTile( pos );
				tile.floorType      = ( FloorType )( FloorType::FT_NOFLOOR );
				tile.floorSpriteUID = 0;
				clearTileFlag( pos, TileFlag::TF_WALKABLE );

				PositionEntry pe;
				if ( Global::inv().getObjectsAtPosition( pos, pe ) )
				{
					for ( auto i : pe )
					{
						Global::inv().moveItemToPos( i, workPos );
					}
				}
				/*				
				if( !ignoreGravity )
				{
					Global::inv().gravity( pos );
				}
				*/
				if ( tile.flags & TileFlag::TF_SUNLIGHT )
				{
					Position tmpPos = pos;
					getFloorLevelBelow( tmpPos, true );
					Tile& tileBelow = getTile( tmpPos );
					tileBelow.flags += TileFlag::TF_SUNLIGHT;
					checkIndirectSunlightForNeighbors( tmpPos );
					discover( tmpPos );
					addToUpdateList( tmpPos );
				}
				m_floorConstructions.remove( pos.toInt() );
			}
		}
	}
	else
	{
		//Position pos( constr.value( "Pos" ) );
		bool isWall = constr.value( "T" ).toString() == "W";
		updateCoords.append( decPos );
		Tile& tile = getTile( decPos );

		if ( isWall )
		{
			tile.wallType      = ( WallType )( WallType::WT_NOWALL );
			tile.wallSpriteUID = 0;
			if ( tile.floorType != ( FloorType )( FloorType::FT_NOFLOOR ) )
			{
				tile.flags += TileFlag::TF_WALKABLE;
			}
			updateLightsInRange( decPos );
		}
		else
		{
			tile.floorType      = ( FloorType )( FloorType::FT_NOFLOOR );
			tile.floorSpriteUID = 0;
			clearTileFlag( decPos, TileFlag::TF_WALKABLE );

			PositionEntry pe;
			if ( Global::inv().getObjectsAtPosition( decPos, pe ) )
			{
				for ( auto i : pe )
				{
					Global::inv().moveItemToPos( i, workPos );
				}
			}

			/*
			if( !ignoreGravity )
			{
				Global::inv().gravity( decPos );
			}
			*/

			if ( m_creaturePositions.contains( decPos.toInt() ) )
			{
				Global::gm().forceMoveGnomes( decPos, workPos );
			}
			if ( tile.flags & TileFlag::TF_SUNLIGHT )
			{
				Position tmpPos = decPos;
				getFloorLevelBelow( tmpPos, true );
				Tile& tileBelow = getTile( tmpPos );
				tileBelow.flags += TileFlag::TF_SUNLIGHT;
				checkIndirectSunlightForNeighbors( tmpPos );
			}
		}
	}

	QVariantList mats  = constr.value( "Materials" ).toList();
	QVariantList items = constr.value( "Items" ).toList();
	int i              = 0;
	for ( auto item : items )
	{
		Global::inv().createItem( workPos, DBH::itemSID( item.toUInt() ), DBH::materialSID( mats[i].toUInt() ) );
	}

	for ( auto updatePos : updateCoords )
	{
		m_regionMap.updatePosition( updatePos );
		addToUpdateList( updatePos );
	}
	return true;
}
