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
#include "../base/global.h"
#include "../base/util.h"
#include "../game/game.h"
#include "../game/inventory.h"
#include "../gui/strings.h"

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

AggregatorInventory::~AggregatorInventory()
{
}

void AggregatorInventory::init( Game* game )
{
	g = game;
}

void AggregatorInventory::onRequestCategories()
{
	if( !g ) return;
	m_categories.clear();
	for ( const auto& cat : g->inv()->categories() )
	{
		GuiInventoryCategory gic;
		gic.id = cat;
		gic.name = S::s( "$CategoryName_" + cat );

		for ( const auto& group : g->inv()->groups( cat ) )
		{
			GuiInventoryGroup gig;
			gig.id = group;
			gig.name = S::s( "$GroupName_" + group );
			gig.cat = cat;


			for ( const auto& item : g->inv()->items( cat, group ) )
			{
				GuiInventoryItem gii;
				gii.id = item;
				gii.name = S::s( "$ItemName_" + item );
				gii.cat = cat;
				gii.group = group;

				for ( const auto& mat : g->inv()->materials( cat, group, item ) )
				{
					auto result   = g->inv()->itemCountDetailed( item, mat );

					GuiInventoryMaterial gim;
					gim.id = mat;
					gim.name = S::s( "$MaterialName_" + mat );
					gim.cat = cat;
					gim.group = group;
					gim.item = item;
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
				gig.countTotal += gii.countTotal;
				gig.countInStockpiles += gii.countInStockpiles;

				gig.items.append( gii );
			}

			

			/*
			std::sort( gig.items.begin(), gig.items.end(), []( const GuiInventoryItem a, const GuiInventoryItem& b ) -> bool {
				if ( a.countTotal != b.countTotal )
					return a.countTotal > b.countTotal;
				if ( a.item != b.item )
					return a.item > b.item;
				return a.material > b.material;
			} );
			*/

			gic.countTotal += gig.countTotal;
			gic.countInStockpiles += gig.countInStockpiles;

			gic.groups.append( gig );
		}
		m_categories.append( gic );
	}

	emit signalInventoryCategories( m_categories );
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
		 
		qDebug() << "Type:" << m_buildSelection2String.value( buildSelection )<< "Category" << category << rows.size();
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