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
#include "ProxyMainView.h"

#include "../aggregatorloadgame.h"
#include "../aggregatorsettings.h"
#include "../eventconnector.h"
#include "ViewModel.h"

ProxyMainView::ProxyMainView( QObject* parent ) :
	QObject( parent )
{
	connect( &EventConnector::getInstance(), &EventConnector::signalWindowSize, this, &ProxyMainView::onWindowSize, Qt::QueuedConnection );
	connect( &EventConnector::getInstance(), &EventConnector::signalPropagateKeyEsc, this, &ProxyMainView::onKeyEsc, Qt::QueuedConnection );

	connect( EventConnector::getInstance().aggregatorSettings(), &AggregatorSettings::signalUIScale, this, &ProxyMainView::onUIScale, Qt::QueuedConnection );

	connect( this, &ProxyMainView::signalRequestLoadScreenUpdate, EventConnector::getInstance().aggregatorLoadGame(), &AggregatorLoadGame::onRequestKingdoms );
}

ProxyMainView::~ProxyMainView()
{
}

void ProxyMainView::setParent( IngnomiaGUI::ViewModel* parent )
{
	m_parent = parent;
}

void ProxyMainView::onWindowSize( int w, int h )
{
	if ( m_parent )
	{
		m_parent->setWindowSize( w, h );
	}
}

void ProxyMainView::onKeyEsc()
{
	if ( m_parent )
	{
		m_parent->OnBack( nullptr );
	}
}

void ProxyMainView::onUIScale( float value )
{
	if ( m_parent )
	{
		m_parent->setUIScale( value );
	}
}

void ProxyMainView::requestLoadScreenUpdate()
{
	emit signalRequestLoadScreenUpdate();
}
