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

#include "../aggregatorneighbors.h"

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

class NeighborsProxy;

namespace Noesis
{
template <class T>
class ObservableCollection;
}

namespace IngnomiaGUI
{

////////////////////////////////////////////////////////////////////////////////////////////////////
class NeighborKingdomInfo final : public Noesis::BaseComponent
{
public:
	NeighborKingdomInfo( const GuiNeighborInfo& info );

	const char* getIDString() const { return m_idString.Str(); }
	const char* getName() const { return m_name.Str(); }
	const char* getDistance() const { return m_distance.Str(); }
	const char* getType() const { return m_type.Str(); }
	const char* getAttitude() const { return m_attitude.Str(); }
	const char* getWealth() const { return m_wealth.Str(); }
	const char* getEconomy() const { return m_economy.Str(); }
	const char* getMilitary() const { return m_military.Str(); }

	const char* getShowDiploBtn() const;
	const char* getShowSpyBtn() const;
	const char* getShowRaidBtn() const;
	const char* getShowSabotageBtn() const;

	unsigned int getID() { return m_id;  }

private:
	unsigned int m_id = 0;
	Noesis::String m_idString;
	Noesis::String m_name;
	Noesis::String m_distance;
	Noesis::String m_type;
	Noesis::String m_attitude;
	Noesis::String m_wealth;
	Noesis::String m_economy;
	Noesis::String m_military;

	bool m_discovered = false;
	bool m_spyMission = false;
	bool m_sabotageMission = false;
	bool m_raidMission = false;
	bool m_diploMission = false;

	NS_DECLARE_REFLECTION( NeighborKingdomInfo, Noesis::BaseComponent )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class MissionInfo final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	MissionInfo( const Mission& info );

	const char* getIDString() const { return m_idString.Str(); }
	const char* getTitle() const { return m_title.Str(); }
	const char* getCurrentAction() const { return m_currentAction.Str(); }
	const char* getTime() const { return m_time.Str(); }
	
	unsigned int getID() { return m_id; }
private:
	unsigned int m_id = 0;
	Noesis::String m_idString;
	Noesis::String m_title;
	Noesis::String m_currentAction;
	Noesis::String m_time;
	
	NS_DECLARE_REFLECTION( MissionInfo, NoesisApp::NotifyPropertyChangedBase )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class AvailableGnome final : public Noesis::BaseComponent
{
public:
	AvailableGnome( unsigned int id, QString name );

	const char* getIDString() const { return m_idString.Str(); }
	const char* getName() const { return m_name.Str(); }
	
	unsigned int getID() { return m_id; }

private:
	unsigned int m_id = 0;
	Noesis::String m_idString;
	Noesis::String m_name;
	
	NS_DECLARE_REFLECTION( AvailableGnome, BaseComponent )
};


enum class NeighborsPage
{
	First,
	Second,
	Third
};







////////////////////////////////////////////////////////////////////////////////////////////////////
class NeighborsModel final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	NeighborsModel();

	void neighborsUpdate( const QList<GuiNeighborInfo>& infos );
	void missionsUpdate( const QList<Mission>& missions );
	void updateAvailableGnomes( const QList<GuiAvailableGnome>& gnomes );
	void updateMission( const Mission& mission );


private:
	NeighborsProxy* m_proxy = nullptr;

	NeighborsPage m_page = NeighborsPage::First;

	const char* GetShowFirst() const;
	const char* GetShowSecond() const;
	const char* GetShowThird() const;

	void onPageCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetPageCmd() const
	{
		return &m_pageCmd;
	}
	NoesisApp::DelegateCommand m_pageCmd;

	Noesis::ObservableCollection<NeighborKingdomInfo>* getNeighborList() const
	{
		return m_neighborList;
	}
	Noesis::Ptr<Noesis::ObservableCollection<NeighborKingdomInfo>> m_neighborList;

	void onDiploMissionCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetDiploMissionCmd() const
	{
		return &m_diploMissionCmd;
	}
	NoesisApp::DelegateCommand m_diploMissionCmd;

	void onSpyMissionCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetSpyMissionCmd() const
	{
		return &m_spyMissionCmd;
	}
	NoesisApp::DelegateCommand m_spyMissionCmd;

	void onRaidMissionCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetRaidMissionCmd() const
	{
		return &m_raidMissionCmd;
	}
	NoesisApp::DelegateCommand m_raidMissionCmd;

	void onSabotageMissionCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetSabotageMissionCmd() const
	{
		return &m_sabotageMissionCmd;
	}
	NoesisApp::DelegateCommand m_sabotageMissionCmd;

	

	Noesis::ObservableCollection<MissionInfo>* getMissionList() const
	{
		return m_missionList;
	}
	Noesis::Ptr<Noesis::ObservableCollection<MissionInfo>> m_missionList;

	NS_DECLARE_REFLECTION( NeighborsModel, NotifyPropertyChangedBase )

	const char* getShowGnomeSelect() const;
	bool m_showSelector = false;

	Noesis::ObservableCollection<AvailableGnome>* getAvailableGnomeList() const
	{
		return m_availGnomeList;
	}
	Noesis::Ptr<Noesis::ObservableCollection<AvailableGnome>> m_availGnomeList;

	Noesis::ObservableCollection<AvailableGnome>* getActionList() const
	{
		return m_actionList;
	}
	Noesis::Ptr<Noesis::ObservableCollection<AvailableGnome>> m_actionList;

	AvailableGnome* m_selectedGnome = nullptr;
	void setSelectedGnome( AvailableGnome* gnome );
	AvailableGnome* getSelectedGnome() const;

	AvailableGnome* m_selectedAction = nullptr;
	void setSelectedAction( AvailableGnome* gnome );
	AvailableGnome* getSelectedAction() const;

	Noesis::String m_missionTarget;
	const char* getMissionTarget() const;

	Noesis::String m_missionTitle;
	const char* getMissionTitle() const;

	void onNewMissionBackCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* getNewMissionBackCmd() const
	{
		return &m_newMissionBackCmd;
	}
	NoesisApp::DelegateCommand m_newMissionBackCmd;

	void onNewMissionStartCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* getNewMissionStartCmd() const
	{
		return &m_newMissionStartCmd;
	}
	NoesisApp::DelegateCommand m_newMissionStartCmd;

	unsigned int m_missionTargetID = 0;

};

} // namespace IngnomiaGUI


