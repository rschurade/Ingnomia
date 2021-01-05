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
#pragma once

#include "../aggregatorworkshop.h"
#include "workshopmodel.h"

#include <QObject>

class WorkshopProxy : public QObject
{
	Q_OBJECT

public:
	WorkshopProxy( QObject* parent = nullptr );
	~WorkshopProxy();

	void setParent( IngnomiaGUI::WorkshopModel* parent );

	void setBasicOptions( unsigned int WorkshopID, QString name, int priority, bool suspended, bool acceptGenerated, bool autoCraftMissing, bool connectStockpile );
	void setButcherOptions( unsigned int WorkshopID, bool butcherCorpses, bool butcherExcess );
	void setFisherOptions( unsigned int WorkshopID, bool catchFish, bool processFish );

	void craftItem( QString sid, int mode, int number, QStringList mats );

	void craftJobCommand( unsigned int craftJobID, QString command );
	void craftJobParams( unsigned int craftJobID, int mode, QString numString, bool suspended, bool moveBack );

	void requestAllTradeItems( unsigned int workshopID );

	void traderStocktoOffer( unsigned int workshopID, QString itemSID, QString materialSID, unsigned char quality, int count );
	void traderOffertoStock( unsigned int workshopID, QString itemSID, QString materialSID, unsigned char quality, int count );
	void playerStocktoOffer( unsigned int workshopID, QString itemSID, QString materialSID, unsigned char quality, int count );
	void playerOffertoStock( unsigned int workshopID, QString itemSID, QString materialSID, unsigned char quality, int count );

	void trade( unsigned int workshopID );

	void blockWriteBack();
	void unblockWriteBack();

private:
	IngnomiaGUI::WorkshopModel* m_parent = nullptr;

	unsigned int m_workshopID = 0;

	bool m_blockWriteBack = false;

private slots:
	void onUpdateInfo( const GuiWorkshopInfo& info );
	void onUpdateCraftList( const GuiWorkshopInfo& info );

	void onUpdateTraderStock( const QList<GuiTradeItem>& items );
	void onUpdatePlayerStock( const QList<GuiTradeItem>& items );
	
	void onUpdateTraderStockItem( const GuiTradeItem& item );
	void onUpdatePlayerStockItem( const GuiTradeItem& item );

	void onUpdateTraderValue( int value );
	void onUpdatePlayerValue( int value );

signals:
	void signalSetBasicOptions( unsigned int WorkshopID, QString name, int priority, bool suspended, bool acceptGenerated, bool autoCraftMissing, bool connectStockpile );
	void signalSetButcherOptions( unsigned int WorkshopID, bool butcherCorpses, bool butcherExcess );
	void signalSetFisherOptions( unsigned int WorkshopID, bool catchFish, bool processFish );
	void signalCraftItem( unsigned int WorkshopID, QString craftID, int mode, int number, QStringList mats );
	void signalCraftJobCommand( unsigned int workshopID, unsigned int craftJobID, QString command );
	void signalCraftJobParams( unsigned int workshopID, unsigned int craftJobID, int mode, int numToCraft, bool suspended, bool moveBack );
	void signalRequestAllTradeItems( unsigned int workshopID );

	void signalTraderStocktoOffer( unsigned int workshopID, QString itemSID, QString materialSID, unsigned char quality, int count );
	void signalTraderOffertoStock( unsigned int workshopID, QString itemSID, QString materialSID, unsigned char quality, int count );
	void signalPlayerStocktoOffer( unsigned int workshopID, QString itemSID, QString materialSID, unsigned char quality, int count );
	void signalPlayerOffertoStock( unsigned int workshopID, QString itemSID, QString materialSID, unsigned char quality, int count );

	void signalTrade( unsigned int workshopID );
};
