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

#include "../aggregatorpopulation.h"
#include "PopulationModel.h"

#include <QObject>

#include <sigslot/signal.hpp>

class PopulationProxy : public QObject
{
	Q_OBJECT

public:
	PopulationProxy( QObject* parent = nullptr );
	~PopulationProxy();

	void setParent( IngnomiaGUI::PopulationModel* parent );
	void setSkillActive( unsigned int gnomeID, const std::string& skillID, bool value );

	void setAllSkillsForGnome( unsigned int gnomeID, bool value );
	void setSkillForAllGnomes( const std::string& skillID, bool value );

	void setProfession( unsigned int gnomeID, const std::string& profession );

	void sortGnomes( const std::string& mode );

	void requestSchedules();
	void requestProfessions();
	void requestSkills( const std::string& profession );

	void setAllHours( unsigned int gnomeID, ScheduleActivity activitiy );
	void setHourForAll( int hour, ScheduleActivity activitiy );
	void setSchedule( unsigned int gnomeID, int hour, ScheduleActivity activitiy );

	void setCurrentActivity( ScheduleActivity activitiy );

	void updateProfession( const std::string& name, const std::string& newName, const std::vector<std::string>& skills );
	void deleteProfession( const std::string& name );
	void newProfession();

private:
	IngnomiaGUI::PopulationModel* m_parent = nullptr;
	ScheduleActivity m_currentActivitiy = ScheduleActivity::None;

private slots:
	void onUpdateInfo( const GuiPopulationInfo& info );
	void onProfessionList( const std::vector<std::string>& professions );
	void onProfessionSkills( const std::string& profession, const QList<GuiSkillInfo>& skills );
	void onUpdateSingleGnome( const GuiGnomeInfo& gnome );

	void onUpdateSchedules( const GuiScheduleInfo& info );
	void onScheduleUpdateSingleGnome( const GuiGnomeScheduleInfo& info );
	void onSelectEditProfession( const std::string& name );

public: // signals:
	sigslot::signal<unsigned int /*gnomeID*/, const std::string& /*skillID*/, bool /*value*/> signalSetSkillActive;

	sigslot::signal<unsigned int /*gnomeID*/, bool /*value*/> signalSetAllSkills;
	sigslot::signal<const std::string& /*skillID*/, bool /*value*/> signalSetAllGnomes;
	sigslot::signal<unsigned int /*gnomeID*/, const std::string& /*profession*/> signalSetProfession;
	sigslot::signal<const std::string& /*mode*/> signalSortGnomes;

	sigslot::signal<> signalRequestSchedules;
	sigslot::signal<> signalRequestProfessions;
	sigslot::signal<const std::string& /*profession*/> signalRequestSkills;
	sigslot::signal<unsigned int /*gnomeID*/, int /*hour*/, ScheduleActivity /*activitiy*/> signalSetSchedule;
	sigslot::signal<unsigned int /*gnomeID*/, ScheduleActivity /*activitiy*/> signalSetAllHours;
	sigslot::signal<int /*hour*/, ScheduleActivity /*activitiy*/> signalSetHourForAll;

	sigslot::signal<const std::string& /*name*/, const std::string& /*newName*/, const std::vector<std::string>& /*skills*/> signalUpdateProfession;
	sigslot::signal<const std::string& /*name*/> signalDeleteProfession;
	sigslot::signal<> signalNewProfession;
};
