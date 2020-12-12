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

#include "../game/militarymanager.h"


#include <QObject>

class Game;

struct GuiSquadGnome
{
	unsigned int id = 0;
	QString name;

	unsigned int roleID = 0;
};
Q_DECLARE_METATYPE( GuiSquadGnome )

struct GuiTargetPriority
{
	QString id;
	QString name;
	MilAttitude attitude;
};
Q_DECLARE_METATYPE( GuiTargetPriority )



struct GuiSquad
{
	unsigned int id = 0;
	QString name;
	bool showLeftArrow = true;
	bool showRightArrow = true;

	QList<GuiTargetPriority> priorities;

	QList<GuiSquadGnome> gnomes;
};
Q_DECLARE_METATYPE( GuiSquad )

struct GuiUniformItem
{
	QString slotName;
	QString armorType = "none"; // plate, heavy plate, chain and so on
	QString material = "any";

	QStringList possibleTypesForSlot; // some slots have no bone armor item, held slots or rings are completely different
};
Q_DECLARE_METATYPE( GuiUniformItem )

struct GuiMilRole
{
	unsigned int id = 0;
	QString name;
	bool showLeftArrow = false;
	bool showRightArrow = false;

	bool isCivilian;

	QList<GuiUniformItem> uniform;
};
Q_DECLARE_METATYPE( GuiMilRole )






class AggregatorMilitary : public QObject
{
	Q_OBJECT

public:
	AggregatorMilitary( QObject* parent = nullptr );
	~AggregatorMilitary();

	void init( Game* game );

private:
	QPointer<Game> g;

	QList<GuiSquad> m_squads;
	QList<GuiTargetPriority> m_tmpPriorities;
	QList<GuiMilRole> m_roles;
	
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
