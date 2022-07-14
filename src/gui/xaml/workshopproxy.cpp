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

#include <QPainter>

WorkshopProxy::WorkshopProxy( QObject* parent ) :
	QObject( parent )
{
	Global::eventConnector->aggregatorWorkshop()->signalUpdateInfo.connect(&WorkshopProxy::onUpdateInfo, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorWorkshop()->signalUpdateCraftList.connect(&WorkshopProxy::onUpdateCraftList, this); // TODO: Qt::QueuedConnection

	this->signalSetBasicOptions.connect(&AggregatorWorkshop::onSetBasicOptions, Global::eventConnector->aggregatorWorkshop()); // TODO: Qt::QueuedConnection
	this->signalSetButcherOptions.connect(&AggregatorWorkshop::onSetButcherOptions, Global::eventConnector->aggregatorWorkshop()); // TODO: Qt::QueuedConnection
	this->signalSetFisherOptions.connect(&AggregatorWorkshop::onSetFisherOptions, Global::eventConnector->aggregatorWorkshop()); // TODO: Qt::QueuedConnection
	this->signalCraftItem.connect(&AggregatorWorkshop::onCraftItem, Global::eventConnector->aggregatorWorkshop()); // TODO: Qt::QueuedConnection
	this->signalCraftJobCommand.connect(&AggregatorWorkshop::onCraftJobCommand, Global::eventConnector->aggregatorWorkshop()); // TODO: Qt::QueuedConnection
	this->signalCraftJobParams.connect(&AggregatorWorkshop::onCraftJobParams, Global::eventConnector->aggregatorWorkshop()); // TODO: Qt::QueuedConnection

	this->signalRequestAllTradeItems.connect(&AggregatorWorkshop::onRequestAllTradeItems, Global::eventConnector->aggregatorWorkshop()); // TODO: Qt::QueuedConnection

	Global::eventConnector->aggregatorWorkshop()->signalTraderStock.connect(&WorkshopProxy::onUpdateTraderStock, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorWorkshop()->signalPlayerStock.connect(&WorkshopProxy::onUpdatePlayerStock, this); // TODO: Qt::QueuedConnection

	Global::eventConnector->aggregatorWorkshop()->signalUpdateTraderStockItem.connect(&WorkshopProxy::onUpdateTraderStockItem, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorWorkshop()->signalUpdatePlayerStockItem.connect(&WorkshopProxy::onUpdatePlayerStockItem, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorWorkshop()->signalUpdateTraderValue.connect(&WorkshopProxy::onUpdateTraderValue, this); // TODO: Qt::QueuedConnection
	Global::eventConnector->aggregatorWorkshop()->signalUpdatePlayerValue.connect(&WorkshopProxy::onUpdatePlayerValue, this); // TODO: Qt::QueuedConnection

	this->signalTraderStocktoOffer.connect(&AggregatorWorkshop::onTraderStocktoOffer, Global::eventConnector->aggregatorWorkshop()); // TODO: Qt::QueuedConnection
	this->signalTraderOffertoStock.connect(&AggregatorWorkshop::onTraderOffertoStock, Global::eventConnector->aggregatorWorkshop()); // TODO: Qt::QueuedConnection
	this->signalPlayerStocktoOffer.connect(&AggregatorWorkshop::onPlayerStocktoOffer, Global::eventConnector->aggregatorWorkshop()); // TODO: Qt::QueuedConnection
	this->signalPlayerOffertoStock.connect(&AggregatorWorkshop::onPlayerOffertoStock, Global::eventConnector->aggregatorWorkshop()); // TODO: Qt::QueuedConnection
	
	this->signalTrade.connect(&AggregatorWorkshop::onTrade, Global::eventConnector->aggregatorWorkshop()); // TODO: Qt::QueuedConnection
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
		signalSetBasicOptions( WorkshopID, name, priority, suspended, acceptGenerated, autoCraftMissing, connectStockpile );
	}
}

void WorkshopProxy::craftItem( QString craftID, int mode, int number, QStringList mats )
{
	signalCraftItem( m_workshopID, craftID, mode, number, mats );
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
	signalCraftJobCommand( m_workshopID, craftJobID, command );
}

void WorkshopProxy::craftJobParams( unsigned int craftJobID, int mode, QString numString, bool suspended, bool moveBack )
{
	int num = qMax( 1, numString.toInt() );
	signalCraftJobParams( m_workshopID, craftJobID, mode, num, suspended, moveBack );
}

void WorkshopProxy::setButcherOptions( unsigned int workshopID, bool butcherCorpses, bool butcherExcess )
{
	signalSetButcherOptions( workshopID, butcherCorpses, butcherExcess );
}

void WorkshopProxy::requestAllTradeItems( unsigned int workshopID )
{
	signalRequestAllTradeItems( workshopID );
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
	signalTraderStocktoOffer( workshopID, itemSID, materialSID, quality, count );
}
	
void WorkshopProxy::traderOffertoStock( unsigned int workshopID, QString itemSID, QString materialSID, unsigned char quality, int count )
{
	signalTraderOffertoStock( workshopID, itemSID, materialSID, quality, count );
}

void WorkshopProxy::playerStocktoOffer( unsigned int workshopID, QString itemSID, QString materialSID, unsigned char quality, int count )
{
	signalPlayerStocktoOffer( workshopID, itemSID, materialSID, quality, count );
}

void WorkshopProxy::playerOffertoStock( unsigned int workshopID, QString itemSID, QString materialSID, unsigned char quality, int count )
{
	signalPlayerOffertoStock( workshopID, itemSID, materialSID, quality, count );
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
	signalTrade( workshopID );
}

void WorkshopProxy::setFisherOptions( unsigned int WorkshopID, bool catchFish, bool processFish )
{
	if( !m_blockWriteBack )
	{
		signalSetFisherOptions( WorkshopID, catchFish, processFish );
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