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
/** @file creatureinfoproxy.cpp
 *  @brief CreatureInfoProxy implementation: connects every signal pair between
 *         CreatureInfoModel and AggregatorCreatureInfo and provides thin pass-through slots.
 */
#include "creatureinfoproxy.h"

#include "../../base/db.h"
#include "../../base/gamestate.h"
#include "../../gfx/sprite.h"
#include "../../gfx/spritefactory.h"
#include "../../base/global.h"
#include "../eventconnector.h"

#include "PopulationModel.h"

#include <QDebug>
#include <QPainter>

/// @brief Constructs the proxy and wires up all aggregator-side signal/slot connections.
/// @param parent Qt parent object.
CreatureInfoProxy::CreatureInfoProxy( QObject* parent ) :
	QObject( parent )
{
	connect( Global::eventConnector->aggregatorCreatureInfo(), &AggregatorCreatureInfo::signalCreatureUpdate, this, &CreatureInfoProxy::onUpdateInfo, Qt::QueuedConnection );
	connect( Global::eventConnector->aggregatorCreatureInfo(), &AggregatorCreatureInfo::signalProfessionList, this, &CreatureInfoProxy::onProfessionList, Qt::QueuedConnection );
	
	connect( this, &CreatureInfoProxy::signalRequestProfessionList, Global::eventConnector->aggregatorCreatureInfo(), &AggregatorCreatureInfo::onRequestProfessionList, Qt::QueuedConnection );
	connect( this, &CreatureInfoProxy::signalSetProfession, Global::eventConnector->aggregatorCreatureInfo(), &AggregatorCreatureInfo::onSetProfession, Qt::QueuedConnection );
	
	connect( this, &CreatureInfoProxy::signalRequestEmptySlotImages, Global::eventConnector->aggregatorCreatureInfo(), &AggregatorCreatureInfo::onRequestEmptySlotImages, Qt::QueuedConnection );

	connect( Global::eventConnector->aggregatorCreatureInfo(), &AggregatorCreatureInfo::signalEmptyPics, this, &CreatureInfoProxy::onEmptyPics, Qt::QueuedConnection );
}

/// @brief Destructor.
CreatureInfoProxy::~CreatureInfoProxy()
{
}

/// @brief Binds the proxy to its owning view model.
void CreatureInfoProxy::setParent( IngnomiaGUI::CreatureInfoModel* parent )
{
	m_parent = parent;
}

/// @brief Slot: relays a fresh creature payload from the aggregator to the model.
void CreatureInfoProxy::onUpdateInfo( const GuiCreatureInfo& info )
{
	if ( m_parent )
	{
		m_parent->updateInfo( info );
	}
}

/// @brief Asks the aggregator for the current profession list.
void CreatureInfoProxy::requestProfessionList()
{
	emit signalRequestProfessionList();
}

/// @brief Slot: relays a fresh profession list to the model so the dropdown updates.
void CreatureInfoProxy::onProfessionList( const QStringList& profs )
{
	if ( m_parent )
	{
		m_parent->updateProfessionList( profs );
	}
}

/// @brief Forwards a profession change for the given gnome to the aggregator.
void CreatureInfoProxy::setProfession( unsigned int gnomeID, QString profession )
{
	emit signalSetProfession( gnomeID, profession );
}

/// @brief Asks the aggregator for the empty-slot placeholder PNG buffers (one-shot at init).
void CreatureInfoProxy::requestEmptySlotImages()
{
	emit signalRequestEmptySlotImages();
}

/// @brief Slot: relays the empty-slot placeholder bitmaps to the model for caching.
void CreatureInfoProxy::onEmptyPics( const QMap< QString, std::vector<unsigned char> >& emptyPics )
{
	if( m_parent )
	{
		m_parent->updateEmptySlotImages( emptyPics );
	}
}