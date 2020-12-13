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
#include "selectionproxy.h"

#include "../../base/global.h"
#include "../eventconnector.h"
#include "../aggregatorselection.h"

#include <QDebug>

SelectionProxy::SelectionProxy( QObject* parent ) :
	QObject( parent )
{
	connect( Global::eventConnector->aggregatorSelection(), &AggregatorSelection::signalAction, this, &SelectionProxy::onAction, Qt::QueuedConnection );
    connect( Global::eventConnector->aggregatorSelection(), &AggregatorSelection::signalCursorPos, this, &SelectionProxy::onCursor, Qt::QueuedConnection );
    connect( Global::eventConnector->aggregatorSelection(), &AggregatorSelection::signalFirstClick, this, &SelectionProxy::onFirstClick, Qt::QueuedConnection );
    connect( Global::eventConnector->aggregatorSelection(), &AggregatorSelection::signalSize, this, &SelectionProxy::onSize, Qt::QueuedConnection );
}

void SelectionProxy::setParent( IngnomiaGUI::SelectionModel* parent )
{
	m_parent = parent;
}

void SelectionProxy::onAction( QString action )
{
    if( m_parent )
	{
        m_parent->updateAction( action );
	}
}

void SelectionProxy::onCursor( QString pos )
{
    if( m_parent )
	{
        m_parent->updateCursor( pos );
	}
}

void SelectionProxy::onFirstClick( QString pos )
{
    if( m_parent )
	{
        m_parent->updateFirstClick( pos );
	}
}

void SelectionProxy::onSize( QString size )
{
    if( m_parent )
	{
        m_parent->updateSize( size );
	}
}