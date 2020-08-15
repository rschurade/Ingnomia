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

#include "../base/db.h"

QMap<QString, QString> DBHelper::m_spriteIDCache;
QMap<QString, bool> DBHelper::m_spriteIsRandomCache;
QMap<int, QString> DBHelper::m_materialSIDCache;
QMap<int, QString> DBHelper::m_itemSIDCache;
QMap<QString, int> DBHelper::m_materialUIDCache;
QMap<QString, int> DBHelper::m_materialToolLevelCache;
QMap<QString, int> DBHelper::m_itemUIDCache;
QMap<int, bool> DBHelper::m_itemIsContainerCache;
QMap<int, QString> DBHelper::m_qualitySIDCache;
QMap<int, float> DBHelper::m_qualityModCache;
QMap<QString, QString> DBHelper::m_itemGroupCache;

/*
QStringList DBHelper::getWorkPositions( QString jobID )
{
	QString wps = DB::select( "WorkPosition", "Jobs", jobID ).toString();
	return wps.split( "|" );
}
*/

QString DBH::spriteID( QString itemID )
{
	if ( m_spriteIDCache.contains( itemID ) )
	{
		return m_spriteIDCache.value( itemID );
	}
	QString spriteID = DB::select( "SpriteID", "Items", itemID ).toString();
	m_spriteIDCache.insert( itemID, spriteID );
	return spriteID;
}

bool DBHelper::spriteIsRandom( QString spriteID )
{
	if ( m_spriteIsRandomCache.contains( spriteID ) )
	{
		return m_spriteIsRandomCache.value( spriteID );
	}
	bool isRandom = DB::select( "HasRandom", "Sprites", spriteID ).toBool();
	m_spriteIsRandomCache.insert( spriteID, isRandom );
	return isRandom;
}

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

int DBH::materialUID( QString material )
{
	if ( m_materialUIDCache.contains( material ) )
	{
		return m_materialUIDCache.value( material );
	}
	int materialUID = DB::execQuery( "SELECT rowid FROM Materials WHERE ID = \"" + material + "\"" ).toInt();
	m_materialUIDCache.insert( material, materialUID );
	return materialUID;
}

QString DBH::materialSID( int material )
{
	if ( m_materialSIDCache.contains( material ) )
	{
		return m_materialSIDCache.value( material );
	}
	QString materialSID = DB::execQuery( "SELECT ID FROM Materials WHERE rowid = \"" + QString::number( material ) + "\"" ).toString();
	m_materialSIDCache.insert( material, materialSID );
	return materialSID;
}

int DBH::itemUID( QString item )
{
	if ( m_itemUIDCache.contains( item ) )
	{
		return m_itemUIDCache.value( item );
	}
	int itemUID = DB::execQuery( "SELECT rowid FROM Items WHERE ID = \"" + item + "\"" ).toInt();
	m_itemUIDCache.insert( item, itemUID );
	return itemUID;
}

QString DBH::itemSID( int item )
{
	if ( m_itemSIDCache.contains( item ) )
	{
		return m_itemSIDCache.value( item );
	}
	QString itemSID = DB::execQuery( "SELECT ID FROM Items WHERE rowid = \"" + QString::number( item ) + "\"" ).toString();
	m_itemSIDCache.insert( item, itemSID );
	return itemSID;
}


bool DBHelper::itemIsContainer( int item )
{
	if ( m_itemIsContainerCache.contains(item) )
	{
		return m_itemIsContainerCache.value( item );
	}
	bool isContainer = DB::select( "IsContainer", "Items", item ).toBool();
	m_itemIsContainerCache.insert( item, isContainer );
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
		return m_qualitySIDCache.value( rank );
	}
	QString qualitySID = DB::execQuery( "SELECT ID FROM Quality WHERE Rank = \"" + QString::number( rank ) + "\"" ).toString();
	m_qualitySIDCache.insert( rank, qualitySID );
	return qualitySID;
}

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