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
/** @file item.cpp
 *  @brief Implementation of the Item class -- individual game item with material,
 *         quality, ownership state machine, and optional extra data.
 */
#include "item.h"

#include "../base/db.h"
#include "../base/position.h"
#include "../base/util.h"
#include "../gfx/spritefactory.h"
#include "../gui/strings.h"

#include "jobmanager.h"

#include <QDebug>
#include <QVariantMap>

/** @brief Default constructor. Creates an item at the origin. */
Item::Item() :
	Object( Position( 0, 0, 0 ) )
{
}

/** @brief Construct a new item of the given type and material at a position.
 *  @param pos World position for the item.
 *  @param itemSID String ID of the item type (DB key).
 *  @param materialSID String ID of the material (DB key).
 */
Item::Item( Position& pos, QString itemSID, QString materialSID ) :
	Object( pos )
{
	m_itemUID     = DBH::itemUID( itemSID );
	m_materialUID = DBH::materialUID( materialSID );
	int value     = DB::select( "Value", "Items", itemSID ).toFloat() * DB::select( "Value", "Materials", materialSID ).toFloat();
	setValue( value );
	{
		setNutritionalValue( DB::select( "EatValue", "Items", itemSID ).toInt() );
		setDrinkValue( DB::select( "DrinkValue", "Items", itemSID ).toInt() );
	}
}

/** @brief Deserialize an item from a saved QVariantMap.
 *  @param in Map containing all serialized item fields.
 */
Item::Item( QVariantMap in ) :
	Object( in )
{
	/*
	m_id = in.value( "ID" ).toUInt();
	m_position = Position( in.value( "Position" ).toString() );
	m_spriteID = in.value( "SpriteID" ).toUInt();
	*/
	QString itemSID = in.value( "ItemSID" ).toString();
	m_itemUID       = DBH::itemUID( itemSID );
	m_materialUID   = DBH::materialUID( in.value( "MaterialSID" ).toString() );
	m_stockpileID = in.value( "StockpileID" ).toUInt();
	m_containerID = in.value( "ContainerID" ).toUInt();

	m_location      = static_cast<ItemLocation>( in.value( "Location" ).toUInt() );
	m_locationOwner = in.value( "LocationOwner" ).toUInt();
	m_claim         = static_cast<ItemClaim>( in.value( "Claim" ).toUInt() );
	m_claimOwner    = in.value( "ClaimOwner" ).toUInt();

	m_value         = in.value( "Value" ).toUInt();
	m_madeBy        = in.value( "MadeBy" ).toUInt();
	m_quality       = in.value( "Quality" ).toUInt();

	if ( in.value( "Extra" ).toBool() )
	{
		m_extraData = new ItemExtraData;
	}

	if ( in.contains( "Components" ) )
	{
		QVariantList vlc = in.value( "Components" ).toList();
		for ( auto item : vlc )
		{
			QVariantMap cvm = item.toMap();
			ItemMaterial im = { (unsigned int)DBH::itemUID( cvm.value( "ItSID" ).toString() ),
								(unsigned int)DBH::materialUID( cvm.value( "MaSID" ).toString() ) };
			m_extraData->components.append( im );
		}
	}

	if ( in.contains( "EatValue" ) )
	{
		m_extraData->nutritionalValue = in.value( "EatValue" ).toUInt();
	}
	if ( in.contains( "DrinkValue" ) )
	{
		m_extraData->drinkValue = in.value( "DrinkValue" ).toUInt();
	}
}

/** @brief Copy constructor. Deep-copies extra data if present.
 *  @param other Item to copy from.
 */
Item::Item( const Item& other ) :
	m_itemUID( other.m_itemUID ),
	m_materialUID( other.m_materialUID ),
	m_location( other.m_location ),
	m_locationOwner( other.m_locationOwner ),
	m_claim( other.m_claim ),
	m_claimOwner( other.m_claimOwner ),
	m_stockpileID( other.m_stockpileID ),
	m_containerID( other.m_containerID ),
	m_value( other.m_value ),
	m_madeBy( other.m_madeBy ),
	m_quality( other.m_quality ),
	m_extraData( nullptr )
{
	m_id       = other.m_id;
	m_position = other.m_position;
	m_spriteID = other.m_spriteID;

	if ( other.m_extraData != nullptr )
	{
		m_extraData                   = new ItemExtraData;
		m_extraData->components       = other.m_extraData->components;
		m_extraData->nutritionalValue = other.m_extraData->nutritionalValue;
		m_extraData->drinkValue       = other.m_extraData->drinkValue;
	}
}

/** @brief Destructor. Frees extra data if allocated. */
Item::~Item()
{
	if ( m_extraData != nullptr )
	{
		delete m_extraData;
	}
}

/** @brief Serialize the item to a QVariant (QVariantMap) for save games.
 *  @return QVariant containing all item state.
 */
QVariant Item::serialize() const
{
	QVariantMap out;
	Object::serialize( out );
	/*
	out.insert( "ID", m_id );
	out.insert( "Position", m_position.toString() );
	out.insert( "SpriteID", m_spriteID );
	*/
	out.insert( "ItemSID", itemSID() );
	out.insert( "MaterialSID", materialSID() );
	out.insert( "StockpileID", m_stockpileID );
	out.insert( "ContainerID", m_containerID );
	out.insert( "Location", static_cast<unsigned int>( m_location ) );
	out.insert( "LocationOwner", m_locationOwner );
	out.insert( "Claim", static_cast<unsigned int>( m_claim ) );
	out.insert( "ClaimOwner", m_claimOwner );
	out.insert( "Value", m_value );
	out.insert( "MadeBy", m_madeBy );
	out.insert( "Quality", m_quality );

	if ( m_extraData != nullptr )
	{
		out.insert( "Extra", true );

		out.insert( "EatValue", m_extraData->nutritionalValue );
		out.insert( "DrinkValue", m_extraData->drinkValue );

		if ( !m_extraData->components.empty() )
		{
			QVariantList vli;
			for ( auto comp : m_extraData->components )
			{
				QVariantMap vm;
				vm.insert( "ItSID", DBH::itemSID( comp.itemUID ) );
				vm.insert( "MaSID", DBH::materialSID( comp.materialUID ) );
				vli.push_back( vm );
			}
			out.insert( "Components", vli );
		}
	}
	return out;
}

/** @brief Return the material database UID.
 *  @return Material UID.
 */
unsigned short Item::materialUID() const
{
	return m_materialUID;
}

/** @brief Return the item type database UID.
 *  @return Item type UID.
 */
unsigned short Item::itemUID() const
{
	return m_itemUID;
}

/** @brief Build a pixmap/sprite string ID from item and material SIDs.
 *  @return Combined sprite identifier string (e.g. "Pickaxe_Iron").
 */
QString Item::getPixmapSID() const
{
	return DBH::spriteID( DBH::itemSID( m_itemUID ) ) + "_" + DBH::materialSID( m_materialUID );
}

/** @brief Return a human-readable designation string (e.g. "Iron Pickaxe").
 *  @return Localized material + item name.
 */
QString Item::getDesignation() const
{
	return S::s( "$MaterialName_" + DBH::materialSID( m_materialUID ) ) + " " + S::s( "$ItemName_" + DBH::itemSID( m_itemUID ) );
}

/** @brief Return the item type string ID (DB key).
 *  @return Item SID such as "Pickaxe".
 */
QString Item::itemSID() const
{
	return DBH::itemSID( m_itemUID );
}

/** @brief Return the material string ID (DB key).
 *  @return Material SID such as "Iron".
 */
QString Item::materialSID() const
{
	return DBH::materialSID( m_materialUID );
}

/** @brief Return a combined "ItemSID_MaterialSID" string.
 *  @return Combined identifier.
 */
QString Item::combinedSID() const
{
	return DBH::itemSID( m_itemUID ) + "_" + DBH::materialSID( m_materialUID );
}

/** @brief Compute squared distance from this item to a position, with optional Z-axis weight.
 *  @param pos Target position.
 *  @param zWeight Multiplier for vertical distance (default 1).
 *  @return Weighted squared distance.
 */
int Item::distanceSquare( const Position& pos, int zWeight ) const
{
	return ( m_position.x - pos.x ) * ( m_position.x - pos.x ) + ( m_position.y - pos.y ) * ( m_position.y - pos.y ) + ( m_position.z - pos.z ) * ( m_position.z - pos.z ) * zWeight;
}

/** @brief Check if this item is claimed by a job.
 *  @return Job ID if claimed, 0 otherwise.
 */
unsigned int Item::isInJob() const
{
	return ( m_claim == ItemClaim::Job ) ? m_claimOwner : 0;
}

/** @brief Claim or release this item for a job.
 *  @param job Job ID to claim for, or 0 to release.
 */
void Item::setInJob( unsigned int job )
{
	if ( job != 0 )
	{
		setClaim( ItemClaim::Job, job );
	}
	else if ( m_claim == ItemClaim::Job )
	{
		setClaim( ItemClaim::None, 0 );
	}
}

/** @brief Check if this item is being carried by a creature.
 *  @return Creature ID if carried, 0 otherwise.
 */
unsigned int Item::isHeldBy() const
{
	return ( m_location == ItemLocation::Carried ) ? m_locationOwner : 0;
}

/** @brief Set the item as carried by a creature, or put it back on the ground.
 *  @param creatureID Creature carrying the item, or 0 to place on ground.
 */
void Item::setHeldBy( unsigned int creatureID )
{
	if ( creatureID != 0 )
	{
		setLocation( ItemLocation::Carried, creatureID );
	}
	else
	{
		setLocation( ItemLocation::Ground, 0 );
	}
}

/** @brief Check if this item is equipped by a creature.
 *  @return Creature ID if equipped, 0 otherwise.
 */
unsigned int Item::isUsedBy() const
{
	return ( m_claim == ItemClaim::Equipped ) ? m_claimOwner : 0;
}

/** @brief Set the item as equipped by a creature, or unequip it.
 *  @param creatureID Creature equipping the item, or 0 to unequip.
 */
void Item::setUsedBy( unsigned int creatureID )
{
	if ( creatureID != 0 )
	{
		setClaim( ItemClaim::Equipped, creatureID );
	}
	else if ( m_claim == ItemClaim::Equipped )
	{
		setClaim( ItemClaim::None, 0 );
	}
}

/** @brief Return the maximum stack size for this item type from the DB.
 *  @return Stack size.
 */
unsigned char Item::stackSize() const
{
	return DB::select( "StackSize", "Items", DBH::itemSID( m_itemUID ) ).toUInt();
}

/** @brief Add a component material to this item, allocating extra data if needed.
 *  @param im Item-material pair to add as a component.
 */
void Item::addComponent( ItemMaterial im )
{
	if ( m_extraData == nullptr )
	{
		m_extraData = new ItemExtraData;
	}
	m_extraData->components.push_back( im );
}

/** @brief Return the list of component materials for this item.
 *  @return List of ItemMaterial pairs, or empty if no components.
 */
QList<ItemMaterial> Item::components() const
{
	if ( m_extraData )
	{
		return m_extraData->components;
	}
	return {};
}

/** @brief Return the item's trade/display value, adjusted by quality modifier.
 *  @return Quality-adjusted value.
 */
unsigned short Item::value() const
{
	return m_value * DBH::qualityMod(m_quality);
}

/** @brief Return the creature ID that crafted this item.
 *  @return Creature ID, or 0 if unknown.
 */
unsigned int Item::madeBy() const
{
	return m_madeBy;
}

/** @brief Set the base value of this item (before quality modifier).
 *  @param value Base value.
 */
void Item::setValue( unsigned short value )
{
	m_value = value;
}

/** @brief Record which creature crafted this item.
 *  @param creatureID ID of the crafter.
 */
void Item::setMadeBy( unsigned int creatureID )
{
	m_madeBy = creatureID;
}

/** @brief Return the quality tier (DB row ID in Quality table, 0 = no quality).
 *  @return Quality level.
 */
unsigned char Item::quality() const
{
	return m_quality;
}

/** @brief Set the quality tier for this item.
 *  @param quality Quality level (row ID in Quality table).
 */
void Item::setQuality( unsigned char quality )
{
	m_quality = quality;
}

/** @brief Return how much hunger this item satisfies when eaten.
 *  @return Nutritional value, or 0 if not edible.
 */
unsigned char Item::nutritionalValue() const
{
	if ( m_extraData )
	{
		return m_extraData->nutritionalValue;
	}
	return 0;
}

/** @brief Set the nutritional value. Allocates extra data on demand.
 *  @param value Nutritional value (0 means not edible, no allocation).
 */
void Item::setNutritionalValue( unsigned char value )
{
	if ( value != 0 )
	{
		if ( m_extraData == nullptr )
		{
			m_extraData = new ItemExtraData;
		}
		m_extraData->nutritionalValue = value;
	}
}

/** @brief Return how much thirst this item satisfies when consumed.
 *  @return Drink value, or 0 if not drinkable.
 */
unsigned char Item::drinkValue() const
{
	if ( m_extraData )
	{
		return m_extraData->drinkValue;
	}
	return 0;
}

/** @brief Set the drink value. Allocates extra data on demand.
 *  @param value Drink value (0 means not drinkable, no allocation).
 */
void Item::setDrinkValue( unsigned char value )
{
	if ( value != 0 )
	{
		if ( m_extraData == nullptr )
		{
			m_extraData = new ItemExtraData;
		}
		m_extraData->drinkValue = value;
	}
}

/** @brief Return the custom color of this item as a packed unsigned int.
 *  @return Color value, or 0 if no custom color.
 */
unsigned int Item::color() const
{
	if ( m_extraData )
	{
		return m_extraData->color;
	}
	return 0;
}

/** @brief Set a custom color from a color string. Allocates extra data on demand.
 *  @param color Color string to parse (e.g. "#FF0000").
 */
void Item::setColor( QString color )
{
	//TODO add alpha
	if ( !color.isEmpty() )
	{
		if ( m_extraData == nullptr )
		{
			m_extraData = new ItemExtraData;
		}
		m_extraData->color = Global::util->string2Color( color );
	}
}

/** @brief Return the attack value of this item type from the DB.
 *  @return Attack value, or 0 for non-weapons.
 */
int Item::attackValue() const
{
	return DB::select( "AttackValue", "Items", DBH::itemSID( m_itemUID ) ).toInt();
}

/** @brief Check whether this item type has a positive attack value (is a weapon).
 *  @return True if the item is a weapon.
 */
bool Item::isWeapon() const
{
	return DB::select( "AttackValue", "Items", DBH::itemSID( m_itemUID ) ).toInt() > 0;
}

/** @brief Check whether this item type is flagged as a tool in the DB.
 *  @return True if the item is a tool.
 */
bool Item::isTool() const
{
	return DB::select( "IsTool", "Items", DBH::itemSID( m_itemUID ) ).toBool();
}

/** @brief Check whether this item is free (on the ground and not claimed).
 *  @return True if location is Ground and claim is None.
 */
bool Item::isFree() const
{
	return m_location == ItemLocation::Ground && m_claim == ItemClaim::None;
}

/** @brief Set the physical location state and owner.
 *  @param loc New location (Ground or Carried).
 *  @param owner ID of the entity holding the item, or 0.
 */
void Item::setLocation( ItemLocation loc, unsigned int owner )
{
	m_location = loc;
	m_locationOwner = owner;
}

/** @brief Set the claim/reservation state and owner.
 *  @param cl New claim state (None, Job, or Equipped).
 *  @param owner ID of the job or creature claiming, or 0.
 */
void Item::setClaim( ItemClaim cl, unsigned int owner )
{
	m_claim = cl;
	m_claimOwner = owner;
}