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
	void setSkillActive( unsigned int gnomeID, QString skillID, bool value );

	void setAllSkillsForGnome( unsigned int gnomeID, bool value );
	void setSkillForAllGnomes( QString skillID, bool value );

	void setProfession( unsigned int gnomeID, QString profession );

	void sortGnomes( QString mode );

	void requestSchedules();
	void requestProfessions();
	void requestSkills( QString profession );

	void setAllHours( unsigned int gnomeID, ScheduleActivity activitiy );
	void setHourForAll( int hour, ScheduleActivity activitiy );
	void setSchedule( unsigned int gnomeID, int hour, ScheduleActivity activitiy );

	void setCurrentActivity( ScheduleActivity activitiy );

	void updateProfession( QString name, QString newName, QStringList skills );
	void deleteProfession( QString name );
	void newProfession();

private:
	IngnomiaGUI::PopulationModel* m_parent = nullptr;
	ScheduleActivity m_currentActivitiy = ScheduleActivity::None;

private slots:
	void onUpdateInfo( const GuiPopulationInfo& info );
	void onProfessionList( const QStringList& professions );
	void onProfessionSkills( const QString profession, const QList<GuiSkillInfo>& skills );
	void onUpdateSingleGnome( const GuiGnomeInfo& gnome );

	void onUpdateSchedules( const GuiScheduleInfo& info );
	void onScheduleUpdateSingleGnome( const GuiGnomeScheduleInfo& info );
	void onSelectEditProfession( const QString name );

public: // signals:
	sigslot::signal<unsigned int /*gnomeID*/, QString /*skillID*/, bool /*value*/> signalSetSkillActive;

	sigslot::signal<unsigned int /*gnomeID*/, bool /*value*/> signalSetAllSkills;
	sigslot::signal<QString /*skillID*/, bool /*value*/> signalSetAllGnomes;
	sigslot::signal<unsigned int /*gnomeID*/, QString /*profession*/> signalSetProfession;
	sigslot::signal<QString /*mode*/> signalSortGnomes;

	sigslot::signal<> signalRequestSchedules;
	sigslot::signal<> signalRequestProfessions;
	sigslot::signal<QString /*profession*/> signalRequestSkills;
	sigslot::signal<unsigned int /*gnomeID*/, int /*hour*/, ScheduleActivity /*activitiy*/> signalSetSchedule;
	sigslot::signal<unsigned int /*gnomeID*/, ScheduleActivity /*activitiy*/> signalSetAllHours;
	sigslot::signal<int /*hour*/, ScheduleActivity /*activitiy*/> signalSetHourForAll;

	sigslot::signal<QString /*name*/, QString /*newName*/, QStringList /*skills*/> signalUpdateProfession;
	sigslot::signal<QString /*name*/> signalDeleteProfession;
	sigslot::signal<> signalNewProfession;
};
