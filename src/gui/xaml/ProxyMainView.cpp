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

#include "../../base/global.h"

#include "../aggregatorloadgame.h"
#include "../aggregatorsettings.h"
#include "../eventconnector.h"
#include "ViewModel.h"

ProxyMainView::ProxyMainView( QObject* parent ) :
	QObject( parent )
{
	connect( Global::eventConnector, &EventConnector::signalWindowSize, this, &ProxyMainView::onWindowSize, Qt::QueuedConnection );
	connect( Global::eventConnector, &EventConnector::signalPropagateKeyEsc, this, &ProxyMainView::onKeyEsc, Qt::QueuedConnection );

	connect( Global::eventConnector->aggregatorSettings(), &AggregatorSettings::signalUIScale, this, &ProxyMainView::onUIScale, Qt::QueuedConnection );

	connect( this, &ProxyMainView::signalRequestLoadScreenUpdate, Global::eventConnector->aggregatorLoadGame(), &AggregatorLoadGame::onRequestKingdoms );

	connect( this, &ProxyMainView::signalRequestUIScale, Global::eventConnector->aggregatorSettings(), &AggregatorSettings::onRequestUIScale, Qt::QueuedConnection );
	connect( this, &ProxyMainView::signalRequestVersion, Global::eventConnector->aggregatorSettings(), &AggregatorSettings::onRequestVersion, Qt::QueuedConnection );
	connect( Global::eventConnector->aggregatorSettings(), &AggregatorSettings::signalVersion, this, &ProxyMainView::onVersion, Qt::QueuedConnection );

	connect( this, &ProxyMainView::signalStartNewGame, Global::eventConnector, &EventConnector::onStartNewGame, Qt::QueuedConnection );
	connect( this, &ProxyMainView::signalContinueLastGame, Global::eventConnector, &EventConnector::onContinueLastGame, Qt::QueuedConnection );
	connect( this, &ProxyMainView::signalLoadGame, Global::eventConnector, &EventConnector::onLoadGame, Qt::QueuedConnection );
	connect( this, &ProxyMainView::signalSaveGame, Global::eventConnector, &EventConnector::onSaveGame, Qt::QueuedConnection );
	connect( this, &ProxyMainView::signalSetShowMainMenu, Global::eventConnector, &EventConnector::onSetShowMainMenu, Qt::QueuedConnection );
	connect( this, &ProxyMainView::signalEndGame, Global::eventConnector, &EventConnector::onEndGame, Qt::QueuedConnection );

	connect( Global::eventConnector, &EventConnector::signalResume, this, &ProxyMainView::onResume, Qt::QueuedConnection );
	connect( Global::eventConnector, &EventConnector::signalLoadGameDone, this, &ProxyMainView::onLoadGameDone, Qt::QueuedConnection );
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

void ProxyMainView::requestUIScale()
{
	emit signalRequestUIScale();
}

void ProxyMainView::startNewGame()
{
	emit signalStartNewGame();
}
	
void ProxyMainView::continueLastGame()
{
	emit signalContinueLastGame();
}

void ProxyMainView::loadGame( QString param )
{
	emit signalLoadGame( param );
}

void ProxyMainView::saveGame()
{
	emit signalSaveGame();
}

void ProxyMainView::setShowMainMenu( bool value )
{
	emit signalSetShowMainMenu( value );
}

void ProxyMainView::onResume()
{
	if ( m_parent )
	{
		m_parent->OnResume();
	}
}

void ProxyMainView::onLoadGameDone( bool value )
{
	if ( m_parent )
	{
		m_parent->OnContinueGameFinished( value );
	}
}

void ProxyMainView::endGame()
{
	emit signalEndGame();
}

void ProxyMainView::requestVersion()
{
	emit signalRequestVersion();
}

void ProxyMainView::onVersion( QString version )
{
	if( m_parent )
	{
		m_parent->updateVersion( version );
	}
}