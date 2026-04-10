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
/** @file dbhelper.h
 * @brief Cached database lookup helpers for frequently accessed game data.
 */

#pragma once

#include <QMap>
#include <QString>
#include <QStringList>

/**
 * @brief Static-only helper providing cached lookups for sprites, materials, items, and qualities.
 *
 * Aliased as DBH for convenience. Each method checks a static QMap cache first;
 * on a miss it queries the DB and caches the result. Material/item UID methods
 * use GameState's bidirectional maps instead of the DB. Cannot be instantiated.
 */
class DBHelper
{
public:
	static QString spriteID( QString itemID );
	static bool spriteIsRandom( QString spriteID );
	static bool spriteHasAnim( QString spriteID );
	static QString materialColor( QString materialID );

	static int materialToolLevel( QString material );

	static int materialUID( QString material );
	static QString materialSID( int material );

	static int itemUID( QString item );
	static QString itemSID( int item );
	static bool itemIsContainer( int item );

	static int rowID( QString table, QString id );
	static QString id( QString table, int rowID );

	static QString qualitySID( int rank );
	static float qualityMod( int rank );

	static QString itemGroup( QString itemID );

	static QMap<QString, QMultiMap<QString, QString>> workshopPossibleCraftResults( QString workshopId );

private:
	DBHelper()  = delete;
	~DBHelper() = delete;

	static QMap<QString, QString> m_spriteIDCache;  ///< Cache: itemID -> spriteID.
	static QMap<QString, bool> m_spriteIsRandomCache; ///< Cache: spriteID -> hasRandom.
	static QMap<QString, bool> m_spriteHasAnimCache;  ///< Cache: spriteID -> hasAnim.
	static QMap<QString, QString> m_materialColorCache; ///< Cache: materialID -> color string.
	static QMap<QString, int> m_materialToolLevelCache; ///< Cache: materialID -> tool level.
	static QMap<int, bool> m_itemIsContainerCache;      ///< Cache: itemUID -> isContainer.
	static QMap<int, QString> m_qualitySIDCache;        ///< Cache: rank -> quality name.
	static QMap<int, float> m_qualityModCache;          ///< Cache: rank -> quality modifier.
	static QMap<QString, QString> m_itemGroupCache;     ///< Cache: itemID -> group name.
	static QMap<QString, QMap<QString, QMultiMap<QString, QString>>> m_workshopCraftResults; ///< Cache: workshopID -> craft results.

	static QMutex m_mutex; ///< Mutex protecting cache access.
};
