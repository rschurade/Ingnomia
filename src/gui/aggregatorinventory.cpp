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
/** @file aggregatorinventory.cpp
 *  @brief AggregatorInventory implementation: builds the inventory category tree, the build
 *         menu catalogue with preview icons, and the top-bar watch list. Also listens for
 *         item add/remove events so watched rows update live.
 */
#include "aggregatorinventory.h"

#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/util.h"
#include "../game/game.h"
#include "../game/inventory.h"
#include "../gui/strings.h"

/// @brief Constructs the AggregatorInventory and seeds the BuildSelection → string/BuildItemType
///        lookup maps used to route build-menu requests.
/// @param parent Qt parent object.
AggregatorInventory::AggregatorInventory( QObject* parent ) :
	QObject( parent )
{

	m_buildSelection2String.insert( BuildSelection::Workshop, "Workshop" );
	m_buildSelection2String.insert( BuildSelection::Wall, "Wall" );
	m_buildSelection2String.insert( BuildSelection::Floor, "Floor" );
	m_buildSelection2String.insert( BuildSelection::Stairs, "Stairs" );
	m_buildSelection2String.insert( BuildSelection::Ramps, "Ramp" );
	m_buildSelection2String.insert( BuildSelection::Containers, "Containers" );
	m_buildSelection2String.insert( BuildSelection::Fence, "Fence" );
	m_buildSelection2String.insert( BuildSelection::Furniture, "Furniture" );
	m_buildSelection2String.insert( BuildSelection::Utility, "Utility" );

	m_buildSelection2buildItem.insert( BuildSelection::Workshop, BuildItemType::Workshop );

	m_buildSelection2buildItem.insert( BuildSelection::Wall, BuildItemType::Terrain );
	m_buildSelection2buildItem.insert( BuildSelection::Floor, BuildItemType::Terrain );
	m_buildSelection2buildItem.insert( BuildSelection::Stairs, BuildItemType::Terrain );
	m_buildSelection2buildItem.insert( BuildSelection::Ramps, BuildItemType::Terrain );
	m_buildSelection2buildItem.insert( BuildSelection::Fence, BuildItemType::Terrain );
	
	m_buildSelection2buildItem.insert( BuildSelection::Containers, BuildItemType::Item );
	m_buildSelection2buildItem.insert( BuildSelection::Furniture, BuildItemType::Item );
	m_buildSelection2buildItem.insert( BuildSelection::Utility, BuildItemType::Item );
}

/// @brief Destructor.
AggregatorInventory::~AggregatorInventory()
{
}

/// @brief Binds the aggregator to a Game instance, pre-caches the item → group and
///        item → category lookups, and restores the watch list from GameState by dispatching
///        each saved watched entry to the correct updateWatchedItem() overload.
/// @param game Game to bind to.
void AggregatorInventory::init( Game* game )
{
	g = game;

	for ( const auto& cat : g->inv()->categories() )
	{
		for ( const auto& group : g->inv()->groups( cat ) )
		{
			for ( const auto& item : g->inv()->items( cat, group ) )
			{
				m_itemToCategoryCache.insert( item, cat );
				m_itemToGroupCache.insert( item, group );
			}
		}
	}
	m_watchedItems.clear();
	// Can't use iterator due to 2nd reference being created in updateWatchedItem
	for( size_t i = 0; i < GameState::watchedItemList.size(); ++i )
	{
		// Reference is valid until first updateWatchedItem only
		const auto& gwi = GameState::watchedItemList[i];
		QString key = gwi.category + gwi.group + gwi.item + gwi.material;
		m_watchedItems.insert( key );

		if( m_watchedItems.contains( gwi.category ) && gwi.category == key )
		{
			updateWatchedItem( gwi.category );
		}
		else if( m_watchedItems.contains( gwi.category + gwi.group ) && gwi.category + gwi.group == key  )
		{
			updateWatchedItem( gwi.category, gwi.group );
		}
		else if( m_watchedItems.contains( gwi.category + gwi.group + gwi.item ) && gwi.category + gwi.group + gwi.item == key )
		{
			updateWatchedItem( gwi.category, gwi.group, gwi.item );
		}
		else //if( m_watchedItems.contains( gwi.category + gwi.group + gwi.item + gwi.material ) && gwi.category + gwi.group + gwi.item + gwi.material == key )
		{
			updateWatchedItem( gwi.category, gwi.group, gwi.item, gwi.material );
		}
	}
}

/// @brief Walks the inventory category/group/item/material tree and emits a populated
///        GuiInventoryCategory list with per-level totals and watch flags.
void AggregatorInventory::onRequestCategories()
{
	if( !g ) return;
	m_categories.clear();
	for ( const auto& cat : g->inv()->categories() )
	{
		GuiInventoryCategory gic;
		gic.id = cat;
		gic.name = S::s( "$CategoryName_" + cat );
		gic.watched = m_watchedItems.contains( cat );

		for ( const auto& group : g->inv()->groups( cat ) )
		{
			GuiInventoryGroup gig;
			gig.id = group;
			gig.name = S::s( "$GroupName_" + group );
			gig.cat = cat;
			gig.watched = m_watchedItems.contains( cat + group );

			for ( const auto& item : g->inv()->items( cat, group ) )
			{
				GuiInventoryItem gii;
				gii.id = item;
				gii.name = S::s( "$ItemName_" + item );
				gii.cat = cat;
				gii.group = group;
				gii.watched = m_watchedItems.contains( cat + group + item );

				for ( const auto& mat : g->inv()->materials( cat, group, item ) )
				{
					auto result   = g->inv()->itemCountDetailed( item, mat );
					//if( result.total > 0 )
					{
						GuiInventoryMaterial gim;
						gim.id = mat;
						gim.name = S::s( "$MaterialName_" + mat );
						gim.cat = cat;
						gim.group = group;
						gim.item = item;
						gim.watched = m_watchedItems.contains( cat + group + item + mat );
						gim.countTotal = result.total; 
						gim.countInJob = result.inJob; 
						gim.countInStockpiles = result.inStockpile;
						gim.countEquipped = result.equipped;
						gim.countConstructed = result.constructed;
						gim.countLoose = result.loose; 
						gim.totalValue = result.totalValue;

						gii.countTotal += result.total;
						gii.countInStockpiles += result.inStockpile;

						gii.materials.append( gim );
					}
				}
				gig.countTotal += gii.countTotal;
				gig.countInStockpiles += gii.countInStockpiles;

				gig.items.append( gii );
			}

			gic.countTotal += gig.countTotal;
			gic.countInStockpiles += gig.countInStockpiles;

			gic.groups.append( gig );
		}
	
		m_categories.append( gic );
	}

	emit signalInventoryCategories( m_categories );
}

/// @brief Collects the buildable entries for the given BuildSelection / category combination
///        (Constructions / Workshops / Containers / Items DB rows), produces preview icons
///        and required-item lists, and emits signalBuildItems.
/// @param buildSelection High-level build category (Wall, Workshop, Furniture, …).
/// @param category       Sub-category (e.g. workshop tab name or construction category).
void AggregatorInventory::onRequestBuildItems( BuildSelection buildSelection, QString category )
{
	if( !g ) return;
	m_buildItems.clear();
	if ( m_buildSelection2String.contains( buildSelection ) )
	{
		QList<QVariantMap> rows;
		QString prefix = "$ConstructionName_";
		switch( buildSelection )
		{
			case BuildSelection::Wall:
			case BuildSelection::Floor:
			case BuildSelection::Stairs:
			case BuildSelection::Ramps:
			case BuildSelection::Fence:
				rows = DB::selectRows( "Constructions", "Type", m_buildSelection2String.value( buildSelection ), "Category", category );
				break;
			case BuildSelection::Workshop:
				rows = DB::selectRows( "Workshops", "Tab", category );
				prefix = "$WorkshopName_";
				break;
			case BuildSelection::Containers:
				rows = DB::selectRows( "Containers", "Buildable", "1" );
				prefix = "$ItemName_";
				break;
			case BuildSelection::Furniture:
				rows = DB::selectRows( "Items", "Category", "Furniture", "ItemGroup", category );
				prefix = "$ItemName_";
				break;
			case BuildSelection::Utility:
				rows = DB::selectRows( "Items", "Category", "Utility", "ItemGroup", category );
				prefix = "$ItemName_";
				break;
		}
		 
		//qDebug() << "Type:" << m_buildSelection2String.value( buildSelection )<< "Category" << category << rows.size();
		for ( auto row : rows )
		{
			GuiBuildItem gbi;
			gbi.id   = row.value( "ID" ).toString();
			gbi.name = S::s( prefix + row.value( "ID" ).toString() );
			gbi.biType = m_buildSelection2buildItem.value( buildSelection );

			setBuildItemValues( gbi, buildSelection );

			m_buildItems.append( gbi );
		}
	}
	emit signalBuildItems( m_buildItems );
}

/// @brief Populates a GuiBuildItem with required components and a PNG-encoded preview icon,
///        using the correct component table and icon builder for the given selection type.
/// @param gbi       Build item to populate (modified in place).
/// @param selection High-level build category used to pick the right component/icon path.
void AggregatorInventory::setBuildItemValues( GuiBuildItem& gbi, BuildSelection selection )
{
	if( !g ) return;
	auto type = m_buildSelection2buildItem.value( selection );
	switch ( type )
	{
		case BuildItemType::Workshop:
		{
			for ( auto row : DB::selectRows( "Workshops_Components", gbi.id ) )
			{
				if ( !row.value( "ItemID" ).toString().isEmpty() )
				{
					GuiBuildRequiredItem gbri;
					gbri.itemID = row.value( "ItemID" ).toString();
					gbri.amount = row.value( "Amount" ).toInt();
					setAvailableMats( gbri );
					gbi.requiredItems.append( gbri );
				}
			}

			QStringList mats;
			for ( int i = 0; i < 25; ++i )
				mats.push_back( "None" );

			QPixmap pm = Global::util->createWorkshopImage( gbi.id, mats );
			Global::util->createBufferForNoesisImage( pm, gbi.buffer );
			gbi.iconWidth = pm.width();
			gbi.iconHeight = pm.height();
		}
		break;
		case BuildItemType::Terrain:
		{
			for ( auto row : DB::selectRows( "Constructions_Components", gbi.id ) )
			{
					GuiBuildRequiredItem gbri;
					gbri.itemID = row.value( "ItemID" ).toString();
					gbri.amount = row.value( "Amount" ).toInt();
					setAvailableMats( gbri );
					gbi.requiredItems.append( gbri );

			}

			QStringList mats;
			for ( int i = 0; i < 25; ++i )
				mats.push_back( "None" );

			QPixmap pm = Global::util->createConstructionImage( gbi.id, mats );

			Global::util->createBufferForNoesisImage( pm, gbi.buffer );
			gbi.iconWidth = pm.width();
			gbi.iconHeight = pm.height();
		}
		break;
		case BuildItemType::Item:
		{
			auto rows = DB::selectRows( "Constructions_Components", gbi.id );

			if( rows.size() )
			{
				for ( auto row : rows )
				{
					GuiBuildRequiredItem gbri;
					gbri.itemID = row.value( "ItemID" ).toString();
					gbri.amount = row.value( "Amount" ).toInt();
					setAvailableMats( gbri );
					gbi.requiredItems.append( gbri );
				}
			}
			else
			{
				GuiBuildRequiredItem gbri;
				gbri.itemID = gbi.id;
				gbri.amount = 1;
				setAvailableMats( gbri );
				gbi.requiredItems.append( gbri );
			}


			QStringList mats;
			for ( int i = 0; i < 25; ++i )
				mats.push_back( "None" );

			QPixmap pm = Global::util->createItemImage( gbi.id, mats );
			Global::util->createBufferForNoesisImage( pm, gbi.buffer );
			gbi.iconWidth = pm.width();
			gbi.iconHeight = pm.height();
		}
		break;
	}
}

/// @brief Fills the availableMats list on @p gbri with (material, count) pairs for the
///        component item, always placing the "any" wildcard first.
/// @param gbri Required component to populate.
void AggregatorInventory::setAvailableMats( GuiBuildRequiredItem& gbri )
{
	if( !g ) return;
	auto mats = g->inv()->materialCountsForItem( gbri.itemID );

	gbri.availableMats.append( { "any", mats["any"] } );
	for ( auto key : mats.keys() )
	{
		if ( key != "any" )
		{
			gbri.availableMats.append( { key, mats[key] } );
		}
	}
}

/// @brief Adds or removes @p gwi from the persistent watch list (GameState::watchedItemList)
///        and refreshes the corresponding count by dispatching to the appropriate
///        updateWatchedItem() overload.
/// @param active True to add to the watch list, false to remove.
/// @param gwi    Watch list entry (category/group/item/material path).
void AggregatorInventory::onSetActive( bool active, const GuiWatchedItem& gwi )
{
	QString key = gwi.category + gwi.group + gwi.item + gwi.material;
	if( active )
	{
		m_watchedItems.insert( key );
		GameState::watchedItemList.append( gwi );
	}
	else
	{
		m_watchedItems.remove( key );
		for( int i = 0; i < GameState::watchedItemList.size(); ++i )
		{
			auto hwi = GameState::watchedItemList[i];
			if( gwi.category == hwi.category && gwi.group == hwi.group && gwi.item == hwi.item && gwi.material == hwi.material )
			{
				GameState::watchedItemList.removeAt( i );
				break;
			}
		}
	}

	if( m_watchedItems.contains( gwi.category ) && gwi.category == key )
	{
		updateWatchedItem( gwi.category );
	}
	else if( m_watchedItems.contains( gwi.category + gwi.group ) && gwi.category + gwi.group == key  )
	{
		updateWatchedItem( gwi.category, gwi.group );
	}
	else if( m_watchedItems.contains( gwi.category + gwi.group + gwi.item ) && gwi.category + gwi.group + gwi.item == key )
	{
		updateWatchedItem( gwi.category, gwi.group, gwi.item );
	}
	else //if( m_watchedItems.contains( gwi.category + gwi.group + gwi.item + gwi.material ) && gwi.category + gwi.group + gwi.item + gwi.material == key )
	{
		updateWatchedItem( gwi.category, gwi.group, gwi.item, gwi.material );
	}


	//onRequestCategories();
}

    
/// @brief Live-update hook invoked when an item is added to the world. Refreshes any watched
///        rows whose path contains this (item, material) pair.
/// @param itemSID     Item string ID.
/// @param materialSID Material string ID.
void AggregatorInventory::onAddItem( QString itemSID, QString materialSID )
{
	QString cat = m_itemToCategoryCache.value( itemSID );
	QString group = m_itemToGroupCache.value( itemSID );

	if( m_watchedItems.contains( cat ) )
	{
		updateWatchedItem( cat );
	}
	if( m_watchedItems.contains( cat + group ) )
	{
		updateWatchedItem( cat, group );
	}
	if( m_watchedItems.contains( cat + group + itemSID ) )
	{
		updateWatchedItem( cat, group, itemSID );
	}
	if( m_watchedItems.contains( cat + group + itemSID + materialSID ) )
	{
		updateWatchedItem( cat, group, itemSID, materialSID );
	}
}

/// @brief Live-update hook invoked when an item is removed from the world. Refreshes any
///        watched rows whose path contains this (item, material) pair.
/// @param itemSID     Item string ID.
/// @param materialSID Material string ID.
void AggregatorInventory::onRemoveItem( QString itemSID, QString materialSID )
{
	QString cat = m_itemToCategoryCache.value( itemSID );
	QString group = m_itemToGroupCache.value( itemSID );

	if( m_watchedItems.contains( cat ) )
	{
		updateWatchedItem( cat );
	}
	if( m_watchedItems.contains( cat + group ) )
	{
		updateWatchedItem( cat, group );
	}
	if( m_watchedItems.contains( cat + group + itemSID ) )
	{
		updateWatchedItem( cat, group, itemSID );
	}
	if( m_watchedItems.contains( cat + group + itemSID + materialSID ) )
	{
		updateWatchedItem( cat, group, itemSID, materialSID );
	}
}

/// @brief Recomputes the count for a category-level watch entry and emits signalWatchList.
/// @param cat Category ID whose watch entry to refresh.
void AggregatorInventory::updateWatchedItem( QString cat )
{
	for( auto& gwi : GameState::watchedItemList )
	{
		if( gwi.category == cat && gwi.group.isEmpty() && gwi.item.isEmpty() && gwi.material.isEmpty() )
		{
			gwi.count = 0;
			for ( const auto& group : g->inv()->groups( cat ) )
			{
				for ( const auto& item : g->inv()->items( cat, group ) )
				{
					for ( const auto& mat : g->inv()->materials( cat, group, item ) )
					{
						gwi.count += g->inv()->itemCount( item, mat );
					}
				}
			}
			gwi.guiString = S::s( "$CategoryName_" + cat ) + ": " + QString::number( gwi.count );
			break;
		}
	}
	emit signalWatchList( GameState::watchedItemList );
}
    
/// @brief Recomputes the count for a group-level watch entry and emits signalWatchList.
/// @param cat   Category ID.
/// @param group Group ID within @p cat.
void AggregatorInventory::updateWatchedItem( QString cat, QString group )
{
	for( auto& gwi : GameState::watchedItemList )
	{
		if( gwi.category == cat && gwi.group == group && gwi.item.isEmpty() && gwi.material.isEmpty() )
		{
			gwi.count = 0;

			for ( const auto& item : g->inv()->items( cat, group ) )
			{
				for ( const auto& mat : g->inv()->materials( cat, group, item ) )
				{
					gwi.count += g->inv()->itemCount( item, mat );
				}
			}
			gwi.guiString = S::s( "$GroupName_" + group ) + ": " + QString::number( gwi.count );

			break;
		}
	}
	emit signalWatchList( GameState::watchedItemList );
}

/// @brief Recomputes the count for an item-level watch entry and emits signalWatchList.
/// @param cat   Category ID.
/// @param group Group ID.
/// @param item  Item ID.
void AggregatorInventory::updateWatchedItem( QString cat, QString group, QString item )
{
	for( auto& gwi : GameState::watchedItemList )
	{
		if( gwi.category == cat && gwi.group == group && gwi.item == item && gwi.material.isEmpty() )
		{
			gwi.count = 0;
			for ( const auto& mat : g->inv()->materials( cat, group, item ) )
			{
				gwi.count += g->inv()->itemCount( item, mat );
			}
			gwi.guiString = S::s( "$ItemName_" + item ) + ": " + QString::number( gwi.count );
			break;
		}
	}
	emit signalWatchList( GameState::watchedItemList );
}

/// @brief Recomputes the count for a fully qualified (item, material) watch entry and emits
///        signalWatchList.
/// @param cat   Category ID.
/// @param group Group ID.
/// @param item  Item ID.
/// @param mat   Material ID.
void AggregatorInventory::updateWatchedItem( QString cat, QString group, QString item, QString mat )
{
	for( auto& gwi : GameState::watchedItemList )
	{
		if( gwi.category == cat && gwi.group == group && gwi.item == item && gwi.material == mat )
		{
			gwi.count = g->inv()->itemCount( item, mat );
			gwi.guiString = S::s( "$MaterialName_" + mat ) + " " + S::s( "$ItemName_" + item ) + ": " + QString::number( gwi.count );
			break;
		}
	}
	emit signalWatchList( GameState::watchedItemList );
}

/// @brief Full refresh: rebuilds the category tree and re-emits the watch list.
void AggregatorInventory::update()
{
	onRequestCategories();

	emit signalWatchList( GameState::watchedItemList );
}