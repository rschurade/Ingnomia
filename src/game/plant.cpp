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
#include "plant.h"

#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/util.h"
#include "../game/creaturemanager.h"
#include "../game/gnomemanager.h"
#include "../game/inventory.h"
#include "../game/world.h"
#include "../gfx/sprite.h"
#include "../gfx/spritefactory.h"
#include "../gui/strings.h"

#include <QDebug>
#include <QPainter>
#include <QRandomGenerator>

#include <random>

Plant::Plant() :
	Object( Position() ),
	m_plantID( "None" )
{
}

Plant::Plant( Position& pos, QString ID, bool fullyGrown ) :
	Object( pos ),
	m_plantID( ID )
{
	QVariantMap row = DB::selectRow( "Plants", ID );

	m_isTree             = ( row.value( "Type" ).toString() == "Tree" );
	m_isFruitTree        = ( !row.value( "FruitItemID" ).toString().isEmpty() );
	m_isPlant            = ( row.value( "Type" ).toString() == "Plant" );
	m_isMushroom         = ( row.value( "Type" ).toString() == "Mushroom" );
	m_isUndergroundPlant = ( row.value( "Type" ).toString() == "UndergroundPlant" );

	m_producesHarvest = row.contains( "OnHarvest" );

	setGrowsThisSeason();

	if ( fullyGrown )
	{
		m_state      = DB::numRows( "Plants_States", m_plantID ) - 1;
		m_fullyGrown = true;
		if ( m_isTree )
		{
			m_matureWood = true;
			if ( isFruitTree() )
			{
				m_harvestable = true;
				m_numFruits   = row.value( "NumFruitsPerSeason" ).toInt();
			}
		}
		if ( m_isPlant )
		{
			m_harvestable = true;
		}
	}

	updateState();

	QString growLight = row.value( "GrowsIn" ).toString();
	if ( growLight == "Sun" )
	{
		m_growLight = GrowLight::SUN;
	}
	else if ( growLight == "Dark" )
	{
		m_growLight = GrowLight::DARK;
	}
	else
	{
		m_growLight = GrowLight::SUN_AND_DARK;
	}

	setGrowTime();
}

Plant::Plant( QVariant values ) :
	Object( Position() )
{
	QVariantMap in = values.toMap();

	m_id               = in.value( "ID" ).toUInt();
	m_position         = Position( in.value( "Pos" ).toString() );
	m_plantID          = in.value( "PlantID" ).toString();
	m_state            = in.value( "State" ).toUInt();
	m_ticksToNextState = in.value( "TTNS" ).toInt();

	QVariantMap row = DB::selectRow( "Plants", m_plantID );
	QVariantList sl = row["States"].toList();
	m_fullyGrown    = ( m_state == ( sl.size() - 1 ) );
	//m_fullyGrown = in.value( "FullyGrown" ).toBool();
	m_matureWood         = in.value( "MatureWood" ).toBool();
	m_harvestable        = in.value( "Harvestable" ).toBool();
	m_producesHarvest    = in.value( "ProdHarvest" ).toBool();
	m_isTree             = in.value( "IsTree" ).toBool();
	m_isFruitTree        = ( !row.value( "FruitItemID" ).toString().isEmpty() ); //in.value( "IsFruitTree" ).toBool(); //TODO for compatibility with old saves
	m_isMulti            = in.value( "IsMulti" ).toBool();
	m_isPlant            = in.value( "IsPlant" ).toBool();
	m_isUndergroundPlant = in.value( "IsUnderground" ).toBool();
	m_numFruits          = in.value( "NumFruits" ).toInt();

	setGrowsThisSeason();

	QString growLight = in.value( "GrowsIn" ).toString();
	if ( growLight == "Sun" )
	{
		m_growLight = GrowLight::SUN;
	}
	else if ( growLight == "Dark" )
	{
		m_growLight = GrowLight::DARK;
	}
	else
	{
		m_growLight = GrowLight::SUN_AND_DARK;
	}

	updateState();
}

Plant::~Plant()
{
}

QVariant Plant::serialize()
{
	QVariantMap out;

	out.insert( "ID", m_id );
	out.insert( "Pos", m_position.toString() );

	out.insert( "PlantID", m_plantID );
	out.insert( "State", m_state );
	out.insert( "TTNS", m_ticksToNextState );
	out.insert( "FullyGrown", m_fullyGrown );
	out.insert( "MatureWood", m_matureWood );
	out.insert( "Harvestable", m_harvestable );
	out.insert( "ProdHarvest", m_producesHarvest );
	out.insert( "IsTree", m_isTree );
	out.insert( "IsFruitTree", m_isFruitTree );
	out.insert( "IsMulti", m_isMulti );
	out.insert( "IsPlant", m_isPlant );
	out.insert( "IsUnderground", m_isUndergroundPlant );
	out.insert( "NumFruits", m_numFruits );
	switch ( m_growLight )
	{
		case GrowLight::SUN:
			out.insert( "GrowsIn", "Sun" );
			break;
		case GrowLight::DARK:
			out.insert( "GrowsIn", "Dark" );
			break;
		case GrowLight::SUN_AND_DARK:
			out.insert( "GrowsIn", "SunAndDark" );
			break;
	}
	return out;
}

void Plant::setGrowsThisSeason()
{
	QVariantMap row     = DB::selectRow( "Plants", m_plantID );
	QString season      = GameState::seasonString;
	auto growSeasonList = row.value( "GrowsInSeason" ).toString().split( "|" );
	for ( auto gs : growSeasonList )
	{
		if ( gs == season )
		{
			m_growsThisSeason = true;
			return;
		}
	}
	m_growsThisSeason = false;
}

bool Plant::growsThisSeason()
{
	return m_growsThisSeason;
}

OnTickReturn Plant::onTick( quint64 tickNumber, bool dayChanged, bool seasonChanged )
{
	return liveOneTick( dayChanged, seasonChanged );
}

OnTickReturn Plant::liveOneTick( bool dayChanged, bool seasonChanged )
{
	if ( m_fullyGrown && !seasonChanged )
	{
		return OnTickReturn::NOOP;
	}

	if ( seasonChanged )
	{
		QVariantMap row = DB::selectRow( "Plants", m_plantID );
		QString season  = GameState::seasonString;

		setGrowsThisSeason();

		// if is killed in season and is that season
		QString isKilledInSeason = row.value( "IsKilledInSeason" ).toString();
		if ( isKilledInSeason == season )
		{
			m_state       = 0;
			m_harvestable = false;
			m_fullyGrown  = false;
			m_numFruits   = 0;
			setGrowTime();
			updateState();
			return OnTickReturn::UPDATE;
		}
		// if loses fruit and has fruit
		else if ( harvestable() )
		{
			QString losesFruitInSeason = row.value( "LosesFruitInSeason" ).toString();
			if ( losesFruitInSeason == season )
			{
				m_harvestable = false;
				m_fullyGrown  = false;
				m_state       = qMax( 0, m_state - 1 );
				m_numFruits   = 0;
				setGrowTime();
				updateState();
				return OnTickReturn::UPDATE;
			}
		}
	}

	if ( m_growsThisSeason )
	{
		switch ( m_growLight )
		{
			case GrowLight::SUN:
				if ( GameState::daylight && Global::w().hasSunlight( m_position ) )
					--m_ticksToNextState;
				break;
			case GrowLight::DARK:
				if ( !GameState::daylight )
					--m_ticksToNextState;
				break;
			case GrowLight::SUN_AND_DARK:
				--m_ticksToNextState;
				break;
		}
	}

	if ( m_ticksToNextState == 0 )
	{
		setGrowTime();
		++m_state;
		updateState();
		return OnTickReturn::UPDATE;
	}
	return OnTickReturn::NOOP;
}

void Plant::setGrowTime()
{
	auto sl = DB::selectRows( "Plants_States", m_plantID );

	if ( m_state < sl.size() - 1 )
	{
		QVariantMap sm     = sl[m_state];
		int ticks          = sm["GrowTime"].toFloat() * Util::ticksPerDay;
		int dev            = ticks * 0.05;
		int rand           = ( QRandomGenerator::global()->generate() % dev ) - ( dev / 2 );
		m_ticksToNextState = ticks + rand;
		//float stddev = mean*0.05;
		//std::normal_distribution<float> dist( mean, stddev );
		//m_ticksToNextState = dist( *QRandomGenerator64::global() );
	}
}

void Plant::updateState()
{
	auto sl = DB::selectRows( "Plants_States", m_plantID );

	if ( m_state < sl.size() )
	{
		QVariantMap sm   = sl[m_state];
		m_isMulti        = !sm.value( "Layout" ).toString().isEmpty();
		QString spriteID = sm.value( "SpriteID" ).toString();

		m_matureWood = sm.value( "Fell" ).toBool();

		if ( m_harvestable = sm.value( "Harvest" ).toBool() )
		{
			m_numFruits = DB::select( "NumFruitsPerSeason", "Plants", m_plantID ).toInt();
		}
		if ( !m_isMulti )
		{
			m_sprite = Global::sf().createSprite( spriteID, { DB::select( "Material", "Plants", m_plantID ).toString() } );
			Global::w().setWallSprite( m_position, m_sprite->uID );
		}
		else
		{
			layoutMulti( sm.value( "Layout" ).toString(), m_numFruits > 0 );
		}
	}
	if ( m_state > sl.size() - 1 )
	{
		m_state = sl.size() - 1;
	}
	if ( m_state == sl.size() - 1 )
	{
		m_fullyGrown = true;
	}
}

bool Plant::isFruitTree()
{
	return m_isFruitTree;
}

QString Plant::getDesignation()
{
	if ( m_isTree )
	{
		QString materialID = DB::select( "Material", "Plants", m_plantID ).toString();
		return S::s( "$MaterialName_" + materialID ) + " " + S::s( "$Tree" );
	}
	else if ( m_isPlant || m_isMushroom )
	{
		return S::s( "$MaterialName_" + m_plantID ) + " " + S::s( "$Plant" );
	}
	return "error: unset plant designation";
}

bool Plant::reduceOneGrowLevel()
{
	if ( m_isTree )
	{
		m_state = qMax( 0, m_state - 1 );

		setGrowTime();
		updateState();

		m_fullyGrown = false;
		if ( m_state == 0 )
		{
			return true;
		}
	}
	return false;
}

bool Plant::fell()
{
	if ( m_isTree )
	{
		auto rows = DB::selectRows( "Plants_OnFell", m_plantID );
		QString itemID;
		QString materialID;
		for ( auto row : rows )
		{
			itemID     = row.value( "ItemID" ).toString();
			materialID = row.value( "MaterialID" ).toString();

			if ( row.value( "Random" ).toInt() > 0 )
			{
				int random = row.value( "Random" ).toInt();

				int randVal = ( rand() % random ) + 1;
				for ( int i = 0; i < randVal; ++i )
				{
					Global::inv().createItem( m_position, itemID, materialID );
				}
				continue;
			}

			if ( m_matureWood )
			{
				auto sl = DB::selectRows( "Plants_States", m_plantID );

				if ( m_state < sl.size() )
				{
					QVariantMap sm = sl[m_state];

					if ( !sm.value( "Layout" ).toString().isEmpty() )
					{
						auto ll      = DB::selectRows( "TreeLayouts_Layout", m_plantID );
						World& world = Global::w();
						for ( auto vm : ll )
						{
							Position offset( vm.value( "Offset" ).toString() );
							Position newPos = m_position + offset;
							world.setWallSprite( newPos, 0, 0 );
							world.clearTileFlag( newPos, TileFlag::TF_OCCUPIED );
							if ( world.floorType( newPos ) & FT_SOLIDFLOOR )
							{
								world.setTileFlag( newPos, TileFlag::TF_WALKABLE );
							}

							if ( newPos.x == 0 || newPos.x == Global::dimX - 1 || newPos.y == 0 || newPos.y == Global::dimX - 1 )
							{
								Global::inv().createItem( m_position, itemID, materialID );
							}
							else
							{
								world.getFloorLevelBelow( newPos, false );
								Global::inv().createItem( newPos, itemID, materialID );
							}
						}
					}
					else
					{
						Global::inv().createItem( m_position, itemID, materialID );
					}
				}
			}
		}
		return true;
	}
	return false;
}

bool Plant::harvest( Position& pos )
{
	auto rows = DB::selectRows( "Plants_OnHarvest", m_plantID );

	for ( auto row : rows )
	{
		auto harvItemsList = DB::selectRows( "Plants_OnHarvest_HarvestedItem", m_plantID );
		for ( auto harvItem : harvItemsList )
		{
			QString itemID     = harvItem.value( "ItemID" ).toString();
			QString materialID = harvItem.value( "MaterialID" ).toString();
			if ( harvItem.contains( "Chance" ) )
			{
				float chance = harvItem.value( "Chance" ).toFloat();
				int ra       = rand() % 100;
				if ( chance * 100 <= ra )
				{
					Global::inv().createItem( pos, itemID, materialID );
				}
			}
			else
			{
				Global::inv().createItem( pos, itemID, materialID );
			}
		}
		QString action = row.value( "Action" ).toString();
		if ( action == "Destroy" )
		{
			return true;
		}
		if ( action == "ReduceFruitCount" )
		{
			if ( m_numFruits == 0 )
			{
				// something else, most likely winter killed all fruits already
				return false;
			}

			m_numFruits = qMax( 0, m_numFruits - 1 );
			if ( m_numFruits == 0 )
			{
				m_harvestable = false;
				m_fullyGrown  = false;
				m_state       = qMax( 0, m_state - 1 );
				setGrowTime();
				updateState();
				return false;
			}
		}
		else if ( action == "StateOneBack" )
		{
			m_harvestable = false;
			m_state       = qMax( 0, m_state - 1 );
			setGrowTime();
			updateState();
			return false;
		}
	}
	return false;
}

bool Plant::harvestable()
{
	return m_harvestable;
}

void Plant::layoutMulti( QString layoutSID, bool withFruit )
{
	World& world      = Global::w();
	SpriteFactory& sf = Global::sf();

	Position extractPos = m_position.eastOf();
	if ( !Global::w().isWalkable( extractPos ) )
	{
		extractPos = m_position.southOf();

		if ( !Global::w().isWalkable( extractPos ) )
		{
			extractPos = m_position.westOf();

			if ( !Global::w().isWalkable( extractPos ) )
			{
				extractPos = m_position.northOf();
			}
		}
	}

	auto ll = DB::selectRows( "TreeLayouts_Layout", layoutSID );

	bool _isFruitTree = isFruitTree();

	for ( auto vm : ll )
	{
		Position offset( vm.value( "Offset" ).toString() );

		QString spriteID = vm.value( "SpriteID" ).toString();

		if ( offset == Position( 0, 0, 0 ) )
		{
			m_sprite = sf.createSprite( spriteID, { "None" } );
		}

		if ( withFruit && _isFruitTree )
		{
			if ( vm.value( "FruitPos" ).toBool() )
			{
				if ( rand() % 3 == 0 )
					spriteID += "WithFruit";
			}
		}
		Sprite* sprite = sf.createSprite( spriteID, { "None" } );
		if ( ( m_position.z + offset.z ) < Global::dimZ )
		{
			Position pos = m_position + offset;

			world.removeWall( pos, extractPos );
			if ( pos.z > m_position.z )
			{
				//world.removeFloor( pos, extractPos );
			}

			world.setWallSprite( pos, sprite->uID, Util::rotString2Char( vm.value( "Rotation" ).toString() ) );
			world.clearTileFlag( pos, TileFlag::TF_WALKABLE );
			world.setTileFlag( pos, TileFlag::TF_OCCUPIED );

			Global::gm().forceMoveGnomes( pos, extractPos );
			Global::cm().forceMoveAnimals( pos, extractPos );
			Global::w().expelTileItems( pos, extractPos );
			if ( m_isTree )
			{
				world.setTileFlag( pos, TileFlag::TF_SUNLIGHT );
			}
		}
	}
}

void Plant::speedUpGrowth( unsigned int ticks )
{
	if ( m_ticksToNextState > 1 )
	{
		ticks = qMin( ticks, m_ticksToNextState - 1 );
	}
	else
	{
		ticks = 0;
	}

	m_ticksToNextState = m_ticksToNextState - ticks;
}
