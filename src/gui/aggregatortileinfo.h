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
#include "../game/creature.h"
#include "../game/roommanager.h"
#include "../game/mechanismmanager.h"
#include "aggregatorstockpile.h"

#include <QObject>

#include <absl/container/flat_hash_set.h>

#include <sigslot/signal.hpp>

class Game;

struct GuiTICreatureInfo
{
	QString text;
	unsigned int id = 0;
	CreatureType type;
	bool refuel;
	QString coreItem;
};

struct GuiItemInfo
{
	QString text;
	unsigned int id = 0;
	QString material;
	unsigned int count = 0;
	bool inStockpile = false;
	bool inContainer = false;
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
	QString requiredSkill;
	std::string requiredTool;
	QString requiredToolAvailable;
	QList<GuiItemInfo> requiredItems;
	QString workPositions;

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
	unsigned int roomValue = 0;

	MechanismData mechInfo;
};

Q_DECLARE_METATYPE( GuiTileInfo )

class AggregatorTileInfo : public QObject
{
	Q_OBJECT

public:
	AggregatorTileInfo( QObject* parent = nullptr );
	~AggregatorTileInfo();

	void init( Game* game );

private:
    QPointer<Game> g;

	unsigned int m_currentTileID = 0;
	bool m_tileInfoDirty         = false;
	GuiTileInfo m_tileInfo;
	GuiStockpileInfo m_spInfo;

public slots:
	void onShowTileInfo( unsigned int tileID );
	void onUpdateAnyTileInfo( const absl::flat_hash_set<unsigned int>& changeSet );
	void onUpdateTileInfo( unsigned int tileID );
	void onRequestStockpileItems( unsigned int tileID );
	void onSetTennant( unsigned int designationID, unsigned int gnomeID );
	void onSetAlarm( unsigned int designationID, bool value );
	void onToggleMechActive( unsigned int id );
	void onToggleMechInvert( unsigned int id );
	void onSetAutomatonRefuel( unsigned int id, bool refuel );
	void onSetAutomatonCore( unsigned int id, QString core );

public: // signals:
	sigslot::signal<unsigned int /*id*/> signalShowTileInfo;
	sigslot::signal<const GuiTileInfo& /*info*/> signalUpdateTileInfo;
	sigslot::signal<const GuiStockpileInfo& /*info*/> signalUpdateSPInfo;
};
