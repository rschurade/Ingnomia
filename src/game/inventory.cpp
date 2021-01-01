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
#include "inventory.h"
#include "game.h"

#include "../base/config.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/octree.h"
#include "../base/position.h"
#include "../base/util.h"
#include "../game/itemhistory.h"
#include "../game/stockpilemanager.h"
#include "../game/world.h"
#include "../gfx/sprite.h"
#include "../gfx/spritefactory.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonValue>

Inventory::Inventory( Game* parent ) :
	g( parent ),
	QObject( parent )
{
	m_itemHistory = new ItemHistory( this );
	init();
}

Inventory::~Inventory()
{
	m_items.clear();
	m_positionHash.clear();
	m_hash.clear();

	m_categoriesSorted.clear();
	m_groupsSorted.clear();
	m_itemsSorted.clear();

	m_materialsInTypes.clear();

	m_foodItems.clear();
	m_drinkItems.clear();

	for ( const auto& octtreeGroup : m_octrees )
	{
		for (const auto& octree : octtreeGroup)
		{
			delete octree;
		}
	}
}

void Inventory::init()
{
	m_dimX = Global::dimX;
	m_dimY = Global::dimY;
	m_dimZ = Global::dimZ;

	for ( auto categoryMap : DB::selectRows( "ItemGrouping" ) )
	{
		QString categoryID = categoryMap.value( "ID" ).toString();
		//qDebug() << "--" << categoryID;

		m_categoriesSorted.push_back( categoryID );
		m_groupsSorted[categoryID];
		m_itemsSorted[categoryID];

		auto groupList = DB::execQuery2( "SELECT DISTINCT \"ItemGroup\" FROM Items WHERE \"Category\" = \"" + categoryID + "\"" );
		for ( auto group : groupList )
		{
			QString groupID = group.toString();
			//qDebug() << "----" << groupID;
			m_groupsSorted[categoryID].push_back( groupID );
			m_itemsSorted[categoryID][groupID];

			auto vItemList = DB::execQuery2( "SELECT ID FROM Items WHERE \"Category\" = \"" + categoryID + "\" AND \"ItemGroup\" = \"" + groupID + "\"" );

			for ( auto vItem : vItemList )
			{
				QString itemID = vItem.toString();
				m_itemsSorted[categoryID][groupID].push_back( itemID );
				m_hash[itemID];

				for ( auto materialID : Global::util->possibleMaterialsForItem( itemID ) )
				{
					m_hash[itemID].insert( materialID, QHash<unsigned int, Item*>() );
				}
			}
		}
	}

	for ( auto row : DB::selectRows( "Materials" ) )
	{
		m_materialsInTypes[row.value( "Type" ).toString()].append( row.value( "ID" ).toString() );
	}
	PriorityQueue<QString, int> pqFood;
	PriorityQueue<QString, int> pqDrink;
	for ( auto row : DB::selectRows( "Items" ) )
	{
		int eatVal = row.value( "EatValue" ).toInt();
		if ( eatVal > 0 )
		{
			pqFood.put( row.value( "ID" ).toString(), eatVal );
		}
		int drinkVal = row.value( "DrinkValue" ).toInt();
		if ( drinkVal > 0 )
		{
			pqDrink.put( row.value( "ID" ).toString(), drinkVal );
		}
	}
	while ( !pqFood.empty() )
	{
		m_foodItemLookup.push_front( pqFood.get() );
	}
	while ( !pqDrink.empty() )
	{
		m_drinkItemLookup.push_front( pqDrink.get() );
	}
}

void Inventory::saveFilter()
{
	QVariantList filter;
	for ( auto itemID : m_hash.keys() )
	{
		for ( auto materialID : m_hash[itemID].keys() )
		{
			filter.append( itemID + "_" + materialID );
		}
	}
	GameState::itemFilter = filter;
}

void Inventory::loadFilter()
{
	auto list = GameState::itemFilter;
	for ( auto vEntry : list )
	{
		auto entry = vEntry.toString();
		auto comp  = entry.split( "_" );
		if ( comp.size() == 2 )
		{
			m_hash[comp[0]].insert( comp[1], QHash<unsigned int, Item*>() );

			int x = Global::dimX / 2;
			int y = Global::dimY / 2;
			int z = Global::dimZ / 2;
			m_octrees[comp[0]].insert( comp[1], new Octree( x, y, z, x, y, z ) );
		}
	}
}

Item* Inventory::getItem( unsigned int itemUID )
{
	auto it = m_items.find( itemUID );
	if ( it != m_items.end() )
	{
		return &(*it);
	}
	return nullptr;
}

bool Inventory::itemExists( unsigned int itemID )
{
	if ( m_items.contains( itemID ) )
	{
		return true;
	}
	return false;
}

unsigned int Inventory::createItem( Position pos, QString itemSID, QString materialSID )
{
	DBH::itemUID( itemSID );
	//qDebug() << "create item " << pos.toString() << baseItem << material;
	Item obj( pos, itemSID, materialSID );
	Sprite* sprite = g->m_sf->createSprite( itemSID, { materialSID } );
	if ( sprite )
	{
		obj.setSpriteID( sprite->uID );
	}
	addObject( obj, itemSID, materialSID );

	return obj.id();
}

unsigned int Inventory::createItem( Position pos, QString itemSID, QList<unsigned int> components )
{
	DBH::itemUID( itemSID );
	//qDebug() << "create item " << pos.toString() << itemSID << components;
	QVariantList compList = DB::select2( "ItemID", "Items_Components", "ID", itemSID );

	if ( compList.isEmpty() )
	{ // item has no components, we use the first material
		if ( components.size() )
		{
			auto item = getItem( components.first() );
			if ( item )
			{
				return createItem( pos, itemSID, item->materialSID() );
			}
			else
			{
				qDebug() << "####### component item" << components.first() << "for createItem" << itemSID << "at" << pos.toString() << "doesn't exist";
				return 0;
			}
		}
		else
		{
			qDebug() << "####### create item, components empty";
			return 0;
		}
	}
	else
	{
		// first component decides the material of the item for material level bonuses and other things
		QString firstCompSID = compList.first().toMap().value( "ItemID" ).toString();
		DBH::itemUID( firstCompSID );
		QString firstMat     = materialSID( components.first() );
		Item obj( pos, itemSID, firstMat );

		QStringList materialSIDs;

		for ( auto UID : components )
		{
			auto item = getItem( UID );
			if ( item )
			{
				unsigned int itID = item->itemUID();
				unsigned int maID = item->materialUID();
				materialSIDs.push_back( item->materialSID() );
				ItemMaterial im = { itID, maID };
				obj.addComponent( im );
			}
		}

		// create combined sprite
		Sprite* sprite = g->m_sf->createSprite( itemSID, materialSIDs );
		if ( sprite )
		{
			obj.setSpriteID( sprite->uID );
		}
		addObject( obj, itemSID, firstMat );

		return obj.id();
	}
}

unsigned int Inventory::createItem( Position pos, QString itemSID, QVariantList components )
{
	return createItem( pos, itemSID, Global::util->variantList2UInt( components ) );
}

unsigned int Inventory::createItem( const QVariantMap& values )
{
	if ( values.value( "ItemSID" ).toString().isEmpty() )
	{
		qDebug() << "missing itemSID" << values.value( "ID" ).toUInt() << "at" << values.value( "Position" ).toString();
		return 0;
	}
	if ( values.value( "MaterialSID" ).toString().isEmpty() )
	{
		qDebug() << "missing materialSID" << values.value( "ID" ).toUInt() << "at" << values.value( "Position" ).toString();
		return 0;
	}
	DBH::itemUID( values.value( "ItemSID" ).toString() );
	//Item obj( pos, baseItem, material );
	Item obj( values );

	QString baseItem = obj.itemSID();
	QString material = obj.materialSID();
	//qDebug() << "inventory create item " << baseItem << material;

	QString spr    = DBH::spriteID( baseItem );
	Sprite* sprite = nullptr;
	if ( obj.components().isEmpty() )
	{
		sprite = g->m_sf->createSprite( baseItem, { material } );
	}
	else
	{
		QList<QString> materialSIDs;
		for ( auto comp : obj.components() )
		{
			materialSIDs.push_back( DBH::materialSID( comp.materialUID ) );
		}
		// create combined sprite
		sprite = g->m_sf->createSprite( baseItem, materialSIDs );
	}
	if ( sprite )
	{
		obj.setSpriteID( sprite->uID );
	}

	addObject( obj, baseItem, material );

	return obj.id();
}

Octree* Inventory::octree( const QString& itemSID, const QString& materialSID )
{
	if ( !m_octrees.contains( itemSID ) || !m_octrees[itemSID].contains( materialSID ) )
	{
		int x = Global::dimX / 2;
		int y = Global::dimY / 2;
		int z = Global::dimZ / 2;
		m_octrees[itemSID].insert( materialSID, new Octree( x, y, z, x, y, z ) );
	}
	return m_octrees[itemSID][materialSID];
}

void Inventory::addObject( Item& object, const QString& itemID, const QString& materialID )
{
	m_items.insert( object.id(), object );
	Item* item = getItem( object.id() );
	
	Octree* ot = octree( itemID, materialID );

	if ( !item->isHeldBy() )
	{
		if ( m_positionHash.contains( item->getPos().toInt() ) )
		{
			m_positionHash[item->getPos().toInt()].insert( item->id() );
		}
		else
		{
			PositionEntry entry;
			entry.insert( item->id() );
			m_positionHash.insert( item->getPos().toInt(), entry );
		}

		Position pos = item->getPos();
		ot->insertItem( pos.x, pos.y, pos.z, item->id() );
		
		if ( !item->isConstructed() && !item->isInContainer() )
		{
			g->m_world->setItemSprite( item->getPos(), object.spriteID() );
		}

		if ( item->isConstructed() || item->isInStockpile() )
		{
			addToWealth( item );
		}
	}

	if ( object.nutritionalValue() != 0 )
	{
		m_foodItems.insert( object.id(), object.nutritionalValue() );
	}
	if ( object.drinkValue() != 0 )
	{
		m_drinkItems.insert( object.id(), object.drinkValue() );
	}

	if ( m_hash[itemID].contains( materialID ) )
	{
		m_hash[itemID][materialID].insert( item->id(), item );
	}
	else
	{
		m_hash[itemID].insert( materialID, QHash<unsigned int, Item*>() );
		m_hash[itemID][materialID].insert( item->id(), item );
	}

	m_itemHistory->plusItem( itemID, materialID );

	m_itemsChanged = true;

	emit signalAddItem( itemID, materialID );
}

void Inventory::destroyObject( unsigned int id )
{
	if ( m_items.contains( id ) )
	{
		Item* item = getItem( id );
		if ( item )
		{
			// clear all pointers in convinience containers
			unsigned int tileID = item->getPos().toInt();
			if ( m_positionHash.contains( tileID ) )
			{
				m_positionHash[tileID].remove( id );
				if ( m_positionHash[tileID].empty() )
				{
					m_positionHash.remove( tileID );
				}
			}

			if ( item->isInContainer() )
			{
				removeItemFromContainer( id );
			}

			QString materialSID = DBH::materialSID( item->materialUID() );
			QString itemSID     = DBH::itemSID( item->itemUID() );

			Position pos = item->getPos();
			Octree* ot = octree( itemSID, materialSID );
			ot->removeItem( pos.x, pos.y, pos.z, id );
			
			m_hash[itemSID][materialSID].remove( id );

			m_foodItems.remove( id );
			m_drinkItems.remove( id );
			// finally remove object
			m_items.remove( id );

			m_itemHistory->minusItem( itemSID, materialSID );

			emit signalRemoveItem( itemSID, materialSID );
		}

		m_itemsChanged = true;
	}
}

unsigned int Inventory::getFirstObjectAtPosition( const Position& pos )
{
	if ( m_positionHash.contains( pos.toInt() ) )
	{
		if ( !m_positionHash[pos.toInt()].empty() )
		{
			for ( auto itemUID : m_positionHash[pos.toInt()].values() )
			{
				if ( !isConstructed( itemUID ) && !isInContainer( itemUID ) )
				{
					return itemUID;
				}
			}
		}
		else
		{
			m_positionHash.remove( pos.toInt() );
		}
	}

	return 0;
}

bool Inventory::getObjectsAtPosition( const Position& pos, PositionEntry& pe )
{
	if ( m_positionHash.contains( pos.toInt() ) && !m_positionHash[pos.toInt()].empty() )
	{
		pe = m_positionHash[pos.toInt()];
		return true;
	}
	else
	{
		m_positionHash.remove( pos.toInt() );
		return false;
	}
}

int Inventory::countItemsAtPos( Position& pos )
{
	if ( m_positionHash.contains( pos.toInt() ) )
	{
		int size = m_positionHash[pos.toInt()].size();
		if ( size > 0 )
		{
			return size;
		}

		m_positionHash.remove( pos.toInt() );
		return 0;
	}
	return 0;
}

unsigned int Inventory::getClosestItem( const Position& pos, bool allowInStockpile, QString itemSID, QString materialSID )
{
	auto out = getClosestItems( pos, allowInStockpile, itemSID, materialSID, 1 );
	if ( !out.isEmpty() )
	{
		return out.first();
	}

	return 0;
}

unsigned int Inventory::getClosestItem2( const Position& pos, bool allowInStockpile, QString itemSID, QSet<QString> materialTypes )
{
	for ( auto matType : materialTypes )
	{
		for ( auto materialSID : m_materialsInTypes[matType] )
		{
			unsigned int item = getClosestItem( pos, allowInStockpile, itemSID, materialSID );
			if ( item )
			{
				return item;
			}
		}
	}
	return 0;
}

bool Inventory::checkReachableItems( Position pos, bool allowInStockpile, int count, QString itemSID, QString materialSID )
{
	int thisCount  = 0;
	int partitions = 0;

	auto predicate = [&thisCount, this, allowInStockpile, pos, count]( unsigned int itemID ) -> bool {
		auto item = getItem( itemID );

		if ( item && ( allowInStockpile || !item->isInStockpile() ) && item->isFree() )
		{
			if ( g->m_world->regionMap().checkConnectedRegions( pos, item->getPos() ) && g->m_world->fluidLevel( item->getPos() ) < 6 )
			{
				++thisCount;
			}
		}
		if ( thisCount == count )
		{
			return false;
		}
		return true;
	};
	
	if ( materialSID == "any" )
	{
		for ( auto material : m_octrees[itemSID].keys() )
		{
			Octree* ot = octree( itemSID, material );
			ot->visit( pos.x, pos.y, pos.z, predicate );
			if ( thisCount >= count )
				break;
		}
	}
	else
	{
		Octree* ot = octree( itemSID, materialSID );
		ot->visit( pos.x, pos.y, pos.z, predicate );
	}

	return thisCount >= count;
}

unsigned int Inventory::getItemAtPos( const Position& pos, bool allowInStockpile, QString itemSID, QString materialSID )
{
	if ( m_positionHash.contains( pos.toInt() ) )
	{
		auto itemIDs = m_positionHash[pos.toInt()];

		if ( materialSID == "any" )
		{
			for ( auto itemID : itemIDs )
			{
				auto item = getItem( itemID );
				if ( item )
				{
					if ( itemSID == item->itemSID() && ( allowInStockpile || !item->isInStockpile() ) && item->isFree() )
					{
						return itemID;
					}
				}
				else
				{
					m_positionHash[pos.toInt()].remove( itemID );
				}
			}
		}
		else
		{
			for ( auto itemID : itemIDs )
			{
				auto item = getItem( itemID );
				if ( item )
				{
					if ( item->itemSID() == itemSID &&
						 item->materialSID() == materialSID &&
						 ( allowInStockpile || !item->isInStockpile() ) && item->isFree() )
					{
						return itemID;
					}
				}
				else
				{
					m_positionHash[pos.toInt()].remove( itemID );
				}
			}
		}
	}
	return 0;
}

QList<unsigned int> Inventory::getClosestItems( const Position& pos, bool allowInStockpile, QString itemSID, QString materialSID, int count )
{
	QList<unsigned int> out;

	auto predicate = [&out, this, allowInStockpile, pos, count]( unsigned int itemID ) -> bool {
		auto item = getItem( itemID );

		if ( item && ( allowInStockpile || !item->isInStockpile() ) && item->isFree() )
		{
			if ( g->m_world->regionMap().checkConnectedRegions( pos, item->getPos() ) && g->m_world->fluidLevel( item->getPos() ) < 6 )
			{
				out.append( itemID );
				if ( out.size() == count )
				{
					return false;
				}
			}
		}
		return true;
	};

	if ( materialSID == "any" )
	{
		for ( auto material : m_octrees[itemSID].keys() )
		{
			Octree* ot = octree( itemSID, material );
			ot->visit( pos.x, pos.y, pos.z, predicate );
		}
	}
	else
	{
		Octree* ot = octree( itemSID, materialSID );
		ot->visit( pos.x, pos.y, pos.z, predicate );
	}

	return out;
}

QList<unsigned int> Inventory::getClosestItems( const Position& pos, bool allowInStockpile, QList<QPair<QString, QString>> filter, int count )
{
	QList<unsigned int> out;

	for ( const auto& filterItem : filter )
	{
		auto& itemSID     = filterItem.first;
		auto& materialSID = filterItem.second;

		out.append( getClosestItems( pos, allowInStockpile, itemSID, materialSID, count ) );
	}
	return out;
}

QList<unsigned int> Inventory::getClosestItemsForStockpile( unsigned int stockpileID, Position& pos, bool allowInStockpile, QSet<QPair<QString, QString>> filter )
{
	QString itemSID;
	QString materialSID;

	QList<unsigned int> out;
	QList<unsigned int> items;

	for ( const auto& filterItem : filter )
	{
		itemSID     = filterItem.first;
		materialSID = filterItem.second;

		if ( materialSID == "any" )
		{
			for ( auto material : m_octrees[itemSID].keys() )
			{
				Octree* ot = octree( itemSID, material );
				items += ot->query( pos.x, pos.y, pos.z );
			}
		}
		else
		{
			Octree* ot = octree( itemSID, materialSID );
			items += ot->query( pos.x, pos.y, pos.z );
		}

		for ( auto itemID : items )
		{
			auto item = getItem( itemID );

			if ( item && item->isFree() )
			{
				if ( item->isInStockpile() == stockpileID )
				{
					//item is in this stockpile
				}
				else if ( !item->isInStockpile() )
				{
					//item isnt in stockpile
					out.append( itemID );
				}
				else
				{
					// item is in stockpile so we need to check if we can pull from stockpiles and if the source stockpile allows it
					if ( allowInStockpile && g->m_spm->hasPriority( stockpileID, item->isInStockpile() ) && g->m_spm->allowsPull( item->isInStockpile() ) )
					{
						out.append( itemID );
					}
				}
			}
		}
	}
	return out;
}

QList<unsigned int> Inventory::tradeInventory( QString itemSID, QString materialSID )
{
	QList<unsigned int> out;

	if ( m_hash.contains( itemSID ) )
	{
		if ( !m_hash[itemSID].empty() )
		{
			for ( auto item : m_hash[itemSID][materialSID] )
			{
				if ( item->isInStockpile() && item->isFree() )
				{
					out.append( item->id() );
				}
			}
		}
	}

	return out;
}

QList<unsigned int> Inventory::tradeInventory( QString itemSID, QString materialSID, unsigned char quality )
{
	QList<unsigned int> out;

	if ( m_hash.contains( itemSID ) )
	{
		if ( !m_hash[itemSID].empty() )
		{
			for ( auto item : m_hash[itemSID][materialSID] )
			{
				if ( item->isInStockpile() && item->isFree() && item->quality() == quality )
				{
					out.append( item->id() );
				}
			}
		}
	}

	return out;
}

unsigned int Inventory::getFoodItem( Position& pos )
{
	for ( auto foodItem : m_foodItemLookup )
	{
		auto item = getClosestItem( pos, true, foodItem, "any" );
		if ( item )
		{
			return item;
		}
	}
	return 0;
}

unsigned int Inventory::getDrinkItem( Position& pos )
{
	for ( auto drinkItem : m_drinkItemLookup )
	{
		auto item = getClosestItem( pos, true, drinkItem, "any" );
		if ( item )
		{
			return item;
		}
	}
	return 0;
}

Position Inventory::getPosition( unsigned int itemID )
{
	Item* item = getItem( itemID );
	if ( item )
	{
		return item->getPos();
	}
	return Position();
}

void Inventory::moveItemToPos( unsigned int id, const Position& newPos )
{
	//qDebug() << "move item from " << item( id].getPos().toString() << " to " << newPos.toString();
	// remove from position hash
	pickUpItem( id, 1 );

	putDownItem( id, newPos );
}

unsigned int Inventory::pickUpItem( unsigned int id, unsigned int creatureID )
{
	auto item = getItem( id );
	if ( item )
	{
		item->setHeldBy( creatureID );

		//remove from possible stockpile
		unsigned int stockpileID = item->isInStockpile();
		if ( stockpileID )
		{
			g->m_spm->removeItem( stockpileID, item->getPos(), id );
			item->setInStockpile( 0 );
			removeFromWealth( item );
		}

		removeItemFromContainer( id );

		const Position& pos = item->getPos();
		if ( m_positionHash.contains( pos.toInt() ) )
		{
			m_positionHash[pos.toInt()].remove( id );
		}
		if ( m_positionHash[pos.toInt()].empty() )
		{
			m_positionHash.remove( pos.toInt() );
		}

		Octree* ot = octree( item->itemSID(), item->materialSID() );
		ot->removeItem( pos.x, pos.y, pos.z, item->id() );

		if ( item->isContainer() )
		{
			for ( auto inItemID : item->containedItems() )
			{
				auto inItem = getItem( inItemID );

				if ( inItem )
				{
					QString inItemSID = inItem->itemSID();
					QString inMatSID  = inItem->materialSID();

					Octree* ot2 = octree( inItemSID, inMatSID );
					ot2->removeItem( pos.x, pos.y, pos.z, inItemID );
					m_positionHash[pos.toInt()].remove( inItemID );
				}
			}
		}

		unsigned int nextItemID = getFirstObjectAtPosition( pos );
		auto nextItem           = getItem( nextItemID );
		if ( nextItem )
		{
			g->m_world->setItemSprite( pos, nextItem->spriteID() );
		}
		else
		{
			g->m_world->setItemSprite( pos, 0 );
		}
		return id;
	}
	return 0;
}

unsigned int Inventory::putDownItem( unsigned int id, const Position& newPos )
{
	// insert at new pos in position hash
	Item* item = getItem( id );
	if ( item )
	{
		item->setHeldBy( 0 );
		if ( m_positionHash.contains( newPos.toInt() ) )
		{
			m_positionHash[newPos.toInt()].insert( id );
		}
		else
		{
			PositionEntry entry;
			entry.insert( id );
			m_positionHash.insert( newPos.toInt(), entry );
		}

		Octree* ot = octree( item->itemSID(), item->materialSID() );
		ot->insertItem( newPos.x, newPos.y, newPos.z, item->id() );
			
		if ( item->isContainer() )
		{
			for ( auto inItemID : item->containedItems() )
			{
				auto inItem = getItem( inItemID );

				if ( inItem )
				{
					QString inItemSID = inItem->itemSID();
					QString inMatSID  = inItem->materialSID();
					if( m_octrees.contains( inItemSID ) )
					{
						if( m_octrees[inItemSID].contains( inMatSID ) )
						{
							auto octree       = m_octrees[inItemSID][inMatSID];
							if ( octree )
							{
								octree->insertItem( newPos.x, newPos.y, newPos.z, inItemID );
								m_positionHash[newPos.toInt()].insert( inItemID );
							}
						}
					}
				}
			}
		}

		// set new position
		item->setPos( newPos );

		unsigned int nextItemID = getFirstObjectAtPosition( newPos );
		auto nextItem           = getItem( nextItemID );
		if ( nextItem )
		{
			g->m_world->setItemSprite( newPos, nextItem->spriteID() );
		}
		else
		{
			g->m_world->setItemSprite( newPos, 0 );
		}
		return id;
	}
	return 0;
}

unsigned int Inventory::isHeldBy( unsigned int id )
{
	Item* item = getItem( id );
	if ( item )
	{
		return item->isHeldBy();
	}
	return false;
}

QList<QString> Inventory::categories()
{
	return m_categoriesSorted;
}

QList<QString> Inventory::groups( QString category )
{
	if ( m_groupsSorted.contains( category ) )
	{
		return m_groupsSorted[category];
	}
	return QList<QString>();
}
QList<QString> Inventory::items( QString category, QString group )
{
	if ( m_itemsSorted.contains( category ) )
	{
		if ( m_itemsSorted[category].contains( group ) )
		{
			return m_itemsSorted[category][group];
		}
	}
	return QList<QString>();
}

QList<QString> Inventory::materials( QString category, QString group, QString item )
{
	if ( m_hash.contains( item ) )
	{
		return m_hash[item].keys();
	}
	return QList<QString>();
}

bool Inventory::isInGroup( QString category, QString group, unsigned int itemID )
{
	if ( m_itemsSorted.contains( category ) )
	{
		if ( m_itemsSorted[category].contains( group ) )
		{
			QList<QString> items = m_itemsSorted[category][group];
			auto item            = getItem( itemID );
			if ( item )
			{
				return items.contains( item->itemSID() );
			}
		}
	}
	return false;
}

QMap<QString, int> Inventory::materialCountsForItem( QString itemSID, bool allowInJob )
{
	QMap<QString, int> mats;

	mats.insert( "any", 0 );

	if ( m_hash.contains( itemSID ) )
	{
		QList<QString> matKeys = m_hash[itemSID].keys();
		for ( auto mk : matKeys )
		{
			mats.insert( mk, 0 );
		}
		if ( !m_hash[itemSID].empty() )
		{
			for ( auto it : m_hash[itemSID] )
			{
				for ( auto item : it )
				{
					if ( ( !item->isInJob() || allowInJob ) && !item->isConstructed() && ( item->isHeldBy() == 0 ) )
					{
						mats["any"]++;
						if ( mats.contains( item->materialSID() ) )
						{
							mats[item->materialSID()]++;
						}
						else
						{
							mats.insert( item->materialSID(), 1 );
						}
					}
				}
			}
		}
	}
	return mats;
}

unsigned int Inventory::itemCount( QString itemID, QString materialID )
{
	unsigned int result = 0;

	if ( materialID == "any" )
	{
		if ( m_hash.contains( itemID ) )
		{
			for ( auto it : m_hash[itemID] )
			{
				for ( auto item : it )
				{
					if ( item->isFree() )
					{
						++result;
					}
				}
			}
		}
	}
	else
	{
		if ( m_hash.contains( itemID ) && m_hash[itemID].contains( materialID ) )
		{
			for ( auto item : m_hash[itemID][materialID] )
			{
				if ( item->isFree() )
				{
					++result;
				}
			}
		}
	}

	return result;
}

unsigned int Inventory::itemCountWithInJob( QString itemID, QString materialID )
{
	unsigned int result = 0;

	if ( materialID == "any" )
	{
		if ( m_hash.contains( itemID ) )
		{
			for ( auto it : m_hash[itemID] )
			{
				for ( auto item : it )
				{
					if ( !item->isConstructed() )
					{
						++result;
					}
				}
			}
		}
	}
	else
	{
		if ( m_hash.contains( itemID ) && m_hash[itemID].contains( materialID ) )
		{
			for ( auto item : m_hash[itemID][materialID] )
			{
				if ( !item->isConstructed() )
				{
					++result;
				}
			}
		}
	}

	return result;
}

unsigned int Inventory::itemCountInStockpile( QString itemID, QString materialID )
{
	unsigned int result = 0;

	if ( materialID == "any" )
	{
		if ( m_hash.contains( itemID ) )
		{
			for ( auto it : m_hash[itemID] )
			{
				for ( auto item : it )
				{
					if ( item->isInStockpile() && item->isFree() )
					{
						++result;
					}
				}
			}
		}
	}
	else
	{
		if ( m_hash.contains( itemID ) && m_hash[itemID].contains( materialID ) )
		{
			for ( auto item : m_hash[itemID][materialID] )
			{
				if ( item->isInStockpile() && item->isFree() )
				{
					result++;
				}
			}
		}
	}

	return result;
}

unsigned int Inventory::itemCountNotInStockpile( QString itemID, QString materialID )
{
	unsigned int result = 0;

	if ( materialID == "any" )
	{
		if ( m_hash.contains( itemID ) )
		{
			for ( auto it : m_hash[itemID] )
			{
				for ( auto item : it )
				{
					if ( !item->isInStockpile() && item->isFree() )
					{
						++result;
					}
				}
			}
		}
	}
	else
	{
		if ( m_hash.contains( itemID ) && m_hash[itemID].contains( materialID ) )
		{
			for ( auto item : m_hash[itemID][materialID] )
			{
				if ( item->materialSID() == materialID && !item->isInStockpile() && item->isFree() )
				{
					result++;
				}
			}
		}
	}

	return result;
}

Inventory::ItemCountDetailed Inventory::itemCountDetailed( QString itemID, QString materialID )
{
	ItemCountDetailed result = { 0, 0, 0, 0, 0, 0, 0 };

	if ( materialID == "any" )
	{
		if ( m_hash.contains( itemID ) )
		{
			for ( auto it : m_hash[itemID] )
			{
				for ( auto item : it )
				{
					result.total++;

					if ( item->isHeldBy() != 0 )
					{
						//TODO determine if the item is equipped or just carried
						result.equipped++;
					}

					if ( item->isConstructed() )
					{
						result.constructed++;
					}
					else if ( item->isInStockpile() )
						result.inStockpile++;
					else
						result.loose++;

					if ( item->isInJob() )
						result.inJob++;

					result.totalValue += item->value();
				}
			}
		}
	}
	else
	{
		if ( m_hash.contains( itemID ) && m_hash[itemID].contains( materialID ) )
		{
			for ( auto item : m_hash[itemID][materialID] )
			{
				if ( item->materialSID() == materialID )
				{
					result.total++;

					if ( item->isHeldBy() != 0 )
					{
						//TODO determine if the item is equipped or just carried
						result.equipped++;
					}
					if ( item->isConstructed() )
					{
						result.constructed++;
					}
					else if ( item->isInStockpile() )
						result.inStockpile++;
					else
						result.loose++;

					if ( item->isInJob() )
						result.inJob++;

					result.totalValue += item->value();
				}
			}
		}
	}

	return result;

	return ItemCountDetailed();
}

bool Inventory::isContainer( unsigned int id )
{
	auto item = getItem( id );
	if ( item )
	{
		return item->isContainer();
	}
	return false;
}

unsigned int Inventory::isInStockpile( unsigned int id )
{
	auto item = getItem( id );
	if ( item )
	{
		return item->isInStockpile();
	}
	return 0;
}

void Inventory::setInStockpile( unsigned int id, unsigned int stockpile )
{
	auto item = getItem( id );
	if ( item )
	{
		item->setInStockpile( stockpile );
		if ( stockpile )
		{
			addToWealth( item );
		}
	}
}

unsigned int Inventory::isInJob( unsigned int id )
{
	auto item = getItem( id );
	if ( item )
	{
		return item->isInJob();
	}
	return 0;
}

void Inventory::setInJob( unsigned int id, unsigned int job )
{
	auto item = getItem( id );
	if ( item )
	{
		item->setInJob( job );
	}
}

unsigned int Inventory::isInContainer( unsigned int id )
{
	auto item = getItem( id );
	if ( item )
	{
		return item->isInContainer();
	}
	return false;
}

void Inventory::setInContainer( unsigned int id, unsigned int container )
{
	auto item = getItem( id );
	if ( item )
	{
		item->setInContainer( container );
	}
}

bool Inventory::isConstructed( unsigned int id )
{
	auto item = getItem( id );
	if ( item )
	{
		return item->isConstructed();
	}
	return false;
}

void Inventory::setConstructed( unsigned int id, bool status )
{
	auto item = getItem( id );
	if ( item )
	{
		if ( item->isConstructed() && !status )
		{
			removeFromWealth( item );
			emit signalRemoveItem( itemSID( id ), materialSID( id ) );
		}
		item->setIsConstructed( status );
		if ( status )
		{
			addToWealth( item );
			emit signalAddItem( itemSID( id ), materialSID( id ) );
		}
	}
}

unsigned int Inventory::isUsedBy( unsigned int id )
{
	auto item = getItem( id );
	if ( item )
	{
		return item->isUsedBy();
	}
	return 0;
}
	
void Inventory::setIsUsedBy( unsigned int id, unsigned int creatureID )
{
	auto item = getItem( id );
	if ( item )
	{
		item->setUsedBy( creatureID );
	}
}

unsigned int Inventory::value( unsigned int id )
{
	auto item = getItem( id );
	if ( item )
	{
		return item->value();
	}
	return 0;
}

void Inventory::setValue( unsigned int id, unsigned int value )
{
	auto item = getItem( id );
	if ( item )
	{
		item->setValue( value );
	}
}

unsigned char Inventory::quality( unsigned int id )
{
	auto item = getItem( id );
	if ( item )
	{
		return item->quality();
	}
	return 0;
}

void Inventory::setQuality( unsigned int id, unsigned char quality )
{
	auto item = getItem( id );
	if ( item )
	{
		item->setQuality( quality );
	}
}

unsigned int Inventory::madeBy( unsigned int id )
{
	auto item = getItem( id );
	if ( item )
	{
		return item->madeBy();
	}
	return 0;
}

void Inventory::setMadeBy( unsigned int id, unsigned int creatureID )
{
	auto item = getItem( id );
	if ( item )
	{
		item->setMadeBy( creatureID );
	}
}

void Inventory::putItemInContainer( unsigned int id, unsigned int containerID )
{
	auto item      = getItem( id );
	auto container = getItem( containerID );
	if ( item != 0 && container != 0 )
	{
		item->setInContainer( containerID );

		container->insertItem( id );

		unsigned int nextItemID = getFirstObjectAtPosition( container->getPos() );
		auto nextItem           = getItem( nextItemID );
		if ( nextItem )
		{
			g->m_world->setItemSprite( container->getPos(), nextItem->spriteID() );
		}
		else
		{
			g->m_world->setItemSprite( container->getPos(), 0 );
		}
	}
}

void Inventory::removeItemFromContainer( unsigned int id )
{
	auto item = getItem( id );
	if ( item )
	{
		unsigned int containerID = item->isInContainer();
		item->setInContainer( 0 );
		auto container = getItem( containerID );
		if ( container )
		{
			container->removeItem( id );
			if ( container->isConstructed() )
			{
				g->m_spm->setInfiNotFull( container->getPos() );
			}
		}
	}
}

QString Inventory::pixmapID( unsigned int id )
{
	// return DBH::spriteID( DBH::itemSID( m_itemUID ) ) + "_" + DBH::materialSID( m_materialUID );
	auto item = getItem( id );
	if ( item )
	{
		return item->getPixmapSID();
	}

	return "";
}

QString Inventory::designation( unsigned int id )
{
	//return S::s( "$MaterialName_" + DBH::materialSID( m_materialUID ) ) + " " + S::s( "$ItemName_" + DBH::itemSID( m_itemUID ) );
	auto item = getItem( id );
	if ( item )
	{
		return item->getDesignation();
	}
	return "";
}

QString Inventory::itemSID( unsigned int id )
{
	auto item = getItem( id );
	if ( item )
	{
		return item->itemSID();
	}
	return "";
}

unsigned int Inventory::itemUID( unsigned int id )
{
	auto item = getItem( id );
	if ( item )
	{
		return item->itemUID();
	}
	return 0;
}

QString Inventory::materialSID( unsigned int id )
{
	auto item = getItem( id );
	if ( item )
	{
		return item->materialSID();
	}
	return "";
}

unsigned int Inventory::materialUID( unsigned int id )
{
	auto item = getItem( id );
	if ( item )
	{
		return item->materialUID();
	}
	return 0;
}

QString Inventory::combinedID( unsigned int id )
{
	auto item = getItem( id );
	if ( item )
	{
		return item->combinedSID();
	}
	return "";
}

unsigned int Inventory::spriteID( unsigned int id )
{
	auto item = getItem( id );
	if ( item )
	{
		return item->spriteID();
	}
	return 0;
}

Position Inventory::getItemPos( unsigned int id )
{
	auto item = getItem( id );
	if ( item )
	{
		return item->getPos();
	}
	return Position();
}

void Inventory::setItemPos( unsigned int id, const Position& pos )
{
	auto item = getItem( id );
	if ( item )
	{
		item->setPos( pos );
	}
}

unsigned char Inventory::stackSize( unsigned int id )
{
	auto item = getItem( id );
	if ( item )
	{
		return item->stackSize();
	}
	return 0;
}

unsigned char Inventory::capacity( unsigned int id )
{
	//get from containers
	auto item = getItem( id );
	if ( item )
	{
		return item->capacity();
	}
	return 0;
}

bool Inventory::requireSame( unsigned int id )
{
	auto item = getItem( id );
	if ( item )
	{
		return item->requireSame();
	}
	return false;
}

const QSet<unsigned int>& Inventory::itemsInContainer( unsigned int containerID )
{
	auto container = getItem( containerID );
	if ( container )
	{
		return container->containedItems();
	}
	static const QSet<unsigned int> nullopt;
	return nullopt;
}

unsigned char Inventory::nutritionalValue( unsigned int id )
{
	auto item = getItem( id );
	if ( item )
	{
		return item->nutritionalValue();
	}
	return 0;
}

unsigned char Inventory::drinkValue( unsigned int id )
{
	auto item = getItem( id );
	if ( item )
	{
		return item->drinkValue();
	}
	return 0;
}

int Inventory::attackValue( unsigned int id )
{
	auto item = getItem( id );
	if ( item )
	{
		return item->attackValue();
	}
	return 0;
}

bool Inventory::isWeapon( unsigned int id )
{
	auto item = getItem( id );
	if ( item )
	{
		return item->isWeapon();
	}
	return false;
}

bool Inventory::isTool( unsigned int id )
{
	auto item = getItem( id );
	if ( item )
	{
		return item->isTool();
	}
	return false;
}

void Inventory::setColor( unsigned int id, QString color )
{
	auto item = getItem( id );
	if ( item )
	{
		item->setColor( color );
	}
}

unsigned int Inventory::color( unsigned int id )
{
	auto item = getItem( id );
	if ( item )
	{
		return item->color();
	}
	return 0;
}

QList<QString> Inventory::materialsForItem( QString itemSID, int count )
{
	QList<QString> out;

	QMap<QString, int> matCount = materialCountsForItem( itemSID );
	for ( auto mc = matCount.begin(); mc != matCount.end(); ++mc )
	{
		if ( mc.key() != "any" )
		{
			if ( mc.value() >= count )
			{
				out.push_back( mc.key() );
			}
		}
	}

	return out;
}

bool Inventory::isSameTypeAndMaterial( unsigned int id, unsigned int id2 )
{
	auto item  = getItem( id );
	auto item2 = getItem( id2 );
	if ( item != nullptr && item2 != nullptr )
	{
		return ( ( item->itemUID() == item2->itemUID() ) && ( item->materialUID() == item2->materialUID() ) );
	}
	return false;
}

void Inventory::gravity( Position pos )
{
	if ( !g->m_world->isSolidFloor( pos ) )
	{
		if ( m_positionHash.contains( pos.toInt() ) )
		{
			Position newPos = pos;
			g->m_world->getFloorLevelBelow( newPos, false );
			if( newPos == pos )
			{
				return;
			}
			auto items = m_positionHash.value( pos.toInt() );
			for ( auto item : items )
			{
				moveItemToPos( item, newPos );
			}
		}
	}
}

int Inventory::distanceSquare( unsigned int itemID, Position pos )
{
	auto item = getItem( itemID );
	if ( item )
	{
		return item->distanceSquare( pos, Global::zWeight );
	}
	return 0;
}

unsigned int Inventory::getTradeValue( QString itemSID, QString materialSID, unsigned int quality )
{
	auto modifiers = DB::select2( "Modifier", "Quality", "Rank", (int)quality );
	if ( modifiers.size() )
	{
		auto modifier = modifiers.first().toFloat();
		int value     = DB::select( "Value", "Items", itemSID ).toFloat() * DB::select( "Value", "Materials", materialSID ).toFloat() * modifier;
		return value;
	}
	return 0;
}

bool Inventory::itemTradeAvailable( unsigned int itemUID )
{
	auto item = getItem( itemUID );
	if ( item )
	{
		return (bool)item->isInStockpile() && item->isFree();
	}
	return false;
}

int Inventory::numFoodItems()
{
	return m_foodItems.size();
}

int Inventory::numDrinkItems()
{
	return m_drinkItems.size();
}

void Inventory::sanityCheck()
{
	qDebug() << "Starting inventory sanity check ...";
	qDebug() << "Checking containers";
	QElapsedTimer timer;
	timer.start();
	int removed = 0;
	for ( auto it : m_hash["Crate"] )
	{
		for ( auto crate : it )
		{
			auto items = crate->containedItems();

			for ( auto itemID : items )
			{
				if ( !itemExists( itemID ) )
				{
					//qDebug() << itemID << "doesn't exist: crate at " << crate->getPos().toString();

					++removed;
					crate->removeItem( itemID );
				}
			}
		}
	}
	qDebug() << "Removed " << QString::number( removed ) << " invalid items from crates.";
	removed = 0;
	for ( auto it : m_hash["Barrel"] )
	{
		for ( auto barrel : it )
		{
			auto items = barrel->containedItems();

			for ( auto itemID : items )
			{
				if ( !itemExists( itemID ) )
				{
					//qDebug() << itemID << "doesn't exist: crate at " << crate->getPos().toString();
					++removed;
					barrel->removeItem( itemID );
				}
			}
		}
	}
	qDebug() << "Removed " << QString::number( removed ) << " invalid items from barrels.";
	removed = 0;
	for ( auto it : m_hash["Sack"] )
	{
		for ( auto sack : it )
		{
			auto items = sack->containedItems();

			for ( auto itemID : items )
			{
				if ( !itemExists( itemID ) )
				{
					//qDebug() << itemID << "doesn't exist: crate at " << crate->getPos().toString();
					++removed;
					sack->removeItem( itemID );
				}
			}
		}
	}
	qDebug() << "Removed " << QString::number( removed ) << " invalid items from sacks.";
	qDebug() << "Sanity check took " << timer.elapsed() << "ms";
}

QVariantList Inventory::components( unsigned int itemID )
{
	auto item = getItem( itemID );
	if ( item )
	{
		auto def = item->serialize().toMap();
		return def.value( "Components" ).toList();
	}
	return QVariantList();
}

int Inventory::numItems()
{
	return m_items.size();
}

int Inventory::kingdomWealth()
{
	return m_wealth;
}

void Inventory::addToWealth( Item* item )
{
	m_wealth += item->value();
}

void Inventory::removeFromWealth( Item* item )
{
	m_wealth = qMax( 0, m_wealth - item->value() );
}

bool Inventory::itemsChanged()
{
	bool out       = m_itemsChanged;
	m_itemsChanged = false;
	return out;
}

void Inventory::setItemsChanged()
{
	m_itemsChanged = true;
}

QString Inventory::itemGroup( unsigned int itemID )
{
	auto item = getItem( itemID );
	if ( item )
	{
		return DBH::itemGroup( item->itemSID() );
	}
	return "";
}

QList<QString> Inventory::allMats( unsigned int itemID )
{
	auto item = getItem( itemID );
	if ( item )
	{
		QStringList out;

		auto comps = item->components();
		if( comps.size() )
		{
			for( const auto& comp : comps )
			{
				out.append( DBH::materialSID( comp.materialUID ) );
			}
		}
		else
		{
			out.append( item->materialSID() );
		}


		return out;
	}
	return QStringList();
}

ItemHistory* Inventory::itemHistory()
{
	return m_itemHistory;
}
