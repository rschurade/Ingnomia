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

	static QMap<QString, QVariantMap> m_windowParams;

	static int dimX;
	static int dimY;
	static int dimZ;
	static int zWeight;
	static double xpMod;
	static bool debugMode;
	static bool debugOpenGL;
	static bool debugSound;

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

	static EventConnector* eventConnector;
	static Util* util;
	static Selection* sel;
	static NewGameSettings* newGameSettings;
	static Config* cfg;

private:
	static Logger m_logger;

	//static KeyBindings m_keyBindings;

	static QMap<QString, QDomElement> m_behaviorTrees;
	static bool loadBehaviorTrees();
	static void initKeyConvert();
};
