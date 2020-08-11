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

#include "../eventconnector.h"

#include <QDebug>

NeighborsProxy::NeighborsProxy( QObject* parent ) :
	QObject( parent )
{
	connect( this, &NeighborsProxy::signalRequestAvailableGnomes, EventConnector::getInstance().aggregatorNeighbors(), &AggregatorNeighbors::onRequestAvailableGnomes, Qt::QueuedConnection );
	connect( this, &NeighborsProxy::signalStartMission, EventConnector::getInstance().aggregatorNeighbors(), &AggregatorNeighbors::onStartMission, Qt::QueuedConnection );
	
	connect( EventConnector::getInstance().aggregatorNeighbors(), &AggregatorNeighbors::signalNeighborsUpdate, this, &NeighborsProxy::onNeighborsUpdate, Qt::QueuedConnection );
	connect( EventConnector::getInstance().aggregatorNeighbors(), &AggregatorNeighbors::signalAvailableGnomes, this, &NeighborsProxy::onAvailableGnomes, Qt::QueuedConnection );
	connect( EventConnector::getInstance().aggregatorNeighbors(), &AggregatorNeighbors::signalMissions, this, &NeighborsProxy::onMissions, Qt::QueuedConnection );

	connect( &Global::em(), &EventManager::signalUpdateMission, this, &NeighborsProxy::onUpdateMission, Qt::QueuedConnection );
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
	emit signalRequestAvailableGnomes();
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
	emit signalStartMission( type, action, targetKingdom, gnomeID );
}

void NeighborsProxy::onUpdateMission( const Mission& mission )
{
	if( m_parent )
	{
		m_parent->updateMission( mission );
	}
}
