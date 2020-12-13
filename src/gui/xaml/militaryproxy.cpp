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
	connect( this, &MilitaryProxy::signalAddSquad, Global::eventConnector->aggregatorMilitary(), &AggregatorMilitary::onAddSquad, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalRemoveSquad, Global::eventConnector->aggregatorMilitary(), &AggregatorMilitary::onRemoveSquad, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalRenameSquad, Global::eventConnector->aggregatorMilitary(), &AggregatorMilitary::onRenameSquad, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalMoveSquadLeft, Global::eventConnector->aggregatorMilitary(), &AggregatorMilitary::onMoveSquadLeft, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalMoveSquadRight, Global::eventConnector->aggregatorMilitary(), &AggregatorMilitary::onMoveSquadRight, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalRemoveGnomeFromSquad, Global::eventConnector->aggregatorMilitary(), &AggregatorMilitary::onRemoveGnomeFromSquad, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalMoveGnomeLeft, Global::eventConnector->aggregatorMilitary(), &AggregatorMilitary::onMoveGnomeLeft, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalMoveGnomeRight, Global::eventConnector->aggregatorMilitary(), &AggregatorMilitary::onMoveGnomeRight, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalSetAttitude, Global::eventConnector->aggregatorMilitary(), &AggregatorMilitary::onSetAttitude, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalMovePrioUp, Global::eventConnector->aggregatorMilitary(), &AggregatorMilitary::onMovePrioUp, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalMovePrioDown, Global::eventConnector->aggregatorMilitary(), &AggregatorMilitary::onMovePrioDown, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalRequestRoles, Global::eventConnector->aggregatorMilitary(), &AggregatorMilitary::onRequestRoles, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalAddRole, Global::eventConnector->aggregatorMilitary(), &AggregatorMilitary::onAddRole, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalRemoveRole, Global::eventConnector->aggregatorMilitary(), &AggregatorMilitary::onRemoveRole, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalRenameRole, Global::eventConnector->aggregatorMilitary(), &AggregatorMilitary::onRenameRole, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalSetArmorType, Global::eventConnector->aggregatorMilitary(), &AggregatorMilitary::onSetArmorType, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalSetRole, Global::eventConnector->aggregatorMilitary(), &AggregatorMilitary::onSetRole, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalSetRoleCivilian, Global::eventConnector->aggregatorMilitary(), &AggregatorMilitary::onSetRoleCivilian, Qt::QueuedConnection );

	connect( Global::eventConnector->aggregatorMilitary(), &AggregatorMilitary::signalSquads, this, &MilitaryProxy::onSquads, Qt::QueuedConnection );
	connect( Global::eventConnector->aggregatorMilitary(), &AggregatorMilitary::signalPriorities, this, &MilitaryProxy::onPriorities, Qt::QueuedConnection );
	connect( Global::eventConnector->aggregatorMilitary(), &AggregatorMilitary::signalRoles, this, &MilitaryProxy::onRoles, Qt::QueuedConnection );
	connect( Global::eventConnector->aggregatorMilitary(), &AggregatorMilitary::signalPossibleMaterials, this, &MilitaryProxy::onPossibleMaterials, Qt::QueuedConnection );


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
	emit signalAddSquad();
}

void MilitaryProxy::removeSquad( unsigned int id )
{
	emit signalRemoveSquad( id );
}

void MilitaryProxy::renameSquad( unsigned int id, QString newName )
{
	emit signalRenameSquad( id, newName );
}

void MilitaryProxy::moveSquadLeft( unsigned int id )
{
	emit signalMoveSquadLeft( id );
}
	
void MilitaryProxy::moveSquadRight( unsigned int id )
{
	emit signalMoveSquadRight( id );
}

void MilitaryProxy::removeGnomeFromSquad( unsigned int gnomeID )
{
	emit signalRemoveGnomeFromSquad( gnomeID );
	
}
	
void MilitaryProxy::moveGnomeLeft( unsigned int gnomeID )
{
	emit signalMoveGnomeLeft( gnomeID );
	
}

void MilitaryProxy::moveGnomeRight( unsigned int gnomeID )
{
	
	emit signalMoveGnomeRight( gnomeID );
}

void MilitaryProxy::setAttitude( unsigned int squadID, QString type, MilAttitude attitude )
{
	emit signalSetAttitude( squadID, type, attitude );
}

void MilitaryProxy::movePrioUp( unsigned int squadID, QString type )
{
	emit signalMovePrioUp( squadID, type );
}
	
void MilitaryProxy::movePrioDown( unsigned int squadID, QString type )
{
	emit signalMovePrioDown( squadID, type );
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
	emit signalRequestRoles();
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
	emit signalAddRole();
}

void MilitaryProxy::removeRole( unsigned int id )
{
	emit signalRemoveRole( id );
}

void MilitaryProxy::renameRole( unsigned int id, QString newName )
{
	emit signalRenameRole( id, newName );
}

void MilitaryProxy::setArmorType( unsigned int roleID, QString slot, QString type, QString material )
{
	emit signalSetArmorType( roleID, slot, type, material );
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
	emit signalSetRole( gnomeID, roleID );
}

void MilitaryProxy::setRoleCivilian( unsigned int roleID, bool value )
{
	emit signalSetRoleCivilian( roleID, value );
}