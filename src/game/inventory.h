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



#include "../base/priorityqueue.h"
#include "../game/item.h"

#include <QPointer>
#include <absl/container/flat_hash_map.h>
#include <QList>
#include <absl/container/btree_map.h>

#include <vector>

#include <sigslot/signal.hpp>

typedef absl::btree_set<unsigned int> PositionEntry;
typedef absl::flat_hash_map<unsigned int, PositionEntry> PositionHash;

class ItemHistory;
class Octree;
class StockpileManager;
class World;
class Game;

class Inventory : public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY_MOVE( Inventory )
public:
	Inventory() = delete;
	Inventory( Game* parent );
	~Inventory();

	void saveFilter();
	void loadFilter();

	bool itemExists( unsigned int itemID );

	unsigned int createItem( Position pos, const std::string& itemSID, const std::string& material );
	unsigned int createItem( Position pos, const std::string& itemSID, std::vector<unsigned int> components );
	unsigned int createItem( Position pos, const std::string& itemSID, QVariantList components );
	unsigned int createItem( const QVariantMap& values );

	void destroyObject( unsigned int id );

	unsigned int getFirstObjectAtPosition( const Position& pos );
	bool getObjectsAtPosition( const Position& pos, PositionEntry& pe );

	unsigned int getClosestItem( const Position& pos, bool allowInStockpile, const std::string& itemID, const std::string& materialID = "any" );
	//from construction dialog with materialTypes selected any
	unsigned int getClosestItem2( const Position& pos, bool allowInStockpile, const std::string& itemID, absl::btree_set<std::string> materialTypes );
	unsigned int getItemAtPos( const Position& pos, bool allowInStockpile, const std::string& itemID, const std::string& materialID = "any" );

	std::vector<unsigned int> getClosestItems( const Position& pos, bool allowInStockpile, std::vector<QPair<std::string, std::string>> filter, int count );

	/**
	* get closest items with connected region check
	*/
	std::vector<unsigned int> getClosestItems( const Position& pos, bool allowInStockpile, const std::string& itemSID, const std::string& materialSID, int count );
	bool checkReachableItems( Position pos, bool allowInStockpile, int count, const std::string& itemSID, const std::string& materialSID = "any" );
	std::vector<unsigned int> getClosestItemsForStockpile( unsigned int stockpileID, Position& pos, bool allowInStockpile, absl::btree_set<QPair<std::string, std::string>> filter );

	unsigned int getFoodItem( Position& pos );
	unsigned int getDrinkItem( Position& pos );

	std::vector<unsigned int> tradeInventory( const std::string& itemSID, const std::string& materialSID );
	std::vector<unsigned int> tradeInventory( const std::string& itemSID, const std::string& materialSID, unsigned char quality );

	void moveItemToPos( unsigned int item, const Position& newPos );

	unsigned int pickUpItem( unsigned int id, unsigned int creatureID );
	unsigned int putDownItem( unsigned int id, const Position& newPos );
	unsigned int isHeldBy( unsigned int id );


	std::vector<std::string> categories();
	std::vector<std::string> groups( const std::string& category );
	std::vector<std::string> items( const std::string& category, const std::string& group );
	std::vector<std::string> materials( const std::string& category, const std::string& group, const std::string& item );

	bool isInGroup( const std::string& category, const std::string& group, unsigned int itemUID );

	absl::btree_map<std::string, int> materialCountsForItem( const std::string& itemID, bool allowInJob = false );
	unsigned int itemCount( const std::string& itemID, const std::string& materialID );
	unsigned int itemCountWithInJob( const std::string& itemID, const std::string& materialID );
	unsigned int itemCountInStockpile( const std::string& itemID, const std::string& materialID );
	unsigned int itemCountNotInStockpile( const std::string& itemID, const std::string& materialID );

	struct ItemCountDetailed
	{
		unsigned int total;
		unsigned int inJob;
		unsigned int inStockpile;
		unsigned int equipped;
		unsigned int constructed;
		unsigned int loose;
		unsigned int totalValue;
	};
	ItemCountDetailed itemCountDetailed( const std::string& itemID, const std::string& materialID );


	bool isContainer( unsigned int item );

	unsigned int isInStockpile( unsigned int id );
	void setInStockpile( unsigned int id, unsigned int stockpile );
	unsigned int isInJob( unsigned int id );
	void setInJob( unsigned int id, unsigned int job );
	unsigned int isInContainer( unsigned int id );
	void setInContainer( unsigned int id, unsigned int container );
	unsigned int isUsedBy( unsigned int id );
	void setIsUsedBy( unsigned int id, unsigned int creatureID );

	

	bool isConstructed( unsigned int id );
	void setConstructed( unsigned int id, bool status );

	void putItemInContainer( unsigned int id, unsigned int container );
	void removeItemFromContainer( unsigned int id );

	std::string pixmapID( unsigned int item );
	std::string designation( unsigned int item );
	std::string itemSID( unsigned int item );
	unsigned int itemUID( unsigned int item );
	std::string materialSID( unsigned int item );
	unsigned int materialUID( unsigned int item );
	std::string combinedID( unsigned int item );
	unsigned int spriteID( unsigned int item );

	Position getItemPos( unsigned int item );
	void setItemPos( unsigned int item, const Position& pos );

	unsigned int value( unsigned int item );
	void setValue( unsigned int item, unsigned int value );
	unsigned int madeBy( unsigned int item );
	void setMadeBy( unsigned int item, unsigned int creatureID );
	unsigned char quality( unsigned int item );
	void setQuality( unsigned int item, unsigned char quality );

	unsigned char stackSize( unsigned int item );
	unsigned char capacity( unsigned int item );
	bool requireSame( unsigned int item );

	unsigned char nutritionalValue( unsigned int item );
	unsigned char drinkValue( unsigned int item );

	int attackValue( unsigned int item );
	bool isWeapon( unsigned int item );

	bool isTool( unsigned int item );

	void setColor( unsigned int item, const std::string& color );
	unsigned int color( unsigned int item );

	const absl::btree_set<unsigned int>& itemsInContainer( unsigned int container );

	int countItemsAtPos( Position& pos );

	absl::flat_hash_map<unsigned int, std::unique_ptr<Item>>& allItems()
	{
		return m_items;
	}

	std::vector<std::string> materialsForItem( const std::string& itemSID, int count );

	bool isSameTypeAndMaterial( unsigned int item, unsigned int item2 );

	void gravity( Position pos );

	int distanceSquare( unsigned int itemID, Position pos );

	unsigned int getTradeValue( const std::string& itemSID, const std::string& materialSID, unsigned int quality );
	bool itemTradeAvailable( unsigned int itemUID );

	Position getPosition( unsigned int itemID );

	int numFoodItems();
	int numDrinkItems();

	void sanityCheck();

	QVariantList components( unsigned int itemID );

	void addToWealth( Item* item );
	void removeFromWealth( Item* item );

	int kingdomWealth();
	int numItems();

	bool itemsChanged();
	void setItemsChanged();

	std::string itemGroup( unsigned int itemID );

	std::vector<std::string> allMats( unsigned int itemID );

	ItemHistory* itemHistory();

private:
	QPointer<Game> g;

	QPointer<ItemHistory> m_itemHistory;

	Octree* octree( const std::string& itemSID, const std::string& materialSID );

	int m_dimX;
	int m_dimY;
	int m_dimZ;

	absl::flat_hash_map<unsigned int, std::unique_ptr<Item>> m_items;

	PositionHash m_positionHash;
	absl::flat_hash_map<std::string, absl::flat_hash_map<std::string, absl::flat_hash_map<unsigned int, Item*>>> m_hash;
	absl::flat_hash_map<std::string, absl::flat_hash_map<std::string, Octree*>> m_octrees;

	std::vector<std::string> m_categoriesSorted;
	absl::btree_map<std::string, std::vector<std::string>> m_groupsSorted;
	absl::btree_map<std::string, absl::btree_map<std::string, std::vector<std::string>>> m_itemsSorted;

	absl::flat_hash_map<std::string, std::vector<std::string>> m_materialsInTypes;

	std::vector<std::string> m_foodItemLookup;
	std::vector<std::string> m_drinkItemLookup;

	absl::btree_map<unsigned int, unsigned char> m_foodItems;
	absl::btree_map<unsigned int, unsigned char> m_drinkItems;

	void addObject( std::unique_ptr<Item> object, const std::string& itemID, const std::string& materialID );
	void init();

	Item* getItem( unsigned int itemUID );

	int m_wealth = 0;

	bool m_itemsChanged = false; //flag is used for updating the stock overview

public: // signals:
	sigslot::signal<const std::string& /*itemSID*/, const std::string& /*materialSID*/> signalAddItem;
	sigslot::signal<const std::string& /*itemSID*/, const std::string& /*materialSID*/> signalRemoveItem;
};
