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
#include <absl/container/btree_map.h>
#include <absl/container/btree_set.h>
#include <absl/container/flat_hash_map.h>
#include <QString>
#include <QVariant>
#include <QtGlobal>

class EventConnector;
class Util;
class Logger;
class Selection;
class NewGameSettings;
class Config;

class Global
{
public:
	Global()  = delete;
	~Global() = delete;

	static void reset();

	static Logger& logger();

	static QDomElement behaviorTree( QString id );

	//static KeyBindings& keyBindings();
	static bool wallsLowered;
	static bool showDesignations;
	static bool showJobs;
	static bool showAxles;

	static unsigned int waterSpriteUID;
	static unsigned int undiscoveredUID;

	static QVariantMap copiedStockpileSettings;

	static absl::btree_map<QString, QVariantMap> m_windowParams;

	static int dimX;
	static int dimY;
	static int dimZ;
	static int zWeight;
	static double xpMod;
	static bool debugMode;
	static bool debugOpenGL;
	static bool debugSound;

	static absl::btree_map<QString, absl::btree_set<QString>> allowedInContainer;

	static QStringList needIDs;
	static absl::btree_map<QString, float> needDecays;

	static unsigned int dirtUID;

	static bool addBehaviorTree( QString id, QString path );

	static absl::flat_hash_map<Qt::Key, Noesis::Key> keyConvertMap;

	static absl::btree_map<QString, CreaturePart> creaturePartLookUp;
	static absl::btree_map<CreaturePart, QString> creaturePartToString;

	static Noesis::Key keyConvert( Qt::Key key );

	static absl::btree_set<QString> craftable;

	static EventConnector* eventConnector;
	static Util* util;
	static Selection* sel;
	static NewGameSettings* newGameSettings;
	static Config* cfg;

private:
	static Logger m_logger;

	//static KeyBindings m_keyBindings;

	static absl::btree_map<QString, QDomElement> m_behaviorTrees;
	static bool loadBehaviorTrees();
	static void initKeyConvert();
};
