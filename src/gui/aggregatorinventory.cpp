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
#include "aggregatorinventory.h"

#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/util.h"
#include "../game/game.h"
#include "../game/inventory.h"
#include "../gui/strings.h"

#include <range/v3/view.hpp>

AggregatorInventory::AggregatorInventory( QObject* parent ) :
	QObject( parent )
{

	m_buildSelection2String.insert_or_assign( BuildSelection::Workshop, "Workshop" );
	m_buildSelection2String.insert_or_assign( BuildSelection::Wall, "Wall" );
	m_buildSelection2String.insert_or_assign( BuildSelection::Floor, "Floor" );
	m_buildSelection2String.insert_or_assign( BuildSelection::Stairs, "Stairs" );
	m_buildSelection2String.insert_or_assign( BuildSelection::Ramps, "Ramp" );
	m_buildSelection2String.insert_or_assign( BuildSelection::Containers, "Containers" );
	m_buildSelection2String.insert_or_assign( BuildSelection::Fence, "Fence" );
	m_buildSelection2String.insert_or_assign( BuildSelection::Furniture, "Furniture" );
	m_buildSelection2String.insert_or_assign( BuildSelection::Utility, "Utility" );

	m_buildSelection2buildItem.insert_or_assign( BuildSelection::Workshop, BuildItemType::Workshop );

	m_buildSelection2buildItem.insert_or_assign( BuildSelection::Wall, BuildItemType::Terrain );
	m_buildSelection2buildItem.insert_or_assign( BuildSelection::Floor, BuildItemType::Terrain );
	m_buildSelection2buildItem.insert_or_assign( BuildSelection::Stairs, BuildItemType::Terrain );
	m_buildSelection2buildItem.insert_or_assign( BuildSelection::Ramps, BuildItemType::Terrain );
	m_buildSelection2buildItem.insert_or_assign( BuildSelection::Fence, BuildItemType::Terrain );
	
	m_buildSelection2buildItem.insert_or_assign( BuildSelection::Containers, BuildItemType::Item );
	m_buildSelection2buildItem.insert_or_assign( BuildSelection::Furniture, BuildItemType::Item );
	m_buildSelection2buildItem.insert_or_assign( BuildSelection::Utility, BuildItemType::Item );
}

AggregatorInventory::~AggregatorInventory()
{
}

void AggregatorInventory::init( Game* game )
{
	g = game;

	for ( const auto& cat : g->inv()->categories() )
	{
		for ( const auto& group : g->inv()->groups( cat ) )
		{
			for ( const auto& item : g->inv()->items( cat, group ) )
			{
				m_itemToCategoryCache.insert_or_assign( QString::fromStdString(item), QString::fromStdString(cat) );
				m_itemToGroupCache.insert_or_assign( QString::fromStdString(item), QString::fromStdString(group) );
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
		m_watchedItems.insert( key.toStdString() );

		if( m_watchedItems.contains( gwi.category.toStdString() ) && gwi.category == key )
		{
			updateWatchedItem( gwi.category.toStdString() );
		}
		else if( m_watchedItems.contains( (gwi.category + gwi.group).toStdString() ) && gwi.category + gwi.group == key  )
		{
			updateWatchedItem( gwi.category.toStdString(), gwi.group.toStdString() );
		}
		else if( m_watchedItems.contains( (gwi.category + gwi.group + gwi.item).toStdString() ) && gwi.category + gwi.group + gwi.item == key )
		{
			updateWatchedItem( gwi.category.toStdString(), gwi.group.toStdString(), gwi.item.toStdString() );
		}
		else //if( m_watchedItems.contains( gwi.category + gwi.group + gwi.item + gwi.material ) && gwi.category + gwi.group + gwi.item + gwi.material == key )
		{
			updateWatchedItem( gwi.category.toStdString(), gwi.group.toStdString(), gwi.item.toStdString(), gwi.material.toStdString() );
		}
	}
}

void AggregatorInventory::onRequestCategories()
{
	if( !g ) return;
	m_categories.clear();
	for ( const auto& _cat : g->inv()->categories() )
	{
		const auto cat = QString::fromStdString(_cat);
		GuiInventoryCategory gic;
		gic.id = cat;
		gic.name = S::s( "$CategoryName_" + cat );
		gic.watched = m_watchedItems.contains( cat.toStdString() );

		for ( const auto& _group : g->inv()->groups( _cat ) )
		{
			const auto group = QString::fromStdString(_group);
			GuiInventoryGroup gig;
			gig.id = group;
			gig.name = S::s( "$GroupName_" + group );
			gig.cat = cat;
			gig.watched = m_watchedItems.contains( (cat + group).toStdString() );

			for ( const auto& _item : g->inv()->items( _cat, _group ) )
			{
				const auto item = QString::fromStdString(_item);
				GuiInventoryItem gii;
				gii.id = item;
				gii.name = S::s( "$ItemName_" + item );
				gii.cat = cat;
				gii.group = group;
				gii.watched = m_watchedItems.contains( (cat + group + item).toStdString() );

				for ( const auto& mat : g->inv()->materials( _cat, _group, _item ) )
				{
					auto result   = g->inv()->itemCountDetailed( item.toStdString(), mat );
					//if( result.total > 0 )
					{
						GuiInventoryMaterial gim;
						gim.id = QString::fromStdString(mat);
						gim.name = S::s( "$MaterialName_" + QString::fromStdString(mat) );
						gim.cat = cat;
						gim.group = group;
						gim.item = item;
						gim.watched = m_watchedItems.contains( (cat + group + item).toStdString() + mat );
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

	signalInventoryCategories( m_categories );
}

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
				rows = DB::selectRows( "Constructions", "Type", m_buildSelection2String.at( buildSelection ), "Category", category );
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
			gbi.biType = m_buildSelection2buildItem.at( buildSelection );

			setBuildItemValues( gbi, buildSelection );

			m_buildItems.append( gbi );
		}
	}
	signalBuildItems( m_buildItems );
}

void AggregatorInventory::setBuildItemValues( GuiBuildItem& gbi, BuildSelection selection )
{
	if( !g ) return;
	auto type = m_buildSelection2buildItem.at( selection );
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

			std::vector<std::string> mats;
			for ( int i = 0; i < 25; ++i )
				mats.push_back( "None" );

			const auto pm = Global::util->createWorkshopImage( gbi.id.toStdString(), mats );
			Global::util->createBufferForNoesisImage( pm.get(), gbi.buffer );
			gbi.iconWidth = pm->w;
			gbi.iconHeight = pm->h;
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

			std::vector<std::string> mats;
			for ( int i = 0; i < 25; ++i )
				mats.push_back( "None" );

			const auto pm = Global::util->createConstructionImage( gbi.id.toStdString(), mats );

			Global::util->createBufferForNoesisImage( pm.get(), gbi.buffer );
			gbi.iconWidth = pm->w;
			gbi.iconHeight = pm->h;
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


			std::vector<std::string> mats;
			for ( int i = 0; i < 25; ++i )
				mats.push_back( "None" );

			const auto pm = Global::util->createItemImage( gbi.id.toStdString(), mats );
			Global::util->createBufferForNoesisImage( pm.get(), gbi.buffer );
			gbi.iconWidth = pm->w;
			gbi.iconHeight = pm->h;
		}
		break;
	}
}

void AggregatorInventory::setAvailableMats( GuiBuildRequiredItem& gbri )
{
	if( !g ) return;
	auto mats = g->inv()->materialCountsForItem( gbri.itemID.toStdString() );

	gbri.availableMats.append( { "any", mats["any"] } );
	for ( auto key : mats | ranges::views::keys )
	{
		if ( key != "any" )
		{
			gbri.availableMats.append( { QString::fromStdString(key), mats[key] } );
		}
	}
}

void AggregatorInventory::onSetActive( bool active, const GuiWatchedItem& gwi )
{
	const auto key = (gwi.category + gwi.group + gwi.item + gwi.material).toStdString();
	if( active )
	{
		m_watchedItems.insert( key );
		GameState::watchedItemList.append( gwi );
	}
	else
	{
		m_watchedItems.erase( key );
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

	if( m_watchedItems.contains( gwi.category.toStdString() ) && gwi.category.toStdString() == key )
	{
		updateWatchedItem( gwi.category.toStdString() );
	}
	else if( m_watchedItems.contains( (gwi.category + gwi.group).toStdString() ) && (gwi.category + gwi.group).toStdString() == key  )
	{
		updateWatchedItem( gwi.category.toStdString(), gwi.group.toStdString() );
	}
	else if( m_watchedItems.contains( (gwi.category + gwi.group + gwi.item).toStdString() ) && (gwi.category + gwi.group + gwi.item).toStdString() == key )
	{
		updateWatchedItem( gwi.category.toStdString(), gwi.group.toStdString(), gwi.item.toStdString() );
	}
	else //if( m_watchedItems.contains( gwi.category + gwi.group + gwi.item + gwi.material ) && gwi.category + gwi.group + gwi.item + gwi.material == key )
	{
		updateWatchedItem( gwi.category.toStdString(), gwi.group.toStdString(), gwi.item.toStdString(), gwi.material.toStdString() );
	}


	//onRequestCategories();
}

    
void AggregatorInventory::onAddItem( const std::string& itemSID, const std::string& materialSID )
{
	const auto cat = m_itemToCategoryCache.at( QString::fromStdString(itemSID) ).toStdString();
	const auto group = m_itemToGroupCache.at( QString::fromStdString(itemSID) ).toStdString();

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

void AggregatorInventory::onRemoveItem( const std::string& itemSID, const std::string& materialSID )
{
	const auto cat = m_itemToCategoryCache.at( QString::fromStdString(itemSID) ).toStdString();
	const auto group = m_itemToGroupCache.at( QString::fromStdString(itemSID) ).toStdString();

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

void AggregatorInventory::updateWatchedItem( const std::string& cat )
{
	for( auto& gwi : GameState::watchedItemList )
	{
		if( gwi.category == QString::fromStdString(cat) && gwi.group.isEmpty() && gwi.item.isEmpty() && gwi.material.isEmpty() )
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
			gwi.guiString = S::s( "$CategoryName_" + QString::fromStdString(cat) ) + ": " + QString::number( gwi.count );
			break;
		}
	}
	signalWatchList( GameState::watchedItemList );
}
    
void AggregatorInventory::updateWatchedItem( const std::string& cat, const std::string& group )
{
	for( auto& gwi : GameState::watchedItemList )
	{
		if( gwi.category == QString::fromStdString(cat) && gwi.group == QString::fromStdString(group) && gwi.item.isEmpty() && gwi.material.isEmpty() )
		{
			gwi.count = 0;

			for ( const auto& item : g->inv()->items( cat, group ) )
			{
				for ( const auto& mat : g->inv()->materials( cat, group, item ) )
				{
					gwi.count += g->inv()->itemCount( item, mat );
				}
			}
			gwi.guiString = S::s( "$GroupName_" + QString::fromStdString(group) ) + ": " + QString::number( gwi.count );

			break;
		}
	}
	signalWatchList( GameState::watchedItemList );
}

void AggregatorInventory::updateWatchedItem( const std::string& cat, const std::string& group, const std::string& item )
{
	for( auto& gwi : GameState::watchedItemList )
	{
		if( gwi.category == QString::fromStdString(cat) && gwi.group == QString::fromStdString(group) && gwi.item == QString::fromStdString(item) && gwi.material.isEmpty() )
		{
			gwi.count = 0;
			for ( const auto& mat : g->inv()->materials( cat, group, item ) )
			{
				gwi.count += g->inv()->itemCount( item, mat );
			}
			gwi.guiString = S::s( "$ItemName_" + QString::fromStdString(item) ) + ": " + QString::number( gwi.count );
			break;
		}
	}
	signalWatchList( GameState::watchedItemList );
}

void AggregatorInventory::updateWatchedItem( const std::string& cat, const std::string& group, const std::string& item, const std::string& mat )
{
	for( auto& gwi : GameState::watchedItemList )
	{
		if( gwi.category == QString::fromStdString(cat) && gwi.group == QString::fromStdString(group) && gwi.item == QString::fromStdString(item) && gwi.material == QString::fromStdString(mat) )
		{
			gwi.count = g->inv()->itemCount( item, mat );
			gwi.guiString = S::s( "$MaterialName_" + QString::fromStdString(mat) ) + " " + S::s( "$ItemName_" + QString::fromStdString(item) ) + ": " + QString::number( gwi.count );
			break;
		}
	}
	signalWatchList( GameState::watchedItemList );
}

void AggregatorInventory::update()
{
	onRequestCategories();

	signalWatchList( GameState::watchedItemList );
}