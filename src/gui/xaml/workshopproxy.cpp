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
#include "workshopproxy.h"

#include "../../base/gamestate.h"
#include "../../gfx/sprite.h"
#include "../../gfx/spritefactory.h"
#include "../../base/global.h"
#include "../eventconnector.h"

#include <QDebug>
#include <QPainter>

WorkshopProxy::WorkshopProxy( QObject* parent ) :
	QObject( parent )
{
	connect( Global::eventConnector->aggregatorWorkshop(), &AggregatorWorkshop::signalUpdateInfo, this, &WorkshopProxy::onUpdateInfo, Qt::QueuedConnection );
	connect( Global::eventConnector->aggregatorWorkshop(), &AggregatorWorkshop::signalUpdateCraftList, this, &WorkshopProxy::onUpdateCraftList, Qt::QueuedConnection );

	connect( this, &WorkshopProxy::signalSetBasicOptions, Global::eventConnector->aggregatorWorkshop(), &AggregatorWorkshop::onSetBasicOptions, Qt::QueuedConnection );
	connect( this, &WorkshopProxy::signalSetButcherOptions, Global::eventConnector->aggregatorWorkshop(), &AggregatorWorkshop::onSetButcherOptions, Qt::QueuedConnection );
	connect( this, &WorkshopProxy::signalSetFisherOptions, Global::eventConnector->aggregatorWorkshop(), &AggregatorWorkshop::onSetFisherOptions, Qt::QueuedConnection );
	connect( this, &WorkshopProxy::signalCraftItem, Global::eventConnector->aggregatorWorkshop(), &AggregatorWorkshop::onCraftItem, Qt::QueuedConnection );
	connect( this, &WorkshopProxy::signalCraftJobCommand, Global::eventConnector->aggregatorWorkshop(), &AggregatorWorkshop::onCraftJobCommand, Qt::QueuedConnection );
	connect( this, &WorkshopProxy::signalCraftJobParams, Global::eventConnector->aggregatorWorkshop(), &AggregatorWorkshop::onCraftJobParams, Qt::QueuedConnection );

	connect( this, &WorkshopProxy::signalRequestAllTradeItems, Global::eventConnector->aggregatorWorkshop(), &AggregatorWorkshop::onRequestAllTradeItems, Qt::QueuedConnection );

	connect( Global::eventConnector->aggregatorWorkshop(), &AggregatorWorkshop::signalTraderStock, this, &WorkshopProxy::onUpdateTraderStock, Qt::QueuedConnection );
	connect( Global::eventConnector->aggregatorWorkshop(), &AggregatorWorkshop::signalPlayerStock, this, &WorkshopProxy::onUpdatePlayerStock, Qt::QueuedConnection );

	connect( Global::eventConnector->aggregatorWorkshop(), &AggregatorWorkshop::signalUpdateTraderStockItem, this, &WorkshopProxy::onUpdateTraderStockItem, Qt::QueuedConnection );
	connect( Global::eventConnector->aggregatorWorkshop(), &AggregatorWorkshop::signalUpdatePlayerStockItem, this, &WorkshopProxy::onUpdatePlayerStockItem, Qt::QueuedConnection );
	connect( Global::eventConnector->aggregatorWorkshop(), &AggregatorWorkshop::signalUpdateTraderValue, this, &WorkshopProxy::onUpdateTraderValue, Qt::QueuedConnection );
	connect( Global::eventConnector->aggregatorWorkshop(), &AggregatorWorkshop::signalUpdatePlayerValue, this, &WorkshopProxy::onUpdatePlayerValue, Qt::QueuedConnection );

	connect( this, &WorkshopProxy::signalTraderStocktoOffer, Global::eventConnector->aggregatorWorkshop(), &AggregatorWorkshop::onTraderStocktoOffer, Qt::QueuedConnection );
	connect( this, &WorkshopProxy::signalTraderOffertoStock, Global::eventConnector->aggregatorWorkshop(), &AggregatorWorkshop::onTraderOffertoStock, Qt::QueuedConnection );
	connect( this, &WorkshopProxy::signalPlayerStocktoOffer, Global::eventConnector->aggregatorWorkshop(), &AggregatorWorkshop::onPlayerStocktoOffer, Qt::QueuedConnection );
	connect( this, &WorkshopProxy::signalPlayerOffertoStock, Global::eventConnector->aggregatorWorkshop(), &AggregatorWorkshop::onPlayerOffertoStock, Qt::QueuedConnection );
	
	connect( this, &WorkshopProxy::signalTrade, Global::eventConnector->aggregatorWorkshop(), &AggregatorWorkshop::onTrade, Qt::QueuedConnection );
}

WorkshopProxy::~WorkshopProxy()
{
}

void WorkshopProxy::setParent( IngnomiaGUI::WorkshopModel* parent )
{
	m_parent = parent;
}

void WorkshopProxy::onUpdateInfo( const GuiWorkshopInfo& info )
{
	if ( m_parent )
	{
		m_workshopID = info.workshopID;
		m_parent->onUpdateInfo( info );
	}
}

void WorkshopProxy::setBasicOptions( unsigned int WorkshopID, QString name, int priority, bool suspended, bool acceptGenerated, bool autoCraftMissing, bool connectStockpile )
{
	if( !m_blockWriteBack )
	{
		emit signalSetBasicOptions( WorkshopID, name, priority, suspended, acceptGenerated, autoCraftMissing, connectStockpile );
	}
}

void WorkshopProxy::craftItem( QString craftID, int mode, int number, QStringList mats )
{
	emit signalCraftItem( m_workshopID, craftID, mode, number, mats );
}

void WorkshopProxy::onUpdateCraftList( const GuiWorkshopInfo& info )
{
	if ( m_parent )
	{
		m_parent->onUpdateCraftList( info );
	}
}

void WorkshopProxy::craftJobCommand( unsigned int craftJobID, QString command )
{
	emit signalCraftJobCommand( m_workshopID, craftJobID, command );
}

void WorkshopProxy::craftJobParams( unsigned int craftJobID, int mode, QString numString, bool suspended, bool moveBack )
{
	int num = qMax( 1, numString.toInt() );
	emit signalCraftJobParams( m_workshopID, craftJobID, mode, num, suspended, moveBack );
}

void WorkshopProxy::setButcherOptions( unsigned int workshopID, bool butcherCorpses, bool butcherExcess )
{
	emit signalSetButcherOptions( workshopID, butcherCorpses, butcherExcess );
}

void WorkshopProxy::requestAllTradeItems( unsigned int workshopID )
{
	emit signalRequestAllTradeItems( workshopID );
}

void WorkshopProxy::onUpdateTraderStock( const QList<GuiTradeItem>& items )
{
	if( m_parent )
	{
		m_parent->updateTraderStock( items );
	}
}

void WorkshopProxy::onUpdatePlayerStock( const QList<GuiTradeItem>& items )
{
	if( m_parent )
	{
		m_parent->updatePlayerStock( items );
	}
}

void WorkshopProxy::traderStocktoOffer( unsigned int workshopID, QString itemSID, QString materialSID, unsigned char quality, int count )
{
	emit signalTraderStocktoOffer( workshopID, itemSID, materialSID, quality, count );
}
	
void WorkshopProxy::traderOffertoStock( unsigned int workshopID, QString itemSID, QString materialSID, unsigned char quality, int count )
{
	emit signalTraderOffertoStock( workshopID, itemSID, materialSID, quality, count );
}

void WorkshopProxy::playerStocktoOffer( unsigned int workshopID, QString itemSID, QString materialSID, unsigned char quality, int count )
{
	emit signalPlayerStocktoOffer( workshopID, itemSID, materialSID, quality, count );
}

void WorkshopProxy::playerOffertoStock( unsigned int workshopID, QString itemSID, QString materialSID, unsigned char quality, int count )
{
	emit signalPlayerOffertoStock( workshopID, itemSID, materialSID, quality, count );
}

void WorkshopProxy::onUpdateTraderStockItem( const GuiTradeItem& item )
{
	if( m_parent )
	{
		m_parent->updateTraderStockItem( item );
	}
}

void WorkshopProxy::onUpdatePlayerStockItem( const GuiTradeItem& item )
{
	if( m_parent )
	{
		m_parent->updatePlayerStockItem( item );
	}
}


void WorkshopProxy::onUpdateTraderValue( int value )
{
	if( m_parent )
	{
		m_parent->updateTraderValue( value );
	}
}
	
void WorkshopProxy::onUpdatePlayerValue( int value )
{
	if( m_parent )
	{
		m_parent->updatePlayerValue( value );
	}
}

void WorkshopProxy::trade( unsigned int workshopID )
{
	emit signalTrade( workshopID );
}

void WorkshopProxy::setFisherOptions( unsigned int WorkshopID, bool catchFish, bool processFish )
{
	if( !m_blockWriteBack )
	{
		emit signalSetFisherOptions( WorkshopID, catchFish, processFish );
	}
}

void WorkshopProxy::blockWriteBack()
{
	m_blockWriteBack = true;
}
	
void WorkshopProxy::unblockWriteBack()
{
	m_blockWriteBack = false;
}