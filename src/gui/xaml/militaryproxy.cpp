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

#include "../eventconnector.h"

#include <QDebug>

MilitaryProxy::MilitaryProxy( QObject* parent ) :
	QObject( parent )
{
	connect( this, &MilitaryProxy::signalAddSquad, EventConnector::getInstance().aggregatorMilitary(), &AggregatorMilitary::onAddSquad, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalRemoveSquad, EventConnector::getInstance().aggregatorMilitary(), &AggregatorMilitary::onRemoveSquad, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalRenameSquad, EventConnector::getInstance().aggregatorMilitary(), &AggregatorMilitary::onRenameSquad, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalMoveSquadLeft, EventConnector::getInstance().aggregatorMilitary(), &AggregatorMilitary::onMoveSquadLeft, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalMoveSquadRight, EventConnector::getInstance().aggregatorMilitary(), &AggregatorMilitary::onMoveSquadRight, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalRemoveGnomeFromSquad, EventConnector::getInstance().aggregatorMilitary(), &AggregatorMilitary::onRemoveGnomeFromSquad, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalMoveGnomeLeft, EventConnector::getInstance().aggregatorMilitary(), &AggregatorMilitary::onMoveGnomeLeft, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalMoveGnomeRight, EventConnector::getInstance().aggregatorMilitary(), &AggregatorMilitary::onMoveGnomeRight, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalSetAttitude, &Global::mil(), &MilitaryManager::onSetAttitude, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalMovePrioUp, EventConnector::getInstance().aggregatorMilitary(), &AggregatorMilitary::onMovePrioUp, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalMovePrioDown, EventConnector::getInstance().aggregatorMilitary(), &AggregatorMilitary::onMovePrioDown, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalRequestRoles, EventConnector::getInstance().aggregatorMilitary(), &AggregatorMilitary::onRequestRoles, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalAddRole, EventConnector::getInstance().aggregatorMilitary(), &AggregatorMilitary::onAddRole, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalRemoveRole, EventConnector::getInstance().aggregatorMilitary(), &AggregatorMilitary::onRemoveRole, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalRenameRole, EventConnector::getInstance().aggregatorMilitary(), &AggregatorMilitary::onRenameRole, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalSetArmorType, EventConnector::getInstance().aggregatorMilitary(), &AggregatorMilitary::onSetArmorType, Qt::QueuedConnection );
	connect( this, &MilitaryProxy::signalSetRole, EventConnector::getInstance().aggregatorMilitary(), &AggregatorMilitary::onSetRole, Qt::QueuedConnection );

	connect( EventConnector::getInstance().aggregatorMilitary(), &AggregatorMilitary::signalSquads, this, &MilitaryProxy::onSquads, Qt::QueuedConnection );
	connect( EventConnector::getInstance().aggregatorMilitary(), &AggregatorMilitary::signalPriorities, this, &MilitaryProxy::onPriorities, Qt::QueuedConnection );
	connect( EventConnector::getInstance().aggregatorMilitary(), &AggregatorMilitary::signalRoles, this, &MilitaryProxy::onRoles, Qt::QueuedConnection );
	connect( EventConnector::getInstance().aggregatorMilitary(), &AggregatorMilitary::signalPossibleMaterials, this, &MilitaryProxy::onPossibleMaterials, Qt::QueuedConnection );


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
