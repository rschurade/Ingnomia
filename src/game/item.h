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

#include "object.h"

enum class ItemLocation : uint8_t
{
	Ground,    // on a tile, in position index + octree
	Carried,   // held by a creature, not in position index
};

enum class ItemClaim : uint8_t
{
	None,      // free to be claimed
	Job,       // reserved for a job
	Equipped,  // equipped by a creature
};

struct ItemMaterial
{
	unsigned int itemUID;
	unsigned int materialUID;
};

struct ItemExtraData
{

	QList<ItemMaterial> components;
	QSet<unsigned int> containedItems;

	unsigned char nutritionalValue = 0;
	unsigned char drinkValue       = 0;

	unsigned int color = 0;

	bool requireSame       = false;
	unsigned char capacity = 0;
};

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

	bool insertItem( unsigned int itemID );
	bool removeItem( unsigned int itemID );

	int distanceSquare( const Position& pos, int zWeight = 1 ) const;

	virtual bool isContainer() const;

	unsigned int isInStockpile() const;
	void setInStockpile( unsigned int stockpile );
	unsigned int isInJob() const;
	void setInJob( unsigned int job );
	unsigned int isInContainer() const;
	void setInContainer( unsigned int container );
	unsigned int isHeldBy() const;
	void setHeldBy( unsigned int creatureID );
	unsigned int isUsedBy() const;
	void setUsedBy( unsigned int creatureID );


	unsigned char stackSize() const;

	unsigned short value() const;
	void setValue( unsigned short value );
	unsigned int madeBy() const;
	void setMadeBy( unsigned int creatureID );

	const QSet<unsigned int>& containedItems() const;
	unsigned char capacity() const;
	bool requireSame() const;

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

	bool isFree() const;

	// New ownership state machine
	ItemLocation location() const { return m_location; }
	unsigned int locationOwner() const { return m_locationOwner; }
	ItemClaim claim() const { return m_claim; }
	unsigned int claimOwner() const { return m_claimOwner; }

	void setLocation( ItemLocation loc, unsigned int owner = 0 );
	void setClaim( ItemClaim cl, unsigned int owner = 0 );

private:
	unsigned short m_materialUID = 0;
	unsigned short m_itemUID     = 0;

	unsigned int m_isInStockpile = 0;
	unsigned int m_isInContainer = 0;
	unsigned int m_isInJob       = 0;
	unsigned int m_isHeldBy      = 0; // indicates the item is carried or equipped by a creature
	unsigned int m_isUsedBy      = 0;

	// New ownership state (dual-write with old flags above)
	ItemLocation m_location      = ItemLocation::Ground;
	unsigned int m_locationOwner = 0;
	ItemClaim m_claim            = ItemClaim::None;
	unsigned int m_claimOwner    = 0;

	unsigned short m_value = 0;
	unsigned int m_madeBy  = 0;

	quint8 m_quality = 0; // 0 means item has no quality, quality is rowid of table Quality

	ItemExtraData* m_extraData = nullptr;
};
