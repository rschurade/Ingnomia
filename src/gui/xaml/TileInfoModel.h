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
#ifndef __TileInfoModel_H__
#define __TileInfoModel_H__

#include "../../base/position.h"
#include "../aggregatortileinfo.h"

#include <NsApp/DelegateCommand.h>
#include <NsApp/NotifyPropertyChangedBase.h>
#include <NsCore/Noesis.h>
#include <NsCore/Ptr.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/ReflectionDeclareEnum.h>
#include <NsCore/String.h>
#include <NsGui/Collection.h>
#include <NsGui/UIElement.h>

#include <QString>

class ProxyTileInfo;

namespace Noesis
{
template <class T>
class ObservableCollection;
}

namespace IngnomiaGUI
{

////////////////////////////////////////////////////////////////////////////////////////////////////
class TabItem final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	TabItem( QString name, QString sid );

	const char* GetName() const;
	const char* GetID() const;
	bool GetChecked() const;
	void SetChecked( bool )
	{
	}

	void setActive( bool active );

private:
	Noesis::String _name;
	Noesis::String _sid;
	bool _active = false;

	NS_DECLARE_REFLECTION( TabItem, NoesisApp::NotifyPropertyChangedBase )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class TerrainTabItem final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	TerrainTabItem( QString name, QString sid, QString action1 = "", QString action2 = "" );

	const char* GetName() const;
	const char* GetID() const;
	const char* GetAction1() const
	{
		return _action1.Str();
	}
	const char* GetAction2() const
	{
		return _action2.Str();
	}
	Noesis::Visibility Action1Visible() const
	{
		return _hasAction1 ? Noesis::Visibility_Visible : Noesis::Visibility_Hidden;
	}
	Noesis::Visibility Action2Visible() const
	{
		return _hasAction2 ? Noesis::Visibility_Visible : Noesis::Visibility_Hidden;
	}

private:
	Noesis::String _name;
	Noesis::String _sid;
	Noesis::String _action1;
	Noesis::String _action2;
	bool _hasAction1;
	bool _hasAction2;

	NS_DECLARE_REFLECTION( TerrainTabItem, NoesisApp::NotifyPropertyChangedBase )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class ItemTabItem final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	ItemTabItem( QString name, unsigned int id );

	const char* GetName() const;
	const unsigned int GetID() const;

private:
	Noesis::String _name;
	unsigned int _id;

	NS_DECLARE_REFLECTION( ItemTabItem, NoesisApp::NotifyPropertyChangedBase )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class CreatureTabItem final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	CreatureTabItem( QString name, unsigned int id );

	const char* GetName() const;
	const char* GetID() const;

	unsigned int uid()
	{
		return m_id;
	}

private:
	Noesis::String m_name;
	unsigned int m_id;
	Noesis::String m_sid;

	NS_DECLARE_REFLECTION( CreatureTabItem, NoesisApp::NotifyPropertyChangedBase )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
enum class TileInfoMode
{
	Terrain,
	Items,
	Creatures,
	Designation,
	Job,
	Stockpile,
	Room

};

////////////////////////////////////////////////////////////////////////////////////////////////////
class TileInfoModel final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	TileInfoModel();

	void onUpdateTileInfo( const GuiTileInfo& tileInfo );
	void updateMiniStockpile( const GuiStockpileInfo& info );

private:
	const char* getTileID() const;

	Noesis::ObservableCollection<TabItem>* getTabItems() const;
	Noesis::ObservableCollection<TerrainTabItem>* getTerrainTabItems() const;
	Noesis::ObservableCollection<ItemTabItem>* getItemTabItems() const;
	Noesis::ObservableCollection<CreatureTabItem>* getCreatureTabItems() const;
	Noesis::ObservableCollection<TabItem>* getMiniSPContents() const;
	Noesis::ObservableCollection<CreatureTabItem>* getPossibleTennants() const;

	const char* GetShowTerrain() const;
	const char* GetShowItems() const;
	const char* GetShowCreatures() const;
	const char* GetShowDesignation() const;
	const char* GetShowDesignationRoom() const;
	const char* GetShowJob() const;
	const char* GetShowMiniSP() const;

	const char* GetJobName() const
	{
		return m_jobName.Str();
	}
	const char* GetJobWorker() const
	{
		return m_jobWorker.Str();
	}
	const char* GetJobPriority() const
	{
		return m_jobPriority.Str();
	}
	const char* GetRequiredTool() const
	{
		return m_requiredTool.Str();
	}
	const char* GetDesignationName() const
	{
		return m_designationName.Str();
	}

	const NoesisApp::DelegateCommand* GetCmdTerrain() const
	{
		return &_cmdTerrain;
	}
	void OnCmdTerrain( BaseComponent* param );
	NoesisApp::DelegateCommand _cmdTerrain;

	const NoesisApp::DelegateCommand* GetCmdManage() const
	{
		return &_cmdManage;
	}
	void OnCmdManage( BaseComponent* param );
	NoesisApp::DelegateCommand _cmdManage;

	const NoesisApp::DelegateCommand* GetCmdTab() const;
	void OnCmdTab( BaseComponent* param );
	NoesisApp::DelegateCommand _cmdTab;

	ProxyTileInfo* m_proxy = nullptr;

	unsigned int m_tileID = 0;
	Noesis::String m_tileIDString;

	Noesis::String m_jobName;
	Noesis::String m_jobWorker;
	Noesis::String m_jobPriority;
	Noesis::String m_requiredTool;

	unsigned int m_designationID = 0;
	Noesis::String m_designationName;
	TileFlag m_designationFlag = TileFlag::TF_NONE;
	RoomType m_roomType        = RoomType::NotSet;
	bool m_hasRoof             = false;
	bool m_isEnclosed          = false;
	bool m_hasAlarmBell        = false;
	Noesis::String m_beds;
	bool m_alarm = false;

	TileInfoMode _mode = TileInfoMode::Terrain;

	struct
	{
		Noesis::Ptr<TabItem> tt;
		Noesis::Ptr<TabItem> tc;
		Noesis::Ptr<TabItem> ti;
		Noesis::Ptr<TabItem> td;
		Noesis::Ptr<TabItem> tj;
	} _tabItemElements;

	Noesis::Ptr<Noesis::ObservableCollection<TabItem>> _tabItems; //left item bar depending on what is present on tile
	Noesis::Ptr<Noesis::ObservableCollection<TerrainTabItem>> _terrainTabItems;
	Noesis::Ptr<Noesis::ObservableCollection<ItemTabItem>> _itemTabItems;
	Noesis::Ptr<Noesis::ObservableCollection<CreatureTabItem>> _creatureTabItems;

	Noesis::Ptr<Noesis::ObservableCollection<CreatureTabItem>> _possibleTennants;
	void SetTennant( CreatureTabItem* tennant );
	CreatureTabItem* GetTennant() const;
	CreatureTabItem* m_tennant = nullptr;

	Noesis::String m_miniStockpileName;
	Noesis::Ptr<Noesis::ObservableCollection<TabItem>> _miniSPContents;
	const char* GetMiniSPName() const
	{
		return m_miniStockpileName.Str();
	}

	const char* GetVisRoomAssign() const;
	const char* GetVisRoomValue() const;
	const char* GetVisBeds() const;
	const char* GetVisAlarm() const;
	const char* GetRoomValue() const;
	const char* GetBeds() const;
	const char* GetEnclosed() const;
	const char* GetRoofed() const;

	bool GetAlarm() const;
	void SetAlarm( bool value );

	NS_DECLARE_REFLECTION( TileInfoModel, NotifyPropertyChangedBase )
};

} // namespace IngnomiaGUI

#endif
