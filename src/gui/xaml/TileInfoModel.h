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
#include "GameModel.h"

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
class TITabItem final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	TITabItem( QString name, QString sid );

	const char* GetName() const { return m_name.Str(); }
	const char* GetID() const { return m_sid.Str(); }
	bool GetChecked() const;
	void SetChecked( bool )
	{
	}

	void setActive( bool active );

private:
	Noesis::String m_name;
	Noesis::String m_sid;
	bool _active = false;

	NS_DECLARE_REFLECTION( TITabItem, NoesisApp::NotifyPropertyChangedBase )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class TerrainTabItem final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	TerrainTabItem( QString name, QString sid, QString action1 = "", QString action2 = "" );

	const char* GetName() const { return m_name.Str(); }
	const char* GetID() const { return m_sid.Str(); }
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
	Noesis::String m_name;
	Noesis::String m_sid;
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

	const char* GetName() const { return m_name.Str(); }
	const char* GetID() const { return m_sid.Str(); }

private:
	Noesis::String m_name;
	unsigned int m_id;
	Noesis::String m_sid;

	NS_DECLARE_REFLECTION( ItemTabItem, NoesisApp::NotifyPropertyChangedBase )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class CreatureTabItem final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	CreatureTabItem( QString name, unsigned int id );

	const char* GetName() const { return m_name.Str(); }
	const char* GetID() const { return m_sid.Str(); }

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
class AutomatonTabItem final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	AutomatonTabItem( QString name, unsigned int id, bool refuel, QString coreItem, ProxyTileInfo* proxy );

	const char* GetName() const { return m_name.Str(); }
	const char* GetID() const { return m_sid.Str(); }
	unsigned int uid() { return m_id; }

	Noesis::ObservableCollection<TITabItem>* getCoreItems() const;

	void setSelectedCore( TITabItem* mat );
	TITabItem* getSelectedCore() const;

	bool getRefuel() const;
	void setRefuel( bool value );

private:
	ProxyTileInfo* m_proxy = nullptr;

	Noesis::String m_name;
	unsigned int m_id = 0;
	Noesis::String m_sid;
	bool m_refuel = false;
	TITabItem* m_selectedCoreItem = nullptr;

	Noesis::Ptr<Noesis::ObservableCollection<TITabItem>> m_coreItems;

	

	NS_DECLARE_REFLECTION( AutomatonTabItem, NoesisApp::NotifyPropertyChangedBase )
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

	Noesis::ObservableCollection<TITabItem>* getTabItems() const;
	Noesis::ObservableCollection<TerrainTabItem>* getTerrainTabItems() const;
	Noesis::ObservableCollection<ItemTabItem>* getItemTabItems() const;
	Noesis::ObservableCollection<CreatureTabItem>* getCreatureTabItems() const;
	Noesis::ObservableCollection<AutomatonTabItem>* getAutomatonTabItems() const;
	Noesis::ObservableCollection<TITabItem>* getMiniSPContents() const;
	Noesis::ObservableCollection<CreatureTabItem>* getPossibleTennants() const;

	const char* GetShowTerrain() const;
	const char* GetShowItems() const;
	const char* GetShowCreatures() const;
	const char* GetShowAutomatons() const;
	const char* GetShowDesignation() const;
	const char* GetShowDesignationSimple() const;
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
	const char* GetRequiredSkill() const
	{
		return m_requiredSkill.Str();
	}
	const char* GetRequiredTool() const
	{
		return m_requiredTool.Str();
	}
	const char* GetRequiredToolAvailable() const
	{
		return m_requiredToolAvailable.Str();
	}
	Noesis::ObservableCollection<NRequiredItem>* GetJobRequiredItems() const;
	const char* GetWorkablePosition() const
	{
		return m_workablePosition.Str();
	}

	const char* GetDesignationName() const
	{
		return m_designationName.Str();
	}

	const char* GetDesignationTitle() const
	{
		return m_designationTitle.Str();
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

	// job related fields
	Noesis::String m_jobName;
	Noesis::String m_jobWorker;
	Noesis::String m_jobPriority;
	Noesis::String m_requiredSkill;
	Noesis::String m_requiredTool;
	Noesis::String m_requiredToolAvailable;
	Noesis::Ptr<Noesis::ObservableCollection<NRequiredItem>> _jobTabRequiredItems;
	Noesis::String m_workablePosition;

	unsigned int m_designationID = 0;
	Noesis::String m_designationName;
	Noesis::String m_designationTitle;
	TileFlag m_designationFlag = TileFlag::TF_NONE;
	RoomType m_roomType        = RoomType::NotSet;
	bool m_hasRoof             = false;
	bool m_isEnclosed          = false;
	bool m_hasAlarmBell        = false;
	Noesis::String m_beds;
	bool m_alarm = false;
	Noesis::String m_roomValue;

	bool m_hasJob			   = false;

	unsigned int m_mechanismID = 0;
	bool m_hasMechanism		   = false;
	QString m_mechGui;
	Noesis::String m_mechActive;
	Noesis::String m_mechInvert;
	Noesis::String m_mechFuel;
	Noesis::String m_mechName;
	


	Noesis::String m_capacity;
	Noesis::String m_itemCount;
	Noesis::String m_reserved;

	TileInfoMode _mode = TileInfoMode::Terrain;

	struct
	{
		Noesis::Ptr<TITabItem> tt;
		Noesis::Ptr<TITabItem> tc;
		Noesis::Ptr<TITabItem> ti;
		Noesis::Ptr<TITabItem> td;
		Noesis::Ptr<TITabItem> tj;
	} _tabItemElements;

	Noesis::Ptr<Noesis::ObservableCollection<TITabItem>> _tabItems; //left item bar depending on what is present on tile
	Noesis::Ptr<Noesis::ObservableCollection<TerrainTabItem>> _terrainTabItems;
	Noesis::Ptr<Noesis::ObservableCollection<ItemTabItem>> _itemTabItems;
	Noesis::Ptr<Noesis::ObservableCollection<CreatureTabItem>> _creatureTabItems;
	Noesis::Ptr<Noesis::ObservableCollection<AutomatonTabItem>> _automatonTabItems;

	Noesis::Ptr<Noesis::ObservableCollection<CreatureTabItem>> _possibleTennants;
	void SetTennant( CreatureTabItem* tennant );
	CreatureTabItem* GetTennant() const;
	CreatureTabItem* m_tennant = nullptr;

	Noesis::String m_miniStockpileName;
	Noesis::Ptr<Noesis::ObservableCollection<TITabItem>> _miniSPContents;
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

	const char* GetCapacity() const;
	const char* GetItemCount() const;
	const char* GetReserved() const;

	bool GetAlarm() const;
	void SetAlarm( bool value );

	const char* GetShowMechanism() const;
	const char* GetShowMechActive() const;
	const char* GetShowMechFuel() const;
	const char* GetShowMechInvert() const;
	const char* GetMechActive() const;
	const char* GetMechFuel() const;
	const char* GetMechInvert() const;
	const char* GetMechName() const;

	const NoesisApp::DelegateCommand* GetCmdMechToggleActive() const
	{
		return &_cmdMechToggleActive;
	}
	void OnCmdMechToggleActive( BaseComponent* param );
	NoesisApp::DelegateCommand _cmdMechToggleActive;

	const NoesisApp::DelegateCommand* GetCmdMechToggleInvert() const
	{
		return &_cmdMechToggleInvert;
	}
	void OnCmdMechToggleInvert( BaseComponent* param );
	NoesisApp::DelegateCommand _cmdMechToggleInvert;


	NS_DECLARE_REFLECTION( TileInfoModel, NotifyPropertyChangedBase )
};

} // namespace IngnomiaGUI

#endif
