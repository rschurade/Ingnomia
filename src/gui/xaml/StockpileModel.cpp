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
#include "StockpileModel.h"

#include "../../base/filter.h"
#include "StockpileProxy.h"

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

GuiLimitItem::GuiLimitItem( QString name )
{
	m_name = name.toStdString().c_str();
}

const char* GuiLimitItem::GetName() const
{
	return m_name.Str();
}

StockedItemSummary::StockedItemSummary( QString name )
{
	m_name = name.toStdString().c_str();
}

const char* StockedItemSummary::GetName() const
{
	return m_name.Str();
}

#pragma region MaterialItem
////////////////////////////////////////////////////////////////////////////////////////////////////

IngnomiaGUI::MaterialItem::MaterialItem( unsigned int stockpileID, QString name, QString sid, QString cat, QString group, QString item, Filter& filter, StockpileProxy* proxy ) :
	m_stockpileID( stockpileID ),
	m_proxy( proxy ),
	m_sid( sid ),
	m_category( cat ),
	m_group( group ),
	m_item( item )
{
	m_name   = name.toStdString().c_str();
	m_active = filter.getCheckState( cat, group, item, sid );
}

const char* IngnomiaGUI::MaterialItem::GetName() const
{
	return m_name.Str();
}

bool IngnomiaGUI::MaterialItem::GetChecked() const
{
	return m_active;
}

void IngnomiaGUI::MaterialItem::SetChecked( bool value )
{
	if ( m_active != value )
	{
		m_proxy->setActive( m_stockpileID, value, m_category, m_group, m_item, m_sid );
	}
}

void IngnomiaGUI::MaterialItem::UpdateChecked( Filter& filter )
{
	if ( filter.getCheckState( m_category, m_group, m_item, m_sid ) != m_active )
	{
		m_active = !m_active;
		OnPropertyChanged( "Checked" );
		OnPropertyChanged( "Expanded" );
	}
}

#pragma endregion MaterialItem

#pragma region ItemItem
////////////////////////////////////////////////////////////////////////////////////////////////////

IngnomiaGUI::ItemItem::ItemItem( unsigned int stockpileID, QString name, QString sid, QString cat, QString group, Filter& filter, StockpileProxy* proxy ) :
	m_stockpileID( stockpileID ),
	m_proxy( proxy ),
	m_sid( sid ),
	m_category( cat ),
	m_group( group )
{
	m_name = name.toStdString().c_str();

	m_materials = *new ObservableCollection<MaterialItem>();

	for ( auto mat : filter.materials( cat, group, sid ) )
	{
		m_materials->Add( MakePtr<MaterialItem>( m_stockpileID, mat, mat, cat, group, sid, filter, proxy ) );
	}

	UpdateState();
}

const char* IngnomiaGUI::ItemItem::GetName() const
{
	return m_name.Str();
}

const Noesis::Nullable<bool>& IngnomiaGUI::ItemItem::GetState() const
{
	return m_state;
}

bool IngnomiaGUI::ItemItem::getExpanded() const
{
	return !GetState().HasValue();
}

void IngnomiaGUI::ItemItem::SetState( const Noesis::Nullable<bool>& value )
{
	bool active = value.HasValue() && value.GetValue();
	m_proxy->setActive( m_stockpileID, active, m_category, m_group, m_sid );
}

void IngnomiaGUI::ItemItem::UpdateChecked( Filter& filter )
{
	auto mats = filter.materials( m_category, m_group, m_sid );

	for ( int i = 0; i < m_materials->Count(); ++i )
	{
		m_materials->Get( i )->UpdateChecked( filter );
	}
	UpdateState();
}

void IngnomiaGUI::ItemItem::UpdateState()
{
	bool allChecked   = true;
	bool allUnchecked = true;
	for ( int i = 0; i < m_materials->Count(); ++i )
	{
		bool checked = m_materials->Get( i )->GetChecked();
		if ( checked != true )
		{
			allChecked = false;
		}
		else if ( checked != false )
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

#pragma endregion ItemItem

#pragma region GroupItem
////////////////////////////////////////////////////////////////////////////////////////////////////

IngnomiaGUI::GroupItem::GroupItem( unsigned int stockpileID, QString name, QString sid, QString cat, Filter& filter, StockpileProxy* proxy ) :
	m_stockpileID( stockpileID ),
	m_proxy( proxy ),
	m_sid( sid ),
	m_category( cat )
{
	m_name = name.toStdString().c_str();

	m_items = *new ObservableCollection<ItemItem>();

	for ( auto item : filter.items( cat, sid ) )
	{
		m_items->Add( MakePtr<ItemItem>( m_stockpileID, item, item, cat, sid, filter, proxy ) );
	}

	UpdateState();
}

const char* IngnomiaGUI::GroupItem::GetName() const
{
	return m_name.Str();
}

const Noesis::Nullable<bool>& IngnomiaGUI::GroupItem::GetState() const
{
	return m_state;
}

bool IngnomiaGUI::GroupItem::getExpanded() const
{
	return !GetState().HasValue();
}

void IngnomiaGUI::GroupItem::SetState( const Noesis::Nullable<bool>& value )
{
	bool active = value.HasValue() && value.GetValue();
	m_proxy->setActive( m_stockpileID, active, m_category, m_sid );
}

void IngnomiaGUI::GroupItem::UpdateChecked( Filter& filter )
{
	auto items = filter.items( m_category, m_sid );

	for ( int i = 0; i < m_items->Count(); ++i )
	{
		m_items->Get( i )->UpdateChecked( filter );
	}

	UpdateState();
}
void IngnomiaGUI::GroupItem::UpdateState()
{
	bool allChecked   = true;
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
#pragma endregion GroupItem

#pragma region CategoryItem
////////////////////////////////////////////////////////////////////////////////////////////////////

IngnomiaGUI::CategoryItem::CategoryItem( unsigned int stockpileID, QString name, QString sid, Filter& filter, StockpileProxy* proxy ) :
	m_stockpileID( stockpileID ),
	m_proxy( proxy ),
	m_sid( sid )
{
	m_name = name.toStdString().c_str();

	m_groups = *new ObservableCollection<GroupItem>();

	for ( auto group : filter.groups( sid ) )
	{
		m_groups->Add( MakePtr<GroupItem>( m_stockpileID, group, group, sid, filter, proxy ) );
	}

	UpdateState();
}

const char* IngnomiaGUI::CategoryItem::GetName() const
{
	return m_name.Str();
}

const Noesis::Nullable<bool>& IngnomiaGUI::CategoryItem::GetState() const
{
	return m_state;
}

bool IngnomiaGUI::CategoryItem::getExpanded() const
{
	return !GetState().HasValue();
}

void IngnomiaGUI::CategoryItem::SetState( const Noesis::Nullable<bool>& value )
{
	bool active = value.HasValue() && value.GetValue();
	m_proxy->setActive( m_stockpileID, active, m_sid );
}

void IngnomiaGUI::CategoryItem::UpdateChecked( Filter& filter )
{
	for ( int i = 0; i < m_groups->Count(); ++i )
	{
		m_groups->Get( i )->UpdateChecked( filter );
	}

	UpdateState();
}
void IngnomiaGUI::CategoryItem::UpdateState()
{
	bool allChecked   = true;
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
#pragma endregion CategoryItem

////////////////////////////////////////////////////////////////////////////////////////////////////
SpPriority::SpPriority( const char* name ) :
	_name( name )
{
}

const char* SpPriority::GetName() const
{
	return _name.Str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
StockpileModel::StockpileModel()
{
	m_proxy = new StockpileProxy;
	m_proxy->setParent( this );

	m_prios = *new ObservableCollection<SpPriority>();
	for ( int i = 0; i < 2; ++i )
	{
		m_prios->Add( MakePtr<SpPriority>( ( QString( "Priority " ) + QString::number( i + 1 ) ).toStdString().c_str() ) );
	}
	m_selectedPrio = m_prios->Get( 0 );

	m_categories = *new ObservableCollection<CategoryItem>();

	m_summary = *new ObservableCollection<StockedItemSummary>();

	m_limitItems = *new ObservableCollection<GuiLimitItem>();
}

void StockpileModel::onUpdateInfo( const GuiStockpileInfo& info )
{
	m_blockSignals = true;

	bool isSameStockpile = ( m_stockpileID == info.stockpileID );
	m_stockpileID        = info.stockpileID;
	m_name               = info.name.toStdString().c_str();
	m_suspended          = info.suspended;
	m_pullFrom           = info.allowPullFromHere;
	m_pullOthers         = info.pullFromOthers;

	m_prios->Clear();
	for ( int i = 0; i < info.maxPriority; ++i )
	{
		m_prios->Add( MakePtr<SpPriority>( ( QString( "Priority " ) + QString::number( i + 1 ) ).toStdString().c_str() ) );
	}
	m_selectedPrio = m_prios->Get( qMin( qMax( 0, info.priority ), m_prios->Count() ) );

	//auto cats = info.filter.categories();
	auto filter = info.filter;
	if ( isSameStockpile )
	{
		for ( int i = 0; i < m_categories->Count(); ++i )
		{
			m_categories->Get( i )->UpdateChecked( filter );
		}
	}
	else
	{
		m_categories->Clear();
		for ( auto cat : filter.categories() )
		{
			m_categories->Add( MakePtr<CategoryItem>( m_stockpileID, cat, cat, filter, m_proxy ) );
		}
	}

	m_summary->Clear();
	m_limitItems->Clear();
	for ( auto is : info.summary )
	{
		if ( is.count > 0 )
		{
			m_summary->Add( MakePtr<StockedItemSummary>( QString::number( is.count ) + " " + is.materialName + " " + is.itemName ) );
		}
		m_limitItems->Add( MakePtr<GuiLimitItem>( is.materialName + " " + is.itemName ) );
	}

	OnPropertyChanged( "Priorities" );
	OnPropertyChanged( "SelectedPrio" );

	OnPropertyChanged( "Name" );
	OnPropertyChanged( "Suspended" );
	OnPropertyChanged( "PullFromHere" );
	OnPropertyChanged( "PullFromOther" );

	OnPropertyChanged( "Summary" );
	OnPropertyChanged( "Limits" );

	m_blockSignals = false;
}

void StockpileModel::setBasicOptions()
{
	if( !m_blockSignals )
	{
		m_proxy->setBasicOptions( m_stockpileID, m_name.Str(), m_prios->IndexOf( m_selectedPrio ), m_suspended, m_pullOthers, m_pullFrom );
	}
}

void StockpileModel::onUpdateContent( const GuiStockpileInfo& info )
{
	bool isSameStockpile = ( m_stockpileID == info.stockpileID );
	if ( isSameStockpile )
	{
		m_summary->Clear();

		for ( auto is : info.summary )
		{
			if ( is.count > 0 )
			{
				m_summary->Add( MakePtr<StockedItemSummary>( QString::number( is.count ) + " " + is.materialName + " " + is.itemName ) );
			}
		}
		OnPropertyChanged( "Summary" );
	}
}

const char* StockpileModel::GetName() const
{
	return m_name.Str();
}

void StockpileModel::SetName( const char* value )
{
	m_name = value;
	setBasicOptions();
	OnPropertyChanged( "Name" );
}

bool StockpileModel::GetSuspended() const
{
	return m_suspended;
}

void StockpileModel::SetSuspended( bool value )
{
	m_suspended = value;
	setBasicOptions();
	OnPropertyChanged( "Suspended" );
}

bool StockpileModel::GetPullFrom() const
{
	return m_pullFrom;
}

void StockpileModel::SetPullFrom( bool value )
{
	m_pullFrom = value;
	setBasicOptions();
	OnPropertyChanged( "PullFromHere" );
}

bool StockpileModel::GetPullOthers() const
{
	return m_pullOthers;
}

void StockpileModel::SetPullOthers( bool value )
{
	m_pullOthers = value;
	setBasicOptions();
	OnPropertyChanged( "PullFromOther" );
}

Noesis::ObservableCollection<SpPriority>* StockpileModel::GetPrios() const
{
	return m_prios;
}

Noesis::ObservableCollection<StockedItemSummary>* StockpileModel::GetSummary() const
{
	return m_summary;
}

Noesis::ObservableCollection<GuiLimitItem>* StockpileModel::GetLimitItems() const
{
	return m_limitItems;
}

void StockpileModel::SetSelectedPriority( SpPriority* prio )
{
	if ( m_selectedPrio != prio )
	{
		m_selectedPrio = prio;
		setBasicOptions();
		OnPropertyChanged( "SelectedPrio" );
	}
}

SpPriority* StockpileModel::GetSelectedPriority() const
{
	return m_selectedPrio;
}

Noesis::ObservableCollection<CategoryItem>* StockpileModel::GetCategories() const
{
	return m_categories;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION( StockpileModel, "IngnomiaGUI.StockpileModel" )
{
	NsProp( "Name", &StockpileModel::GetName, &StockpileModel::SetName );
	NsProp( "Suspended", &StockpileModel::GetSuspended, &StockpileModel::SetSuspended );
	NsProp( "PullFromHere", &StockpileModel::GetPullFrom, &StockpileModel::SetPullFrom );
	NsProp( "PullFromOther", &StockpileModel::GetPullOthers, &StockpileModel::SetPullOthers );

	NsProp( "Priorities", &StockpileModel::GetPrios );
	NsProp( "SelectedPrio", &StockpileModel::GetSelectedPriority, &StockpileModel::SetSelectedPriority );

	NsProp( "Filters", &StockpileModel::GetCategories );
	NsProp( "StockedItems", &StockpileModel::GetSummary );
	NsProp( "Limits", &StockpileModel::GetLimitItems );
	//NsProp( "", &StockpileModel::Get );
}

NS_IMPLEMENT_REFLECTION( SpPriority )
{
	NsProp( "Name", &SpPriority::GetName );
}

NS_IMPLEMENT_REFLECTION( IngnomiaGUI::CategoryItem )
{
	NsProp( "Name", &CategoryItem::GetName );
	NsProp( "Checked", &CategoryItem::GetState, &CategoryItem::SetState );
	NsProp( "Expanded", &CategoryItem::getExpanded );
	NsProp( "Children", &CategoryItem::GetGroups );
}

NS_IMPLEMENT_REFLECTION( IngnomiaGUI::GroupItem )
{
	NsProp( "Name", &GroupItem::GetName );
	NsProp( "Checked", &GroupItem::GetState, &GroupItem::SetState );
	NsProp( "Expanded", &GroupItem::getExpanded );
	NsProp( "Children", &GroupItem::GetItems );
}

NS_IMPLEMENT_REFLECTION( IngnomiaGUI::ItemItem )
{
	NsProp( "Name", &ItemItem::GetName );
	NsProp( "Checked", &ItemItem::GetState, &ItemItem::SetState );
	NsProp( "Expanded", &ItemItem::getExpanded );
	NsProp( "Children", &ItemItem::GetMaterials );
}

NS_IMPLEMENT_REFLECTION( IngnomiaGUI::MaterialItem )
{
	NsProp( "Name", &MaterialItem::GetName );
	NsProp( "Checked", &MaterialItem::GetChecked, &MaterialItem::SetChecked );
}

NS_IMPLEMENT_REFLECTION( IngnomiaGUI::StockedItemSummary )
{
	NsProp( "Name", &StockedItemSummary::GetName );
}

NS_IMPLEMENT_REFLECTION( IngnomiaGUI::GuiLimitItem )
{
	NsProp( "Name", &GuiLimitItem::GetName );
}