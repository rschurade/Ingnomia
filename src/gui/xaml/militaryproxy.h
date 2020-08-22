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

#include "../aggregatormilitary.h"
#include "militarymodel.h"

#include <QObject>

class MilitaryProxy : public QObject
{
	Q_OBJECT

public:
	MilitaryProxy( QObject* parent = nullptr );
	void setParent( IngnomiaGUI::MilitaryModel* parent );

	void addSquad();
	void removeSquad( unsigned int id );
	void renameSquad( unsigned int id, QString newName );
	void moveSquadLeft( unsigned int id );
	void moveSquadRight( unsigned int id );
	void removeGnomeFromSquad( unsigned int gnomeID );
	void moveGnomeLeft( unsigned int id );
	void moveGnomeRight( unsigned int id );
	void movePrioUp( unsigned int squadID, QString type );
	void movePrioDown( unsigned int squadID, QString type );

	void setAttitude( unsigned int squadID, QString type, MilAttitude attitude );

	void requestRoles();
	void addRole();
	void removeRole( unsigned int id );
	void renameRole( unsigned int id, QString newName );

	void setArmorType( unsigned int roleID, QString slot, QString type, QString material );
	void setRole( unsigned int gnomeID, unsigned int roleID );

	void setRoleCivilian( unsigned int roleID, bool value );

private:
	IngnomiaGUI::MilitaryModel* m_parent = nullptr;



private slots:
	void onSquads( const QList<GuiSquad>& squads );
	void onPriorities( unsigned int squadID, const QList<GuiTargetPriority>& priorities );
	void onRoles( const QList<GuiMilRole> roles );
	void onPossibleMaterials( unsigned int roleID, const QString slot, const QStringList mats );

signals:
	void signalAddSquad();
	void signalRemoveSquad( unsigned int id );
	void signalRenameSquad( unsigned int id, QString newName );
	void signalMoveSquadLeft( unsigned int id );
	void signalMoveSquadRight( unsigned int id );
	void signalRemoveGnomeFromSquad( unsigned int gnomeID );
	void signalMoveGnomeLeft( unsigned int id );
	void signalMoveGnomeRight( unsigned int id );
	void signalSetAttitude( unsigned int squadID, QString type, MilAttitude attitude );
	void signalMovePrioUp( unsigned int squadID, QString type );
	void signalMovePrioDown( unsigned int squadID, QString type );
	void signalRequestRoles();
	void signalAddRole();
	void signalRemoveRole( unsigned int id );
	void signalRenameRole( unsigned int id, QString newName );
	void signalSetArmorType( unsigned int roleID, QString slot, QString type, QString material );
	void signalSetRole( unsigned int gnomeID, unsigned int roleID );
	void signalSetRoleCivilian( unsigned int roleID, bool value );
};
