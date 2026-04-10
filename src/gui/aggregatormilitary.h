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
/** @file aggregatormilitary.h
 *  @brief Data types and aggregator feeding the Military XAML window with squads, roles,
 *         target priorities, and uniform configuration.
 */
#pragma once

#include "../game/militarymanager.h"


#include <QObject>

class Game;

/// @brief Gnome shown in the squad roster.
struct GuiSquadGnome
{
	unsigned int id = 0;    ///< Creature UID.
	QString name;           ///< Display name.

	unsigned int roleID = 0;///< Assigned military role UID.
};
Q_DECLARE_METATYPE( GuiSquadGnome )

/// @brief Target-priority row in the squad engagement rules.
struct GuiTargetPriority
{
	QString id;             ///< Creature type string ID targeted by this row.
	QString name;           ///< Localised display name.
	MilAttitude attitude;   ///< Attack / defend / flee attitude.
};
Q_DECLARE_METATYPE( GuiTargetPriority )



/// @brief One squad shown in the Military window, with its priority list and gnome roster.
struct GuiSquad
{
	unsigned int id = 0;       ///< Squad UID.
	QString name;              ///< Display name.
	bool showLeftArrow = true; ///< Enable left (move-earlier) button in GUI.
	bool showRightArrow = true;///< Enable right (move-later) button in GUI.

	QList<GuiTargetPriority> priorities; ///< Engagement priority list for this squad.

	QList<GuiSquadGnome> gnomes;         ///< Gnomes currently assigned to this squad.
};
Q_DECLARE_METATYPE( GuiSquad )

/// @brief One uniform slot entry (per body part) inside a military role.
struct GuiUniformItem
{
	QString slotName;                    ///< Slot key (HeadArmor, ChestArmor, …).
	QString armorType = "none";          ///< plate, heavy plate, chain, etc.
	QString material = "any";            ///< Required material, or "any".

	QStringList possibleTypesForSlot;    ///< Legal armor types for this slot (e.g. no bone for hands).
};
Q_DECLARE_METATYPE( GuiUniformItem )

/// @brief A military role definition (e.g. "Swordsman") with its uniform configuration.
struct GuiMilRole
{
	unsigned int id = 0;                 ///< Role UID.
	QString name;                        ///< Display name.
	bool showLeftArrow = false;          ///< Enable reorder-left button in GUI.
	bool showRightArrow = false;         ///< Enable reorder-right button in GUI.

	bool isCivilian;                     ///< True if this role is a civilian (non-combat) role.

	QList<GuiUniformItem> uniform;       ///< Per-slot uniform entries.
};
Q_DECLARE_METATYPE( GuiMilRole )






/// @brief Bridges the Military XAML window with the game-side MilitaryManager. Produces
///        GuiSquad/GuiMilRole lists for display and routes squad/role edits back to the game.
class AggregatorMilitary : public QObject
{
	Q_OBJECT

public:
	AggregatorMilitary( QObject* parent = nullptr );
	~AggregatorMilitary();

	void init( Game* game );

private:
	QPointer<Game> g;                         ///< Game instance (weak ownership).

	QList<GuiSquad> m_squads;                 ///< Cached squad list for the GUI.
	QList<GuiTargetPriority> m_tmpPriorities; ///< Scratch buffer reused when emitting priority updates.
	QList<GuiMilRole> m_roles;                ///< Cached military role list for the GUI.
	
	void sendSquadUpdate();
	void sendPriorityUpdate( unsigned int squadID );
	void sendRoleUpdate();

	GuiUniformItem createUniformItem( const QString slot, QVariantMap uniVM );

public slots:
	void onRequestMilitary();
	void onRequestRoles();

	void onAddSquad();
	void onRemoveSquad( unsigned int id );
	void onRenameSquad( unsigned int id, QString newName );
	void onMoveSquadLeft( unsigned int id );
	void onMoveSquadRight( unsigned int id );
	void onRemoveGnomeFromSquad( unsigned int gnomeID );
	void onMoveGnomeLeft( unsigned int id );
	void onMoveGnomeRight( unsigned int id );

	void onMovePrioUp( unsigned int squadID, QString type );
	void onMovePrioDown( unsigned int squadID, QString type );

	void onAddRole();
	void onRemoveRole( unsigned int id );
	void onRenameRole( unsigned int id, QString newName );
	void onSetRole( unsigned int gnomeID, unsigned int roleID );
	void onSetRoleCivilian( unsigned int roleID, bool value );

	void onSetArmorType( unsigned int roleID, QString slot, QString type, QString material );

	void onSetAttitude( unsigned int squadID, QString type, MilAttitude attitude );

signals:
	void signalSquads( const QList<GuiSquad>& squads );
	void signalPriorities( unsigned int squadID, const QList<GuiTargetPriority>& priorities );
	void signalRoles( const QList<GuiMilRole>& roles );
	void signalPossibleMaterials( unsigned int roleID, const QString slot, const QStringList mats );
};
