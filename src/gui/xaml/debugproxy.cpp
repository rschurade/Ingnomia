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
#include "debugproxy.h"

#include "../../base/global.h"
#include "../eventconnector.h"

#include <QDebug>

DebugProxy::DebugProxy( QObject* parent ) :
	QObject( parent )
{
	connect( this, &DebugProxy::signalSpawnCreature, Global::eventConnector->aggregatorDebug(), &AggregatorDebug::onSpawnCreature, Qt::QueuedConnection );
    connect( this, &DebugProxy::signalSetWindowSize, Global::eventConnector->aggregatorDebug(), &AggregatorDebug::onSetWindowSize, Qt::QueuedConnection );
}

void DebugProxy::setParent( IngnomiaGUI::DebugModel* parent )
{
	m_parent = parent;
}

void DebugProxy::spawnCreature( QString type )
{
	emit signalSpawnCreature( type );
}

void DebugProxy::setWindowSize( int width, int height )
{
    emit signalSetWindowSize( width, height );
}
