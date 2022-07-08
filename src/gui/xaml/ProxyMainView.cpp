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
	Global::eventConnector->signalWindowSize.connect(&ProxyMainView::onWindowSize, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->signalPropagateKeyEsc.connect(&ProxyMainView::onKeyEsc, this); // TODO: Qt::QueuedConnection

	Global::eventConnector->aggregatorSettings()->signalUIScale.connect(&ProxyMainView::onUIScale, this); // TODO: Qt::QueuedConnection

	this->signalRequestLoadScreenUpdate.connect(&AggregatorLoadGame::onRequestKingdoms, Global::eventConnector->aggregatorLoadGame());

	this->signalRequestUIScale.connect(&AggregatorSettings::onRequestUIScale, Global::eventConnector->aggregatorSettings()); // TODO: Qt::QueuedConnection
	this->signalRequestVersion.connect(&AggregatorSettings::onRequestVersion, Global::eventConnector->aggregatorSettings()); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorSettings()->signalVersion.connect(&ProxyMainView::onVersion, this); // TODO: Qt::QueuedConnection

	this->signalStartNewGame.connect(&EventConnector::onStartNewGame, Global::eventConnector); // TODO: Qt::QueuedConnection
	this->signalContinueLastGame.connect(&EventConnector::onContinueLastGame, Global::eventConnector); // TODO: Qt::QueuedConnection
	this->signalLoadGame.connect(&EventConnector::onLoadGame, Global::eventConnector); // TODO: Qt::QueuedConnection
	this->signalSaveGame.connect(&EventConnector::onSaveGame, Global::eventConnector); // TODO: Qt::QueuedConnection
	this->signalSetShowMainMenu.connect(&EventConnector::onSetShowMainMenu, Global::eventConnector); // TODO: Qt::QueuedConnection
	this->signalEndGame.connect(&EventConnector::onEndGame, Global::eventConnector); // TODO: Qt::QueuedConnection

	Global::eventConnector->signalResume.connect(&ProxyMainView::onResume, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->signalLoadGameDone.connect(&ProxyMainView::onLoadGameDone, this); // TODO: Qt::QueuedConnection
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
	signalRequestLoadScreenUpdate();
}

void ProxyMainView::requestUIScale()
{
	signalRequestUIScale();
}

void ProxyMainView::startNewGame()
{
	signalStartNewGame();
}
	
void ProxyMainView::continueLastGame()
{
	signalContinueLastGame();
}

void ProxyMainView::loadGame( const std::string& param )
{
	signalLoadGame( param );
}

void ProxyMainView::saveGame()
{
	signalSaveGame();
}

void ProxyMainView::setShowMainMenu( bool value )
{
	signalSetShowMainMenu( value );
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
	signalEndGame();
}

void ProxyMainView::requestVersion()
{
	signalRequestVersion();
}

void ProxyMainView::onVersion( const std::string& version )
{
	if( m_parent )
	{
		m_parent->updateVersion( version );
	}
}