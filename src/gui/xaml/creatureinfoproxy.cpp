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
#include "creatureinfoproxy.h"

#include "../../base/db.h"
#include "../../base/gamestate.h"
#include "../../gfx/sprite.h"
#include "../../gfx/spritefactory.h"
#include "../../base/global.h"
#include "../eventconnector.h"

#include "PopulationModel.h"

CreatureInfoProxy::CreatureInfoProxy( QObject* parent ) :
	QObject( parent )
{
	Global::eventConnector->aggregatorCreatureInfo()->signalCreatureUpdate.connect(&CreatureInfoProxy::onUpdateInfo, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorCreatureInfo()->signalProfessionList.connect(&CreatureInfoProxy::onProfessionList, this); // TODO: Qt::QueuedConnection
	
	this->signalRequestProfessionList.connect(&AggregatorCreatureInfo::onRequestProfessionList, Global::eventConnector->aggregatorCreatureInfo()); // TODO: Qt::QueuedConnection
	this->signalSetProfession.connect(&AggregatorCreatureInfo::onSetProfession, Global::eventConnector->aggregatorCreatureInfo()); // TODO: Qt::QueuedConnection
	
	this->signalRequestEmptySlotImages.connect(&AggregatorCreatureInfo::onRequestEmptySlotImages, Global::eventConnector->aggregatorCreatureInfo()); // TODO: Qt::QueuedConnection

	Global::eventConnector->aggregatorCreatureInfo()->signalEmptyPics.connect(&CreatureInfoProxy::onEmptyPics, this); // TODO: Qt::QueuedConnection
}

CreatureInfoProxy::~CreatureInfoProxy()
{
}

void CreatureInfoProxy::setParent( IngnomiaGUI::CreatureInfoModel* parent )
{
	m_parent = parent;
}

void CreatureInfoProxy::onUpdateInfo( const GuiCreatureInfo& info )
{
	if ( m_parent )
	{
		m_parent->updateInfo( info );
	}
}

void CreatureInfoProxy::requestProfessionList()
{
	signalRequestProfessionList();
}

void CreatureInfoProxy::onProfessionList( const QStringList& profs )
{
	if ( m_parent )
	{
		m_parent->updateProfessionList( profs );
	}
}

void CreatureInfoProxy::setProfession( unsigned int gnomeID, QString profession )
{
	signalSetProfession( gnomeID, profession );
}

void CreatureInfoProxy::requestEmptySlotImages()
{
	signalRequestEmptySlotImages();
}

void CreatureInfoProxy::onEmptyPics( const absl::btree_map< QString, std::vector<unsigned char> >& emptyPics )
{
	if( m_parent )
	{
		m_parent->updateEmptySlotImages( emptyPics );
	}
}