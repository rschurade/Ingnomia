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
#include "selection.h"

#include "../base/config.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/util.h"
#include "../game/game.h"
#include "../game/farmingmanager.h"
#include "../game/job.h"
#include "../game/jobmanager.h"
#include "../game/mechanismmanager.h"
#include "../game/plant.h"
#include "../game/roommanager.h"
#include "../game/stockpilemanager.h"
#include "../game/workshopmanager.h"
#include "../game/world.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonValue>

Selection::Selection( Game* game ) :
	g( game ),
	m_rotation( 0 ),
	m_firstClicked( false ),
	m_action( "" ),
	m_debug( false ),
	QObject( game )
{
	m_selectionSize.first  = 0;
	m_selectionSize.second = 0;

	m_reqMap.insert( "none", SEL_NONE );
	m_reqMap.insert( "Floor", SEL_FLOOR );
	m_reqMap.insert( "FloorOrScaffold", SEL_FLOOR_OR_SCAFFOLD );
	m_reqMap.insert( "Wall", SEL_WALL );
	m_reqMap.insert( "MineableWall", SEL_MINEABLE_WALL );
	m_reqMap.insert( "Construction", SEL_CONSTRUCTION );
	m_reqMap.insert( "Job", SEL_JOB );
	m_reqMap.insert( "Designation", SEL_DESIGNATION );
	m_reqMap.insert( "Stockpile", SEL_STOCKPILE );
	m_reqMap.insert( "Room", SEL_ROOM );
	m_reqMap.insert( "AllowBell", SEL_ALLOW_BELL );
	m_reqMap.insert( "AllowFurniture", SEL_ALLOW_FURNITURE );
	m_reqMap.insert( "Pasture", SEL_PASTURE );
	m_reqMap.insert( "Walkable", SEL_WALKABLE );
	m_reqMap.insert( "Mechanism", SEL_MECHANISM );
	m_reqMap.insert( "GearBox", SEL_GEARBOX );

	m_reqMap.insert( "Stairs", SEL_STAIRS );
	m_reqMap.insert( "StairsTop", SEL_STAIRSTOP );
	m_reqMap.insert( "Ramp", SEL_RAMP );
	m_reqMap.insert( "RampTop", SEL_RAMPTOP );

	m_reqMap.insert( "Soil", SEL_SOIL );
	m_reqMap.insert( "Tree", SEL_TREE );
	m_reqMap.insert( "Plant", SEL_PLANT );
	m_reqMap.insert( "TreeCLip", SEL_TREECLIP );
	m_reqMap.insert( "PlantWithFruit", SEL_PLANT_WITH_FRUIT );
	m_reqMap.insert( "TreeInRange", SEL_TREE_IN_RANGE );

	m_reqMap.insert( "AnyWall", SEL_ANYWALL );
}

Selection::~Selection()
{
}

QVariantMap Selection::serialize()
{
	QVariantMap out;

	out.insert( "Rot", m_rotation );
	out.insert( "Pos", m_firstClick.toString() );
	out.insert( "Action", m_action );
	out.insert( "Item", m_item );
	out.insert( "Mats", m_materials.join( '_' ) );

	QVariantList spl; // selection position list
	for ( auto s : m_selection )
	{
		spl.push_back( s.first.toString() );
	}
	out.insert( "Selection", spl );

	return out;
}

void Selection::clear()
{
	m_action = "";
	m_selection.clear();
	m_firstClicked         = false;
	m_selectionSize.first  = 0;
	m_selectionSize.second = 0;
	m_changed              = true;
	m_isFloor              = false;
	m_isMulti              = false;
	m_tileCheckList.clear();
	m_item = "";
	m_materials.clear();
	m_canRotate = false;

	emit signalActionChanged( "" );
}

void Selection::rotate()
{
	++m_rotation;
	if ( m_rotation > 3 )
		m_rotation = 0;
	m_changed = true;
}

bool Selection::leftClick( Position& pos, bool shift, bool ctrl )
{
	if ( !m_firstClicked )
	{
		// first click
		m_firstClick          = pos;
		m_firstClicked        = true;
		QVariantMap actionMap = DB::selectRow( "Actions", m_action );
		m_isFloor             = actionMap.value( "IsFloor" ).toBool();
		m_isMulti             = actionMap.value( "Multi" ).toBool();
		if ( m_action == "BuildItem" && DB::select( "IsContainer", "Items", m_item ).toBool() )
		{
			m_isMulti = true;
		}

		m_isMultiZ  = actionMap.value( "MultiZ" ).toBool();
		m_canRotate = actionMap.value( "Rotate" ).toBool();
		m_selection.push_back( QPair<Position, bool>( pos, testTileForJobSelection( pos ) ) );
		
		emit signalFirstClick( m_firstClick.toString() );

		if ( !m_isMulti && !shift )
		{
			onSecondClick( shift, ctrl );
			m_firstClicked = false;
			m_changed      = true;
			updateSelection( pos, shift, ctrl );
			return true;
		}
		return false;
	}
	else
	{
		// second click creates whatever we want to create
		pos.z = m_firstClick.z;
		onSecondClick( shift, ctrl );
		m_firstClicked = false;
		m_changed      = true;
		emit signalFirstClick( "" );
		emit signalSize( "" );
		updateSelection( pos, shift, ctrl );
		return true;
	}
}

void Selection::setAction( QString action )
{
	if ( Global::debugMode )
		qDebug() << "Selection set action: " << action;

	clear();

	m_action              = action;
	QVariantMap actionMap = DB::selectRow( "Actions", m_action );

	m_isFloor   = actionMap.value( "IsFloor" ).toBool();
	m_isMulti   = actionMap.value( "Multi" ).toBool();
	m_isMultiZ  = actionMap.value( "MultiZ" ).toBool();
	m_canRotate = actionMap.value( "Rotate" ).toBool();

	emit signalActionChanged( m_action );
}

void Selection::updateSelection( Position& pos, bool shift, bool ctrl )
{
	if ( m_action.isEmpty() )
	{
		return;
	}
	// a job is selected but we haven't clicked into the world yet
	if ( m_firstClicked == false )
	{
		m_selection.clear();
		m_selectionSize.first  = 0;
		m_selectionSize.second = 0;
		m_selection.push_back( QPair<Position, bool>( pos, testTileForJobSelection( pos ) ) );
		m_changed = true;
	}
	// for single tile job we just return here
	if ( !m_isMulti && !shift )
	{
		return;
	}

	// clicked one, we span the selection between the clicked tile and the cursor position
	if ( m_firstClicked )
	{
		int beginX = ( m_firstClick.x < pos.x ) ? m_firstClick.x : pos.x;
		int endX   = ( m_firstClick.x > pos.x ) ? m_firstClick.x : pos.x;
		int beginY = ( m_firstClick.y < pos.y ) ? m_firstClick.y : pos.y;
		int endY   = ( m_firstClick.y > pos.y ) ? m_firstClick.y : pos.y;

		m_selectionSize.first  = qMax( 1, endX - beginX + 1 );
		m_selectionSize.second = qMax( 1, endY - beginY + 1 );

		m_selection.clear();

		if ( m_isMultiZ )
		{
			int z0 = qMin( m_firstClick.z, pos.z );
			int zn = qMax( m_firstClick.z, pos.z );
			for ( int z = z0; z <= zn; ++z )
			{
				for ( int y = beginY; y <= endY; ++y )
				{
					for ( int x = beginX; x <= endX; ++x )
					{
						if ( m_ctrlActive )
						{
							if ( x > beginX && x < endX && y > beginY && y < endY )
								continue;
						}
						pos = Position( x, y, z );
						m_selection.push_back( QPair<Position, bool>( pos, testTileForJobSelection( pos ) ) );
					}
				}
			}
		}
		else
		{
			for ( int y = beginY; y <= endY; ++y )
			{
				for ( int x = beginX; x <= endX; ++x )
				{
					if ( m_ctrlActive )
					{
						if ( x > beginX && x < endX && y > beginY && y < endY )
							continue;
					}
					pos = Position( x, y, m_firstClick.z );
					m_selection.push_back( QPair<Position, bool>( pos, testTileForJobSelection( pos ) ) );
				}
			}
		}
	}

	if( m_firstClicked )
	{
		emit signalSize( QString::number( m_selectionSize.first ) + " x " + QString::number( m_selectionSize.second ) );
	}
	else
	{
		emit signalSize( "" );
	}

	m_changed = true;
}

void Selection::rightClick( Position& pos )
{
	if ( m_firstClicked )
	{
		m_selection.clear();
		QVariantMap actionMap = DB::selectRow( "Actions", m_action );
		m_selection.push_back( QPair<Position, bool>( pos, testTileForJobSelection( pos ) ) );
		m_firstClicked         = false;
		m_selectionSize.first  = 0;
		m_selectionSize.second = 0;
	}
	else
	{
		clear();
	}

	emit signalFirstClick( "" );
	emit signalSize( "" );
		
	m_changed = true;
}

bool Selection::testTileForJobSelection( const Position& pos )
{
	// TODO cache plants and posID and make it faster :)
	Tile* tile;
	int dim = Global::dimX;

	if ( m_tileCheckList.empty() )
	{
		if ( m_action == "BuildWorkshop" )
		{
			// Workshops_Components
			m_tileCheckList = DB::selectRows( "Workshops_Components", "ID", m_item );
		}
		else if ( m_action == "BuildItem" )
		{
			// Item_Tiles
			m_tileCheckList = DB::selectRows( "Items_Tiles", "ID", m_item );
		}
		else if ( m_action == "BuildWall" && m_item == "Palisade" )
		{
			// Actions_Tiles
			if ( Global::debugMode )
				qDebug() << "test tiles: "
						 << "Actions_Tiles BuildWallPalisade";
			m_tileCheckList = DB::selectRows( "Actions_Tiles", "ID", "BuildWallPalisade" );
		}
		else
		{
			// Actions_Tiles
			m_tileCheckList = DB::selectRows( "Actions_Tiles", "ID", m_action );
		}
	}

	for ( auto tm : m_tileCheckList )
	{
		Position testPos;
		Position offset;
		if ( tm.contains( "Offset" ) )
		{
			offset   = Position( tm.value( "Offset" ).toString() );
			int rotX = offset.x;
			int rotY = offset.y;
			switch ( m_rotation )
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

			testPos = Position( pos.x + offset.x, pos.y + offset.y, pos.z + offset.z );
			if ( !testPos.valid() )
			{
				return false;
			}
			tile = &g->w()->getTile( testPos );
		}
		else
		{
			testPos = pos;
			if ( !testPos.valid() )
			{
				return false;
			}
			tile    = &g->w()->getTile( pos );
		}

		QStringList required = tm.value( "Required" ).toString().split( "|" );
		if ( m_action == "BuildWorkshop" )
		{
			if ( required.size() == 1 && required[0].isEmpty() && offset.z == 0 )
			{
				required.clear();
				required.append( "Floor" );
			}
		}
		if( Global::debugMode)
		{
			qDebug() << testPos.toString() << QString::number( (quint64)tile->flags, 16 ) << tile->wallType << tile->floorType;
		}

		for ( auto req : required )
		{
			//if ( Global::debugMode )
			//	qDebug() << "test requirement: " << req << "...";
			switch ( m_reqMap.value( req ) )
			{
				case SEL_NONE:
					break;
				case SEL_FLOOR:
					if ( !(bool)( tile->floorType & FloorType::FT_SOLIDFLOOR ) )
						return false;
					break;
				case SEL_FLOOR_OR_SCAFFOLD:
					if ( !( (bool)( tile->floorType & FloorType::FT_SOLIDFLOOR ) || (bool)( tile->floorType & FloorType::FT_SCAFFOLD ) ) )
						return false;
					break;
				case SEL_WALL:
					if ( !( (bool)( tile->wallType & WallType::WT_ROUGH || (bool)( tile->wallType & WallType::WT_SOLIDWALL ) ) ) )
						return false;
					break;
				case SEL_MINEABLE_WALL:
					if ( !( (bool)( tile->wallType & WallType::WT_ROUGH || (bool)( tile->wallType & WallType::WT_SOLIDWALL ) || (bool)( tile->wallType & WallType::WT_RAMP ) || (bool)( tile->wallType & WallType::WT_RAMPCORNER ) ) ) )
						return false;
					break;
				case SEL_CONSTRUCTION:
					if ( !( (bool)( tile->wallType & WallType::WT_CONSTRUCTED ) || (bool)( tile->floorType & FloorType::FT_CONSTRUCTION ) ) && !g->mcm()->hasMechanism( pos ) )
						return false;
					break;
				case SEL_MECHANISM:
					if ( !g->mcm()->hasMechanism( pos ) )
						return false;
					break;
				case SEL_GEARBOX:
					if ( !g->mcm()->hasGearBox( pos ) )
						return false;
					break;
				case SEL_JOB:
					if ( !g->w()->hasJob( testPos ) )
						return false;
					break;
				case SEL_DESIGNATION:
					if ( !( tile->flags & ( TileFlag::TF_STOCKPILE + TileFlag::TF_GROVE + TileFlag::TF_FARM + TileFlag::TF_PASTURE + TileFlag::TF_ROOM + TileFlag::TF_NOPASS ) ) )
						return false;
					break;
				case SEL_STOCKPILE:
					if ( !( tile->flags & TileFlag::TF_STOCKPILE ) )
						return false;
					break;
				case SEL_ROOM:
					if ( !( tile->flags & TileFlag::TF_ROOM ) )
						return false;
					break;
				case SEL_ALLOW_BELL:
					if ( !( tile->flags & TileFlag::TF_ROOM ) || !g->rm()->allowBell( testPos ) )
						return false;
					break;
				case SEL_ALLOW_FURNITURE:
					if ( ( tile->flags & ( TileFlag::TF_STOCKPILE + TileFlag::TF_GROVE + TileFlag::TF_FARM + TileFlag::TF_PASTURE + TileFlag::TF_NOPASS ) ) )
						return false;
					break;
				case SEL_PASTURE:
					if ( !( tile->flags & TileFlag::TF_PASTURE ) )
						return false;
					break;
				case SEL_WALKABLE:
					if ( !( tile->flags & TileFlag::TF_WALKABLE ) )
						return false;
					break;
				case SEL_STAIRS:
					if ( !( tile->wallType & WallType::WT_STAIR ) )
						return false;
					break;
				case SEL_STAIRSTOP:
					if ( !( tile->floorType & FloorType::FT_STAIRTOP ) )
						return false;
					break;
				case SEL_RAMP:
					if ( !( tile->wallType & WallType::WT_RAMP ) )
						return false;
					break;
				case SEL_RAMPTOP:
					if ( !( tile->floorType & FloorType::FT_RAMPTOP ) )
						return false;
					break;
				case SEL_SOIL:
				{
					QString matSID = DBH::materialSID( tile->floorMaterial );

					if ( DB::select( "Type", "Materials", matSID ).toString() != "Soil" )
					{
						return false;
					}
				}
				break;
				case SEL_TREE:
				{
					bool isTree = false;
					if ( g->w()->plants().contains( testPos.toInt() ) )
					{ // need to guard [] access otherwise it constructs a default object for that location and furth checks fails
						isTree = g->w()->plants()[testPos.toInt()].isTree();
					}
					if ( !isTree )
						return false;
				}
				break;
				case SEL_PLANT:
				{
					bool isPlant = false;
					if ( g->w()->plants().contains( testPos.toInt() ) )
					{ // need to guard [] access otherwise it constructs a default object for that location and further checks fail
						isPlant = g->w()->plants()[testPos.toInt()].isPlant();
					}
					if ( !isPlant )
						return false;
				}
				break;
				case SEL_TREECLIP:
					break;
				case SEL_PLANT_WITH_FRUIT:
				{
					if ( !( g->w()->plants().contains( testPos.toInt() ) && g->w()->plants()[testPos.toInt()].harvestable() ) )
					{
						return false;
					}
				}
				break;
			}

			//if ( Global::debugMode )
			//	qDebug() << "passed.";
		}

		QStringList forbidden = tm.value( "Forbidden" ).toString().split( "|" );
		if ( m_action == "BuildWorkshop" && forbidden.size() == 1 && forbidden[0].isEmpty() )
		{
			forbidden.append( "AnyWall" );
		}
		for ( auto forb : forbidden )
		{
			//if ( Global::debugMode )
			//	qDebug() << "test forbidden: " << forb << "...";
			if ( m_action.startsWith( "Build" ) && tile->flags & TileFlag::TF_OCCUPIED )
			{
				return false;
			}

			switch ( m_reqMap.value( forb ) )
			{
				case SEL_NONE:
					break;
				case SEL_FLOOR:
					if ( (bool)( tile->floorType & FloorType::FT_SOLIDFLOOR ) )
						return false;
					break;
				case SEL_FLOOR_OR_SCAFFOLD:
					if ( ( (bool)( tile->floorType & FloorType::FT_SOLIDFLOOR ) || (bool)( tile->floorType & FloorType::FT_SCAFFOLD ) ) )
						return false;
					break;
				case SEL_WALL:
					if ( ( (bool)( tile->wallType & WallType::WT_ROUGH || (bool)( tile->wallType & WallType::WT_SOLIDWALL ) ) ) )
						return false;
					//if( (bool)tile->wallType ) return false;
					break;
				case SEL_CONSTRUCTION:
					if ( (bool)( tile->wallType & WallType::WT_CONSTRUCTED ) )
						return false;
					break;
				case SEL_MECHANISM:
					if ( g->mcm()->hasMechanism( pos ) )
						return false;
					break;
				case SEL_JOB:
					if ( g->w()->hasJob( testPos ) )
						return false;
					break;
				case SEL_DESIGNATION:
					if ( tile->flags & ( TileFlag::TF_STOCKPILE + TileFlag::TF_GROVE + TileFlag::TF_FARM + TileFlag::TF_ROOM + TileFlag::TF_PASTURE + TileFlag::TF_NOPASS ) )
						return false;
					break;
				case SEL_STOCKPILE:
					if ( tile->flags & TileFlag::TF_STOCKPILE )
						return false;
					break;
				case SEL_ROOM:
					if ( tile->flags & TileFlag::TF_ROOM )
						return false;
					break;
				case SEL_PASTURE:
					if ( tile->flags & TileFlag::TF_PASTURE )
						return false;
				case SEL_STAIRS:
					if ( (bool)( tile->wallType & WallType::WT_STAIR ) )
						return false;
					break;
				case SEL_STAIRSTOP:
					if ( (bool)( tile->floorType & FloorType::FT_STAIRTOP ) )
						return false;
					break;
				case SEL_RAMP:
					if ( (bool)( tile->wallType & WallType::WT_RAMP ) )
						return false;
					break;
				case SEL_RAMPTOP:
					if ( (bool)( tile->floorType & FloorType::FT_RAMPTOP ) )
						return false;
					break;
				case SEL_SOIL:
				{
					QString matSID = DBH::materialSID( tile->floorMaterial );
					if ( DB::select( "Type", "Materials", matSID ).toString() == "Soil" )
					{
						return false;
					}
				}
				break;
				case SEL_TREE:
				{
					bool isTree = false;
					if ( g->w()->plants().contains( testPos.toInt() ) )
					{ // need to guard [] access otherwise it constructs a default object for that location and furth checks fails
						isTree = g->w()->plants()[testPos.toInt()].isTree();
					}
					if ( isTree )
						return false;
				}
				break;
				case SEL_PLANT:
				{
					bool isPlant = false;
					if ( g->w()->plants().contains( testPos.toInt() ) )
					{ // need to guard [] access otherwise it constructs a default object for that location and further checks fail
						isPlant = g->w()->plants()[testPos.toInt()].isPlant();
					}
					if ( isPlant )
						return false;
				}
				break;
				case SEL_TREECLIP:
					break;
				case SEL_TREE_IN_RANGE:
				{
					if ( testPos.x < 3 || testPos.x > Global::dimX - 4 || testPos.y < 3 || testPos.y > Global::dimY - 4 )
					{
						return false;
					}

					if ( !g->w()->noTree( testPos, 2, 2 ) )
					{
						return false;
					}
				}
				break;
				case SEL_PLANT_WITH_FRUIT:
				{
					if ( g->w()->plants().contains( testPos.toInt() ) && g->w()->plants()[testPos.toInt()].harvestable() )
					{
						return false;
					}
				}
				break;
				case SEL_ANYWALL:
					if ( tile->wallType )
						return false;
					if ( g->w()->plants().contains( testPos.toInt() ) )
						return false;
					if ( tile->flags & ( TileFlag::TF_STOCKPILE + TileFlag::TF_GROVE + TileFlag::TF_FARM + TileFlag::TF_ROOM + TileFlag::TF_NOPASS ) )
						return false;
					if ( g->w()->hasJob( testPos ) )
						return false;
					break;
			}

			//if ( Global::debugMode )
			//	qDebug() << "passed.";
		}
	}

	return true;
}

void Selection::onSecondClick( bool shift, bool ctrl )
{
	m_changed = true;
	if ( m_action == "RemoveDesignation" )
	{
		for ( auto p : m_selection )
		{
			if ( p.second )
			{
				g->w()->removeDesignation( p.first );
			}
		}
		return;
	}

	if ( m_action == "CancelJob" )
	{
		for ( auto p : m_selection )
		{
			if ( p.second )
			{
				g->jm()->cancelJob( p.first );
			}
		}
		return;
	}

	if ( m_action == "RaisePrio" )
	{
		for ( auto p : m_selection )
		{
			if ( p.second )
			{
				g->jm()->raisePrio( p.first );
			}
		}
		return;
	}
	if ( m_action == "LowerPrio" )
	{
		for ( auto p : m_selection )
		{
			if ( p.second )
			{
				g->jm()->lowerPrio( p.first );
			}
		}
		return;
	}

	if ( m_action == "CreateStockpile" )
	{
		g->spm()->addStockpile( m_firstClick, m_selection );
		return;
	}
	if ( m_action == "CreateGrove" )
	{
		g->fm()->addGrove( m_firstClick, m_selection );
		return;
	}
	if ( m_action == "CreateFarm" )
	{
		g->fm()->addFarm( m_firstClick, m_selection );
		return;
	}
	if ( m_action == "CreatePasture" )
	{
		g->fm()->addPasture( m_firstClick, m_selection );
		return;
	}
	if ( m_action == "CreateRoom" )
	{
		g->rm()->addRoom( m_firstClick, m_selection, RoomType::PersonalRoom );
		return;
	}
	if ( m_action == "CreateDorm" )
	{
		g->rm()->addRoom( m_firstClick, m_selection, RoomType::Dorm );
		return;
	}
	if ( m_action == "CreateDining" )
	{
		g->rm()->addRoom( m_firstClick, m_selection, RoomType::Dining );
		return;
	}
	if ( m_action == "CreateHospital" )
	{
		g->rm()->addRoom( m_firstClick, m_selection, RoomType::Hospital );
		return;
	}
	if ( m_action == "CreateNoPass" )
	{
		g->rm()->addNoPass( m_firstClick, m_selection );
		return;
	}
	QString jobId = DB::select( "Job", "Actions", m_action ).toString();

	QVariantList vmats;
	for ( auto m : m_materials )
	{
		vmats.push_back( m );
	}
	QVariantList vUMats;
	if ( m_debug )
	{
		for ( auto m : m_materials )
		{
			vUMats.push_back( DBH::materialUID( m ) );
		}
		if ( m_isMulti )
		{
			for ( auto p : m_selection )
			{
				if ( p.second )
				{
					g->w()->construct( m_item, p.first, 0, Global::util->variantList2UInt( vUMats ), p.first );
				}
			}
		}

		m_debug = false;
		return;
	}

	if ( m_isMulti || shift )
	{
		if ( m_action == "Deconstruct" )
		{
			QSet<unsigned int> workshops;

			QList<QPair<Position, bool>> newSelection;

			for ( auto p : m_selection )
			{
				if ( p.second )
				{
					//if tile belongs to workshop, put in workshop set, else add position to newList
					auto ws = g->wsm()->workshopAt( p.first );
					if ( ws )
					{
						if ( !workshops.contains( ws->id() ) )
						{
							workshops.insert( ws->id() );
							newSelection.append( QPair<Position, bool>( ws->pos(), true ) );
						}
					}
					else
					{
						newSelection.append( p );
					}
				}
			}
			for ( auto p : newSelection )
			{
				if ( p.second )
				{
					g->jm()->addJob( jobId, p.first, m_rotation );
				}
			}

			return;
		}
		else
		{
			for ( auto p : m_selection )
			{
				if ( p.second )
				{
					if ( m_materials.empty() )
					{
						if ( Global::debugMode )
							qDebug() << "Selection::onSecondclick1" << jobId << p.first.toString() << m_item << m_materials << m_rotation;
						g->jm()->addJob( jobId, p.first, m_rotation );
					}
					else
					{
						if ( Global::debugMode )
							qDebug() << "Selection::onSecondclick2" << jobId << p.first.toString() << m_item << m_materials << m_rotation;
						g->jm()->addJob( jobId, p.first, m_item, m_materials, m_rotation );
					}
				}
			}
		}
	}
	else
	{
		if ( !m_selection.empty() )
		{
			auto& p = m_selection.first();
			if ( p.second )
			{
				if ( m_materials.empty() )
				{
					if ( Global::debugMode )
						qDebug() << jobId << p.first.toString() << m_item << m_materials << m_rotation;
					g->jm()->addJob( jobId, p.first, m_rotation );
				}
				else
				{
					if ( Global::debugMode )
						qDebug() << jobId << p.first.toString() << m_item << m_materials << m_rotation;
					g->jm()->addJob( jobId, p.first, m_item, m_materials, m_rotation );
				}
				p.second = false;
			}
		}
	}
}

int Selection::rotation()
{
	if ( m_canRotate )
	{
		return m_rotation;
	}
	else
	{
		return 0;
	}
}

bool Selection::changed()
{
	bool out  = m_changed;
	m_changed = false;
	return out;
}

void Selection::updateGui()
{
	emit signalActionChanged( "" );
	emit signalSize( "" );
}