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
/** @file aggregatorcreatureinfo.h
 *  @brief Data types and aggregator feeding the Creature Info XAML window with per-gnome
 *         attributes, needs, profession, activity, and equipment icons.
 */
#pragma once

#include "../game/creature.h"

#include <QObject>

#include "../game/gnome.h"
#include "../game/militarymanager.h"

class Game;

/// @brief Creature-info payload sent to the GUI for display on the selected gnome.
struct GuiCreatureInfo
{
	QString name;           ///< Gnome display name.
	unsigned int id = 0;    ///< Creature UID.
	QString profession;     ///< Current profession name.
	int str = 0;            ///< Strength attribute.
	int dex = 0;            ///< Dexterity attribute.
	int con = 0;            ///< Constitution attribute.
	int intel = 0;          ///< Intelligence attribute.
	int wis = 0;            ///< Wisdom attribute.
	int cha = 0;            ///< Charisma attribute.
	int hunger = 0;         ///< Current hunger level.
	int thirst = 0;         ///< Current thirst level.
	int sleep = 0;          ///< Current sleep level.
	int happiness = 0;      ///< Current happiness level.

	QString activity;       ///< Short description of what the gnome is doing right now.

	Uniform uniform;        ///< Current military uniform (empty if unassigned).
	Equipment equipment;    ///< Currently worn equipment.

	QMap< QString, std::vector<unsigned char> > itemPics; ///< Per-equipment-slot encoded PNG bytes for the GUI icons.
};
Q_DECLARE_METATYPE( GuiCreatureInfo )


/// @brief Aggregates live creature state for the Creature Info XAML window. Produces PNG icon
///        bytes for equipment/uniform slots so the Noesis view model can display them.
class AggregatorCreatureInfo : public QObject
{
	Q_OBJECT

public:
	AggregatorCreatureInfo( QObject* parent = nullptr );

	void init( Game* game );

	void update();

private:
	QPointer<Game> g;                                     ///< Game instance (weak ownership).

	GuiCreatureInfo m_info;                               ///< Cached payload for the currently viewed creature.
	QMap< QString, std::vector<unsigned char> > m_emptyPics; ///< Cached blank-slot PNGs keyed by uniform slot name.

	unsigned int m_currentID = 0;                         ///< Creature currently shown in the GUI.
	unsigned int m_previousID = 0;                        ///< Previously shown creature (unused but reserved).

	void createItemImg( QString slot, EquipmentItem& eItem );
	void createUniformImg( QString slot, const UniformItem& uItem, EquipmentItem& eItem );
	void createEmptyUniformImg( QString spriteID );

public slots:
	void onRequestCreatureUpdate( unsigned int creatureID );
	void onRequestProfessionList();
	void onSetProfession( unsigned int gnomeID, QString profession );

	void onRequestEmptySlotImages();

signals:
	void signalCreatureUpdate( const GuiCreatureInfo& info );
	void signalProfessionList( const QStringList& profs );
	
	void signalEmptyPics( const QMap< QString, std::vector<unsigned char> >& emptyPics );

};