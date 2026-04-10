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
/** @file aggregatorpopulation.h
 *  @brief Data types and aggregator feeding the Population XAML window with per-gnome skills,
 *         schedules, and profession management.
 */
#pragma once

#include "../game/creature.h"

#include <QObject>

class Game;

/// @brief One skill row in the population grid.
struct GuiSkillInfo
{
	QString sid;       ///< Skill string ID.
	QString name;      ///< Localised name.
	int level;         ///< Current level.
	float xpValue;     ///< Progress into the next level.
	bool active;       ///< True if the gnome may take jobs using this skill.
	QString group;     ///< Skill group (Combat, Crafting, …).
	QString color;     ///< GUI color key associated with the group.
};
Q_DECLARE_METATYPE( GuiSkillInfo )


/// @brief One gnome row in the population grid with its skill breakdown.
struct GuiGnomeInfo
{
	QString name;                  ///< Display name.
	unsigned int id;               ///< Creature UID.
	QList<GuiSkillInfo> skills;    ///< Per-skill level and activation.
	QString profession;            ///< Currently assigned profession.
};
Q_DECLARE_METATYPE( GuiGnomeInfo )

/// @brief Wrapper holding the complete population list for the GUI.
struct GuiPopulationInfo
{
	QList<GuiGnomeInfo> gnomes;    ///< All live gnomes.
};
Q_DECLARE_METATYPE( GuiPopulationInfo )

/// @brief One gnome's daily schedule as shown in the Schedule tab.
struct GuiGnomeScheduleInfo
{
	QString name;                           ///< Display name.
	unsigned int id;                        ///< Creature UID.
	QList<ScheduleActivity> schedule;       ///< 24-hour activity array.
};
Q_DECLARE_METATYPE( GuiGnomeScheduleInfo )

/// @brief Wrapper holding all gnome schedules for the GUI.
struct GuiScheduleInfo
{
	QList<GuiGnomeScheduleInfo> schedules;
};
Q_DECLARE_METATYPE( GuiScheduleInfo )



/// @brief Bridges the Population XAML window with the game: builds the gnome grid, handles
///        skill/profession edits, and manages per-gnome daily schedules.
class AggregatorPopulation : public QObject
{
	Q_OBJECT

public:
	AggregatorPopulation( QObject* parent = nullptr );

	void init( Game* game );

private:
	QPointer<Game> g;                        ///< Game instance (weak ownership).

	GuiPopulationInfo m_populationInfo;      ///< Cached population grid.

	GuiScheduleInfo m_scheduleInfo;          ///< Cached schedule table.

	QList<GuiSkillInfo> m_skillIds;          ///< All skills from DB (template list).

	QList<GuiSkillInfo> m_profSkills;        ///< Skills selected while editing a profession.

	QString m_sortMode = "Name";             ///< Current sort column for the population grid.
	bool m_revertSort = false;               ///< True if the current sort is descending.

public slots:
	void onRequestPopulationUpdate();
	void onUpdateSingleGnome( unsigned int gnomeID );
	void onSetSkillActive( unsigned int gnomeID, QString skillID, bool value );
	void onSetAllSkills( unsigned int gnomeID, bool value );
	void onSetAllGnomes( QString skillID, bool value );
	void onSetProfession( unsigned int gnomeID, QString profession );
	void onSortGnomes( QString mode );

	void onRequestSchedules();
	void onSetSchedule( unsigned int gnomeID, int hour, ScheduleActivity activity );
	void onSetAllHours( unsigned int gnomeID, ScheduleActivity activity );
	void onSetHourForAll( int hour, ScheduleActivity activity );

	void onRequestProfessions();
	void onRequestSkills( QString profession );

	void onUpdateProfession( QString name, QString newName, QStringList skills );
	void onDeleteProfession( QString name );
	void onNewProfession();
signals:
	void signalPopulationUpdate( const GuiPopulationInfo& info );
	void signalProfessionList( const QStringList& professions );
	void signalProfessionSkills( QString profession, const QList<GuiSkillInfo>& skills );

	void signalUpdateSingleGnome( const GuiGnomeInfo& gnome );

	void signalScheduleUpdate( const GuiScheduleInfo& info );
	void signalScheduleUpdateSingleGnome( const GuiGnomeScheduleInfo& info );
	void signalSelectEditProfession( const QString name );
};