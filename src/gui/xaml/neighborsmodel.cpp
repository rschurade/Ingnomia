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
#include "neighborsmodel.h"
#include "neighborsproxy.h"

#include "../strings.h"

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

////////////////////////////////////////////////////////////////////////////////////////////////////
NeighborKingdomInfo::NeighborKingdomInfo( const GuiNeighborInfo& info ) :
	m_id( info.id ),
	m_idString( QString::number( info.id ).toStdString().c_str() ),
	m_name( info.name.toStdString().c_str() ),
	m_distance( info.distance.toStdString().c_str() ),
	m_type( info.type.toStdString().c_str() ),
	m_attitude( info.attitude.toStdString().c_str() ),
	m_wealth( info.wealth.toStdString().c_str() ),
	m_economy( info.economy.toStdString().c_str() ),
	m_military( info.military.toStdString().c_str() ),

	m_discovered( info.discovered ),
	m_spyMission( info.spyMission ),
	m_sabotageMission( info.sabotageMission ),
	m_raidMission( info.raidMission ),
	m_diploMission( info.diploMission )
{
}

const char* NeighborKingdomInfo::getShowDiploBtn() const
{
	if( m_discovered && m_diploMission )
	{
		return "Visible";
	}
	return "Hidden";
}
	
const char* NeighborKingdomInfo::getShowSpyBtn() const
{
	if( m_discovered && m_spyMission )
	{
		return "Visible";
	}
	return "Hidden";
}

const char* NeighborKingdomInfo::getShowRaidBtn() const
{
	if( m_discovered && m_raidMission )
	{
		return "Visible";
	}
	return "Hidden";
}

const char* NeighborKingdomInfo::getShowSabotageBtn() const
{
	if( m_discovered && m_sabotageMission )
	{
		return "Visible";
	}
	return "Hidden";
}

////////////////////////////////////////////////////////////////////////////////////////////////////
MissionInfo::MissionInfo( const Mission& mission ) :
	m_id( mission.id ),
	m_idString( QString::number( mission.id ).toStdString().c_str() )
{
	QString qTime;	

	switch( mission.type )
	{
	case MissionType::EXPLORE: 
		m_title = "Explore"; 
		switch( mission.step )
		{
			case MissionStep::LEAVE_MAP:
				m_currentAction = "Leaving the area.";
				qTime = "I haven't left yet.";
				break;
			case MissionStep::TRAVEL:
				m_currentAction = "Exploring the surrounding lands.";
				qTime = "It's been " + QString::number( qMax( 1, mission.time ) ) + " hours since I left.";
				break;
			case MissionStep::RETURN:
				m_currentAction = "Returning from the mission.";
				qTime = "It's been " + QString::number( qMax( 1, mission.time ) ) + " hours since I left.";
				break;
			case MissionStep::RETURNED:
			{
				bool success = mission.result.value( "Success" ).toBool();
				if( success )
				{
					m_currentAction = "I found a neighboring kingdom.";
				}
				else
				{
					m_currentAction = "I failed to find something new.";
				}
				qTime = "It took " + S::gi().numberWord( mission.result.value( "TotalTime" ).toInt() / 24 ) + " days.";
				break;
			}
		}
		break;
	case MissionType::SPY: 
		m_title = "Spy"; 
		switch( mission.step )
		{
			case MissionStep::LEAVE_MAP:
				m_currentAction = "Leaving the area.";
				qTime = "I haven't left yet.";
				break;
			case MissionStep::TRAVEL:
				m_currentAction = "Traveling to the kingdom.";
				qTime = "It's been " + QString::number( qMax( 1, mission.time ) ) + " hours since I left.";
				break;
			case MissionStep::RETURN:
				m_currentAction = "Returning from the mission.";
				qTime = "It's been " + QString::number( qMax( 1, mission.time ) ) + " hours since I left.";
				break;
			case MissionStep::RETURNED:
			{
				bool success = mission.result.value( "Success" ).toBool();
				if( success )
				{
					m_currentAction = "Success.";
				}
				else
				{
					m_currentAction = "Failure.";
				}
				qTime = "It took " + S::gi().numberWord( mission.result.value( "TotalTime" ).toInt() / 24 ) + " days.";
				break;
			}
		}
		break;
	case MissionType::EMISSARY: 
		m_title = "Emissary";
		switch( mission.step )
		{
			case MissionStep::LEAVE_MAP:
				m_currentAction = "Leaving the area.";
				qTime = "I haven't left yet.";
				break;
			case MissionStep::TRAVEL:
				m_currentAction = "Traveling to the kingdom.";
				qTime = "It's been " + QString::number( qMax( 1, mission.time ) ) + " hours since I left.";
				break;
			case MissionStep::RETURN:
				m_currentAction = "Returning from the mission.";
				qTime = "It's been " + QString::number( qMax( 1, mission.time ) ) + " hours since I left.";
				break;
			case MissionStep::RETURNED:
			{
				bool success = mission.result.value( "Success" ).toBool();
				if( success )
				{
					m_currentAction = "Success." ;
				}
				else
				{
					m_currentAction = "Failure.";
				}
				qTime = "It took " + S::gi().numberWord( mission.result.value( "TotalTime" ).toInt() / 24 ) + " days.";
				break;
			}
		}
		break;
	case MissionType::RAID: 
		m_title = "Raid";
		switch( mission.step )
		{
			case MissionStep::LEAVE_MAP:
				m_currentAction = "Leaving the area.";
				qTime = "We haven't left yet.";
				break;
			case MissionStep::TRAVEL:
				m_currentAction = "Traveling to the kingdom.";
				qTime = "It's been " + QString::number( qMax( 1, mission.time ) ) + " hours since we left.";
				break;
			case MissionStep::RETURN:
				m_currentAction = "Returning from the mission.";
				qTime = "It's been " + QString::number( qMax( 1, mission.time ) ) + " hours since we left.";
				break;
			case MissionStep::RETURNED:
			{
				bool success = mission.result.value( "Success" ).toBool();
				if( success )
				{
					m_currentAction = "Success.";
				}
				else
				{
					m_currentAction = "Failure.";
				}
				qTime = "It took " + S::gi().numberWord( mission.result.value( "TotalTime" ).toInt() / 24 ) + " days.";
				break;
			}
		}
		break;
	case MissionType::SABOTAGE: 
		m_title = "Sabotage"; 
		switch( mission.step )
		{
			case MissionStep::LEAVE_MAP:
				m_currentAction = "Leaving the area.";
				qTime = "We haven't left yet.";
				break;
			case MissionStep::TRAVEL:
				m_currentAction = "Traveling to the kingdom.";
				qTime = "It's been " + QString::number( qMax( 1, mission.time ) ) + " hours since we left.";
				break;
			case MissionStep::RETURN:
				m_currentAction = "Returning from the mission.";
				qTime = "It's been " + QString::number( qMax( 1, mission.time ) ) + " hours since we left.";
				break;
			case MissionStep::RETURNED:
			{
				bool success = mission.result.value( "Success" ).toBool();
				if( success )
				{
					m_currentAction = "Success.";
				}
				else
				{
					m_currentAction = "Failure.";
				}
				qTime = "It took " + S::gi().numberWord( mission.result.value( "TotalTime" ).toInt() / 24 ) + " days.";
				break;
			}
		}
		break;
	}

	m_time = qTime.toStdString().c_str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AvailableGnome::AvailableGnome( unsigned int id, QString name ) :
	m_id( id ),
	m_idString( QString::number( id ).toStdString().c_str() ),
	m_name( name.toStdString().c_str() )
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NeighborsModel::NeighborsModel()
{
	m_proxy = new NeighborsProxy;
	m_proxy->setParent( this );

	m_pageCmd.SetExecuteFunc( MakeDelegate( this, &NeighborsModel::onPageCmd ) );
	m_diploMissionCmd.SetExecuteFunc( MakeDelegate( this, &NeighborsModel::onDiploMissionCmd ) );
	m_spyMissionCmd.SetExecuteFunc( MakeDelegate( this, &NeighborsModel::onSpyMissionCmd ) );
	m_raidMissionCmd.SetExecuteFunc( MakeDelegate( this, &NeighborsModel::onRaidMissionCmd ) );
	m_sabotageMissionCmd.SetExecuteFunc( MakeDelegate( this, &NeighborsModel::onSabotageMissionCmd ) );

	m_newMissionBackCmd.SetExecuteFunc( MakeDelegate( this, &NeighborsModel::onNewMissionBackCmd ) );
	m_newMissionStartCmd.SetExecuteFunc( MakeDelegate( this, &NeighborsModel::onNewMissionStartCmd ) );

	m_neighborList = *new ObservableCollection<NeighborKingdomInfo>();
	m_missionList = *new ObservableCollection<MissionInfo>();
	m_availGnomeList = *new ObservableCollection<AvailableGnome>();
	m_actionList = *new ObservableCollection<AvailableGnome>();
}

const char* NeighborsModel::GetShowFirst() const
{
	if ( m_page == NeighborsPage::First )
	{
		return "Visible";
	}
	return "Hidden";
}
	
const char* NeighborsModel::GetShowSecond() const
{
	if ( m_page == NeighborsPage::Second )
	{
		return "Visible";
	}
	return "Hidden";
}

const char* NeighborsModel::GetShowThird() const
{
	if ( m_page == NeighborsPage::Third )
	{
		return "Visible";
	}
	return "Hidden";
}

void NeighborsModel::onPageCmd( BaseComponent* param )
{
	if( param->ToString() == "First" )
	{
		m_page = NeighborsPage::First;
	}
	else if( param->ToString() == "Second" )
	{
		m_page = NeighborsPage::Second;
	}
	else
	{
		m_page = NeighborsPage::Third;
	}
	
	OnPropertyChanged( "ShowFirst" );
	OnPropertyChanged( "ShowSecond" );
	OnPropertyChanged( "ShowThird" );
}

void NeighborsModel::onDiploMissionCmd( BaseComponent* param )
{
	m_missionTitle = "New diplomatic mission to";
	OnPropertyChanged( "MissionTitle" );

	for( int i = 0; i < m_neighborList->Count(); ++i )
	{
		auto neighbor = m_neighborList->Get( i );
		auto ids = neighbor->getIDString();

		if( strcmp( param->ToString().Str(), ids ) == 0 )
		{
			m_missionTarget = neighbor->getName();
			m_missionTargetID = neighbor->getID();
			OnPropertyChanged( "MissionTarget" );
			break;
		}

	}

	m_actionList->Clear();

	m_actionList->Add( MakePtr<AvailableGnome>( (int)MissionAction::IMPROVE, "Improve Relations" ) );
	m_actionList->Add( MakePtr<AvailableGnome>( (int)MissionAction::INSULT, "Insult" ) );
	m_actionList->Add( MakePtr<AvailableGnome>( (int)MissionAction::INVITE_TRADER, "Invite Trader" ) );
	m_actionList->Add( MakePtr<AvailableGnome>( (int)MissionAction::INVITE_AMBASSADOR, "Invite Ambassador" ) );

	OnPropertyChanged( "MissionActions" );

	setSelectedAction( m_actionList->Get( 0 ) );
	
	m_showSelector = true;
	OnPropertyChanged( "ShowGnomeSelect" );


	m_proxy->requestAvailableGnomes();


	qDebug() << "Diplo" << param->ToString().Str();
}

void NeighborsModel::onSpyMissionCmd( BaseComponent* param )
{
	qDebug() << "Spy" << param->ToString().Str();
}

void NeighborsModel::onRaidMissionCmd( BaseComponent* param )
{
	qDebug() << "Raid" << param->ToString().Str();
}

void NeighborsModel::onSabotageMissionCmd( BaseComponent* param )
{
	qDebug() << "Sabotage" << param->ToString().Str();
}

void NeighborsModel::neighborsUpdate( const QList<GuiNeighborInfo>& infos )
{
	m_neighborList->Clear();

	for( const auto& info : infos )
	{
		m_neighborList->Add( MakePtr<NeighborKingdomInfo>( info ) );
	}

	OnPropertyChanged( "NeighborList" );
}

void NeighborsModel::missionsUpdate( const QList<Mission>& missions )
{
	m_missionList->Clear();

	for( const auto& info : missions )
	{
		m_missionList->Add( MakePtr<MissionInfo>( info ) );
	}

	OnPropertyChanged( "MissionList" );
}

const char* NeighborsModel::getShowGnomeSelect() const
{
	return m_showSelector ? "Visible" : "Hidden";
}

void NeighborsModel::setSelectedGnome( AvailableGnome* gnome )
{
	if( m_selectedGnome != gnome )
	{
		m_selectedGnome = gnome;
		OnPropertyChanged( "SelectedGnome" );
	}
}
	
AvailableGnome* NeighborsModel::getSelectedGnome() const
{
	return m_selectedGnome;
}

void NeighborsModel::setSelectedAction( AvailableGnome* action )
{
	if( m_selectedAction != action )
	{
		m_selectedAction = action;
		OnPropertyChanged( "SelectedAction" );
	}
}
	
AvailableGnome* NeighborsModel::getSelectedAction() const
{
	return m_selectedAction;
}

const char* NeighborsModel::getMissionTarget() const
{
	return m_missionTarget.Str();
}

const char* NeighborsModel::getMissionTitle() const
{
	return m_missionTitle.Str();
}


void NeighborsModel::updateAvailableGnomes( const QList<GuiAvailableGnome>& gnomes )
{
	m_availGnomeList->Clear();
	for( const auto& gnome : gnomes )
	{
		m_availGnomeList->Add( MakePtr<AvailableGnome>( gnome.id, gnome.name ) );
	}
	OnPropertyChanged( "MissionGnomes" );

	if( m_availGnomeList->Count() > 0 )
	{
		setSelectedGnome( m_availGnomeList->Get( 0 ) );
	}
}

void NeighborsModel::onNewMissionBackCmd( BaseComponent* param )
{
	m_showSelector = false;
	OnPropertyChanged( "ShowGnomeSelect" );
	m_selectedGnome = nullptr;
	m_selectedAction = nullptr;
	m_missionTargetID = 0;
}

void NeighborsModel::onNewMissionStartCmd( BaseComponent* param )
{
	if( m_selectedGnome && m_selectedAction && m_missionTargetID )
	{
		m_showSelector = false;
		OnPropertyChanged( "ShowGnomeSelect" );
		
		m_proxy->startMission( MissionType::EMISSARY, (MissionAction)m_selectedAction->getID(), m_missionTargetID, m_selectedGnome->getID() );
	}
}

void NeighborsModel::updateMission( const Mission& mission )
{
	for( int i = 0; i < m_missionList->Count(); ++i )
	{
		auto mis = m_missionList->Get( i );
		if( mis->getID() == mission.id )
		{
			m_missionList->RemoveAt( i );
			m_missionList->Insert( i, MakePtr<MissionInfo>( mission ) );
			OnPropertyChanged( "MissionList" );
			break;
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION( NeighborsModel, "IngnomiaGUI.NeighborsModel" )
{
	NsProp( "PageCmd", &NeighborsModel::GetPageCmd );
	NsProp( "ShowFirst", &NeighborsModel::GetShowFirst );
	NsProp( "ShowSecond", &NeighborsModel::GetShowSecond );
	NsProp( "ShowThird", &NeighborsModel::GetShowThird );
	NsProp( "NeighborList", &NeighborsModel::getNeighborList );
	NsProp( "MissionList", &NeighborsModel::getMissionList );

	NsProp( "CmdDiploMission", &NeighborsModel::GetDiploMissionCmd );
	NsProp( "CmdSpyMission", &NeighborsModel::GetSpyMissionCmd );
	NsProp( "CmdRaidMission", &NeighborsModel::GetRaidMissionCmd );
	NsProp( "CmdSabotageMission", &NeighborsModel::GetSabotageMissionCmd );

	NsProp( "CmdNewMissionBack", &NeighborsModel::getNewMissionBackCmd );
	NsProp( "CmdNewMissionStart", &NeighborsModel::getNewMissionStartCmd );

	NsProp( "ShowGnomeSelect", &NeighborsModel::getShowGnomeSelect );
	NsProp( "MissionGnomes", &NeighborsModel::getAvailableGnomeList );
	NsProp( "MissionActions", &NeighborsModel::getActionList );
	NsProp( "SelectedGnome", &NeighborsModel::getSelectedGnome, &NeighborsModel::setSelectedGnome );
	NsProp( "SelectedAction", &NeighborsModel::getSelectedAction, &NeighborsModel::setSelectedAction );
	NsProp( "MissionTarget", &NeighborsModel::getMissionTarget );
	NsProp( "MissionTitle", &NeighborsModel::getMissionTitle );
}

NS_IMPLEMENT_REFLECTION( NeighborKingdomInfo, "IngnomiaGUI.NeighborKingdomInfo" )
{
	NsProp( "ID", &NeighborKingdomInfo::getIDString );
	NsProp( "Name", &NeighborKingdomInfo::getName );
	NsProp( "Distance", &NeighborKingdomInfo::getDistance );
	NsProp( "Wealth", &NeighborKingdomInfo::getWealth );
	NsProp( "Economy", &NeighborKingdomInfo::getEconomy );
	NsProp( "Military", &NeighborKingdomInfo::getMilitary );
	NsProp( "Attitude", &NeighborKingdomInfo::getAttitude );

	NsProp( "ShowDiploButton", &NeighborKingdomInfo::getShowDiploBtn );
	NsProp( "ShowSpyButton", &NeighborKingdomInfo::getShowSpyBtn );
	NsProp( "ShowRaidButton", &NeighborKingdomInfo::getShowRaidBtn );
	NsProp( "ShowSabotageButton", &NeighborKingdomInfo::getShowSabotageBtn );
}

NS_IMPLEMENT_REFLECTION( MissionInfo, "IngnomiaGUI.MissionInfo" )
{
	NsProp( "ID", &MissionInfo::getIDString );
	NsProp( "Title", &MissionInfo::getTitle );
	NsProp( "Action", &MissionInfo::getCurrentAction );
	NsProp( "Time", &MissionInfo::getTime );
}

NS_IMPLEMENT_REFLECTION( AvailableGnome, "IngnomiaGUI.AvailableGnome" )
{
	NsProp( "ID", &AvailableGnome::getIDString );
	NsProp( "Name", &AvailableGnome::getName );
}
