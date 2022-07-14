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

SelectionProxy::SelectionProxy( QObject* parent ) :
	QObject( parent )
{
	Global::eventConnector->aggregatorSelection()->signalAction.connect(&SelectionProxy::onAction, this);   // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorSelection()->signalCursorPos.connect(&SelectionProxy::onCursor, this);   // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorSelection()->signalFirstClick.connect(&SelectionProxy::onFirstClick, this);   // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorSelection()->signalSize.connect(&SelectionProxy::onSize, this);   // TODO: Qt::QueuedConnection
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