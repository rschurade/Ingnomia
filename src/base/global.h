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

#include "../base/enums.h"
#include "../base/logger.h"

#include <NsGui/InputEnums.h>

#include <QDomElement>
#include <QMap>
#include <QSet>
#include <QString>
#include <QVariant>
#include <QtGlobal>

class FarmingManager;
class Inventory;
class ItemHistory;
class StockpileManager;
class World;
class WorkshopManager;
class JobManager;
class RoomManager;
class GnomeManager;
class CreatureManager;
//class KeyBindings;
class EventManager;
class Logger;
class MechanismManager;
class FluidManager;
class NeighborManager;
class MilitaryManager;

class SpriteFactory;

class Global
{
public:
	Global()  = delete;
	~Global() = delete;

	static void reset();

	static Logger& logger();

	static Inventory& inv();
	static ItemHistory& ih();
	static JobManager& jm();
	static StockpileManager& spm();
	static FarmingManager& fm();
	static WorkshopManager& wsm();
	static World& w();
	static SpriteFactory& sf();
	static RoomManager& rm();
	static GnomeManager& gm();
	static CreatureManager& cm();
	static EventManager& em();
	static MechanismManager& mcm();
	static FluidManager& flm();
	static NeighborManager& nm();
	static MilitaryManager& mil();

	static QDomElement behaviorTree( QString id );

	//static KeyBindings& keyBindings();
	static bool wallsLowered;
	static bool showAxles;

	static unsigned int waterSpriteUID;
	static unsigned int undiscoveredUID;

	static QVariantMap copiedStockpileSettings;

	static QMap<QString, QVariantMap> m_windowParams;

	static int dimX;
	static int dimY;
	static int dimZ;
	static int zWeight;
	static double xpMod;
	static bool debugMode;

	static QMap<QString, QSet<QString>> allowedInContainer;

	static QStringList needIDs;
	static QMap<QString, float> needDecays;

	static unsigned int dirtUID;

	static bool addBehaviorTree( QString id, QString path );

	static QHash<Qt::Key, Noesis::Key> keyConvertMap;

	static QMap<QString, CreaturePart> creaturePartLookUp;
	static QMap<CreaturePart, QString> creaturePartToString;

	static Noesis::Key keyConvert( Qt::Key key );

	static QSet<QString> craftable;

private:
	static Logger m_logger;

	static Inventory m_inventory;
	static ItemHistory m_itemHistory;
	static JobManager m_jobManager;
	static StockpileManager m_stockpileManager;
	static FarmingManager m_farmingManager;
	static WorkshopManager m_workshopManager;
	static RoomManager m_roomManager;
	static World m_world;
	static SpriteFactory m_spriteFactory;
	static GnomeManager m_gnomeManager;
	static CreatureManager m_creatureManager;
	static EventManager m_eventManager;
	static MechanismManager m_mechanismManager;
	static FluidManager m_fluidManager;
	static NeighborManager m_neighborManager;
	static MilitaryManager m_militaryManager;

	//static KeyBindings m_keyBindings;

	static QMap<QString, QDomElement> m_behaviorTrees;
	static bool loadBehaviorTrees();
	static void initKeyConvert();
};
