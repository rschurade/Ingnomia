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
/** @file db.h
 * @brief Thread-safe static database access layer for the game's SQLite data.
 */

#pragma once

#include "../base/counter.h"
#include "../base/dbhelper.h"
#include "../base/dbstructs.h"
#include "../base/position.h"

#include <QList>
#include <QMutex>
#include <QSqlDatabase>
#include <QVariant>

typedef DBHelper DBH;

class Item;

/**
 * @brief Static-only database access layer wrapping an in-memory SQLite database.
 *
 * Provides thread-safe query methods for the game's data tables (Items, Materials,
 * Workshops, Jobs, etc.). Uses per-thread SQLite connections with shared cache.
 * All public methods are mutex-locked and track access counts for profiling.
 * Cannot be instantiated — all methods are static.
 */
class DB
{
public:
	static void init();
	static void initStructs();

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
	static int numRows2( QString table, QString bsid );

	static QVariantMap selectRow( QString table, QString whereVal );
	static QList<QVariantMap> selectRows( QString table, QString whereCol, QString whereVal );
	static QList<QVariantMap> selectRows( QString table, QString whereCol, QString whereVal, QString whereCol2, QString whereVal2 );
	static QList<QVariantMap> selectRows( QString table );
	static QList<QVariantMap> selectRows( QString table, QString id );

	static int getAccessCounter();
	static Counter<QString>& getQueryCounter();

	static QStringList tables();

	static bool updateRow( QString table, QVariantMap values );

	static bool addRow( QString table, QVariantMap values );
	static bool removeRows( QString table, QString id );
	static bool addTranslation( QString id, QString text );

	static QSharedPointer<DBS::Workshop> workshop( QString id );
	static QSharedPointer<DBS::Job> job( QString id );
	static QList<QString> jobIds();

private:
	static QSqlDatabase& getDB();

	static QMutex m_mutex;                       ///< Mutex protecting all DB operations.
	static int accessCounter;                    ///< Total DB access count (reset on read).
	static Counter<QString> m_counter;           ///< Per-query string counter for profiling.
	static QMap<Qt::HANDLE, QSqlDatabase> m_connections; ///< Per-thread database connections.
	static QHash<QString, QSharedPointer<DBS::Workshop>> m_workshops; ///< Cached Workshop structs.
	static QHash<QString, QSharedPointer<DBS::Job>> m_jobs;           ///< Cached Job structs.

	DB()  = delete;
	~DB() = delete;
};
