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

#include "../base/tile.h"
#include "../game/roommanager.h"
#include "aggregatorstockpile.h"

#include <QObject>

struct GuiTICreatureInfo
{
	QString text;
	unsigned int id = 0;
};

struct GuiItemInfo
{
	QString text;
	unsigned int id;
};

struct GuiTileInfo
{
	unsigned int tileID = 0;
	TileFlag flags      = TileFlag::TF_NONE;

	int numGnomes   = 0;
	int numAnimals  = 0;
	int numMonsters = 0;

	int numItems = 0;

	QString wall;
	QString floor;
	QString embedded;
	QString plant;
	bool plantIsTree;
	bool plantIsHarvestable;
	QString water;
	QString constructed;

	QString jobName;
	QString jobWorker;
	QString jobPriority;
	QString requiredTool;

	QList<GuiTICreatureInfo> creatures;
	QList<GuiItemInfo> items;
	QList<GuiTICreatureInfo> possibleTennants;
	unsigned int tennant = 0;

	unsigned int designationID = 0;
	TileFlag designationFlag   = TileFlag::TF_NONE;
	QString designationName;

	RoomType roomType = RoomType::NotSet;
	bool hasAlarmBell = false;
	bool isEnclosed   = false;
	bool hasRoof      = false;
	QString beds;
	bool alarm = false;
};

Q_DECLARE_METATYPE( GuiTileInfo )

class AggregatorTileInfo : public QObject
{
	Q_OBJECT

public:
	AggregatorTileInfo( QObject* parent = nullptr );
	~AggregatorTileInfo();

private:
	unsigned int m_currentTileID = 0;
	bool m_tileInfoDirty         = false;
	GuiTileInfo m_tileInfo;
	GuiStockpileInfo m_spInfo;

public slots:
	void onShowTileInfo( unsigned int tileID );
	void onUpdateAnyTileInfo( const QSet<unsigned int>& changeSet );
	void onUpdateTileInfo( unsigned int tileID );
	void onRequestStockpileItems( unsigned int tileID );
	void onSetTennant( unsigned int designationID, unsigned int gnomeID );
	void onSetAlarm( unsigned int designationID, bool value );

signals:
	void signalShowTileInfo( unsigned int id );
	void signalUpdateTileInfo( const GuiTileInfo& info );
	void signalUpdateSPInfo( const GuiStockpileInfo& info );
};
