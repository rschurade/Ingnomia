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
#include "militaryproxy.h"

#include "../../base/global.h"
#include "../eventconnector.h"

#include <QDebug>

MilitaryProxy::MilitaryProxy( QObject* parent ) :
	QObject( parent )
{
	this->signalAddSquad.connect(&AggregatorMilitary::onAddSquad, Global::eventConnector->aggregatorMilitary()); // TODO: Qt::QueuedConnection
	this->signalRemoveSquad.connect(&AggregatorMilitary::onRemoveSquad, Global::eventConnector->aggregatorMilitary()); // TODO: Qt::QueuedConnection
	this->signalRenameSquad.connect(&AggregatorMilitary::onRenameSquad, Global::eventConnector->aggregatorMilitary()); // TODO: Qt::QueuedConnection
	this->signalMoveSquadLeft.connect(&AggregatorMilitary::onMoveSquadLeft, Global::eventConnector->aggregatorMilitary()); // TODO: Qt::QueuedConnection
	this->signalMoveSquadRight.connect(&AggregatorMilitary::onMoveSquadRight, Global::eventConnector->aggregatorMilitary()); // TODO: Qt::QueuedConnection
	this->signalRemoveGnomeFromSquad.connect(&AggregatorMilitary::onRemoveGnomeFromSquad, Global::eventConnector->aggregatorMilitary()); // TODO: Qt::QueuedConnection
	this->signalMoveGnomeLeft.connect(&AggregatorMilitary::onMoveGnomeLeft, Global::eventConnector->aggregatorMilitary()); // TODO: Qt::QueuedConnection
	this->signalMoveGnomeRight.connect(&AggregatorMilitary::onMoveGnomeRight, Global::eventConnector->aggregatorMilitary()); // TODO: Qt::QueuedConnection
	this->signalSetAttitude.connect(&AggregatorMilitary::onSetAttitude, Global::eventConnector->aggregatorMilitary()); // TODO: Qt::QueuedConnection
	this->signalMovePrioUp.connect(&AggregatorMilitary::onMovePrioUp, Global::eventConnector->aggregatorMilitary()); // TODO: Qt::QueuedConnection
	this->signalMovePrioDown.connect(&AggregatorMilitary::onMovePrioDown, Global::eventConnector->aggregatorMilitary()); // TODO: Qt::QueuedConnection
	this->signalRequestRoles.connect(&AggregatorMilitary::onRequestRoles, Global::eventConnector->aggregatorMilitary()); // TODO: Qt::QueuedConnection
	this->signalAddRole.connect(&AggregatorMilitary::onAddRole, Global::eventConnector->aggregatorMilitary()); // TODO: Qt::QueuedConnection
	this->signalRemoveRole.connect(&AggregatorMilitary::onRemoveRole, Global::eventConnector->aggregatorMilitary()); // TODO: Qt::QueuedConnection
	this->signalRenameRole.connect(&AggregatorMilitary::onRenameRole, Global::eventConnector->aggregatorMilitary()); // TODO: Qt::QueuedConnection
	this->signalSetArmorType.connect(&AggregatorMilitary::onSetArmorType, Global::eventConnector->aggregatorMilitary()); // TODO: Qt::QueuedConnection
	this->signalSetRole.connect(&AggregatorMilitary::onSetRole, Global::eventConnector->aggregatorMilitary()); // TODO: Qt::QueuedConnection
	this->signalSetRoleCivilian.connect(&AggregatorMilitary::onSetRoleCivilian, Global::eventConnector->aggregatorMilitary()); // TODO: Qt::QueuedConnection

	Global::eventConnector->aggregatorMilitary()->signalSquads.connect(&MilitaryProxy::onSquads, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorMilitary()->signalPriorities.connect(&MilitaryProxy::onPriorities, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorMilitary()->signalRoles.connect(&MilitaryProxy::onRoles, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorMilitary()->signalPossibleMaterials.connect(&MilitaryProxy::onPossibleMaterials, this); // TODO: Qt::QueuedConnection


}

void MilitaryProxy::setParent( IngnomiaGUI::MilitaryModel* parent )
{
	m_parent = parent;
}

void MilitaryProxy::onSquads( const QList<GuiSquad>& squads )
{
	if( m_parent )
	{
		m_parent->updateSquads( squads );
	}
}

void MilitaryProxy::addSquad()
{
	signalAddSquad();
}

void MilitaryProxy::removeSquad( unsigned int id )
{
	signalRemoveSquad( id );
}

void MilitaryProxy::renameSquad( unsigned int id, QString newName )
{
	signalRenameSquad( id, newName );
}

void MilitaryProxy::moveSquadLeft( unsigned int id )
{
	signalMoveSquadLeft( id );
}
	
void MilitaryProxy::moveSquadRight( unsigned int id )
{
	signalMoveSquadRight( id );
}

void MilitaryProxy::removeGnomeFromSquad( unsigned int gnomeID )
{
	signalRemoveGnomeFromSquad( gnomeID );
	
}
	
void MilitaryProxy::moveGnomeLeft( unsigned int gnomeID )
{
	signalMoveGnomeLeft( gnomeID );
	
}

void MilitaryProxy::moveGnomeRight( unsigned int gnomeID )
{
	
	signalMoveGnomeRight( gnomeID );
}

void MilitaryProxy::setAttitude( unsigned int squadID, QString type, MilAttitude attitude )
{
	signalSetAttitude( squadID, type, attitude );
}

void MilitaryProxy::movePrioUp( unsigned int squadID, QString type )
{
	signalMovePrioUp( squadID, type );
}
	
void MilitaryProxy::movePrioDown( unsigned int squadID, QString type )
{
	signalMovePrioDown( squadID, type );
}

void MilitaryProxy::onPriorities( unsigned int squadID, const QList<GuiTargetPriority>& priorities )
{
	if( m_parent )
	{
		m_parent->updatePriorities( squadID, priorities );
	}
}

void MilitaryProxy::requestRoles()
{
	signalRequestRoles();
}

void MilitaryProxy::onRoles( const QList<GuiMilRole> roles )
{
	if( m_parent )
	{
		m_parent->updateRoles( roles );
	}
}

void MilitaryProxy::addRole()
{
	signalAddRole();
}

void MilitaryProxy::removeRole( unsigned int id )
{
	signalRemoveRole( id );
}

void MilitaryProxy::renameRole( unsigned int id, QString newName )
{
	signalRenameRole( id, newName );
}

void MilitaryProxy::setArmorType( unsigned int roleID, QString slot, QString type, QString material )
{
	signalSetArmorType( roleID, slot, type, material );
}

void MilitaryProxy::onPossibleMaterials( unsigned int roleID, const QString slot, const QStringList mats )
{
	if( m_parent )
	{
		m_parent->updatePossibleMaterials( roleID, slot, mats );
	}
}

void MilitaryProxy::setRole( unsigned int gnomeID, unsigned int roleID )
{
	signalSetRole( gnomeID, roleID );
}

void MilitaryProxy::setRoleCivilian( unsigned int roleID, bool value )
{
	signalSetRoleCivilian( roleID, value );
}