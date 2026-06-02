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
/** @file aggregatortileinfo.h
 *  @brief Data types and aggregator feeding the Tile Info XAML window: flags, terrain, items,
 *         creatures, active jobs, room membership, mechanism status, and tennant assignment.
 */
#pragma once

#include "../base/tile.h"
#include "../game/creature.h"
#include "../game/roommanager.h"
#include "../game/mechanismmanager.h"
#include "aggregatorstockpile.h"

#include <QObject>

class Game;

/// @brief Creature summary shown on the Tile Info window.
struct GuiTICreatureInfo
{
	QString text;        ///< Display label ("Jim, Swordsman").
	unsigned int id = 0; ///< Creature UID.
	CreatureType type;   ///< Creature kind (gnome/animal/monster/automaton).
	bool refuel;         ///< Automaton refuel flag (for automatons only).
	QString coreItem;    ///< Automaton core item ID (for automatons only).
};

/// @brief Single item row shown on the Tile Info window.
struct GuiItemInfo
{
	QString text;              ///< Display label.
	unsigned int id = 0;       ///< Item UID.
	QString material;          ///< Material string ID.
	unsigned int count = 0;    ///< Stack count on this tile.
	bool inStockpile = false;  ///< True if the item is held by a stockpile.
	bool inContainer = false;  ///< True if the item is inside a container.
};

/// @brief Full Tile Info payload.
struct GuiTileInfo
{
	unsigned int tileID = 0;         ///< Tile ID (Position::toInt()).
	TileFlag flags      = TileFlag::TF_NONE; ///< Raw tile flags.

	int numGnomes   = 0;             ///< Number of gnomes on this tile.
	int numAnimals  = 0;             ///< Number of animals on this tile.
	int numMonsters = 0;             ///< Number of monsters on this tile.

	int numItems = 0;                ///< Number of items on this tile.

	QString wall;                    ///< Wall description.
	QString floor;                   ///< Floor description.
	QString embedded;                ///< Embedded ore/gem description.
	QString plant;                   ///< Plant description.
	bool plantIsTree;                ///< True if the plant is a tree.
	bool plantIsHarvestable;         ///< True if the plant currently has harvestable produce.
	QString water;                   ///< Water level description.
	QString constructed;             ///< Constructed item description.

	QString jobName;                 ///< Active job name, if any.
	QString jobWorker;               ///< Name of the gnome assigned to the job.
	QString jobPriority;             ///< Job priority label.
	QString requiredSkill;           ///< Required skill for the job.
	QString requiredTool;            ///< Required tool name.
	QString requiredToolAvailable;   ///< Availability label for the tool.
	QList<GuiItemInfo> requiredItems;///< Required component items.
	QString workPositions;           ///< Work position descriptor.

	QList<GuiTICreatureInfo> creatures; ///< Creatures on this tile.
	QList<GuiItemInfo> items;           ///< Items on this tile.
	QList<GuiTICreatureInfo> possibleTennants; ///< Eligible tennants for a bed/chair.
	unsigned int tennant = 0;           ///< Currently assigned tennant UID.

	unsigned int designationID = 0;     ///< UID of a designation containing this tile.
	TileFlag designationFlag   = TileFlag::TF_NONE; ///< Designation flag identifying its kind.
	QString designationName;            ///< Designation name.

	RoomType roomType = RoomType::NotSet; ///< Room kind (dorm, dining, …) if applicable.
	bool hasAlarmBell = false;         ///< True if the room has an alarm bell.
	bool isEnclosed   = false;         ///< True if the room is fully enclosed.
	bool hasRoof      = false;         ///< True if the room has a roof.
	QString beds;                      ///< Bed availability description.
	bool alarm = false;                ///< True if the alarm is currently on.
	unsigned int roomValue = 0;        ///< Aggregated room value (furniture quality).

	MechanismData mechInfo;            ///< Mechanism state if a gear/axle/lever/pressure plate is here.
};

Q_DECLARE_METATYPE( GuiTileInfo )

/// @brief Bridges the Tile Info XAML window with the game: inspects a clicked tile and
///        exposes terrain, jobs, creatures, items, rooms, and mechanism state.
class AggregatorTileInfo : public QObject
{
	Q_OBJECT

public:
	AggregatorTileInfo( QObject* parent = nullptr );
	~AggregatorTileInfo();

	void init( Game* game );

private:
    QPointer<Game> g;                    ///< Game instance (weak ownership).

	unsigned int m_currentTileID = 0;    ///< Currently shown tile UID.
	bool m_tileInfoDirty         = false;///< Reserved for batched refresh.
	GuiTileInfo m_tileInfo;              ///< Cached payload for the current tile.
	GuiStockpileInfo m_spInfo;           ///< Cached payload when the tile is a stockpile.

public slots:
	void onShowTileInfo( unsigned int tileID );
	void onUpdateAnyTileInfo( const QSet<unsigned int>& changeSet );
	void onUpdateTileInfo( unsigned int tileID );
	void onRequestStockpileItems( unsigned int tileID );
	void onSetTennant( unsigned int designationID, unsigned int gnomeID );
	void onSetAlarm( unsigned int designationID, bool value );
	void onToggleMechActive( unsigned int id );
	void onToggleMechInvert( unsigned int id );
	void onSetAutomatonRefuel( unsigned int id, bool refuel );
	void onSetAutomatonCore( unsigned int id, QString core );

signals:
	void signalShowTileInfo( unsigned int id );
	void signalUpdateTileInfo( const GuiTileInfo& info );
	void signalUpdateSPInfo( const GuiStockpileInfo& info );
};
