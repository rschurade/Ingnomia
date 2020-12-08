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
#include "loadgameproxy.h"

#include "../../base/global.h"
#include "../eventconnector.h"

#include <QDebug>

LoadGameProxy::LoadGameProxy( QObject* parent ) :
	QObject( parent )
{
	connect( this, &LoadGameProxy::signalRequestKingdoms, Global::eventConnector->aggregatorLoadGame(), &AggregatorLoadGame::onRequestKingdoms, Qt::QueuedConnection );
	connect( this, &LoadGameProxy::signalRequestSaveGames, Global::eventConnector->aggregatorLoadGame(), &AggregatorLoadGame::onRequestSaveGames, Qt::QueuedConnection );

	connect( Global::eventConnector->aggregatorLoadGame(), &AggregatorLoadGame::signalKingdoms, this, &LoadGameProxy::onKingdoms, Qt::QueuedConnection );
	connect( Global::eventConnector->aggregatorLoadGame(), &AggregatorLoadGame::signalSaveGames, this, &LoadGameProxy::onSaveGames, Qt::QueuedConnection );
}

void LoadGameProxy::setParent( IngnomiaGUI::LoadGameModel* parent )
{
	m_parent = parent;
}

void LoadGameProxy::requestKingdoms()
{
	emit signalRequestKingdoms();
}

void LoadGameProxy::onKingdoms( const QList<GuiSaveInfo>& kingdoms )
{
	if ( m_parent )
	{
		m_parent->updateSavedKingdoms( kingdoms );
	}
}

void LoadGameProxy::requestSaveGames( const QString path )
{
	emit signalRequestSaveGames( path );
}

void LoadGameProxy::onSaveGames( const QList<GuiSaveInfo>& saveGames )
{
	if ( m_parent )
	{
		m_parent->updateSaveGames( saveGames );
	}
}
