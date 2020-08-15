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

#include <QHash>
#include <QList>
#include <QMap>
#include <QMutex>
#include <QString>

typedef QSet<unsigned int> PositionEntry;
typedef QHash<unsigned int, PositionEntry> PositionHash;

class Octree;

class Inventory : public QObject
{
	Q_OBJECT

public:
	Inventory();
	~Inventory();

	void reset();

	void saveFilter();
	void loadFilter();

	bool itemExists( unsigned int itemID );

	unsigned int createItem( Position pos, QString itemSID, QString material );
	unsigned int createItem( Position pos, QString itemSID, QList<unsigned int> components );
	unsigned int createItem( Position pos, QString itemSID, QVariantList components );
	unsigned int createItem( const QVariantMap& values );

	void destroyObject( unsigned int id );

	unsigned int getFirstObjectAtPosition( const Position& pos );
	bool getObjectsAtPosition( const Position& pos, PositionEntry& pe );

	unsigned int getClosestItem( const Position& pos, bool allowInStockpile, QString itemID, QString materialID = "any" );
	//from construction dialog with materialTypes selected any
	unsigned int getClosestItem2( const Position& pos, bool allowInStockpile, QString itemID, QSet<QString> materialTypes );
	unsigned int getItemAtPos( const Position& pos, bool allowInStockpile, QString itemID, QString materialID = "any" );

	QList<unsigned int> getClosestItems( const Position& pos, bool allowInStockpile, QList<QPair<QString, QString>> filter, int count );
	QList<unsigned int> getClosestItems( const Position& pos, bool allowInStockpile, QString itemSID, QString materialSID, int count );
	bool checkReachableItems( Position pos, bool allowInStockpile, int count, QString itemSID, QString materialSID = "any" );
	QList<unsigned int> getClosestItemsForStockpile( unsigned int stockpileID, Position& pos, bool allowInStockpile, QSet<QPair<QString, QString>> filter );

	unsigned int getFoodItem( Position& pos );
	unsigned int getDrinkItem( Position& pos );

	QList<unsigned int> tradeInventory( QString itemSID, QString materialSID );
	QList<unsigned int> tradeInventory( QString itemSID, QString materialSID, unsigned char quality );

	void moveItemToPos( unsigned int item, const Position& newPos );

	unsigned int pickUpItem( unsigned int id );
	unsigned int putDownItem( unsigned int id, const Position& newPos );

	QList<QString> categories();
	QList<QString> groups( QString category );
	QList<QString> items( QString category, QString group );
	QList<QString> materials( QString category, QString group, QString item );

	bool isInGroup( QString category, QString group, unsigned int itemUID );

	QMap<QString, int> materialCountsForItem( QString itemID, bool allowInJob = false );
	unsigned int itemCount( QString itemID, QString materialID );
	unsigned int itemCountWithInJob( QString itemID, QString materialID );
	unsigned int itemCountInStockpile( QString itemID, QString materialID );
	unsigned int itemCountNotInStockpile( QString itemID, QString materialID );

	bool isContainer( unsigned int item );

	unsigned int isInStockpile( unsigned int id );
	void setInStockpile( unsigned int id, unsigned int stockpile );
	unsigned int isInJob( unsigned int id );
	void setInJob( unsigned int id, unsigned int job );
	unsigned int isInContainer( unsigned int id );
	void setInContainer( unsigned int id, unsigned int container );

	bool isPickedUp( unsigned int id );

	bool isConstructedOrEquipped( unsigned int id );
	void setConstructedOrEquipped( unsigned int id, bool status );

	void putItemInContainer( unsigned int id, unsigned int container );
	void removeItemFromContainer( unsigned int id );

	QString pixmapID( unsigned int item );
	QString designation( unsigned int item );
	QString itemSID( unsigned int item );
	unsigned int itemUID( unsigned int item );
	QString materialSID( unsigned int item );
	unsigned int materialUID( unsigned int item );
	QString combinedID( unsigned int item );
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

	void setColor( unsigned int item, QString color );
	unsigned int color( unsigned int item );

	const QSet<unsigned int>& itemsInContainer( unsigned int container );

	int countItemsAtPos( Position& pos );

	QHash<unsigned int, Item>& allItems()
	{
		return m_items;
	}

	QList<QString> materialsForItem( QString itemSID, int count );

	bool isSameTypeAndMaterial( unsigned int item, unsigned int item2 );

	void gravity( Position pos );

	int distanceSquare( unsigned int itemID, Position pos );

	unsigned int getTradeValue( QString itemSID, QString materialSID, unsigned int quality );
	bool itemTradeAvailable( unsigned int itemUID );

	Position getPosition( unsigned int itemID );

	int numFoodItems();
	int numDrinkItems();

	void sanityCheck();

	QVariantList components( unsigned int itemID );

	void addToWealth( Item* item );
	void removeFromWealth( Item* item );

	int kingdomWealth();

	bool itemsChanged();
	void setItemsChanged();

	QString itemGroup( unsigned int itemID );

private:
	int m_dimX;
	int m_dimY;
	int m_dimZ;

	QHash<unsigned int, Item> m_items;

	PositionHash m_positionHash;
	QHash<QString, QHash<QString, QHash<unsigned int, Item*>>> m_hash;
	QHash<QString, QHash<QString, Octree*>> m_octrees;

	QMutex m_mutex;

	QList<QString> m_categoriesSorted;
	QMap<QString, QList<QString>> m_groupsSorted;
	QMap<QString, QMap<QString, QList<QString>>> m_itemsSorted;

	QHash<QString, QStringList> m_materialsInTypes;

	QStringList m_foodItemLookup;
	QStringList m_drinkItemLookup;

	QMap<unsigned int, unsigned char> m_foodItems;
	QMap<unsigned int, unsigned char> m_drinkItems;

	void addObject( Item& object, const QString& itemID, const QString& materialID );
	void init();

	Item* getItem( unsigned int itemUID );

	int m_wealth = 0;

	bool m_itemsChanged = false; //flag is used for updating the stock overview
};
