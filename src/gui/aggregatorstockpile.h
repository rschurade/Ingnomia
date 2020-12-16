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

#include "../base/filter.h"
#include "../game/stockpile.h"

#include <QObject>

class Game;

struct ItemsSummary
{
	QString itemName;
	QString materialName;
	int count;
	//QList<unsigned int> ids;
};

struct GuiStockpileInfo
{
	unsigned int stockpileID = 0;

	QString name;
	int priority           = 1;
	int maxPriority        = 1;
	bool suspended         = false;
	bool allowPullFromHere = false;
	bool pullFromOthers    = false;
	int capacity           = 0;
	int itemCount          = 0;
	int reserved           = 0;

	Filter filter;

	QList<ItemsSummary> summary;
};

Q_DECLARE_METATYPE( GuiStockpileInfo )

class AggregatorStockpile : public QObject
{
	Q_OBJECT

public:
	AggregatorStockpile( QObject* parent = nullptr );
	~AggregatorStockpile();

	void init( Game* game );

private:
	QPointer<Game> g;

	bool m_infoDirty    = false;
	bool m_contentDirty = false;

	GuiStockpileInfo m_info;

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