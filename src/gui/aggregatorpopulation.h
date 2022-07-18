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

#include "../game/creature.h"

#include <QObject>

#include <sigslot/signal.hpp>

class Game;

struct GuiSkillInfo
{
	std::string sid;
	QString name;
	int level;
	float xpValue;
	bool active;
	QString group;
	QString color;
};
Q_DECLARE_METATYPE( GuiSkillInfo )


struct GuiGnomeInfo
{
	QString name;
	unsigned int id;
	QList<GuiSkillInfo> skills;
	std::string profession;
};
Q_DECLARE_METATYPE( GuiGnomeInfo )

struct GuiPopulationInfo
{
	QList<GuiGnomeInfo> gnomes;
};
Q_DECLARE_METATYPE( GuiPopulationInfo )

struct GuiGnomeScheduleInfo
{
	QString name;
	unsigned int id;
	QList<ScheduleActivity> schedule;
};
Q_DECLARE_METATYPE( GuiGnomeScheduleInfo )

struct GuiScheduleInfo
{
	QList<GuiGnomeScheduleInfo> schedules;
};
Q_DECLARE_METATYPE( GuiScheduleInfo )



class AggregatorPopulation : public QObject
{
	Q_OBJECT

public:
	AggregatorPopulation( QObject* parent = nullptr );

	void init( Game* game );

private:
	QPointer<Game> g;

	GuiPopulationInfo m_populationInfo;

	GuiScheduleInfo m_scheduleInfo;

	QList<GuiSkillInfo> m_skillIds;
	
	QList<GuiSkillInfo> m_profSkills;

	std::string m_sortMode = "Name";
	bool m_revertSort = false;

public slots:
	void onRequestPopulationUpdate();
	void onUpdateSingleGnome( unsigned int gnomeID );
	void onSetSkillActive( unsigned int gnomeID, const std::string& skillID, bool value );
	void onSetAllSkills( unsigned int gnomeID, bool value );
	void onSetAllGnomes( const std::string& skillID, bool value );
	void onSetProfession( unsigned int gnomeID, const std::string& profession );
	void onSortGnomes( const std::string& mode );

	void onRequestSchedules();
	void onSetSchedule( unsigned int gnomeID, int hour, ScheduleActivity activity );
	void onSetAllHours( unsigned int gnomeID, ScheduleActivity activity );
	void onSetHourForAll( int hour, ScheduleActivity activity );

	void onRequestProfessions();
	void onRequestSkills( const std::string& profession );

	void onUpdateProfession( const std::string& name, const std::string& newName, const std::vector<std::string>& skills );
	void onDeleteProfession( const std::string& name );
	void onNewProfession();
public: // signals:
	sigslot::signal<const GuiPopulationInfo& /*info*/> signalPopulationUpdate;
	sigslot::signal<const std::vector<std::string>& /*professions*/> signalProfessionList;
	sigslot::signal<const std::string& /*profession*/, const QList<GuiSkillInfo>& /*skills*/> signalProfessionSkills;

	sigslot::signal<const GuiGnomeInfo& /*gnome*/> signalUpdateSingleGnome;

	sigslot::signal<const GuiScheduleInfo& /*info*/> signalScheduleUpdate;
	sigslot::signal<const GuiGnomeScheduleInfo& /*info*/> signalScheduleUpdateSingleGnome;
	sigslot::signal<const std::string& /*name*/> signalSelectEditProfession;
};