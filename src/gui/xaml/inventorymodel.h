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

#include "../aggregatorinventory.h"

#include <QString>

#include <NsApp/DelegateCommand.h>
#include <NsApp/NotifyPropertyChangedBase.h>
#include <NsCore/Noesis.h>
#include <NsCore/Nullable.h>
#include <NsCore/Ptr.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/ReflectionDeclareEnum.h>
#include <NsCore/String.h>
#include <NsGui/Collection.h>

class InventoryProxy;

namespace Noesis
{
template <class T>
class ObservableCollection;
}

namespace IngnomiaGUI
{

#pragma region FilterItems
////////////////////////////////////////////////////////////////////////////////////////////////////
class InvMaterialItem final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	InvMaterialItem( const GuiInventoryMaterial& mat, InventoryProxy* proxy );

	const char* GetName() const;

	bool GetChecked() const;
	void SetChecked( bool value );

	const char* getTotal() const;
	const char* getInStock() const;

private:
	Noesis::String m_name;
	Noesis::String m_inStock;
	Noesis::String m_total;
	QString m_sid;
	QString m_item;
	QString m_group;
	QString m_category;

	bool m_active = false;

	InventoryProxy* m_proxy = nullptr;

	NS_DECLARE_REFLECTION( InvMaterialItem, NoesisApp::NotifyPropertyChangedBase )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class InvItemItem final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	InvItemItem( const GuiInventoryItem& gii, InventoryProxy* proxy );

	const char* GetName() const;
	const Noesis::Nullable<bool>& GetState() const;
	void SetState( const Noesis::Nullable<bool>& value );
	bool getExpanded() const;

	const char* getTotal() const;
	const char* getInStock() const;

private:
	void UpdateState();
	Noesis::String m_name;
	Noesis::String m_inStock;
	Noesis::String m_total;
	QString m_sid;
	QString m_group;
	QString m_category;

	unsigned char m_active = 0;
	Noesis::Nullable<bool> m_state;

	InventoryProxy* m_proxy = nullptr;

	Noesis::ObservableCollection<InvMaterialItem>* GetMaterials() const
	{
		return m_materials;
	}
	Noesis::Ptr<Noesis::ObservableCollection<InvMaterialItem>> m_materials;

	NS_DECLARE_REFLECTION( InvItemItem, NoesisApp::NotifyPropertyChangedBase )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class InvGroupItem final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	InvGroupItem( const GuiInventoryGroup& gig, InventoryProxy* proxy );

	const char* GetName() const;

	const Noesis::Nullable<bool>& GetState() const;
	void SetState( const Noesis::Nullable<bool>& value );
	bool getExpanded() const;

	const char* getTotal() const;
	const char* getInStock() const;

private:
	void UpdateState();
	Noesis::String m_name;
	Noesis::String m_inStock;
	Noesis::String m_total;
	unsigned int m_stockpileID = 0;
	QString m_sid;
	QString m_category;

	unsigned char m_active = 0;
	Noesis::Nullable<bool> m_state;

	InventoryProxy* m_proxy = nullptr;

	Noesis::ObservableCollection<InvItemItem>* GetItems() const
	{
		return m_items;
	}
	Noesis::Ptr<Noesis::ObservableCollection<InvItemItem>> m_items;

	NS_DECLARE_REFLECTION( InvGroupItem, NoesisApp::NotifyPropertyChangedBase )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class InvCategoryItem final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	InvCategoryItem( const GuiInventoryCategory& gic, InventoryProxy* proxy );

	const char* GetName() const;

	const Noesis::Nullable<bool>& GetState() const;
	void SetState( const Noesis::Nullable<bool>& value );
	bool getExpanded() const;

	const char* getTotal() const;
	const char* getInStock() const;

private:
	void UpdateState();
	Noesis::String m_name;
	Noesis::String m_inStock;
	Noesis::String m_total;
	QString m_sid;

	unsigned char m_active = 0;
	Noesis::Nullable<bool> m_state;

	InventoryProxy* m_proxy = nullptr;

	Noesis::ObservableCollection<InvGroupItem>* GetGroups() const
	{
		return m_groups;
	}
	Noesis::Ptr<Noesis::ObservableCollection<InvGroupItem>> m_groups;

	NS_DECLARE_REFLECTION( InvCategoryItem, NoesisApp::NotifyPropertyChangedBase )
};
#pragma endregion FilterItems



////////////////////////////////////////////////////////////////////////////////////////////////////
class InventoryModel final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	InventoryModel();

	void updateCategories( const QList<GuiInventoryCategory>& categories );

private:
	InventoryProxy* m_proxy = nullptr;

	Noesis::ObservableCollection<InvCategoryItem>* GetCategories() const;
	Noesis::Ptr<Noesis::ObservableCollection<InvCategoryItem>> m_categories;


	NS_DECLARE_REFLECTION( InventoryModel, NotifyPropertyChangedBase )
};

} // namespace IngnomiaGUI
