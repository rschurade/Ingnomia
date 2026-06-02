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
/** @file aggregatorstockpile.h
 *  @brief Data types and aggregator for the Stockpile XAML window: exposes priority, capacity,
 *         pull/allow-pull flags, the active item filter, and a human-readable content summary.
 */
#pragma once

#include "../base/filter.h"
#include "../game/stockpile.h"

#include <QObject>

class Game;

/// @brief One line in the stockpile content summary (item type × material × count).
struct ItemsSummary
{
	QString itemName;      ///< Localised item name.
	QString materialName;  ///< Localised material name.
	int count;             ///< Count in this stockpile.
	//QList<unsigned int> ids;
};

/// @brief Full state of one stockpile sent to the Stockpile window.
struct GuiStockpileInfo
{
	unsigned int stockpileID = 0;  ///< Stockpile UID.

	QString name;                  ///< Display name.
	int priority           = 1;    ///< Current priority index.
	int maxPriority        = 1;    ///< Priority range upper bound.
	bool suspended         = false;///< True if the stockpile is paused.
	bool allowPullFromHere = false;///< True if other stockpiles may pull from this one.
	bool pullFromOthers    = false;///< True if this stockpile pulls from others.
	int capacity           = 0;    ///< Total storage slots.
	int itemCount          = 0;    ///< Items currently stored.
	int reserved           = 0;    ///< Items in-transit (reserved).

	Filter filter;                 ///< Item filter tree (category → group → item → material).

	QList<ItemsSummary> summary;   ///< Aggregated per-line content rows.
};

Q_DECLARE_METATYPE( GuiStockpileInfo )

/// @brief Bridges the Stockpile XAML window with the game-side StockpileManager. Produces
///        GuiStockpileInfo payloads and forwards edits (priority, pull flags, filter toggles).
class AggregatorStockpile : public QObject
{
	Q_OBJECT

public:
	AggregatorStockpile( QObject* parent = nullptr );
	~AggregatorStockpile();

	void init( Game* game );

private:
	QPointer<Game> g;                ///< Game instance (weak ownership).

	bool m_infoDirty    = false;     ///< True when basic info needs re-emit.
	bool m_contentDirty = false;     ///< True when the content summary needs re-emit.

	GuiStockpileInfo m_info;         ///< Cached payload for the currently open stockpile.

	bool aggregate( unsigned int stockpileID );

public slots:
	void onOpenStockpileInfoOnTile( unsigned int tileID );
	void onOpenStockpileInfo( unsigned int stockpileID );
	void onUpdateStockpileInfo( unsigned int stockpileID );
	void onUpdateStockpileContent( unsigned int stockpileID );
	void onUpdateAfterTick();

	void onSetBasicOptions( unsigned int stockpileID, QString name, int priority, bool suspended, bool pull, bool allowPull );
	void onSetActive( unsigned int stockpileID, bool active, QString category, QString group, QString item, QString material );

	void onCloseWindow();
signals:
	void signalOpenStockpileWindow( unsigned int stockpileID );
	void signalUpdateInfo( const GuiStockpileInfo& info );
	void signalUpdateContent( const GuiStockpileInfo& info );
};