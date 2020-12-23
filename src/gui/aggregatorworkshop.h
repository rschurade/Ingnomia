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

#include "../game/workshop.h"


#include <QObject>

class Game;
struct TraderItem;

struct GuiWorkshopMaterial
{
	QString sid;
	int amount;
};

struct GuiWorkshopComponent
{
	QPointer<Game> g;
	QString sid;
	int amount;
	bool requireSame;

	void updateMaterials( QVariantMap row );

	// the available materials with count for that item
	QList<GuiWorkshopMaterial> materials;
};

struct GuiWorkshopProduct
{
	QPointer<Game> g;
	QString sid;

	void updateComponents();

	QList<GuiWorkshopComponent> components;
};

struct GuiWorkshopInfo
{
	unsigned int workshopID = 0;

	QString name;
	int priority          = 1;
	int maxPriority       = 1;
	bool suspended        = false;
	bool autoCraftMissing = false;
	bool acceptGenerated  = false;
	bool linkStockpile    = false;
	bool butcherExcess    = false;
	bool butcherCorpses   = false;
	bool catchFish        = false;
	bool processFish      = false;

	QString gui;

	QList<GuiWorkshopProduct> products;

	QList<CraftJob> jobList;
};
Q_DECLARE_METATYPE( GuiWorkshopInfo )


struct GuiTradeItem
{
	QString name;
	QString itemSID;
	QString materialSIDorGender;
	unsigned char quality = 0;
	int count = 0;
	int reserved = 0;
	int value = 0;
};
Q_DECLARE_METATYPE( GuiTradeItem )



class AggregatorWorkshop : public QObject
{
	Q_OBJECT

public:
	AggregatorWorkshop( QObject* parent = nullptr );
	~AggregatorWorkshop();

	void init( Game* game );

private:
    QPointer<Game> g;

	bool m_infoDirty    = false;
	bool m_contentDirty = false;

	GuiWorkshopInfo m_info;

	bool aggregate( unsigned int workshopID );
	bool updateCraftList( unsigned int workshopID );

	QList<GuiTradeItem> m_traderStock;
	QList<GuiTradeItem> m_playerStock;

	unsigned int m_traderID = 0;

	int m_traderOfferValue = 0;
	int m_playerOfferValue = 0;

	void updateTraderStock( unsigned int workshopID );
	void updatePlayerStock( unsigned int workshopID );

	void updateTraderStock( const QList<TraderItem>& source );

	void updateTraderValue();
	void updatePlayerValue();

public slots:
	void onOpenWorkshopInfoOnTile( unsigned int tileID );
	void onOpenWorkshopInfo( unsigned int workshopID );
	void onUpdateWorkshopInfo( unsigned int workshopID );
	void onUpdateWorkshopContent( unsigned int workshopID );
	void onUpdateAfterTick();

	void onSetBasicOptions( unsigned int workshopID, QString name, int priority, bool suspended, bool acceptGenerated, bool autoCraftMissing, bool connectStockpile );
	void onSetButcherOptions( unsigned int WorkshopID, bool butcherCorpses, bool butcherExcess );
	void onSetFisherOptions( unsigned int WorkshopID, bool catchFish, bool processFish );
	void onCraftItem( unsigned int workshopID, QString craftID, int mode, int number, QStringList mats );
	void onCraftJobCommand( unsigned int workshopID, unsigned int craftJobID, QString command );
	void onCraftJobParams( unsigned int workshopID, unsigned int craftJobID, int mode, int numToCraft, bool suspended, bool moveBack );
	void onCraftListChanged( unsigned int workshopID );

	void onRequestAllTradeItems( unsigned int workshopID );

	void onTraderStocktoOffer( unsigned int workshopID, QString itemSID, QString materialSID, unsigned char quality, int count );
	void onTraderOffertoStock( unsigned int workshopID, QString itemSID, QString materialSID, unsigned char quality, int count );
	void onPlayerStocktoOffer( unsigned int workshopID, QString itemSID, QString materialSID, unsigned char quality, int count );
	void onPlayerOffertoStock( unsigned int workshopID, QString itemSID, QString materialSID, unsigned char quality, int count );

	void onTrade( unsigned int workshopID );

	void onCloseWindow();
signals:
	void signalOpenWorkshopWindow( unsigned int workshopID );
	void signalUpdateInfo( const GuiWorkshopInfo& info );
	void signalUpdateContent( const GuiWorkshopInfo& info );
	void signalUpdateCraftList( const GuiWorkshopInfo& info );

	void signalTraderStock( const QList<GuiTradeItem>& items );
	void signalPlayerStock( const QList<GuiTradeItem>& items );
	
	void signalUpdateTraderStockItem( const GuiTradeItem& item );
	void signalUpdatePlayerStockItem( const GuiTradeItem& item );

	void signalUpdateTraderValue( int value );
	void signalUpdatePlayerValue( int value );
};