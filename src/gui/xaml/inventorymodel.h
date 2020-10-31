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

////////////////////////////////////////////////////////////////////////////////////////////////////
class CategoryButton final : public Noesis::BaseComponent
{
public:
	CategoryButton( QString name, QString sid, QString image );

	const char* GetName() const;
	const char* GetID() const;
	const char* GetImage() const;

private:
	Noesis::String m_name;
	Noesis::String m_sid;
	Noesis::String m_image;

	NS_DECLARE_REFLECTION( CategoryButton, Noesis::BaseComponent )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class InvItemEntry final : public Noesis::BaseComponent
{
public:
	InvItemEntry( QString item, QString material, QString inStockpile, QString total );

	const char* GetItem() const;
	const char* GetMaterial() const;
	const char* GetInStockpile() const;
	const char* GetTotal() const;

private:
	Noesis::String m_item;
	Noesis::String m_material;
	Noesis::String m_inStockpile;
	Noesis::String m_total;
	

	NS_DECLARE_REFLECTION( InvItemEntry, Noesis::BaseComponent )
};


////////////////////////////////////////////////////////////////////////////////////////////////////
class InventoryModel final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	InventoryModel();

	void updateCategories( const QList<GuiInventoryCategory>& categories );
	void updateGroups( const QList<GuiInventoryGroup>& groups );
	void updateItems( const QList<GuiInventoryItem>& items );

private:
	InventoryProxy* m_proxy = nullptr;

	NoesisApp::DelegateCommand m_categoryCmd;
	void onCategoryCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* getCategoryCmd() const
	{
		return &m_categoryCmd;
	}
	NoesisApp::DelegateCommand m_groupCmd;
	void onGroupCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* getGroupCmd() const
	{
		return &m_groupCmd;
	}
	

	Noesis::Ptr<Noesis::ObservableCollection<CategoryButton>> m_categoryButtons;
	Noesis::ObservableCollection<CategoryButton>* getCategoryButtons() const
	{
		return m_categoryButtons;
	}

	Noesis::Ptr<Noesis::ObservableCollection<CategoryButton>> m_groupButtons;
	Noesis::ObservableCollection<CategoryButton>* getGroupButtons() const
	{
		return m_groupButtons;
	}

	Noesis::Ptr<Noesis::ObservableCollection<InvItemEntry>> m_items;
	Noesis::ObservableCollection<InvItemEntry>* getItems() const
	{
		return m_items;
	}

	QString m_category;


	NS_DECLARE_REFLECTION( InventoryModel, NotifyPropertyChangedBase )
};

} // namespace IngnomiaGUI
