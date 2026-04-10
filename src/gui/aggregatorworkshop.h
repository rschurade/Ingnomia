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
/** @file aggregatorworkshop.h
 *  @brief Data types and aggregator feeding the Workshop XAML window: per-workshop settings,
 *         product list with available component materials, craft job queue, and the trader
 *         stock/offer view for the TradingPost workshop.
 */
#pragma once

#include "../game/workshop.h"


#include <QObject>

class Game;
struct TraderItem;

/// @brief One available material row in the craft component picker.
struct GuiWorkshopMaterial
{
	QString sid;  ///< Material string ID.
	int amount;   ///< Currently available count.
};

/// @brief One required component of a craftable product.
struct GuiWorkshopComponent
{
	QPointer<Game> g;                      ///< Game instance (weak).
	QString sid;                           ///< Component item string ID.
	int amount;                            ///< Required amount per craft.
	bool requireSame;                      ///< True if all copies must share a material.

	void updateMaterials( QVariantMap row );

	QList<GuiWorkshopMaterial> materials;  ///< Available materials (with counts) for this component.
};

/// @brief One craftable product entry in the workshop product list.
struct GuiWorkshopProduct
{
	QPointer<Game> g;                      ///< Game instance (weak).
	QString sid;                           ///< Product item string ID.

	void updateComponents();

	QList<GuiWorkshopComponent> components;///< Required components with material options.
};

/// @brief Full workshop payload sent to the Workshop window.
struct GuiWorkshopInfo
{
	unsigned int workshopID = 0;      ///< Workshop UID.

	QString name;                     ///< Display name.
	int priority          = 1;        ///< Current priority index.
	int maxPriority       = 1;        ///< Priority range upper bound.
	bool suspended        = false;    ///< True if the workshop is paused.
	bool autoCraftMissing = false;    ///< Auto-queue craft jobs to refill requirements.
	bool acceptGenerated  = false;    ///< Accept auto-generated craft jobs from other systems.
	bool linkStockpile    = false;    ///< Pull components directly from linked stockpile.
	bool butcherExcess    = false;    ///< Butcher excess pasture animals (butcher workshop).
	bool butcherCorpses   = false;    ///< Butcher creature corpses (butcher workshop).
	bool catchFish        = false;    ///< Generate catch-fish jobs (fisher workshop).
	bool processFish      = false;    ///< Generate process-fish jobs (fisher workshop).

	QString gui;                      ///< XAML template name for this workshop's special GUI.

	QList<GuiWorkshopProduct> products; ///< Products this workshop can craft.

	QList<CraftJob> jobList;          ///< Current craft job queue.
};
Q_DECLARE_METATYPE( GuiWorkshopInfo )


/// @brief One row in the trade stock / offer list.
struct GuiTradeItem
{
	QString name;                      ///< Display name.
	QString itemSID;                   ///< Item string ID.
	QString materialSIDorGender;       ///< Material ID (items) or gender ("Male"/"Female") for animals.
	unsigned char quality = 0;         ///< Quality level.
	int count = 0;                     ///< Count in stock.
	int reserved = 0;                  ///< Count moved to the offer.
	int value = 0;                     ///< Unit trade value.
};
Q_DECLARE_METATYPE( GuiTradeItem )



/// @brief Bridges the Workshop XAML window (including the trader variant) with the game. Pushes
///        workshop state, craft job lists, and trader stock views; routes user edits back.
class AggregatorWorkshop : public QObject
{
	Q_OBJECT

public:
	AggregatorWorkshop( QObject* parent = nullptr );
	~AggregatorWorkshop();

	void init( Game* game );

private:
    QPointer<Game> g;                         ///< Game instance (weak ownership).

	bool m_infoDirty    = false;              ///< Unused (reserved for batch refresh).
	bool m_contentDirty = false;              ///< Unused (reserved for batch refresh).

	GuiWorkshopInfo m_info;                   ///< Cached payload for the open workshop.

	bool aggregate( unsigned int workshopID );
	bool updateCraftList( unsigned int workshopID );

	QList<GuiTradeItem> m_traderStock;        ///< Current trader stock view.
	QList<GuiTradeItem> m_playerStock;        ///< Current player stock view.

	unsigned int m_traderID = 0;              ///< UID of the active trader creature.

	int m_traderOfferValue = 0;               ///< Sum of values in the trader's offer column.
	int m_playerOfferValue = 0;               ///< Sum of values in the player's offer column.

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