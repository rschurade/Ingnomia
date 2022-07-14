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
#include "neighborsproxy.h"

#include "../../base/global.h"
#include "../eventconnector.h"

NeighborsProxy::NeighborsProxy( QObject* parent ) :
	QObject( parent )
{
	this->signalRequestAvailableGnomes.connect(&AggregatorNeighbors::onRequestAvailableGnomes, Global::eventConnector->aggregatorNeighbors()); // TODO: Qt::QueuedConnection
	this->signalStartMission.connect(&AggregatorNeighbors::onStartMission, Global::eventConnector->aggregatorNeighbors()); // TODO: Qt::QueuedConnection
	
	Global::eventConnector->aggregatorNeighbors()->signalNeighborsUpdate.connect(&NeighborsProxy::onNeighborsUpdate, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorNeighbors()->signalAvailableGnomes.connect(&NeighborsProxy::onAvailableGnomes, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorNeighbors()->signalMissions.connect(&NeighborsProxy::onMissions, this); // TODO: Qt::QueuedConnection

	Global::eventConnector->aggregatorNeighbors()->signalUpdateMission.connect(&NeighborsProxy::onUpdateMission, this); // TODO: Qt::QueuedConnection
}

void NeighborsProxy::setParent( IngnomiaGUI::NeighborsModel* parent )
{
	m_parent = parent;
}

void NeighborsProxy::onNeighborsUpdate( const QList<GuiNeighborInfo>& infos )
{
	if( m_parent )
	{
		m_parent->neighborsUpdate( infos );
	}
}

void NeighborsProxy::onMissions( const QList<Mission>& missions )
{
	if( m_parent )
	{
		m_parent->missionsUpdate( missions );
	}
}

void NeighborsProxy::requestAvailableGnomes()
{
	signalRequestAvailableGnomes();
}

void NeighborsProxy::onAvailableGnomes( const QList<GuiAvailableGnome>& gnomes )
{
	if( m_parent )
	{
		m_parent->updateAvailableGnomes( gnomes );
	}
}

void NeighborsProxy::startMission( MissionType type, MissionAction action, unsigned int targetKingdom, unsigned int gnomeID )
{
	signalStartMission( type, action, targetKingdom, gnomeID );
}

void NeighborsProxy::onUpdateMission( const Mission& mission )
{
	if( m_parent )
	{
		m_parent->updateMission( mission );
	}
}
