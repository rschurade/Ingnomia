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
/** @file techtree.cpp
 *  @brief Tech-tree level computation: assigns item unlock levels based on craft dependency depth.
 */
#include "techtree.h"

#include "../base/db.h"

#include <QDebug>
#include <QQueue>
#include <QSqlQuery>

/// @brief Constructs the TechTree.
/// @param parent Qt parent object.
TechTree::TechTree( QObject* parent ) :
	QObject( parent )
{
}

/// @brief Destructor.
TechTree::~TechTree()
{
}

/// @brief Computes and writes tech levels for all craftable items.
///        Base items start at level 1. Each craftable item's level is
///        max(component levels × amounts) + 1, resolved iteratively via a queue
///        until all dependencies are satisfied. Writes results back to the Items DB table.
void TechTree::create()
{
	QHash<QString, int> itemLevels;

	for ( auto id : DB::ids( "BaseItems" ) )
	{
		itemLevels.insert( id, 1 );
	}

	QList<QString> crafts = DB::ids( "Crafts" );

	QQueue<QString> workQueue;
	for ( auto craft : crafts )
	{
		workQueue.enqueue( craft );
	}
	int breaker = 0;
	while ( !workQueue.isEmpty() )
	{
		QString id = workQueue.dequeue();
		auto craft = DB::selectRow( "Crafts", id );

		if ( compsLowerLevel( id, itemLevels ) )
		{
			int level = levelPlusOne( id, itemLevels );
			itemLevels.insert( craft.value( "ItemID" ).toString(), level );
		}
		else
		{
			workQueue.enqueue( id );
		}

		++breaker;
		if ( breaker == 100000 )
		{
			qDebug() << "breaker reached";
			qDebug() << "workqueue size:" << workQueue.size();
			while ( !workQueue.isEmpty() )
			{
				QString id = workQueue.dequeue();
				qDebug() << id;
			}
		}
	}
	qDebug() << "####################################################";
	int maxLevel = 0;
	for ( auto key : itemLevels.keys() )
	{
		maxLevel = qMax( maxLevel, itemLevels[key] );
	}
	int currentLevel = 1;
	auto keys        = itemLevels.keys();
	keys.sort();

	while ( currentLevel <= maxLevel )
	{
		//qDebug() << "-->" << currentLevel;
		for ( auto key : keys )
		{
			if ( currentLevel == itemLevels[key] )
			{
				qDebug() << currentLevel << key;
			}
		}
		++currentLevel;
	}
	qDebug() << "####################################################";
	for ( auto key : keys )
	{
		qDebug() << itemLevels[key] << key;

		bool ok;
		DB::execQuery3( "UPDATE Items SET \"VALUE\" = \"" + QString::number( itemLevels[key] ) + "\" WHERE ID = \"" + key + "\"", ok );
	}
	qDebug() << "####################################################";
}

/// @brief Returns true if all component items for the given craft already have an assigned level.
/// @param craftID    DB key of the craft to check.
/// @param itemLevels Current level assignments (ItemID → level).
/// @return true if every component's ItemID is present in @p itemLevels.
bool TechTree::compsLowerLevel( QString craftID, QHash<QString, int>& itemLevels )
{
	for ( auto row : DB::selectRows( "Crafts_Components", craftID ) )
	{
		if ( !itemLevels.contains( row.value( "ItemID" ).toString() ) )
		{
			return false;
		}
	}
	return true;
}

/// @brief Computes the tech level for a craft's output as max(componentLevel × amount) + 1.
/// @param craftID    DB key of the craft.
/// @param itemLevels Current level assignments (ItemID → level).
/// @return The computed level for the craft's output item.
int TechTree::levelPlusOne( QString craftID, QHash<QString, int>& itemLevels )
{
	int level = 0;
	for ( auto row : DB::selectRows( "Crafts_Components", craftID ) )
	{
		level = qMax( level, itemLevels.value( row.value( "ItemID" ).toString() ) * row.value( "Amount" ).toInt() );
	}
	return level + 1;
}