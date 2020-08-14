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

#include "../base/counter.h"
#include "../base/dbhelper.h"
#include "../base/position.h"

#include <QList>
#include <QMutex>
#include <QSqlDatabase>
#include <QVariant>

typedef DBHelper DBH;

class Item;

class DB
{
public:
	static void init();
	static void resetLiveTables();

	static QVariant execQuery( QString query );
	static QVariantList execQuery2( QString query );
	static QSqlQuery execQuery3( QString query, bool& ok );

	static QVariant select( QString selectCol, QString table, QString whereVal );
	static QVariant select( QString selectCol, QString table, int whereVal );

	static QVariantList select2( QString selectCol, QString table, QString whereCol, QString whereVal );
	static QVariantList select2( QString selectCol, QString table, QString whereCol, int whereVal );
	static QVariantList select2( QString selectCol, QString table, QString whereCol, float whereVal );

	static QVariant select3( QString selectCol, QString table, QString whereCol, QString whereVal, QString whereCol2, QString whereVal2 );

	static QStringList ids( QString table );
	static QStringList ids( QString table, QString whereCol, QString whereVal );
	static int numRows( QString table );
	static int numRows( QString table, QString id );

	static QVariantMap selectRow( QString table, QString whereVal );
	static QVariantMap selectRow( QString table, int whereVal );
	static QList<QVariantMap> selectRows( QString table, QString whereCol, QString whereVal );
	static QList<QVariantMap> selectRows( QString table );
	static QList<QVariantMap> selectRows( QString table, QString id );
	static QList<QVariantMap> selectRows( QString table, int rowid );

	static int getAccessCounter();
	static Counter<QString>& getQueryCounter();

	static QStringList tables();

	static bool createItem( const Item& item, QString itemSID, QString materialSID );
	static bool createItem( const QVariantMap& in );
	static bool destroyItem( unsigned int itemID );

	static bool updateRow( QString table, QVariantMap values );
	static bool addRow( QString table, QVariantMap values );
	static bool removeRows( QString table, QString id );
	static bool addTranslation( QString id, QString text );

private:
	static QSqlDatabase& getDB();

	static QMutex m_mutex;

	static int accessCounter;

	static Counter<QString> m_counter;

	static QSqlQuery m_itemCreateQuery;

	static QMap<Qt::HANDLE, QSqlDatabase> m_connections;

	DB()  = delete;
	~DB() = delete;
};
