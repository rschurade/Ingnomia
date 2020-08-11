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
#ifndef __PopulationModel_H__
#define __PopulationModel_H__

#include "../aggregatorpopulation.h"

#include <NsApp/DelegateCommand.h>
#include <NsApp/NotifyPropertyChangedBase.h>
#include <NsCore/Noesis.h>
#include <NsCore/Ptr.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/ReflectionDeclareEnum.h>
#include <NsCore/String.h>
#include <NsGui/Collection.h>


#include <QString>

class PopulationProxy;

namespace Noesis
{
template <class T>
class ObservableCollection;
}

namespace IngnomiaGUI
{

enum class PopState
{
	Skills,
	Schedule,
	ProfEdit
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class ProfItem final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	ProfItem( QString name );

	const char* GetName() const;
	void SetName( const char* name );

private:
	Noesis::String m_name;

	NS_DECLARE_REFLECTION( ProfItem, NoesisApp::NotifyPropertyChangedBase )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class GnomeSkill : public Noesis::BaseComponent
{
public:
	GnomeSkill( const GuiSkillInfo& skill, unsigned int gnomeID, PopulationProxy* proxy );

	const char* GetName() const;
	const char* GetID() const;
	bool GetChecked() const;
	void SetChecked( bool value );
	const char* GetLevel() const;
	const char* GetColor() const;

private:
	Noesis::String m_name;
	Noesis::String m_level = "-1";
	bool m_checked         = false;
	unsigned int m_gnomeID = 0;
	Noesis::String m_color;
	Noesis::String m_skillID;

	PopulationProxy* m_proxy = nullptr;

	NS_DECLARE_REFLECTION( GnomeSkill, Noesis::BaseComponent )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class GnomeRow : public NoesisApp::NotifyPropertyChangedBase
{
public:
	GnomeRow( const GuiGnomeInfo& gnome, Noesis::Ptr<Noesis::ObservableCollection<ProfItem>> professions, PopulationProxy* proxy );

	const char* GetName() const;
	const char* GetID() const;

	unsigned int gnomeID() { return m_id; }

private:
	Noesis::String m_name;
	unsigned int m_id;
	Noesis::String m_idString;

	Noesis::ObservableCollection<GnomeSkill>* GetSkills() const
	{
		return m_skills;
	}
	Noesis::Ptr<Noesis::ObservableCollection<GnomeSkill>> m_skills;

	Noesis::ObservableCollection<ProfItem>* GetProfessions() const;
	void SetProfession( ProfItem* item );
	ProfItem* GetProfession() const;
	Noesis::Ptr<Noesis::ObservableCollection<ProfItem>> m_professions;
	ProfItem* m_selectedProfession = nullptr;


	PopulationProxy* m_proxy = nullptr;

	NS_DECLARE_REFLECTION( GnomeRow, NoesisApp::NotifyPropertyChangedBase )
};


////////////////////////////////////////////////////////////////////////////////////////////////////
class GnomeScheduleEntry : public NoesisApp::NotifyPropertyChangedBase
{
public:
	GnomeScheduleEntry( unsigned int id, int hour, ScheduleActivity activity, PopulationProxy* proxy );

	PopulationProxy* m_proxy = nullptr;

	const char* GetActivity() const;
	
private:
	ScheduleActivity m_activity;
	Noesis::String m_activityString;
	unsigned int m_id = 0;
	int m_hour = 0;

	void onSetHourCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetSetHourCmd() const
	{
		return &m_setHourCmd;
	}
	NoesisApp::DelegateCommand m_setHourCmd;

NS_DECLARE_REFLECTION( GnomeScheduleEntry, NoesisApp::NotifyPropertyChangedBase )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class GnomeScheduleRow : public NoesisApp::NotifyPropertyChangedBase
{
public:
	GnomeScheduleRow( const GuiGnomeScheduleInfo& gnome, PopulationProxy* proxy );

	const char* GetName() const;
	const char* GetID() const;

	unsigned int gnomeID() { return m_id; }

private:
	Noesis::String m_name;
	unsigned int m_id;
	Noesis::String m_idString;

	Noesis::ObservableCollection<GnomeScheduleEntry>* GetSchedule() const
	{
		return m_schedule;
	}
	Noesis::Ptr<Noesis::ObservableCollection<GnomeScheduleEntry>> m_schedule;

	PopulationProxy* m_proxy = nullptr;

	NS_DECLARE_REFLECTION( GnomeScheduleRow, NoesisApp::NotifyPropertyChangedBase )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class PopulationModel final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	PopulationModel();

	void updateInfo( const GuiPopulationInfo& info );
	void updateProfessionList( const QStringList& professions );
	void updateProfessionSkills( const QString profession, const QList<GuiSkillInfo>& skills );
	void updateSingleGnome( const GuiGnomeInfo& gnome );

	void updateSchedules( const GuiScheduleInfo& info );
	void updateScheduleSingleGnome( const GuiGnomeScheduleInfo& info );

	void selectEditProfession( const QString name );

private:
	PopulationProxy* m_proxy = nullptr;
	Noesis::Ptr<Noesis::ObservableCollection<ProfItem>> m_professions;
	ProfItem* m_selectedProfession = nullptr;

	Noesis::ObservableCollection<GnomeRow>* GetGnomes() const
	{
		return m_gnomes;
	}
	Noesis::Ptr<Noesis::ObservableCollection<GnomeRow>> m_gnomes;

	Noesis::ObservableCollection<GnomeSkill>* GetSkills() const
	{
		return m_skills;
	}
	Noesis::Ptr<Noesis::ObservableCollection<GnomeSkill>> m_skills;

	Noesis::ObservableCollection<GnomeSkill>* GetProfessionSkills() const
	{
		return m_profSkills;
	}
	Noesis::Ptr<Noesis::ObservableCollection<GnomeSkill>> m_profSkills;


	void onAllGnomesCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetAllGnomesCmd() const
	{
		return &m_allGnomesCmd;
	}
	NoesisApp::DelegateCommand m_allGnomesCmd;

	void onRemoveAllGnomesCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetRemoveAllGnomesCmd() const
	{
		return &m_removeAllGnomesCmd;
	}
	NoesisApp::DelegateCommand m_removeAllGnomesCmd;

	void onAllSkillsCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetAllSkillsCmd() const
	{
		return &m_allSkillsCmd;
	}
	NoesisApp::DelegateCommand m_allSkillsCmd;

	void onRemoveAllSkillsCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetRemoveAllSkillsCmd() const
	{
		return &m_removeAllSkillsCmd;
	}
	NoesisApp::DelegateCommand m_removeAllSkillsCmd;

	void onSortCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetSortCmd() const
	{
		return &m_sortCmd;
	}
	NoesisApp::DelegateCommand m_sortCmd;

	void onPageCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetPageCmd() const
	{
		return &m_pageCmd;
	}
	NoesisApp::DelegateCommand m_pageCmd;

	void onSetActivityCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetSetActivityCmd() const
	{
		return &m_setActivityCmd;
	}
	NoesisApp::DelegateCommand m_setActivityCmd;

	const char* GetShowSkills() const;
	const char* GetShowSchedule() const;
	const char* GetShowProfEdit() const;

	PopState m_state = PopState::Skills;

	ScheduleActivity m_scheduleActivity = ScheduleActivity::None;

	Noesis::ObservableCollection<GnomeScheduleRow>* GetScheduleGnomes() const
	{
		return m_scheduleGnomes;
	}
	Noesis::Ptr<Noesis::ObservableCollection<GnomeScheduleRow>> m_scheduleGnomes;

	const char* GetScheduleActivity() const;

	void onSetAllHoursCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetSetAllHoursCmd() const
	{
		return &m_setAllHoursCmd;
	}
	NoesisApp::DelegateCommand m_setAllHoursCmd;

	void onSetHoursForAllCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetSetHoursForAllCmd() const
	{
		return &m_setHoursForAllCmd;
	}
	NoesisApp::DelegateCommand m_setHoursForAllCmd;


	Noesis::ObservableCollection<ProfItem>* GetProfessions() const;
	void SetProfession( ProfItem* item );
	ProfItem* GetProfession() const;

	void onProfSkillUpCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetProfSkillUpCmd() const
	{
		return &m_profSkillUpCmd;
	}
	NoesisApp::DelegateCommand m_profSkillUpCmd;

	void onProfSkillDownCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetProfSkillDownCmd() const
	{
		return &m_profSkillDownCmd;
	}
	NoesisApp::DelegateCommand m_profSkillDownCmd;

	void onProfSkillAddCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetProfSkillAddCmd() const
	{
		return &m_profSkillAddCmd;
	}
	NoesisApp::DelegateCommand m_profSkillAddCmd;

	void onProfSkillRemoveCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetProfSkillRemoveCmd() const
	{
		return &m_profSkillRemoveCmd;
	}
	NoesisApp::DelegateCommand m_profSkillRemoveCmd;

	bool m_lockProfessionEdit = false;
	void sendModifiedProfession();

	Noesis::String m_profName;
	const char* GetProfName() const;
	void SetProfName( const char* value );


	void onNewProfCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetNewProfCmd() const
	{
		return &m_newProfCmd;
	}
	NoesisApp::DelegateCommand m_newProfCmd;

	void onDeleteProfCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetDeleteProfCmd() const
	{
		return &m_deleteProfCmd;
	}
	NoesisApp::DelegateCommand m_deleteProfCmd;


	NS_DECLARE_REFLECTION( PopulationModel, NotifyPropertyChangedBase )
};

} // namespace IngnomiaGUI

#endif
