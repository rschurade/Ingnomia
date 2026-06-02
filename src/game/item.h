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
/** @file item.h
 *  @brief Individual item instance with material, quality, sprite, position, and
 *         location/claim state machine for ownership tracking.
 */
#pragma once

#include "object.h"

/** @brief Where the item physically exists in the world. */
enum class ItemLocation : uint8_t
{
	Ground,    // on a tile, in position index + octree
	Carried,   // held by a creature, not in position index
};

/** @brief Reservation/claim state for an item. */
enum class ItemClaim : uint8_t
{
	None,      // free to be claimed
	Job,       // reserved for a job
	Equipped,  // equipped by a creature
};

/** @brief Pair of item-type UID and material UID, used for component tracking. */
struct ItemMaterial
{
	unsigned int itemUID;
	unsigned int materialUID;
};

/** @brief Optional extra data for items that have components, food/drink values, or color.
 *
 *  Allocated on demand to save memory for simple items.
 */
struct ItemExtraData
{
	QList<ItemMaterial> components;

	unsigned char nutritionalValue = 0;
	unsigned char drinkValue       = 0;

	unsigned int color = 0;
};

/** @brief A single item instance in the game world.
 *
 *  Tracks item type and material (as DB UIDs), ownership via a location/claim
 *  state machine, value, quality, optional components, and food/drink values.
 *  Inherits position and sprite from Object.
 */
class Item : public Object
{
public:
	Item();
	Item( Position& pos, QString itemSID, QString materialSID );
	Item( QVariantMap in );
	Item( const Item& other );
	~Item();

	virtual QVariant serialize() const;

	unsigned short materialUID() const;
	unsigned short itemUID() const;

	QString getPixmapSID() const;
	QString getDesignation() const;
	QString itemSID() const;
	QString materialSID() const;
	QString combinedSID() const;

	int distanceSquare( const Position& pos, int zWeight = 1 ) const;

	// Ownership
	ItemLocation location() const { return m_location; }
	unsigned int locationOwner() const { return m_locationOwner; }
	ItemClaim claim() const { return m_claim; }
	unsigned int claimOwner() const { return m_claimOwner; }
	void setLocation( ItemLocation loc, unsigned int owner = 0 );
	void setClaim( ItemClaim cl, unsigned int owner = 0 );

	// Legacy API — delegates to location/claim
	unsigned int isInStockpile() const { return m_stockpileID; }
	void setInStockpile( unsigned int stockpile ) { m_stockpileID = stockpile; }
	unsigned int isInJob() const;
	void setInJob( unsigned int job );
	unsigned int isInContainer() const { return m_containerID; }
	void setInContainer( unsigned int container ) { m_containerID = container; }
	unsigned int isHeldBy() const;
	void setHeldBy( unsigned int creatureID );
	unsigned int isUsedBy() const;
	void setUsedBy( unsigned int creatureID );
	bool isFree() const;

	unsigned char stackSize() const;
	unsigned short value() const;
	void setValue( unsigned short value );
	unsigned int madeBy() const;
	void setMadeBy( unsigned int creatureID );

	void addComponent( ItemMaterial im );
	QList<ItemMaterial> components() const;
	unsigned char quality() const;
	void setQuality( unsigned char quality );
	unsigned char nutritionalValue() const;
	void setNutritionalValue( unsigned char value );
	unsigned char drinkValue() const;
	void setDrinkValue( unsigned char value );
	int attackValue() const;
	bool isWeapon() const;
	bool isTool() const;
	unsigned int color() const;
	void setColor( QString color );

private:
	unsigned short m_materialUID = 0;
	unsigned short m_itemUID     = 0;

	// Ownership state
	ItemLocation m_location      = ItemLocation::Ground;
	unsigned int m_locationOwner = 0;
	ItemClaim m_claim            = ItemClaim::None;
	unsigned int m_claimOwner    = 0;

	// Auxiliary tracking (not part of location/claim state machine)
	unsigned int m_stockpileID = 0;
	unsigned int m_containerID = 0;

	unsigned short m_value = 0;
	unsigned int m_madeBy  = 0;

	quint8 m_quality = 0; // 0 means item has no quality, quality is rowid of table Quality

	ItemExtraData* m_extraData = nullptr;
};
