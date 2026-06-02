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
/** @file aggregatorcreatureinfo.cpp
 *  @brief AggregatorCreatureInfo implementation: fills GuiCreatureInfo for gnomes, monsters,
 *         and animals; encodes per-slot equipment/uniform sprites as PNG byte buffers for the
 *         Noesis view model to consume.
 */
#include "aggregatorcreatureinfo.h"

#include "../base/db.h"
#include "../base/dbhelper.h"
#include "../base/global.h"
#include "../base/util.h"

#include "../game/game.h"
#include "../game/creaturemanager.h"
#include "../game/gnomemanager.h"
#include "../game/militarymanager.h"

#include "../gfx/spritefactory.h"

#include "../gui/strings.h"

/// @brief Constructs the AggregatorCreatureInfo.
/// @param parent Qt parent object.
AggregatorCreatureInfo::AggregatorCreatureInfo( QObject* parent ) :
	QObject(parent)
{
}

/// @brief Binds the aggregator to a Game instance.
/// @param game Game to bind to.
void AggregatorCreatureInfo::init( Game* game )
{
	g = game;
}

/// @brief Re-sends the current creature payload if a creature is currently being displayed.
void AggregatorCreatureInfo::update()
{
	if( m_currentID != 0 )
	{
		onRequestCreatureUpdate( m_currentID );
	}
}

/// @brief Fills GuiCreatureInfo from the given creature (gnome first, then monster, then
///        animal) and emits signalCreatureUpdate. For gnomes it also rebuilds the per-slot
///        equipment PNG buffers when the equipment has changed or the creature has switched.
/// @param id Creature UID to display.
void AggregatorCreatureInfo::onRequestCreatureUpdate( unsigned int id )
{
	if( !g ) return;
	m_currentID = id;
	auto gnome = g->gm()->gnome( id );
	if( gnome )
	{
		m_info.name = gnome->name();
		m_info.id = id;
		m_info.profession = gnome->profession();

		m_info.str = gnome->attribute( "Str" );
		m_info.con = gnome->attribute( "Con" );
		m_info.dex = gnome->attribute( "Dex" );
		m_info.intel = gnome->attribute( "Int" );
		m_info.wis = gnome->attribute( "Wis" );
		m_info.cha = gnome->attribute( "Cha" );

		m_info.hunger = gnome->need( "Hunger" );
		m_info.thirst = gnome->need( "Thirst" );
		m_info.sleep = gnome->need( "Sleep" );
		m_info.happiness = gnome->need( "Happiness" );

		m_info.activity = "Doing something. tbi";

		if( gnome->roleID() )
		{
			m_info.uniform = g->mil()->uniformCopy( gnome->roleID() );
		}
		m_info.equipment = gnome->equipment();
		
		if( m_previousID != m_currentID || gnome->equipmentChanged() )
		{
			m_previousID = m_currentID;

			m_info.itemPics.clear();
			if( m_info.equipment.head.itemID )
			{
				createUniformImg( "ArmorHead", m_info.uniform.parts.value( "HeadArmor" ), m_info.equipment.head );
			}
			if( m_info.equipment.chest.itemID )
			{
				createUniformImg( "ArmorChest", m_info.uniform.parts.value( "ChestArmor" ), m_info.equipment.chest );
			}
			if( m_info.equipment.arm.itemID )
			{
				createUniformImg( "ArmorArms", m_info.uniform.parts.value( "ArmArmor" ), m_info.equipment.arm );
			}
			if( m_info.equipment.hand.itemID )
			{
				createUniformImg( "ArmorHands", m_info.uniform.parts.value( "HandArmor" ), m_info.equipment.hand );
			}
			if( m_info.equipment.leg.itemID )
			{
				createUniformImg( "ArmorLegs", m_info.uniform.parts.value( "LegArmor" ), m_info.equipment.leg );
			}
			if( m_info.equipment.foot.itemID )
			{
				createUniformImg( "ArmorFeet", m_info.uniform.parts.value( "FootArmor" ), m_info.equipment.foot );
			}
			if( m_info.equipment.leftHandHeld.itemID )
			{
				createItemImg( "LeftHandHeld", m_info.equipment.leftHandHeld );
			}
			if( m_info.equipment.rightHandHeld.itemID )
			{
				createItemImg( "RightHandHeld", m_info.equipment.rightHandHeld );
			}
		}

		emit signalCreatureUpdate( m_info );
		return;
	}
	else
	{
		auto monster = g->cm()->monster( id );
		if( monster )
		{
			m_info.name = monster->name();
			m_info.id = id;
			//m_info.profession = monster->profession();

			m_info.str = monster->attribute( "Str" );
			m_info.con = monster->attribute( "Con" );
			m_info.dex = monster->attribute( "Dex" );
			m_info.intel = monster->attribute( "Int" );
			m_info.wis = monster->attribute( "Wis" );
			m_info.cha = monster->attribute( "Cha" );

			m_info.hunger = 100; //monster->need( "Hunger" );
			m_info.thirst = 100; //monster->need( "Thirst" );
			m_info.sleep = 100; //monster->need( "Sleep" );
			m_info.happiness = 100; //monster->need( "Happiness" );
			
			m_info.activity = "Doing something. tbi";
			emit signalCreatureUpdate( m_info );
			return;
		}
		else
		{
			auto animal = g->cm()->animal( id );
			if( animal )
			{
				m_info.name = animal->name();
				m_info.id = id;
				//m_info.profession = animal->profession();

				m_info.str = animal->attribute( "Str" );
				m_info.con = animal->attribute( "Con" );
				m_info.dex = animal->attribute( "Dex" );
				m_info.intel = animal->attribute( "Int" );
				m_info.wis = animal->attribute( "Wis" );
				m_info.cha = animal->attribute( "Cha" );

				m_info.hunger = animal->hunger();
				m_info.thirst = 100; //animal->need( "Thirst" );
				m_info.sleep = 100; //animal->need( "Sleep" );
				m_info.happiness = 100; //animal->need( "Happiness" );
			
				m_info.activity = "Doing something. tbi";
				emit signalCreatureUpdate( m_info );
				return;
			}
		}
		
	}
	m_currentID = 0;
}


/// @brief Emits the list of available profession names to the GUI.
void AggregatorCreatureInfo::onRequestProfessionList()
{
	if( !g ) return;
	emit signalProfessionList( g->gm()->professions() );
}

/// @brief Assigns a new profession to the given gnome (no-op if it's the current profession).
/// @param gnomeID    Creature UID of the gnome.
/// @param profession New profession name.
void AggregatorCreatureInfo::onSetProfession( unsigned int gnomeID, QString profession )
{
	if( !g ) return;
	auto gnome = g->gm()->gnome( gnomeID );
	if( gnome )
	{
		QString oldProf = gnome->profession();
		if( oldProf != profession )
		{
			gnome->selectProfession( profession );
			//onUpdateSingleGnome( gnomeID );
		}
	}
}

/// @brief Builds a PNG byte buffer for a held item (sword, shield, etc.) and stores it in
///        GuiCreatureInfo::itemPics under @p slot. If the sprite cannot be created, the item
///        is cleared so the GUI draws the empty-slot icon instead.
/// @param slot  GUI slot key.
/// @param eItem Equipment item (may be mutated to clear itemID on failure).
void AggregatorCreatureInfo::createItemImg( QString slot, EquipmentItem& eItem )
{
	if( !g ) return;
	if( eItem.itemID == 0 )
	{
		return;
	}
	QStringList mats;
	if( eItem.allMats.size() )
	{
		mats = eItem.allMats;
	}
	else
	{
		mats.append( eItem.material );
		mats.append( "Pine" );
	}

	auto sprite = g->sf()->createSprite( "UI" + eItem.item, mats );
	if( sprite )
	{
		QPixmap pm = sprite->pixmap( "Spring", 0, 0 );

		std::vector<unsigned char> buffer;

		Global::util->createBufferForNoesisImage( pm, buffer );
		m_info.itemPics.insert( slot, buffer );
	}
	else
	{
		eItem.itemID = 0;
	}
}

/// @brief Builds a PNG byte buffer for an armour slot by combining the uniform type prefix
///        ("UI" + uniform type + slot) with the equipped material, storing the result under
///        @p slot. Clears the equipment item's itemID on sprite-lookup failure.
/// @param slot  GUI slot key.
/// @param uItem Uniform definition for this slot.
/// @param eItem Currently equipped item (may be mutated to clear itemID on failure).
void AggregatorCreatureInfo::createUniformImg( QString slot, const UniformItem& uItem, EquipmentItem& eItem )
{
	if( !g ) return;
	if( uItem.item.isEmpty() || eItem.itemID == 0 )
	{
		return; 
	}
	QStringList mats;
	mats.append( eItem.material );

	auto sprite = g->sf()->createSprite( "UI" + uItem.type + slot, mats );
	if( sprite )
	{
		QPixmap pm = sprite->pixmap( "Spring", 0, 0 );

		std::vector<unsigned char> buffer;

		Global::util->createBufferForNoesisImage( pm, buffer );
		m_info.itemPics.insert( slot, buffer );
	}
	else
	{
		eItem.itemID = 0;
	}
}

/// @brief Builds a PNG buffer for the given empty-slot placeholder sprite and stores it in
///        m_emptyPics. Falls back to a fully-transparent 32×32 buffer if sprite creation fails.
/// @param spriteID Placeholder sprite ID (e.g. "UIEmptySlotHead").
void AggregatorCreatureInfo::createEmptyUniformImg( QString spriteID )
{
	if( !g ) return;
	QStringList mats; 
	mats.append( "any" );
	
	auto sprite = g->sf()->createSprite( spriteID, mats );
	if( sprite )
	{
		QPixmap pm = sprite->pixmap( "Spring", 0, 0 );

		std::vector<unsigned char> buffer;

		Global::util->createBufferForNoesisImage( pm, buffer );

		m_emptyPics.insert( spriteID, buffer );
	}
	else
	{
		std::vector<unsigned char> buffer;
		buffer.resize( 32 * 32 * 4, 0 );
		m_emptyPics.insert( spriteID, buffer );
	}
}

/// @brief Builds PNG buffers for all empty-slot placeholder sprites (head/chest/arms/hands/
///        legs/feet/shield/weapon/back/neck/ring) and emits signalEmptyPics.
void AggregatorCreatureInfo::onRequestEmptySlotImages()
{
	if( !g ) return;
	m_emptyPics.clear();

	createEmptyUniformImg( "UIEmptySlotHead" );
	createEmptyUniformImg( "UIEmptySlotChest" );
	createEmptyUniformImg( "UIEmptySlotArms" );
	createEmptyUniformImg( "UIEmptySlotHands" );
	createEmptyUniformImg( "UIEmptySlotLegs" );
	createEmptyUniformImg( "UIEmptySlotFeet" );
	createEmptyUniformImg( "UIEmptySlotShield" );
	createEmptyUniformImg( "UIEmptySlotWeapon" );
	createEmptyUniformImg( "UIEmptySlotBack" );
	createEmptyUniformImg( "UIEmptySlotNeck" );
	createEmptyUniformImg( "UIEmptySlotRing" );
	createEmptyUniformImg( "UIEmptySlotRing" );

	emit signalEmptyPics( m_emptyPics );
}