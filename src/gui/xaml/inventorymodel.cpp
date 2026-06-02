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

/** @file inventorymodel.cpp
 *  @brief InventoryModel and Inv*Item implementations. Each tree-row class wraps a Gui*Info
 *         payload, exposes its name and counts, and forwards checkbox toggles to the
 *         InventoryProxy. The tri-state UpdateState() helpers compute partial-check states
 *         from the children's individual states. Trivial Get/Set accessors are XAML plumbing.
 */

using namespace IngnomiaGUI;
using namespace Noesis;
using namespace NoesisApp;


#pragma region MaterialItem
////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Builds a leaf material row from a GuiInventoryMaterial payload.
IngnomiaGUI::InvMaterialItem::InvMaterialItem( const GuiInventoryMaterial& mat, InventoryProxy* proxy ) :
	m_proxy( proxy ),
	m_sid( mat.id ),
	m_category( mat.cat ),
	m_group( mat.group ),
	m_item( mat.item )
{
	m_name   = mat.name.toStdString().c_str();
	m_active = mat.watched;

	m_inStock = QString::number( mat.countInStockpiles ).toStdString().c_str();
	m_total = QString::number( mat.countTotal ).toStdString().c_str();
}

const char* IngnomiaGUI::InvMaterialItem::GetName() const
{
	return m_name.Str();
}

bool IngnomiaGUI::InvMaterialItem::GetChecked() const
{
	return m_active;
}

/// @brief Toggles the watch flag for this material via the proxy.
void IngnomiaGUI::InvMaterialItem::SetChecked( bool value )
{
	if ( m_active != value )
	{
		m_active = value;
		GuiWatchedItem gwi{ m_category, m_group, m_item, m_sid };
		m_proxy->setActive( value, gwi );
	}
}

const char* IngnomiaGUI::InvMaterialItem::getTotal() const
{
	return m_total.Str();
}
	
const char* IngnomiaGUI::InvMaterialItem::getInStock() const
{
	return m_inStock.Str();
}

#pragma endregion MaterialItem

#pragma region ItemItem
////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Builds an item row and recursively constructs InvMaterialItem children.
IngnomiaGUI::InvItemItem::InvItemItem( const GuiInventoryItem& gii, InventoryProxy* proxy ) :
	m_proxy( proxy ),
	m_sid( gii.id ),
	m_category( gii.cat ),
	m_group( gii.group )
{
	m_name = gii.name.toStdString().c_str();
	m_inStock = QString::number( gii.countInStockpiles ).toStdString().c_str();
	m_total = QString::number( gii.countTotal ).toStdString().c_str();

	m_materials = *new ObservableCollection<InvMaterialItem>();

	for( const auto& mat : gii.materials )
	{
		m_materials->Add( MakePtr<InvMaterialItem>( mat, proxy ) );
	}

	m_state = gii.watched;
	if( !m_state )
	{
		UpdateState();
	}
}

const char* IngnomiaGUI::InvItemItem::GetName() const
{
	return m_name.Str();
}

const Noesis::Nullable<bool>& IngnomiaGUI::InvItemItem::GetState() const
{
	return m_state;
}

bool IngnomiaGUI::InvItemItem::getExpanded() const
{
	return !GetState().HasValue();
}

/// @brief Tri-state checkbox setter: when fully checked or unchecked, propagates the new
///        state to every child material via the proxy.
void IngnomiaGUI::InvItemItem::SetState( const Noesis::Nullable<bool>& value )
{
	bool active = value.HasValue() && value.GetValue();
	m_state = active;
	GuiWatchedItem gwi{ m_category, m_group, m_sid };
	m_proxy->setActive( active, gwi );
}

/// @brief Recomputes the tri-state checkbox value by counting how many child materials are
///        currently watched (none → unchecked, all → checked, mix → partial).
void IngnomiaGUI::InvItemItem::UpdateState()
{
	bool allChecked   = false;
	bool allUnchecked = true;
	for ( int i = 0; i < m_materials->Count(); ++i )
	{
		bool checked = m_materials->Get( i )->GetChecked();
		if ( !checked )
		{
			allChecked = false;
		}
		else if ( checked )
		{
			allUnchecked = false;
		}
	}
	if ( allChecked && !allUnchecked )
	{
		m_state = true;
	}
	else if ( !allChecked && allUnchecked )
	{
		m_state = false;
	}
	else
	{
		m_state = nullptr;
	}

	OnPropertyChanged( "Checked" );
	OnPropertyChanged( "Expanded" );
}

const char* IngnomiaGUI::InvItemItem::getTotal() const
{
	return m_total.Str();
}
	
const char* IngnomiaGUI::InvItemItem::getInStock() const
{
	return m_inStock.Str();
}

#pragma endregion ItemItem

#pragma region GroupItem
////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Builds a group row and recursively constructs InvItemItem children.
IngnomiaGUI::InvGroupItem::InvGroupItem( const GuiInventoryGroup& gig, InventoryProxy* proxy ) :
	m_proxy( proxy ),
	m_sid( gig.id ),
	m_category( gig.cat )
{
	m_name = gig.name.toStdString().c_str();
	m_inStock = QString::number( gig.countInStockpiles ).toStdString().c_str();
	m_total = QString::number( gig.countTotal ).toStdString().c_str();

	m_items = *new ObservableCollection<InvItemItem>();

	for ( auto item : gig.items )
	{
		m_items->Add( MakePtr<InvItemItem>( item, proxy ) );
	}

	m_state = gig.watched;
	if( !m_state )
	{
		UpdateState();
	}
}

const char* IngnomiaGUI::InvGroupItem::GetName() const
{
	return m_name.Str();
}

const Noesis::Nullable<bool>& IngnomiaGUI::InvGroupItem::GetState() const
{
	return m_state;
}

bool IngnomiaGUI::InvGroupItem::getExpanded() const
{
	return !GetState().HasValue();
}

/// @brief Tri-state checkbox setter for the group level: cascades changes to every child item.
void IngnomiaGUI::InvGroupItem::SetState( const Noesis::Nullable<bool>& value )
{
	bool active = value.HasValue() && value.GetValue();
	m_state = active;
	GuiWatchedItem gwi{ m_category, m_sid };
	m_proxy->setActive( active, gwi );
}

/// @brief Recomputes the group's tri-state from its child items' states.
void IngnomiaGUI::InvGroupItem::UpdateState()
{
	bool allChecked   = false;
	bool allUnchecked = true;
	for ( int i = 0; i < m_items->Count(); ++i )
	{
		auto checked = m_items->Get( i )->GetState();
		if ( checked.HasValue() && checked )
		{
			allUnchecked = false;
		}
		else if ( checked.HasValue() && !checked )
		{
			allChecked = false;
		}
		else
		{
			allChecked   = false;
			allUnchecked = false;
		}
	}
	if ( allChecked && !allUnchecked )
	{
		m_state = true;
	}
	else if ( !allChecked && allUnchecked )
	{
		m_state = false;
	}
	else
	{
		m_state = nullptr;
	}
	OnPropertyChanged( "Checked" );
	OnPropertyChanged( "Expanded" );
}

const char* IngnomiaGUI::InvGroupItem::getTotal() const
{
	return m_total.Str();
}
	
const char* IngnomiaGUI::InvGroupItem::getInStock() const
{
	return m_inStock.Str();
}


#pragma endregion GroupItem

#pragma region CategoryItem
////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Builds a category row and recursively constructs InvGroupItem children.
IngnomiaGUI::InvCategoryItem::InvCategoryItem( const GuiInventoryCategory& gic, InventoryProxy* proxy ) :
	m_proxy( proxy ),
	m_sid( gic.id )
{
	m_name = gic.name.toStdString().c_str();
	m_inStock = QString::number( gic.countInStockpiles ).toStdString().c_str();
	m_total = QString::number( gic.countTotal ).toStdString().c_str();

	m_groups = *new ObservableCollection<InvGroupItem>();
	
	for ( auto group : gic.groups )
	{
		m_groups->Add( MakePtr<InvGroupItem>( group, proxy ) );
	}
	m_state = gic.watched;
	if( !m_state )
	{
		UpdateState();
	}
}

const char* IngnomiaGUI::InvCategoryItem::GetName() const
{
	return m_name.Str();
}

const Noesis::Nullable<bool>& IngnomiaGUI::InvCategoryItem::GetState() const
{
	return m_state;
}

bool IngnomiaGUI::InvCategoryItem::getExpanded() const
{
	return !GetState().HasValue();
}

/// @brief Tri-state checkbox setter for the category level: cascades changes to every child group.
void IngnomiaGUI::InvCategoryItem::SetState( const Noesis::Nullable<bool>& value )
{
	//qDebug() << value.HasValue() << value.GetValue() << m_state.HasValue() << m_state.GetValue();
	bool active = value.HasValue() && value.GetValue();
	if( value.HasValue() && !value.GetValue() && !m_state.HasValue() && !m_state.GetValue() )
	{
		active = true;
	}
	m_state = active;
	GuiWatchedItem gwi{ m_sid };
	m_proxy->setActive( active, gwi );
}

/// @brief Recomputes the category's tri-state from its child groups' states.
void IngnomiaGUI::InvCategoryItem::UpdateState()
{
	bool allChecked   = false;
	bool allUnchecked = true;
	for ( int i = 0; i < m_groups->Count(); ++i )
	{
		auto checked = m_groups->Get( i )->GetState();
		if ( checked.HasValue() && checked )
		{
			allUnchecked = false;
		}
		else if ( checked.HasValue() && !checked )
		{
			allChecked = false;
		}
		else
		{
			allChecked   = false;
			allUnchecked = false;
		}
	}
	if ( allChecked && !allUnchecked )
	{
		m_state = true;
	}
	else if ( !allChecked && allUnchecked )
	{
		m_state = false;
	}
	else
	{
		m_state = nullptr;
	}
	OnPropertyChanged( "Checked" );
	OnPropertyChanged( "Expanded" );
}

const char* IngnomiaGUI::InvCategoryItem::getTotal() const
{
	return m_total.Str();
}
	
const char* IngnomiaGUI::InvCategoryItem::getInStock() const
{
	return m_inStock.Str();
}


#pragma endregion CategoryItem



////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Constructs the InventoryModel: instantiates the proxy and an empty category collection,
///        then asks the proxy to push the initial category tree.
InventoryModel::InventoryModel()
{
	m_proxy = new InventoryProxy;
	m_proxy->setParent( this );

	m_categories = *new ObservableCollection<InvCategoryItem>();

	m_proxy->requestCategories();
}

/// @brief Replaces the m_categories observable collection with fresh InvCategoryItem rows.
void InventoryModel::updateCategories( const QList<GuiInventoryCategory>& categories )
{
	m_categories->Clear();
	for ( const auto& cat : categories )
	{
		m_categories->Add( MakePtr<InvCategoryItem>( cat, m_proxy ) );
	}

	OnPropertyChanged( "Categories" );
}

Noesis::ObservableCollection<InvCategoryItem>* InventoryModel::GetCategories() const
{
	return m_categories;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION( InventoryModel, "IngnomiaGUI.InventoryModel" )
{
	NsProp( "Categories", &InventoryModel::GetCategories );
}


NS_IMPLEMENT_REFLECTION( IngnomiaGUI::InvCategoryItem )
{
	NsProp( "Name", &InvCategoryItem::GetName );
	NsProp( "Checked", &InvCategoryItem::GetState, &InvCategoryItem::SetState );
	NsProp( "Expanded", &InvCategoryItem::getExpanded );
	NsProp( "Children", &InvCategoryItem::GetGroups );
	NsProp( "Total", &InvCategoryItem::getTotal );
	NsProp( "InStock", &InvCategoryItem::getInStock );
}

NS_IMPLEMENT_REFLECTION( IngnomiaGUI::InvGroupItem )
{
	NsProp( "Name", &InvGroupItem::GetName );
	NsProp( "Checked", &InvGroupItem::GetState, &InvGroupItem::SetState );
	NsProp( "Expanded", &InvGroupItem::getExpanded );
	NsProp( "Children", &InvGroupItem::GetItems );
	NsProp( "Total", &InvGroupItem::getTotal );
	NsProp( "InStock", &InvGroupItem::getInStock );
}

NS_IMPLEMENT_REFLECTION( IngnomiaGUI::InvItemItem )
{
	NsProp( "Name", &InvItemItem::GetName );
	NsProp( "Checked", &InvItemItem::GetState, &InvItemItem::SetState );
	NsProp( "Expanded", &InvItemItem::getExpanded );
	NsProp( "Children", &InvItemItem::GetMaterials );
	NsProp( "Total", &InvItemItem::getTotal );
	NsProp( "InStock", &InvItemItem::getInStock );
}

NS_IMPLEMENT_REFLECTION( IngnomiaGUI::InvMaterialItem )
{
	NsProp( "Name", &InvMaterialItem::GetName );
	NsProp( "Checked", &InvMaterialItem::GetChecked, &InvMaterialItem::SetChecked );
	NsProp( "Total", &InvMaterialItem::getTotal );
	NsProp( "InStock", &InvMaterialItem::getInStock );
}
