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
#include "inventorymodel.h"
#include "inventoryproxy.h"

#include <NsApp/Application.h>
#include <NsCore/Log.h>
#include <NsCore/ReflectionImplement.h>
#include <NsCore/ReflectionImplementEnum.h>
#include <NsGui/ObservableCollection.h>
#include <NsGui/UIElement.h>

#include <QDebug>

using namespace IngnomiaGUI;
using namespace Noesis;
using namespace NoesisApp;



////////////////////////////////////////////////////////////////////////////////////////////////////
CategoryButton::CategoryButton( QString name, QString sid, QString image )
{
	m_name  = name.toStdString().c_str();
	m_sid   = sid.toStdString().c_str();
	m_image = image.toStdString().c_str();
}

const char* CategoryButton::GetName() const
{
	return m_name.Str();
}

const char* CategoryButton::GetID() const
{
	return m_sid.Str();
}

const char* CategoryButton::GetImage() const
{
	return m_image.Str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
InvItemEntry::InvItemEntry( QString item, QString material, QString inStockpile, QString total ) :
	m_item( item.toStdString().c_str() ),
	m_material( material.toStdString().c_str() ),
	m_inStockpile( inStockpile.toStdString().c_str() ),
	m_total( total.toStdString().c_str() )
{
}
	
const char* InvItemEntry::GetItem() const
{
	return m_item.Str();
}

const char* InvItemEntry::GetMaterial() const
{
	return m_material.Str();
}

const char* InvItemEntry::GetInStockpile() const
{
	return m_inStockpile.Str();
}

const char* InvItemEntry::GetTotal() const
{
	return m_total.Str();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
InventoryModel::InventoryModel()
{
	m_proxy = new InventoryProxy;
	m_proxy->setParent( this );

	m_categoryCmd.SetExecuteFunc( MakeDelegate( this, &InventoryModel::onCategoryCmd ) );
	m_groupCmd.SetExecuteFunc( MakeDelegate( this, &InventoryModel::onGroupCmd ) );

	m_categoryButtons   = *new ObservableCollection<CategoryButton>();
	m_groupButtons   = *new ObservableCollection<CategoryButton>();
	m_items   = *new ObservableCollection<InvItemEntry>();
}

void InventoryModel::updateCategories( const QList<GuiInventoryCategory>& categories )
{
	m_categoryButtons->Clear();
	
	for( const auto& cat : categories )
	{
		m_categoryButtons->Add( MakePtr<CategoryButton>( cat.name, cat.id, "buttons/wood.png" ) );
	}
	
	if( !categories.isEmpty() )
	{
		m_category = categories.first().id;
		m_proxy->requestGroups( m_category );
	}
	OnPropertyChanged( "CategoryButtons" );
}
	
void InventoryModel::updateGroups( const QList<GuiInventoryGroup>& groups )
{
	m_groupButtons->Clear();
	
	for( const auto& group : groups )
	{
		m_groupButtons->Add( MakePtr<CategoryButton>( group.name, group.id, "buttons/wood.png" ) );
	}
	if( !groups.isEmpty() )
	{
		m_proxy->requestItems( m_category, groups.first().id );
	}

	OnPropertyChanged( "GroupButtons" );
}

void InventoryModel::updateItems( const QList<GuiInventoryItem>& items )
{
	m_items->Clear();
	
	m_items->Add( MakePtr<InvItemEntry>( "Item", "Material", "In Stockpiles", "Total in world" ) );
	for( const auto& item : items )
	{
		if( item.countTotal > 0 )
		{
			m_items->Add( MakePtr<InvItemEntry>( item.item, item.material, QString::number( item.countInStockpiles ), QString::number( item.countTotal ) ) );
		}
	}

	OnPropertyChanged( "Items" );
}

void InventoryModel::onCategoryCmd( BaseComponent* param )
{
	if( param )
	{
		m_category = param->ToString().Str();
		m_proxy->requestGroups( m_category );
	}
}

void InventoryModel::onGroupCmd( BaseComponent* param )
{
	if( param )
	{
		m_proxy->requestItems( m_category, param->ToString().Str() );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION( InventoryModel, "IngnomiaGUI.InventoryModel" )
{
	NsProp( "CmdCategory", &InventoryModel::getCategoryCmd );
	NsProp( "CmdGroup", &InventoryModel::getGroupCmd );

	NsProp( "CategoryButtons", &InventoryModel::getCategoryButtons );
	NsProp( "GroupButtons", &InventoryModel::getGroupButtons );
	NsProp( "Items", &InventoryModel::getItems );
}

NS_IMPLEMENT_REFLECTION( CategoryButton )
{
	NsProp( "Name", &CategoryButton::GetName );
	NsProp( "ID", &CategoryButton::GetID );
	NsProp( "Image", &CategoryButton::GetImage );
}

NS_IMPLEMENT_REFLECTION( InvItemEntry )
{
	NsProp( "Item", &InvItemEntry::GetItem );
	NsProp( "Material", &InvItemEntry::GetMaterial );
	NsProp( "InStock", &InvItemEntry::GetInStockpile );
	NsProp( "Total", &InvItemEntry::GetTotal );
}

