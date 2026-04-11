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
#include "PopulationModel.h"

#include "populationproxy.h"

#include <NsApp/Application.h>
#include <NsCore/Log.h>
#include <NsCore/ReflectionImplement.h>
#include <NsCore/ReflectionImplementEnum.h>
#include <NsGui/ObservableCollection.h>
#include <NsGui/UIElement.h>

#include <QDebug>

/** @file PopulationModel.cpp
 *  @brief PopulationModel and helper-component implementations. Constructors wrap the
 *         Gui*Info payloads; update* helpers refresh the nested observable collections
 *         after aggregator-side changes; command handlers route user edits back through
 *         PopulationProxy. Trivial Get/Set accessors are standard XAML plumbing.
 */

using namespace IngnomiaGUI;
using namespace Noesis;
using namespace NoesisApp;

////////////////////////////////////////////////////////////////////////////////////////////////////
ProfItem::ProfItem( QString name )
{
	m_name = name.toStdString().c_str();
}

const char* ProfItem::GetName() const
{
	return m_name.Str();
}

void ProfItem::SetName( const char* name )
{
	m_name = name;
	OnPropertyChanged( "Name" );
}

////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Builds a skill cell row from a GuiSkillInfo payload.
GnomeSkill::GnomeSkill( const GuiSkillInfo& skill, unsigned int gnomeID, PopulationProxy* proxy ) :
	m_gnomeID( gnomeID ),
	m_proxy( proxy )
{
	m_name      = skill.name.toStdString().c_str();
	m_level	= QString::number( skill.level ).toStdString().c_str();
	m_checked   = skill.active;
	m_color = skill.color.toStdString().c_str();
	m_skillID = skill.sid.toStdString().c_str();
}

const char* GnomeSkill::GetName() const
{
	return m_name.Str();
}

const char* GnomeSkill::GetID() const
{
	return m_skillID.Str();
}


const char* GnomeSkill::GetLevel() const
{
	return m_level.Str();
}

bool GnomeSkill::GetChecked() const
{
	return m_checked;
}

/// @brief Toggles the skill-active flag on this gnome via the proxy.
void GnomeSkill::SetChecked( bool value )
{
	if( m_checked == value )
	{
		return;
	}
	m_checked = value;
	OnPropertyChanged( "Checked" );
	if( m_proxy )
	{
		m_proxy->setSkillActive( m_gnomeID, m_skillID.Str(), value );
	}
}

const char* GnomeSkill::GetColor() const
{
	return m_color.Str();
}

void GnomeSkill::applyUpdate( const GuiSkillInfo& skill )
{
	Noesis::String newLevel = QString::number( skill.level ).toStdString().c_str();
	if( m_level != newLevel )
	{
		m_level = newLevel;
		OnPropertyChanged( "Level" );
	}
	if( m_checked != skill.active )
	{
		m_checked = skill.active;
		OnPropertyChanged( "Checked" );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Builds a skill-grid row: name, profession dropdown (shared collection), and
///        per-skill GnomeSkill cells.
GnomeRow::GnomeRow( const GuiGnomeInfo& gnome, Noesis::Ptr<Noesis::ObservableCollection<ProfItem>> professions, PopulationProxy* proxy ) :
	m_proxy( proxy )
{
	m_id        = gnome.id;
	m_idString  = QString::number( gnome.id ).toStdString().c_str();
	m_name      = gnome.name.toStdString().c_str();
	m_professionName = gnome.profession;

	m_skills = *new ObservableCollection<GnomeSkill>();

	for( const auto& skill : gnome.skills )
	{
		m_skills->Add( MakePtr<GnomeSkill>( skill, m_id, proxy ) );
	}

	m_professions = professions;

	for( int i = 0; i < m_professions->Count(); ++i )
	{
		if( m_professions->Get( i )->GetName() == gnome.profession )
		{
			SetProfession( m_professions->Get( i ) );
			break;
		}
	}
}

void GnomeRow::applyUpdate( const GuiGnomeInfo& gnome )
{
	Noesis::String newName = gnome.name.toStdString().c_str();
	if( m_name != newName )
	{
		m_name = newName;
		OnPropertyChanged( "Name" );
	}

	if( m_professionName != gnome.profession )
	{
		m_professionName = gnome.profession;
		for( int i = 0; i < m_professions->Count(); ++i )
		{
			if( m_professions->Get( i )->GetName() == m_professionName )
			{
				SetProfession( m_professions->Get( i ) );
				break;
			}
		}
	}

	const int skillCount = m_skills->Count();
	const int incoming = gnome.skills.size();
	for( int i = 0; i < skillCount && i < incoming; ++i )
	{
		m_skills->Get( i )->applyUpdate( gnome.skills[i] );
	}
}

/// @brief Rebinds this row's profession dropdown to a new shared collection and re-selects
///        the ProfItem that matches m_professionName.
void GnomeRow::updateProfessionList( Noesis::Ptr<Noesis::ObservableCollection<ProfItem>> professions )
{
	m_professions = professions;
	m_selectedProfession = nullptr;
	for( int i = 0; i < m_professions->Count(); ++i )
	{
		if( m_professions->Get( i )->GetName() == m_professionName )
		{
			SetProfession( m_professions->Get( i ) );
			break;
		}
	}
	if ( m_selectedProfession == nullptr )
	{
		SetProfession( m_professions->Get( 0 ) );
	}
}

const char* GnomeRow::GetName() const
{
	return m_name.Str();
}

const char* GnomeRow::GetID() const
{
	return m_idString.Str();
}

Noesis::ObservableCollection<ProfItem>* GnomeRow::GetProfessions() const
{
	return m_professions;
}
	
/// @brief Forwards a profession change on this gnome to the proxy.
void GnomeRow::SetProfession( ProfItem* item )
{
	if ( item && m_selectedProfession != item )
	{
		m_selectedProfession = item;
		m_proxy->setProfession( m_id, item->GetName() );

		OnPropertyChanged( "Profession" );
	}
}

ProfItem* GnomeRow::GetProfession() const
{
	return m_selectedProfession;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Builds a schedule cell for the given gnome, hour, and initial activity. Registers
///        the click command that cycles the activity.
GnomeScheduleEntry::GnomeScheduleEntry( unsigned int id, int hour, ScheduleActivity activity, PopulationProxy* proxy ) :
	m_proxy( proxy ),
	m_id( id ),
	m_hour( hour ),
	m_activity( activity )
{
	switch ( activity )
	{
		case ScheduleActivity::Eat:
			m_activityString = "E";
			break;
		case ScheduleActivity::Sleep:
			m_activityString = "S";
			break;
		case ScheduleActivity::Training:
			m_activityString = "T";
			break;
		default:
			m_activityString = "N";
			break;
	}
	m_setHourCmd.SetExecuteFunc( MakeDelegate( this, &GnomeScheduleEntry::onSetHourCmd ) );

}

const char* GnomeScheduleEntry::GetActivity() const
{
	return m_activityString.Str();
}

/// @brief Click handler: cycles the activity of this hour cell and forwards it to the proxy.
void GnomeScheduleEntry::onSetHourCmd( BaseComponent* param )
{
	m_proxy->setSchedule( m_id, m_hour, m_activity );
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Builds a schedule row with 24 GnomeScheduleEntry hour cells.
GnomeScheduleRow::GnomeScheduleRow( const GuiGnomeScheduleInfo& gnome, PopulationProxy* proxy  ) :
	m_proxy( proxy )
{
	m_id        = gnome.id;
	m_idString  = QString::number( gnome.id ).toStdString().c_str();
	m_name      = gnome.name.toStdString().c_str();

	m_schedule = *new ObservableCollection<GnomeScheduleEntry>();
	int hour = 0;

	for( const auto& activity : gnome.schedule )
	{
		m_schedule->Add( MakePtr<GnomeScheduleEntry>( m_id, hour++, activity, proxy ) );
	}

}

const char* GnomeScheduleRow::GetName() const
{
	return m_name.Str();
}

const char* GnomeScheduleRow::GetID() const
{
	return m_idString.Str();
}






////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Constructs the PopulationModel: instantiates the proxy, registers every
///        DelegateCommand, and asks the proxy for the initial gnome list.
PopulationModel::PopulationModel()
{
	m_proxy = new PopulationProxy;
	m_proxy->setParent( this );

	m_gnomes = *new ObservableCollection<GnomeRow>();
	m_skills = *new ObservableCollection<GnomeSkill>();
	m_profSkills = *new ObservableCollection<GnomeSkill>();
	m_scheduleGnomes = *new ObservableCollection<GnomeScheduleRow>();

	m_allGnomesCmd.SetExecuteFunc( MakeDelegate( this, &PopulationModel::onAllGnomesCmd ) );
	m_removeAllGnomesCmd.SetExecuteFunc( MakeDelegate( this, &PopulationModel::onRemoveAllGnomesCmd ) );
	m_allSkillsCmd.SetExecuteFunc( MakeDelegate( this, &PopulationModel::onAllSkillsCmd ) );
	m_removeAllSkillsCmd.SetExecuteFunc( MakeDelegate( this, &PopulationModel::onRemoveAllSkillsCmd ) );
	m_sortCmd.SetExecuteFunc( MakeDelegate( this, &PopulationModel::onSortCmd ) );
	m_pageCmd.SetExecuteFunc( MakeDelegate( this, &PopulationModel::onPageCmd ) );
	m_setActivityCmd.SetExecuteFunc( MakeDelegate( this, &PopulationModel::onSetActivityCmd ) );
	m_setAllHoursCmd.SetExecuteFunc( MakeDelegate( this, &PopulationModel::onSetAllHoursCmd ) );
	m_setHoursForAllCmd.SetExecuteFunc( MakeDelegate( this, &PopulationModel::onSetHoursForAllCmd ) );

	m_profSkillUpCmd.SetExecuteFunc( MakeDelegate( this, &PopulationModel::onProfSkillUpCmd ) );
	m_profSkillDownCmd.SetExecuteFunc( MakeDelegate( this, &PopulationModel::onProfSkillDownCmd ) );
	m_profSkillAddCmd.SetExecuteFunc( MakeDelegate( this, &PopulationModel::onProfSkillAddCmd ) );
	m_profSkillRemoveCmd.SetExecuteFunc( MakeDelegate( this, &PopulationModel::onProfSkillRemoveCmd ) );
	
	m_newProfCmd.SetExecuteFunc( MakeDelegate( this, &PopulationModel::onNewProfCmd ) );
	m_deleteProfCmd.SetExecuteFunc( MakeDelegate( this, &PopulationModel::onDeleteProfCmd ) );

	m_professions = *new ObservableCollection<ProfItem>();
}

/// @brief Rebuilds the shared profession dropdown collection and rebinds every gnome row's
///        selection so existing rows pick up the new items.
void PopulationModel::updateProfessionList( const QStringList& professions )
{
	m_professions->Clear();
	for( const auto& prof : professions )
	{
		m_professions->Add( MakePtr<ProfItem>( prof.toStdString().c_str() ) );
	}
	if( m_professions->Count() > 0 )
	{
		SetProfession( m_professions->Get( 0 ) );
	}

	for( int i = 0; i < m_gnomes->Count(); ++i )
	{
		m_gnomes->Get( i )->updateProfessionList( m_professions );
	}

	OnPropertyChanged( "Professions" );
}

/// @brief Replaces the skill-grid rows with fresh GnomeRow rows from a full info payload.
///        If the incoming gnome IDs match the existing rows 1:1 (the common case — e.g.
///        toggling one skill on every gnome), updates each row in place so virtualized
///        containers keep their DataContext and no transient binding errors fire.
///        Otherwise falls back to Clear+rebuild.
void PopulationModel::updateInfo( const GuiPopulationInfo& info )
{
	const int existing = m_gnomes->Count();
	const int incoming = info.gnomes.size();
	bool sameRoster = ( existing == incoming );
	if( sameRoster )
	{
		for( int i = 0; i < existing; ++i )
		{
			if( m_gnomes->Get( i )->gnomeID() != info.gnomes[i].id )
			{
				sameRoster = false;
				break;
			}
		}
	}

	if( sameRoster )
	{
		for( int i = 0; i < existing; ++i )
		{
			m_gnomes->Get( i )->applyUpdate( info.gnomes[i] );
		}
	}
	else
	{
		m_gnomes->Clear();
		for( const auto& gnome : info.gnomes )
		{
			m_gnomes->Add( MakePtr<GnomeRow>( gnome, m_professions, m_proxy ) );
		}
		OnPropertyChanged( "Gnomes" );
	}

	if( m_skills->Count() == 0 )
	{
		if( info.gnomes.size() )
		{
			auto& first = info.gnomes.first();
			for( const auto& skill : first.skills )
			{
				m_skills->Add( MakePtr<GnomeSkill>( skill, 0, m_proxy ) );
			}
			OnPropertyChanged( "SkillHeaders" );
		}
	}
}

/// @brief Refreshes a single gnome row in the skill grid (matched by UID).
void PopulationModel::updateSingleGnome( const GuiGnomeInfo& gnome )
{
	for( int i = 0; i < m_gnomes->Count(); ++i )
	{
		auto info = m_gnomes->Get( i );
		if( info->gnomeID() == gnome.id )
		{
			m_gnomes->RemoveAt( i );
			m_gnomes->Insert( i, MakePtr<GnomeRow>( gnome, m_professions, m_proxy ) );
			OnPropertyChanged( "Gnomes" );
			break;
		}
	}
}


/// @brief "Activate this skill for all gnomes" button handler.
void PopulationModel::onAllGnomesCmd( BaseComponent* param )
{
	m_proxy->setSkillForAllGnomes( param->ToString().Str(), true );
}

/// @brief "Deactivate this skill for all gnomes" button handler.
void PopulationModel::onRemoveAllGnomesCmd( BaseComponent* param )
{
	m_proxy->setSkillForAllGnomes( param->ToString().Str(), false );
}


/// @brief "Activate all skills for this gnome" button handler.
void PopulationModel::onAllSkillsCmd( BaseComponent* param )
{
	m_proxy->setAllSkillsForGnome( QString(param->ToString().Str() ).toUInt(), true );
}

/// @brief "Deactivate all skills for this gnome" button handler.
void PopulationModel::onRemoveAllSkillsCmd( BaseComponent* param )
{
	m_proxy->setAllSkillsForGnome( QString(param->ToString().Str() ).toUInt(), false );
}

/// @brief Column header sort click. Forwards the column key to the proxy so the grid rebuilds
///        in the new order.
void PopulationModel::onSortCmd( BaseComponent* param )
{
	m_proxy->sortGnomes( param->ToString().Str() );
}

/// @brief Tab switcher: parses @p param to switch between Skills, Schedule, and ProfEdit tabs.
void PopulationModel::onPageCmd( BaseComponent* param )
{
	m_state = PopState::Skills;
	if( param->ToString() == "Schedule" )
	{
		m_state = PopState::Schedule;
		m_proxy->requestSchedules();
	}
	if( param->ToString() == "ProfEdit" )
	{
		m_state = PopState::ProfEdit;
		//m_proxy->requestProfessions();
	}

	OnPropertyChanged( "ShowSkills" );
	OnPropertyChanged( "ShowSchedule" );
	OnPropertyChanged( "ShowProfEdit" );
}

const char* PopulationModel::GetShowSkills() const
{
	if( m_state == PopState::Skills )
	{
		return "Visible";
	}
	return "Hidden";
}
	
const char* PopulationModel::GetShowSchedule() const
{
	if( m_state == PopState::Schedule )
	{
		return "Visible";
	}
	return "Hidden";
}

const char* PopulationModel::GetShowProfEdit() const
{
	if( m_state == PopState::ProfEdit )
	{
		return "Visible";
	}
	return "Hidden";
}

/// @brief Replaces the schedule grid with fresh GnomeScheduleRow rows.
void PopulationModel::updateSchedules( const GuiScheduleInfo& info )
{
	m_scheduleGnomes->Clear();

	for( const auto& gnome : info.schedules )
	{
		m_scheduleGnomes->Add( MakePtr<GnomeScheduleRow>( gnome, m_proxy ) );
	}
	OnPropertyChanged( "Schedules" );
}

/// @brief Refreshes a single schedule row (matched by UID).
void PopulationModel::updateScheduleSingleGnome( const GuiGnomeScheduleInfo& gnome )
{
	for( int i = 0; i < m_scheduleGnomes->Count(); ++i )
	{
		auto info = m_scheduleGnomes->Get( i );
		if( info->gnomeID() == gnome.id )
		{
			m_scheduleGnomes->RemoveAt( i );
			m_scheduleGnomes->Insert( i, MakePtr<GnomeScheduleRow>( gnome, m_proxy ) );
			OnPropertyChanged( "Schedules" );
			break;
		}
	}
}

/// @brief Handler for the "set activity" button in the ProfEdit tab.
void PopulationModel::onSetActivityCmd( BaseComponent* param )
{
	QString qParam = param->ToString().Str();
	if( qParam == "Eat" ) m_scheduleActivity = ScheduleActivity::Eat;
	else if( qParam == "Sleep" ) m_scheduleActivity = ScheduleActivity::Sleep;
	else if( qParam == "Train" ) m_scheduleActivity = ScheduleActivity::Training;
	else m_scheduleActivity = ScheduleActivity::None;

	m_proxy->setCurrentActivity( m_scheduleActivity );

	OnPropertyChanged( "ScheduleActivity" );
}

const char* PopulationModel::GetScheduleActivity() const
{
	switch( m_scheduleActivity )
	{
		case ScheduleActivity::Eat:
			return "E";
			break;
		case ScheduleActivity::Sleep:
			return "S";
			break;
		case ScheduleActivity::Training:
			return "T";
			break;
		default:
			return "N";
	}
}

/// @brief "Set all 24 hours to activity X for this gnome" button handler.
void PopulationModel::onSetAllHoursCmd( BaseComponent* param )
{
	m_proxy->setAllHours( QString( param->ToString().Str() ).toUInt(), m_scheduleActivity );
}

/// @brief "Set this hour to activity X on all gnomes" button handler.
void PopulationModel::onSetHoursForAllCmd( BaseComponent* param )
{
	m_proxy->setHourForAll( QString( param->ToString().Str() ).toInt(), m_scheduleActivity );
}

Noesis::ObservableCollection<ProfItem>* PopulationModel::GetProfessions() const
{
	return m_professions;
}
	
/// @brief Setter for the profession editor's profession selector. Opens the picked profession.
void PopulationModel::SetProfession( ProfItem* item )
{
	if ( item && m_selectedProfession != item )
	{
		m_selectedProfession = item;
		m_proxy->requestSkills( m_selectedProfession->GetName() );

		m_lockProfessionEdit = ( item->GetName() == QString( "Gnomad" ) );
		
		m_profName = item->GetName();

		OnPropertyChanged( "Profession" );
		OnPropertyChanged( "ProfName" );
	}
}

ProfItem* PopulationModel::GetProfession() const
{
	return m_selectedProfession;
}

/// @brief Populates the profession editor's left-side skill list for the given profession.
void PopulationModel::updateProfessionSkills( const QString profession, const QList<GuiSkillInfo>& skills )
{
	m_profSkills->Clear();

	for( const auto& skill : skills )
	{
		m_profSkills->Add( MakePtr<GnomeSkill>( skill, 0, m_proxy ) );
	}

	OnPropertyChanged( "ProfSkills" );
}

/// @brief Profession editor: moves the picked skill one position up the list.
void PopulationModel::onProfSkillUpCmd( BaseComponent* param )
{
	if( m_lockProfessionEdit ) return;
	if( param )
	{
		Noesis::String skillID = param->ToString();
		for( int i = 1; i < m_profSkills->Count(); ++i )
		{
			if( m_profSkills->Get( i )->GetID() == skillID )
			{
				m_profSkills->Move( i, i - 1 );
				break;
			}
		}
		sendModifiedProfession();
	}
}

/// @brief Profession editor: moves the picked skill one position down the list.
void PopulationModel::onProfSkillDownCmd( BaseComponent* param )
{
	if( m_lockProfessionEdit ) return;
	if( param )
	{
		Noesis::String skillID = param->ToString();
		for( int i = 0; i < m_profSkills->Count() - 1; ++i )
		{
			if( m_profSkills->Get( i )->GetID() == skillID )
			{
				m_profSkills->Move( i, i + 1 );
				break;
			}
		}
		sendModifiedProfession();
	}
}

/// @brief Profession editor: adds the picked skill from the available pool into the profession.
void PopulationModel::onProfSkillAddCmd( BaseComponent* param )
{
	if( m_lockProfessionEdit ) return;
	if( param )
	{
		Noesis::String skillID = param->ToString();
		for( int i = 0; i < m_profSkills->Count(); ++i )
		{
			if( m_profSkills->Get( i )->GetID() == skillID )
			{
				return;
			}
		}
		for( int i = 0; i < m_skills->Count(); ++i )
		{
			if( m_skills->Get( i )->GetID() == skillID )
			{
				GuiSkillInfo gsi;
				gsi.sid = skillID.Str();
				gsi.name = m_skills->Get( i )->GetName();
				m_profSkills->Add( MakePtr<GnomeSkill>( gsi, 0, m_proxy ) );
				OnPropertyChanged( "ProfSkills" );
				sendModifiedProfession();
				return;
			}
		}
	}
}

/// @brief Profession editor: removes the picked skill from the profession and returns it to the pool.
void PopulationModel::onProfSkillRemoveCmd( BaseComponent* param )
{
	if( m_lockProfessionEdit ) return;
	if( param )
	{
		Noesis::String skillID = param->ToString();
		for( int i = 0; i < m_profSkills->Count(); ++i )
		{
			if( m_profSkills->Get( i )->GetID() == skillID )
			{
				m_profSkills->RemoveAt( i );
				OnPropertyChanged( "ProfSkills" );
				sendModifiedProfession();
				return;
			}
		}
	}
}

/// @brief Flushes pending profession edits (renamed profession or skill list changes) to
///        the proxy.
void PopulationModel::sendModifiedProfession()
{
	if( m_selectedProfession )
	{
		QStringList skills;
		for( int i = 0; i < m_profSkills->Count(); ++i )
		{
			skills.append( m_profSkills->Get( i )->GetID() );
		}

		QString oldName = m_selectedProfession->GetName();
		m_selectedProfession->SetName( m_profName.Str() );
		/*
		QString name = m_selectedProfession->GetName();
		for( int i = 0; i < m_professions->Count(); ++i )
		{
			if( m_professions->Get( i )->GetName() == name )
			{
				m_professions->Get( i )->SetName( m_profName.Str() );
				break;
			}
		}
		*/
		OnPropertyChanged( "Profession" );
		OnPropertyChanged( "Professions" );

		m_proxy->updateProfession( oldName, m_profName.Str(), skills );
	}
}

const char* PopulationModel::GetProfName() const
{
	return m_profName.Str();
}

/// @brief Profession editor: sets the profession name text field and schedules a modified-flush.
void PopulationModel::SetProfName( const char* value )
{
	if( strcmp( value, m_profName.Str() ) != 0 )
	{
		QString newName = value; 
	
		bool nameExists = false;

		for( int i = 0; i < m_professions->Count(); ++i )
		{
			if( m_professions->Get( i )->GetName() == newName )
			{
				nameExists = true;
				break;
			}
		}
		m_profName = value;
	
		if( newName.length() > 3 && !nameExists )
		{
			sendModifiedProfession();
		}
		OnPropertyChanged( "ProfName" );
	}
}


/// @brief Creates a new empty profession and opens it in the editor.
void PopulationModel::onNewProfCmd( BaseComponent* param )
{
	m_proxy->newProfession();
}

/// @brief Deletes the selected profession and removes it from the dropdown.
void PopulationModel::onDeleteProfCmd( BaseComponent* param )
{
	if( m_selectedProfession )
	{
		QString name = m_selectedProfession->GetName();
		m_proxy->deleteProfession( name );
	}
}

/// @brief Switches the profession editor to show the profession named @p name.
void PopulationModel::selectEditProfession( const QString name )
{
	for( int i = 0; i < m_professions->Count(); ++i )
	{
		if( m_professions->Get( i )->GetName() == name )
		{
			SetProfession( m_professions->Get( i ) );
			break;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION


NS_IMPLEMENT_REFLECTION( ProfItem )
{
	NsProp( "Name", &ProfItem::GetName );
}

NS_IMPLEMENT_REFLECTION( PopulationModel, "IngnomiaGUI.PopulationModel" )
{
	NsProp( "Gnomes", &PopulationModel::GetGnomes );
	NsProp( "SkillHeaders", &PopulationModel::GetSkills );
	NsProp( "CmdAllGnomes", &PopulationModel::GetAllGnomesCmd );
	NsProp( "CmdRemoveAllGnomes", &PopulationModel::GetRemoveAllGnomesCmd );
	NsProp( "CmdAllSkills", &PopulationModel::GetAllSkillsCmd );
	NsProp( "CmdRemoveAllSkills", &PopulationModel::GetRemoveAllSkillsCmd );
	NsProp( "CmdSort", &PopulationModel::GetSortCmd );
	NsProp( "ShowSkills", &PopulationModel::GetShowSkills );
	NsProp( "ShowSchedule", &PopulationModel::GetShowSchedule );
	NsProp( "ShowProfEdit", &PopulationModel::GetShowProfEdit );
	NsProp( "PageCmd", &PopulationModel::GetPageCmd );
	NsProp( "Schedules", &PopulationModel::GetScheduleGnomes );
	NsProp( "CmdSetActivityType", &PopulationModel::GetSetActivityCmd );
	NsProp( "ScheduleActivity", &PopulationModel::GetScheduleActivity );
	NsProp( "CmdAllHours", &PopulationModel::GetSetAllHoursCmd );
	NsProp( "CmdSetHourForAll", &PopulationModel::GetSetHoursForAllCmd );
	NsProp( "Professions", &PopulationModel::GetProfessions );
	NsProp( "Profession", &PopulationModel::GetProfession, &PopulationModel::SetProfession );
	NsProp( "AllProfSkills", &PopulationModel::GetSkills );
	NsProp( "ProfSkills", &PopulationModel::GetProfessionSkills );

	NsProp( "CmdSkillUp", &PopulationModel::GetProfSkillUpCmd );
	NsProp( "CmdSkillDown", &PopulationModel::GetProfSkillDownCmd );
	NsProp( "CmdSkillAdd", &PopulationModel::GetProfSkillAddCmd );
	NsProp( "CmdSkillRemove", &PopulationModel::GetProfSkillRemoveCmd );
	NsProp( "ProfName", &PopulationModel::GetProfName, &PopulationModel::SetProfName );

	NsProp( "CmdProfNew", &PopulationModel::GetNewProfCmd );
	NsProp( "CmdProfDelete", &PopulationModel::GetDeleteProfCmd );
}

NS_IMPLEMENT_REFLECTION( GnomeRow )
{
	NsProp( "Name", &GnomeRow::GetName );
	NsProp( "GnomeID", &GnomeRow::GetID );
	NsProp( "Skills", &GnomeRow::GetSkills );
	NsProp( "Professions", &GnomeRow::GetProfessions );
	NsProp( "Profession", &GnomeRow::GetProfession, &GnomeRow::SetProfession );
}

NS_IMPLEMENT_REFLECTION( GnomeSkill )
{
	NsProp( "Name", &GnomeSkill::GetName );
	NsProp( "SkillID", &GnomeSkill::GetID );
	NsProp( "Level", &GnomeSkill::GetLevel );
	NsProp( "Checked", &GnomeSkill::GetChecked, &GnomeSkill::SetChecked );
	NsProp( "Color", &GnomeSkill::GetColor );
}

NS_IMPLEMENT_REFLECTION( GnomeScheduleRow )
{
	NsProp( "Name", &GnomeScheduleRow::GetName );
	NsProp( "GnomeID", &GnomeScheduleRow::GetID );
	NsProp( "Schedule", &GnomeScheduleRow::GetSchedule );
	
}

NS_IMPLEMENT_REFLECTION( GnomeScheduleEntry )
{
	NsProp( "Activity", &GnomeScheduleEntry::GetActivity );
	NsProp( "CmdSetHour", &GnomeScheduleEntry::GetSetHourCmd );
}
