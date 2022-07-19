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
#include "dbhelper.h"

#include "dbstructs.h"

#include "../base/db.h"
#include "../base/gamestate.h"
#include "containersHelper.h"
#include "spdlog/spdlog.h"

absl::btree_map<QString, QString> DBHelper::m_spriteIDCache;
absl::btree_map<QString, bool> DBHelper::m_spriteIsRandomCache;
absl::btree_map<QString, bool> DBHelper::m_spriteHasAnimCache;
absl::btree_map<std::string, std::string> DBHelper::m_materialColorCache;
absl::btree_map<QString, int> DBHelper::m_materialToolLevelCache;
absl::btree_map<int, bool> DBHelper::m_itemIsContainerCache;
absl::btree_map<int, QString> DBHelper::m_qualitySIDCache;
absl::btree_map<int, float> DBHelper::m_qualityModCache;
absl::btree_map<QString, QString> DBHelper::m_itemGroupCache;
absl::btree_map<QString, absl::btree_map<QString, QMultiMap<QString, QString>>> DBHelper::m_workshopCraftResults;

QMutex DBHelper::m_mutex;

/*
QStringList DBHelper::getWorkPositions( QString jobID )
{
	QString wps = DB::select( "WorkPosition", "Jobs", jobID ).toString();
	return wps.split( "|" );
}
*/

QString DBH::spriteID( QString itemID )
{
	QMutexLocker ml( &m_mutex );
	if ( m_spriteIDCache.contains( itemID ) )
	{
		return m_spriteIDCache.at( itemID );
	}
	QString spriteID = DB::select( "SpriteID", "Items", itemID ).toString();
	m_spriteIDCache.insert_or_assign( itemID, spriteID );
	return spriteID;
}

bool DBHelper::spriteIsRandom( QString spriteID )
{
	QMutexLocker ml( &m_mutex );
	if ( m_spriteIsRandomCache.contains( spriteID ) )
	{
		return m_spriteIsRandomCache.at( spriteID );
	}
	bool isRandom = DB::select( "HasRandom", "Sprites", spriteID ).toBool();
	m_spriteIsRandomCache.insert_or_assign( spriteID, isRandom );
	return isRandom;
}

bool DBHelper::spriteHasAnim( QString spriteID )
{
	QMutexLocker ml( &m_mutex );
	if ( m_spriteHasAnimCache.contains( spriteID ) )
	{
		return m_spriteHasAnimCache.at( spriteID );
	}
	bool hasAnim = DB::select( "Anim", "Sprites", spriteID ).toBool();
	m_spriteHasAnimCache.insert_or_assign( spriteID, hasAnim );
	return hasAnim;
}

std::string DBHelper::materialColor( const std::string& materialID )
{
	QMutexLocker ml( &m_mutex );
	if ( m_materialColorCache.contains( materialID ) )
	{
		return m_materialColorCache.at( materialID );
	}
	const auto color = DB::select( "Color", "Materials", QString::fromStdString(materialID) ).toString().toStdString();
	m_materialColorCache.insert_or_assign( materialID, color );
	return color;
}

int DBHelper::materialToolLevel( QString material )
{
	if ( m_materialToolLevelCache.contains( material ) )
	{
		return m_materialToolLevelCache.at( material );
	}
	int tl = DB::select( "ToolLevel", "MaterialToToolLevel", material ).toInt();
	if ( tl == 0 )
	{
		tl = DB::select( "ToolLevel", "MaterialToToolLevel", DB::select( "Type", "Materials", material ).toString() ).toInt();
	}
	m_materialToolLevelCache.insert_or_assign( material, tl );
	return tl;
}

int DBH::materialUID( QString material )
{
	if( !GameState::materialSID2ID.contains( material ) )
	{
		GameState::materialID2SID.insert_or_assign( GameState::materialSID2ID.size(), material );
		GameState::materialSID2ID.insert_or_assign( material, GameState::materialSID2ID.size() );
	}
	return GameState::materialSID2ID.at( material );
}

QString DBH::materialSID( int material )
{
	if ( const auto &it = GameState::materialID2SID.find( material ); it != GameState::materialID2SID.end() )
	{
		return it->second;
	}
	spdlog::debug( "***ERROR*** DBH::materialSID : no entry for material: {}", material );
	return "NONE";
}

int DBH::itemUID( QString item )
{
	if( !GameState::itemSID2ID.contains( item ) )
	{
		GameState::itemID2SID.insert_or_assign( GameState::itemSID2ID.size(), item );
		GameState::itemSID2ID.insert_or_assign( item, GameState::itemSID2ID.size() );
	}
	return GameState::itemSID2ID.at( item );
}

QString DBH::itemSID( int item )
{
	if ( const auto &it = GameState::itemID2SID.find( item ); it != GameState::itemID2SID.end() )
	{
		return it->second;
	}
	spdlog::debug( "***ERROR*** DBH::itemSID : no entry for item: {}", item );
	return "NONE";
}


bool DBHelper::itemIsContainer( int item )
{
	if ( m_itemIsContainerCache.contains(item) )
	{
		return m_itemIsContainerCache.at( item );
	}
	bool isContainer = DB::select( "IsContainer", "Items", DBH::itemSID( item ) ).toBool();
	m_itemIsContainerCache.insert_or_assign( item, isContainer );
	return isContainer;
}

int DBH::rowID( QString table, QString id )
{
	return DB::execQuery( "SELECT rowid FROM " + table + " WHERE ID = \"" + id + "\"" ).toInt();
}

QString DBH::id( QString table, int rowID )
{
	return DB::execQuery( "SELECT ID FROM " + table + " WHERE rowid = \"" + QString::number( rowID ) + "\"" ).toString();
}

QString DBH::qualitySID( int rank )
{
	if ( m_qualitySIDCache.contains( rank ) )
	{
		return m_qualitySIDCache.at( rank );
	}
	QString qualitySID = DB::execQuery( "SELECT ID FROM Quality WHERE Rank = \"" + QString::number( rank ) + "\"" ).toString();
	m_qualitySIDCache.insert_or_assign( rank, qualitySID );
	return qualitySID;
}

float DBHelper::qualityMod( int rank )
{
	if ( m_qualityModCache.contains(rank) )
	{
		return m_qualityModCache.at( rank );
	}
	auto modifiers = DB::select2( "Modifier", "Quality", "Rank", rank );
	float modifier = 1.0;
	if ( modifiers.size() )
	{
		modifier = modifiers.first().toFloat();
	}
	m_qualityModCache.insert_or_assign( rank, modifier );
	return modifier;
}

absl::btree_map<QString, QMultiMap<QString, QString>> DBHelper::workshopPossibleCraftResults( QString workshopId )
{
	if ( const auto &it = m_workshopCraftResults.find( workshopId ); it != m_workshopCraftResults.end() )
	{
		return it->second;
	}

	absl::btree_map<QString, QMultiMap<QString, QString>> workshopProduces;

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
		workshopProduces.insert_or_assign( itemSID, craftVariants );
	}

	m_workshopCraftResults.insert_or_assign( workshopId, workshopProduces );
	return workshopProduces;
}

QString DBH::itemGroup( QString itemID )
{
	if ( m_itemGroupCache.contains( itemID ) )
	{
		return m_itemGroupCache.at( itemID );
	}
	QString itemGroup = DB::select( "ItemGroup", "Items", itemID ).toString();
	m_itemGroupCache.insert_or_assign( itemID, itemGroup );
	return itemGroup;
}