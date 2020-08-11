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

signals:
	void signalSetSkillActive( unsigned int gnomeID, QString skillID, bool value );

	void signalSetAllSkills( unsigned int gnomeID, bool value );
	void signalSetAllGnomes( QString skillID, bool value );
	void signalSetProfession( unsigned int gnomeID, QString profession );
	void signalSortGnomes( QString mode );

	void signalRequestSchedules();
	void signalRequestProfessions();
	void signalRequestSkills( QString profession );
	void signalSetSchedule( unsigned int gnomeID, int hour, ScheduleActivity activitiy );
	void signalSetAllHours( unsigned int gnomeID, ScheduleActivity activitiy );
	void signalSetHourForAll( int hour, ScheduleActivity activitiy );

	void signalUpdateProfession( QString name, QString newName, QStringList skills );
	void signalDeleteProfession( QString name );
	void signalNewProfession();
};
