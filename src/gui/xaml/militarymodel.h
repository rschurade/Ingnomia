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

#include "../aggregatormilitary.h"

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

class MilitaryProxy;

namespace Noesis
{
template <class T>
class ObservableCollection;
}

namespace IngnomiaGUI
{

enum class MilitaryPage
{
	First,
	Second,
	Third
};



////////////////////////////////////////////////////////////////////////////////////////////////////
class SquadPriority final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	SquadPriority( const GuiTargetPriority& prio, unsigned int squadID, MilitaryProxy* proxy );

	const char* getIDString() const { return m_idString.Str(); }
	const char* getName() const { return m_name.Str(); }

	QString getID() { return m_idString.Str(); }

	bool getIgnore() const { return m_attitude == MilAttitude::_IGNORE; }
	void setIgnore( bool value );
	bool getAvoid() const { return m_attitude == MilAttitude::AVOID; }
	void setAvoid( bool value );
	bool getAttack() const { return m_attitude == MilAttitude::ATTACK; }
	void setAttack( bool value );
	bool getHunt() const { return m_attitude == MilAttitude::HUNT; }
	void setHunt( bool value );

private:
	Noesis::String m_idString;
	Noesis::String m_name;
	MilAttitude m_attitude = MilAttitude::_IGNORE;

	unsigned int m_squadID = 0;
	MilitaryProxy* m_proxy = nullptr;

	void onMoveUpCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* getMoveUpCmd() const
	{
		return &m_moveUpCmd;
	}
	NoesisApp::DelegateCommand m_moveUpCmd;

	void onMoveDownCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* getMoveDownCmd() const
	{
		return &m_moveDownCmd;
	}
	NoesisApp::DelegateCommand m_moveDownCmd;
	
	NS_DECLARE_REFLECTION( SquadPriority, NoesisApp::NotifyPropertyChangedBase )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class RoleItem;
class SquadGnome final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	SquadGnome( const GuiSquadGnome& gnome, bool showLeft, bool showRight, bool showX, MilitaryProxy* proxy );

	const char* getIDString() const { return m_idString.Str(); }
	const char* getName() const { return m_name.Str(); }

	unsigned int getID() { return m_id; }

	const char* getShowLeftArrow() const { return m_showLeftArrow ? "Visible" : "Hidden"; }
	const char* getShowRightArrow() const { return m_showRightArrow ? "Visible" : "Hidden"; }
	const char* getShowX() const { return m_showX ? "Visible" : "Hidden"; }

	RoleItem* getRole() const;
	void setRole( RoleItem* role );
	void nullifyRole() { m_role = nullptr; }
	unsigned int roleID() { return m_roleID; }
private:
	unsigned int m_id = 0;
	Noesis::String m_idString;
	Noesis::String m_name;
	MilitaryProxy* m_proxy = nullptr;

	bool m_showLeftArrow = true;
	bool m_showRightArrow = true;
	bool m_showX = true;

	unsigned int m_roleID = 0;
	RoleItem* m_role = nullptr;
	
	NS_DECLARE_REFLECTION( SquadGnome, NoesisApp::NotifyPropertyChangedBase )
};


////////////////////////////////////////////////////////////////////////////////////////////////////
class SquadItem final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	SquadItem( const GuiSquad& squad, MilitaryProxy* proxy );

	const char* getIDString() const { return m_idString.Str(); }
	const char* getName() const { return m_name.Str(); }
	void setName( const char* value );

	unsigned int getID() { return m_id; }

	void updatePriorities( const QList<GuiTargetPriority>& prios );

	Noesis::ObservableCollection<SquadGnome>* getGnomes() const
	{
		return m_gnomes;
	}

private:
	unsigned int m_id = 0;
	Noesis::String m_idString;
	Noesis::String m_name;
	MilitaryProxy* m_proxy = nullptr;

	bool m_showLeftArrow = true;
	bool m_showRightArrow = true;

	bool m_showConfig = false;

	const char* getShowLeftArrow() const { return m_showLeftArrow ? "Visible" : "Hidden"; }
	const char* getShowRightArrow() const { return m_showRightArrow ? "Visible" : "Hidden"; }
	const char* getShowX() const { return ( m_id != 0 ) ? "Visible" : "Hidden"; }
	const char* getShowConfig() const { return m_showConfig ? "Visible" : "Collapsed"; }

	
	Noesis::Ptr<Noesis::ObservableCollection<SquadGnome>> m_gnomes;

	void onShowConfigCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* getShowConfigCmd() const
	{
		return &m_showConfigCmd;
	}
	NoesisApp::DelegateCommand m_showConfigCmd;

	Noesis::ObservableCollection<SquadPriority>* getPriorities() const
	{
		return m_priorities;
	}
	Noesis::Ptr<Noesis::ObservableCollection<SquadPriority>> m_priorities;

	NS_DECLARE_REFLECTION( SquadItem, NoesisApp::NotifyPropertyChangedBase )
};


////////////////////////////////////////////////////////////////////////////////////////////////////
class UniformModelType final : public Noesis::BaseComponent
{
public:
	UniformModelType( QString sid, QString name );

	const char* getName() const { return m_name.Str(); }
	const char* sid() const { return m_sid.Str(); }

private:
	Noesis::String m_name;
	Noesis::String m_sid;

	NS_DECLARE_REFLECTION( UniformModelType, Noesis::BaseComponent )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class UniformModelMaterial final : public Noesis::BaseComponent
{
public:
	UniformModelMaterial( QString sid, QString name );

	const char* getName() const { return m_name.Str(); }
	const char* sid() const { return m_sid.Str(); }

private:
	Noesis::String m_name;
	Noesis::String m_sid;

	NS_DECLARE_REFLECTION( UniformModelMaterial, Noesis::BaseComponent )
};


////////////////////////////////////////////////////////////////////////////////////////////////////
class UniformModelItem final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	UniformModelItem( const GuiUniformItem& gui, unsigned int roleID, MilitaryProxy* proxy );

	const char* getName() const { return m_name.Str(); }
	const char* sid() const { return m_sid.Str(); }

	void setPossibleMats( QStringList mats );

private:
	Noesis::String m_name;
	Noesis::String m_sid;
	MilitaryProxy* m_proxy = nullptr;
	unsigned int m_roleID = 0;

	QString m_mat;
	
	Noesis::ObservableCollection<UniformModelType>* availableTypes() const { return m_availableTypes; }
	Noesis::ObservableCollection<UniformModelMaterial>* availableMaterials() const { return m_availableMats; }

	void setSelectedType( UniformModelType* type );
	UniformModelType* getSelectedType() const { return m_selectedType; };
	void setSelectedMaterial( UniformModelMaterial* mat );
	UniformModelMaterial* getSelectedMaterial() const { return m_selectedMat; };

	UniformModelType* m_selectedType = nullptr;
	UniformModelMaterial* m_selectedMat = nullptr;

	Noesis::Ptr<Noesis::ObservableCollection<UniformModelType>> m_availableTypes;
	Noesis::Ptr<Noesis::ObservableCollection<UniformModelMaterial>> m_availableMats;


	NS_DECLARE_REFLECTION( UniformModelItem,  NoesisApp::NotifyPropertyChangedBase )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class RoleItem final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	RoleItem( const GuiMilRole& role, MilitaryProxy* proxy );

	const char* getIDString() const { return m_idString.Str(); }
	const char* getName() const { return m_name.Str(); }
	void setName( const char* value );

	unsigned int getID() { return m_id; }
	void updatePossibleMaterials( QString slot, QStringList mats );

private:
	unsigned int m_id = 0;
	Noesis::String m_idString;
	Noesis::String m_name;
	MilitaryProxy* m_proxy = nullptr;

	bool m_showLeftArrow = false;
	bool m_showRightArrow = false;

	bool m_showConfig = false;

	const char* getShowLeftArrow() const { return m_showLeftArrow ? "Visible" : "Hidden"; }
	const char* getShowRightArrow() const { return m_showRightArrow ? "Visible" : "Hidden"; }
	const char* getShowX() const { return ( m_id != 0 ) ? "Visible" : "Hidden"; }
	const char* getShowConfig() const { return m_showConfig ? "Visible" : "Collapsed"; }

	
	void onShowConfigCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* getShowConfigCmd() const
	{
		return &m_showConfigCmd;
	}
	NoesisApp::DelegateCommand m_showConfigCmd;

	Noesis::ObservableCollection<UniformModelItem>* getUniformItems() const
	{
		return m_uniformItems;
	}
	Noesis::Ptr<Noesis::ObservableCollection<UniformModelItem>> m_uniformItems;


	NS_DECLARE_REFLECTION( RoleItem, NoesisApp::NotifyPropertyChangedBase )
};










////////////////////////////////////////////////////////////////////////////////////////////////////
class MilitaryModel final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	MilitaryModel();

	void updateSquads( const QList<GuiSquad>& squads );
	void updatePriorities( unsigned int squadID, const QList<GuiTargetPriority>& priorities );
	void updateRoles( const QList<GuiMilRole>& roles );
	void updatePossibleMaterials( unsigned int roleID, const QString slot, const QStringList mats );

private:
	MilitaryProxy* m_proxy = nullptr;

	MilitaryPage m_page = MilitaryPage::First;

	const char* getShowFirst() const;
	const char* getShowSecond() const;
	const char* getShowThird() const;

	void onPageCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* getPageCmd() const
	{
		return &m_pageCmd;
	}
	NoesisApp::DelegateCommand m_pageCmd;

	Noesis::ObservableCollection<SquadItem>* getSquads() const
	{
		return m_squads;
	}
	Noesis::Ptr<Noesis::ObservableCollection<SquadItem>> m_squads;
	
	void onAddSquadCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* getAddSquadCmd() const
	{
		return &m_addSquadCmd;
	}
	NoesisApp::DelegateCommand m_addSquadCmd;

	void onRemoveSquadCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* getRemoveSquadCmd() const
	{
		return &m_removeSquadCmd;
	}
	NoesisApp::DelegateCommand m_removeSquadCmd;

	void onMoveSquadLeftCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* getMoveSquadLeftCmd() const
	{
		return &m_moveSquadLeftCmd;
	}
	NoesisApp::DelegateCommand m_moveSquadLeftCmd;

	void onMoveSquadRightCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* getMoveSquadRightCmd() const
	{
		return &m_moveSquadRightCmd;
	}
	NoesisApp::DelegateCommand m_moveSquadRightCmd;

	void onRemoveGnomeFromSquadCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* getRemoveGnomeFromSquadCmd() const
	{
		return &m_removeGnomeFromSquadCmd;
	}
	NoesisApp::DelegateCommand m_removeGnomeFromSquadCmd;

	void onMoveGnomeLeftCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* getMoveGnomeLeftCmd() const
	{
		return &m_moveGnomeLeftCmd;
	}
	NoesisApp::DelegateCommand m_moveGnomeLeftCmd;

	void onMoveGnomeRightCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* getMoveGnomeRightCmd() const
	{
		return &m_moveGnomeRightCmd;
	}
	NoesisApp::DelegateCommand m_moveGnomeRightCmd;


	Noesis::ObservableCollection<RoleItem>* getRoles() const
	{
		return m_roles;
	}
	Noesis::Ptr<Noesis::ObservableCollection<RoleItem>> m_roles;


	void onAddRoleCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* getAddRoleCmd() const
	{
		return &m_addRoleCmd;
	}
	NoesisApp::DelegateCommand m_addRoleCmd;

	void onRemoveRoleCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* getRemoveRoleCmd() const
	{
		return &m_removeRoleCmd;
	}
	NoesisApp::DelegateCommand m_removeRoleCmd;




	NS_DECLARE_REFLECTION( MilitaryModel, NotifyPropertyChangedBase )
};

} // namespace IngnomiaGUI
