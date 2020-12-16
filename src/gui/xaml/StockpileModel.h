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
class StockpileModel final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	StockpileModel();

	void onUpdateInfo( const GuiStockpileInfo& stockpileInfo );
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

	StockpileProxy* m_proxy = nullptr;
	bool m_blockSignals = false;

	unsigned int m_stockpileID = 0;
	Noesis::String m_name      = "-Stockpile-";
	bool m_suspended           = false;
	bool m_pullFrom            = true;
	bool m_pullOthers          = false;

	NS_DECLARE_REFLECTION( StockpileModel, NotifyPropertyChangedBase )
};

} // namespace IngnomiaGUI

#endif
