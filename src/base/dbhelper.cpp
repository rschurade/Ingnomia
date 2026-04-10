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

/** @file dbhelper.cpp
 *  @brief Implementation of cached database lookup helpers (DBHelper / DBH).
 *
 *  Each public method checks its corresponding static QMap cache first. On a
 *  cache miss, the value is fetched from the SQLite database via DB:: calls,
 *  inserted into the cache, and returned. Material/item UID methods delegate
 *  to GameState's bidirectional maps instead of the DB.
 */

#include "dbhelper.h"

#include "../base/db.h"
#include "../base/gamestate.h"

QMap<QString, QString> DBHelper::m_spriteIDCache;
QMap<QString, bool> DBHelper::m_spriteIsRandomCache;
QMap<QString, bool> DBHelper::m_spriteHasAnimCache;
QMap<QString, QString> DBHelper::m_materialColorCache;
QMap<QString, int> DBHelper::m_materialToolLevelCache;
QMap<int, bool> DBHelper::m_itemIsContainerCache;
QMap<int, QString> DBHelper::m_qualitySIDCache;
QMap<int, float> DBHelper::m_qualityModCache;
QMap<QString, QString> DBHelper::m_itemGroupCache;
QMap<QString, QMap<QString, QMultiMap<QString, QString>>> DBHelper::m_workshopCraftResults;

QMutex DBHelper::m_mutex;

/*
QStringList DBHelper::getWorkPositions( QString jobID )
{
	QString wps = DB::select( "WorkPosition", "Jobs", jobID ).toString();
	return wps.split( "|" );
}
*/

/**
 * @brief Look up the sprite ID for a given item, with caching.
 *
 * Queries the "SpriteID" column from the "Items" table. The result is cached
 * in m_spriteIDCache for subsequent calls. Thread-safe via m_mutex.
 *
 * @param itemID The string ID of the item.
 * @return The sprite ID string associated with the item.
 */
QString DBH::spriteID( QString itemID )
{
	QMutexLocker ml( &m_mutex );
	if ( m_spriteIDCache.contains( itemID ) )
	{
		return m_spriteIDCache.value( itemID );
	}
	QString spriteID = DB::select( "SpriteID", "Items", itemID ).toString();
	m_spriteIDCache.insert( itemID, spriteID );
	return spriteID;
}

/**
 * @brief Check whether a sprite has random variants, with caching.
 *
 * Queries the "HasRandom" column from the "Sprites" table. Thread-safe via m_mutex.
 *
 * @param spriteID The string ID of the sprite.
 * @return True if the sprite supports random variant selection.
 */
bool DBHelper::spriteIsRandom( QString spriteID )
{
	QMutexLocker ml( &m_mutex );
	if ( m_spriteIsRandomCache.contains( spriteID ) )
	{
		return m_spriteIsRandomCache.value( spriteID );
	}
	bool isRandom = DB::select( "HasRandom", "Sprites", spriteID ).toBool();
	m_spriteIsRandomCache.insert( spriteID, isRandom );
	return isRandom;
}

/**
 * @brief Check whether a sprite has animation data, with caching.
 *
 * Queries the "Anim" column from the "Sprites" table. Thread-safe via m_mutex.
 *
 * @param spriteID The string ID of the sprite.
 * @return True if the sprite has animation frames.
 */
bool DBHelper::spriteHasAnim( QString spriteID )
{
	QMutexLocker ml( &m_mutex );
	if ( m_spriteHasAnimCache.contains( spriteID ) )
	{
		return m_spriteHasAnimCache.value( spriteID );
	}
	bool hasAnim = DB::select( "Anim", "Sprites", spriteID ).toBool();
	m_spriteHasAnimCache.insert( spriteID, hasAnim );
	return hasAnim;
}

/**
 * @brief Look up the color string for a material, with caching.
 *
 * Queries the "Color" column from the "Materials" table. Thread-safe via m_mutex.
 *
 * @param materialID The string ID of the material.
 * @return The color string for the material (e.g. hex color or named color).
 */
QString DBHelper::materialColor( QString materialID )
{
	QMutexLocker ml( &m_mutex );
	if ( m_materialColorCache.contains( materialID ) )
	{
		return m_materialColorCache.value( materialID );
	}
	QString color = DB::select( "Color", "Materials", materialID ).toString();
	m_materialColorCache.insert( materialID, color );
	return color;
}

/**
 * @brief Get the tool level for a material, with caching and type fallback.
 *
 * First queries "ToolLevel" from "MaterialToToolLevel" using the material ID
 * directly. If the result is 0, falls back to looking up the material's Type
 * and querying the tool level for that type instead.
 *
 * @param material The string ID of the material.
 * @return The tool level as an integer.
 */
int DBHelper::materialToolLevel( QString material )
{
	if ( m_materialToolLevelCache.contains( material ) )
	{
		return m_materialToolLevelCache.value( material );
	}
	int tl = DB::select( "ToolLevel", "MaterialToToolLevel", material ).toInt();
	if ( tl == 0 )
	{
		tl = DB::select( "ToolLevel", "MaterialToToolLevel", DB::select( "Type", "Materials", material ).toString() ).toInt();
	}
	m_materialToolLevelCache.insert( material, tl );
	return tl;
}

/**
 * @brief Convert a material string ID to a numeric UID, registering it if new.
 *
 * Uses GameState::materialSID2ID for the mapping. If the material is not yet
 * registered, it is added to both the SID-to-ID and ID-to-SID maps in GameState.
 *
 * @param material The string ID of the material.
 * @return The numeric UID assigned to the material.
 */
int DBH::materialUID( QString material )
{
	if( !GameState::materialSID2ID.contains( material ) )
	{
		GameState::materialID2SID.insert( GameState::materialSID2ID.size(), material );
		GameState::materialSID2ID.insert( material, GameState::materialSID2ID.size() );
	}
	return GameState::materialSID2ID.value( material );
}

/**
 * @brief Convert a material numeric UID back to its string ID.
 *
 * Uses GameState::materialID2SID for the reverse lookup. Logs an error
 * and returns "NONE" if the UID is not found.
 *
 * @param material The numeric UID of the material.
 * @return The string ID, or "NONE" if not found.
 */
QString DBH::materialSID( int material )
{
	if ( GameState::materialID2SID.contains( material ) )
	{
		return GameState::materialID2SID.value( material );
	}
	qDebug() << "***ERROR*** DBH::materialSID : no entry for material:" << material;
	return "NONE";
}

/**
 * @brief Convert an item string ID to a numeric UID, registering it if new.
 *
 * Uses GameState::itemSID2ID for the mapping. If the item is not yet
 * registered, it is added to both the SID-to-ID and ID-to-SID maps in GameState.
 *
 * @param item The string ID of the item.
 * @return The numeric UID assigned to the item.
 */
int DBH::itemUID( QString item )
{
	if( !GameState::itemSID2ID.contains( item ) )
	{
		GameState::itemID2SID.insert( GameState::itemSID2ID.size(), item );
		GameState::itemSID2ID.insert( item, GameState::itemSID2ID.size() );
	}
	return GameState::itemSID2ID.value( item );
}

/**
 * @brief Convert an item numeric UID back to its string ID.
 *
 * Uses GameState::itemID2SID for the reverse lookup. Logs an error
 * and returns "NONE" if the UID is not found.
 *
 * @param item The numeric UID of the item.
 * @return The string ID, or "NONE" if not found.
 */
QString DBH::itemSID( int item )
{
	if ( GameState::itemID2SID.contains( item ) )
	{
		return GameState::itemID2SID.value( item );
	}
	qDebug() << "***ERROR*** DBH::itemSID : no entry for item:" << item;
	return "NONE";
}

/**
 * @brief Check whether an item is a container, with caching.
 *
 * Queries the "IsContainer" column from the "Items" table using the item's
 * string ID (resolved from the numeric UID via itemSID()).
 *
 * @param item The numeric UID of the item.
 * @return True if the item is a container.
 */
bool DBHelper::itemIsContainer( int item )
{
	if ( m_itemIsContainerCache.contains(item) )
	{
		return m_itemIsContainerCache.value( item );
	}
	bool isContainer = DB::select( "IsContainer", "Items", DBH::itemSID( item ) ).toBool();
	m_itemIsContainerCache.insert( item, isContainer );
	return isContainer;
}

/**
 * @brief Get the SQLite rowid for a given string ID in a database table.
 * @param table The database table name.
 * @param id The string ID to look up.
 * @return The rowid as an integer.
 */
int DBH::rowID( QString table, QString id )
{
	return DB::execQuery( "SELECT rowid FROM " + table + " WHERE ID = \"" + id + "\"" ).toInt();
}

/**
 * @brief Get the string ID for a given rowid in a database table.
 * @param table The database table name.
 * @param rowID The numeric rowid to look up.
 * @return The string ID corresponding to the rowid.
 */
QString DBH::id( QString table, int rowID )
{
	return DB::execQuery( "SELECT ID FROM " + table + " WHERE rowid = \"" + QString::number( rowID ) + "\"" ).toString();
}

/**
 * @brief Get the quality name string for a numeric rank, with caching.
 *
 * Queries the "Quality" table using the rank value.
 *
 * @param rank The numeric quality rank.
 * @return The quality string ID (e.g. "Normal", "Fine").
 */
QString DBH::qualitySID( int rank )
{
	if ( m_qualitySIDCache.contains( rank ) )
	{
		return m_qualitySIDCache.value( rank );
	}
	QString qualitySID = DB::execQuery( "SELECT ID FROM Quality WHERE Rank = \"" + QString::number( rank ) + "\"" ).toString();
	m_qualitySIDCache.insert( rank, qualitySID );
	return qualitySID;
}

/**
 * @brief Get the stat modifier for a quality rank, with caching.
 *
 * Queries the "Modifier" column from the "Quality" table for the given rank.
 * Returns 1.0 if no modifier is found.
 *
 * @param rank The numeric quality rank.
 * @return The float modifier value (1.0 = no modification).
 */
float DBHelper::qualityMod( int rank )
{
	if ( m_qualityModCache.contains(rank) )
	{
		return m_qualityModCache.value( rank );
	}
	auto modifiers = DB::select2( "Modifier", "Quality", "Rank", rank );
	float modifier = 1.0;
	if ( modifiers.size() )
	{
		modifier = modifiers.first().toFloat();
	}
	m_qualityModCache.insert( rank, modifier );
	return modifier;
}

/**
 * @brief Get all possible craft results for a workshop, with caching.
 *
 * Iterates through all craft IDs associated with the workshop, collecting the
 * result item IDs and the material types each craft can produce. The result is
 * a nested map: itemSID -> (materialType -> craftID).
 *
 * @param workshopId The string ID of the workshop.
 * @return Nested map of craft possibilities, or empty map if workshop not found.
 */
QMap<QString, QMultiMap<QString, QString>> DBHelper::workshopPossibleCraftResults( QString workshopId )
{
	if ( m_workshopCraftResults.contains( workshopId ) )
	{
		return m_workshopCraftResults.value( workshopId );
	}

	QMap<QString, QMultiMap<QString, QString>> workshopProduces;

	auto dbws = DB::workshop( workshopId );
	if( !dbws )
	{
		// this should never be reached
		return workshopProduces;
	}
	const auto craftIds = dbws->Crafts;

	for ( const auto& craftId : craftIds )
	{
		QMultiMap<QString, QString> craftVariants;
		auto itemSID = DB::select( "ItemID", "Crafts", craftId ).toString();
		// can this workshop craft an item of that material?
		auto possibleResultMatTypes = DB::select( "ResultMaterialTypes", "Crafts", craftId ).toString().split( "|" );
		for ( const auto& materialType : possibleResultMatTypes )
		{
			craftVariants.insertMulti( materialType, craftId );
		}
		workshopProduces.insert( itemSID, craftVariants );
	}

	m_workshopCraftResults.insert( workshopId, workshopProduces );
	return workshopProduces;
}

/**
 * @brief Get the item group for a given item, with caching.
 *
 * Queries the "ItemGroup" column from the "Items" table.
 *
 * @param itemID The string ID of the item.
 * @return The item group string ID.
 */
QString DBH::itemGroup( QString itemID )
{
	if ( m_itemGroupCache.contains( itemID ) )
	{
		return m_itemGroupCache.value( itemID );
	}
	QString itemGroup = DB::select( "ItemGroup", "Items", itemID ).toString();
	m_itemGroupCache.insert( itemID, itemGroup );
	return itemGroup;
}
