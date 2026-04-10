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
/** @file StockpileModel.h
 *  @brief View model and helper-component types for the Stockpile window. Exposes the
 *         four-level category/group/item/material filter tree, pull/pull-others flags,
 *         priority, limit list, and stocked-items summary.
 */
#ifndef __StockpileModel_H__
#define __StockpileModel_H__

#include "../aggregatorstockpile.h"

#include <NsApp/DelegateCommand.h>
#include <NsApp/NotifyPropertyChangedBase.h>
#include <NsCore/Noesis.h>
#include <NsCore/Nullable.h>
#include <NsCore/Ptr.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/ReflectionDeclareEnum.h>
#include <NsCore/String.h>
#include <NsGui/Collection.h>

#include <QString>

class StockpileProxy;

namespace Noesis
{
template <class T>
class ObservableCollection;
}

namespace IngnomiaGUI
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief One row in the stockpile's per-item limit list (soft cap on how many to keep).
class GuiLimitItem final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	GuiLimitItem( QString name );

	const char* GetName() const;

private:
	Noesis::String m_name;

	NS_DECLARE_REFLECTION( GuiLimitItem, NoesisApp::NotifyPropertyChangedBase )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief One row in the "currently stocked" summary list (item name + count).
class StockedItemSummary final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	StockedItemSummary( QString name );

	const char* GetName() const;

private:
	Noesis::String m_name;

	NS_DECLARE_REFLECTION( StockedItemSummary, NoesisApp::NotifyPropertyChangedBase )
};

#pragma region FilterItems
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Leaf node of the filter tree: one material of one item of one group of one
///        category. Toggling its checkbox updates the stockpile's filter via the proxy.
class MaterialItem final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	MaterialItem( unsigned int stockpileID, QString name, QString sid, QString cat, QString group, QString item, Filter& filter, StockpileProxy* proxy );

	const char* GetName() const;

	bool GetChecked() const;
	void SetChecked( bool value );
	void UpdateChecked( Filter& filter );

private:
	Noesis::String m_name;
	unsigned int m_stockpileID = 0;
	QString m_sid;
	QString m_item;
	QString m_group;
	QString m_category;

	bool m_active = false;

	StockpileProxy* m_proxy = nullptr;

	NS_DECLARE_REFLECTION( MaterialItem, NoesisApp::NotifyPropertyChangedBase )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Third level of the filter tree: one item (e.g. "Sword") holding a list of
///        MaterialItem children. Tri-state checkbox reflects its children.
class ItemItem final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	ItemItem( unsigned int stockpileID, QString name, QString sid, QString cat, QString group, Filter& filter, StockpileProxy* proxy );

	const char* GetName() const;
	const Noesis::Nullable<bool>& GetState() const;
	void SetState( const Noesis::Nullable<bool>& value );
	bool getExpanded() const;
	void UpdateChecked( Filter& filter );

private:
	void UpdateState();
	Noesis::String m_name;
	unsigned int m_stockpileID = 0;
	QString m_sid;
	QString m_group;
	QString m_category;

	unsigned char m_active = 0;
	Noesis::Nullable<bool> m_state;

	StockpileProxy* m_proxy = nullptr;

	Noesis::ObservableCollection<MaterialItem>* GetMaterials() const
	{
		return m_materials;
	}
	Noesis::Ptr<Noesis::ObservableCollection<MaterialItem>> m_materials;

	NS_DECLARE_REFLECTION( ItemItem, NoesisApp::NotifyPropertyChangedBase )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Second level of the filter tree: one group (e.g. "Weapons") holding a list of
///        ItemItem children. Tri-state checkbox reflects its children.
class GroupItem final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	GroupItem( unsigned int stockpileID, QString name, QString sid, QString cat, Filter& filter, StockpileProxy* proxy );

	const char* GetName() const;

	const Noesis::Nullable<bool>& GetState() const;
	void SetState( const Noesis::Nullable<bool>& value );
	bool getExpanded() const;
	void UpdateChecked( Filter& filter );

private:
	void UpdateState();
	Noesis::String m_name;
	unsigned int m_stockpileID = 0;
	QString m_sid;
	QString m_category;

	unsigned char m_active = 0;
	Noesis::Nullable<bool> m_state;

	StockpileProxy* m_proxy = nullptr;

	Noesis::ObservableCollection<ItemItem>* GetItems() const
	{
		return m_items;
	}
	Noesis::Ptr<Noesis::ObservableCollection<ItemItem>> m_items;

	NS_DECLARE_REFLECTION( GroupItem, NoesisApp::NotifyPropertyChangedBase )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Top level of the filter tree: one category (e.g. "Tools") holding a list of
///        GroupItem children. Tri-state checkbox reflects its children.
class CategoryItem final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	CategoryItem( unsigned int stockpileID, QString name, QString sid, Filter& filter, StockpileProxy* proxy );

	const char* GetName() const;

	const Noesis::Nullable<bool>& GetState() const;
	void SetState( const Noesis::Nullable<bool>& value );
	bool getExpanded() const;

	void UpdateChecked( Filter& filter );

private:
	void UpdateState();
	Noesis::String m_name;
	unsigned int m_stockpileID = 0;
	QString m_sid;

	unsigned char m_active = 0;
	Noesis::Nullable<bool> m_state;

	StockpileProxy* m_proxy = nullptr;

	Noesis::ObservableCollection<GroupItem>* GetGroups() const
	{
		return m_groups;
	}
	Noesis::Ptr<Noesis::ObservableCollection<GroupItem>> m_groups;

	NS_DECLARE_REFLECTION( CategoryItem, NoesisApp::NotifyPropertyChangedBase )
};
#pragma endregion FilterItems

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief One entry in the stockpile priority dropdown (e.g. "High", "Normal", "Low").
class SpPriority final : public Noesis::BaseComponent
{
public:
	SpPriority( const char* name );

	const char* GetName() const;

private:
	Noesis::String _name;

	NS_DECLARE_REFLECTION( SpPriority, Noesis::BaseComponent )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Stockpile window view model. Exposes the filter tree, priority, limit list,
///        stocked-items summary, and basic flags (name, suspended, pull from, pull others).
///        Talks to the game side via StockpileProxy.
class StockpileModel final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	StockpileModel();

	/// @brief Replaces the top-level info (name, flags, priority, filter tree) for a new stockpile.
	void onUpdateInfo( const GuiStockpileInfo& stockpileInfo );
	/// @brief Refreshes the content lists (summary + limits) without rebuilding the filter tree.
	void onUpdateContent( const GuiStockpileInfo& stockpileInfo );

private:
	const char* GetName() const;
	void SetName( const char* value );
	bool GetSuspended() const;
	void SetSuspended( bool value );
	bool GetPullFrom() const;
	void SetPullFrom( bool value );
	bool GetPullOthers() const;
	void SetPullOthers( bool value );

	void setBasicOptions();

	Noesis::ObservableCollection<SpPriority>* GetPrios() const;
	void SetSelectedPriority( SpPriority* prio );
	SpPriority* GetSelectedPriority() const;
	Noesis::Ptr<Noesis::ObservableCollection<SpPriority>> m_prios;
	SpPriority* m_selectedPrio;

	Noesis::ObservableCollection<CategoryItem>* GetCategories() const;
	Noesis::Ptr<Noesis::ObservableCollection<CategoryItem>> m_categories;

	Noesis::ObservableCollection<StockedItemSummary>* GetSummary() const;
	Noesis::Ptr<Noesis::ObservableCollection<StockedItemSummary>> m_summary;

	Noesis::ObservableCollection<GuiLimitItem>* GetLimitItems() const;
	Noesis::Ptr<Noesis::ObservableCollection<GuiLimitItem>> m_limitItems;

	StockpileProxy* m_proxy = nullptr; ///< Qt-side proxy for signal/slot bridge to the game.
	bool m_blockSignals = false;       ///< Suppresses setters while rebuilding the tree.

	unsigned int m_stockpileID = 0;             ///< Backing stockpile ID in the game world.
	Noesis::String m_name      = "-Stockpile-"; ///< Display name.
	bool m_suspended           = false;         ///< Paused intake / output flag.
	bool m_pullFrom            = true;          ///< Allow haulers to pull from other stockpiles.
	bool m_pullOthers          = false;         ///< Allow other stockpiles to pull from this one.

	NS_DECLARE_REFLECTION( StockpileModel, NotifyPropertyChangedBase )
};

} // namespace IngnomiaGUI

#endif
