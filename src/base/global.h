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
/** @file global.h
 * @brief Central static registry for shared subsystem pointers, world dimensions, and global flags.
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

/**
 * @brief Static-only class holding shared global state for the entire application.
 *
 * Provides access to major subsystems (Config, EventConnector, Util, Selection),
 * world dimensions, rendering flags, behavior trees, key mappings, and debug settings.
 * Cannot be instantiated — all members are static.
 */
class Global
{
public:
	Global()  = delete;
	~Global() = delete;

	static void reset();
	static Logger& logger();
	static QDomElement behaviorTree( QString id );

	static bool wallsLowered;          ///< Whether walls are rendered in lowered (cutaway) mode.
	static bool showDesignations;      ///< Whether designation overlays are visible.
	static bool showJobs;              ///< Whether job sprite overlays are visible.
	static bool showAxles;             ///< Whether axle/mechanism overlays are visible.

	static unsigned int waterSpriteUID; ///< Sprite UID for water rendering.
	static unsigned int undiscoveredUID; ///< Sprite UID for undiscovered/fog-of-war tiles.

	static QVariantMap copiedStockpileSettings; ///< Clipboard for copy/paste stockpile filter settings.

	static QMap<QString, QVariantMap> m_windowParams; ///< Saved window positions and sizes.

	static int dimX;    ///< World width in tiles.
	static int dimY;    ///< World depth in tiles.
	static int dimZ;    ///< World height in z-levels.
	static int zWeight; ///< Z-level weight for pathfinding cost calculations.
	static double xpMod; ///< Experience gain multiplier.
	static bool debugMode;   ///< Whether debug mode is enabled.
	static bool debugOpenGL; ///< Whether OpenGL debug output is enabled.
	static bool debugSound;  ///< Whether sound debug output is enabled.

	static float debugNeedDecayMultiplier;  ///< Debug multiplier for gnome need decay rate.
	static QSet<QString> disabledNeedDecays; ///< Set of need IDs with decay disabled (debug).

	static QMap<QString, QSet<QString>> allowedInContainer; ///< Item types allowed in each container type.

	static QStringList needIDs;            ///< List of all gnome need identifiers.
	static QMap<QString, float> needDecays; ///< Base decay rate per need ID.

	static unsigned int dirtUID; ///< Sprite UID for dirt/soil.

	static bool addBehaviorTree( QString id, QString path );

	static QHash<Qt::Key, Noesis::Key> keyConvertMap; ///< Qt key to Noesis key mapping table.

	static QMap<QString, CreaturePart> creaturePartLookUp;  ///< String to CreaturePart enum lookup.
	static QMap<CreaturePart, QString> creaturePartToString; ///< CreaturePart enum to string lookup.

	static Noesis::Key keyConvert( Qt::Key key );

	static QSet<QString> craftable; ///< Set of item SIDs that can be crafted.

	static EventConnector* eventConnector; ///< Global event connector for GUI-game thread communication.
	static Util* util;                     ///< Global utility functions instance.
	static Selection* sel;                 ///< Current user selection state.
	static NewGameSettings* newGameSettings; ///< New game configuration settings.
	static Config* cfg;                    ///< Application configuration store.

private:
	static Logger m_logger; ///< Application-wide logger instance.

	static QMap<QString, QDomElement> m_behaviorTrees; ///< Loaded behavior tree XML elements by ID.

	static bool loadBehaviorTrees();
	static void initKeyConvert();
};
