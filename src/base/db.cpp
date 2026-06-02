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
/** @file db.cpp
 *  @brief Implementation of the DB class, providing thread-safe in-memory
 *         SQLite database access and cached struct lookups.
 */
#include "db.h"

#include "../base/config.h"
#include "../game/item.h"

#include <QDebug>
#include <QMutexLocker>
#include <QSqlError>
#include <QSqlField>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QThread>

QMutex DB::m_mutex;
int DB::accessCounter = 0;
Counter<QString> DB::m_counter;
QMap<Qt::HANDLE, QSqlDatabase> DB::m_connections;


QHash<QString, QSharedPointer<DBS::Workshop>> DB::m_workshops;
QHash<QString, QSharedPointer<DBS::Job>> DB::m_jobs;

/** @brief Initialize the database by loading and executing ingnomia.db.sql
 *         into the in-memory shared-cache SQLite database.
 *
 *  Reads the SQL dump file from the configured data path (content/db/),
 *  splits it into individual statements, and executes each one. Must be
 *  called once at application startup before any queries.
 */
void DB::init()
{
	QMutexLocker lock( &DB::m_mutex );

	QFile file( Global::cfg->get( "dataPath" ).toString() + "/db/" + "ingnomia.db.sql" );
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString sql = file.readAll();
    file.close();

	QSqlQuery destQuery( getDB() );

	auto statements = sql.split( ";" );
	for( auto s: statements )
	{
		auto trimmed = s.trimmed();
		if ( trimmed.isEmpty() )
			continue;
		if ( !destQuery.exec( trimmed ) )
		{
			qDebug() << destQuery.lastError();
		}
	}
}

/** @brief Pre-load Workshop and Job records from the database into cached
 *         QHash maps for fast lookup.
 *
 *  Populates m_workshops and m_jobs by querying the Workshops, Workshops_Components,
 *  Jobs, Jobs_Tasks, and Jobs_SpriteID tables. Should be called after init().
 */
void DB::initStructs()
{
	m_workshops.clear();
	auto rows = DB::selectRows( "Workshops" );
	for( const auto& row : rows )
	{
		QSharedPointer<DBS::Workshop> ws( new DBS::Workshop );
		ws->ID = row.value( "ID" ).toString();
		ws->Crafts = row.value( "Crafts" ).toString().split( "|" );
		ws->GUI = row.value( "GUI" ).toString();
		ws->InputTile = Position( row.value( "InputTile" ) );
		ws->OutputTile = row.value( "OutputTile" ).toString();
		ws->Size = row.value( "Size" ).toString();
		ws->NoAutoGenerate = row.value( "NoAutoGenerate" ).toBool();
		ws->Icon = row.value( "Icon" ).toString();
		ws->Tab = row.value( "Tab" ).toString();

		auto crows = DB::selectRows( "Workshops_Components", ws->ID );
		for( const auto& crow : crows )
		{
			DBS::Workshop_Component wsc;
			wsc.Amount = crow.value( "Amount" ).toInt();
			wsc.ItemID = crow.value( "ItemID" ).toString();
			wsc.MaterialItem = crow.value( "MaterialItem" ).toString();
			wsc.Offset = Position( crow.value( "Offset" ) );
			wsc.Required = crow.value( "Required" ).toString();
			wsc.Forbidden = crow.value( "Forbidden" ).toString();
			wsc.SpriteID = crow.value( "SpriteID" ).toString();
			wsc.SpriteID2 = crow.value( "SpriteID2" ).toString();
			wsc.Type = crow.value( "Type" ).toString();
			wsc.WallRotation = crow.value( "WallRotation" ).toString();
			wsc.IsFloor = crow.value( "IsFloor" ).toBool();
			ws->components.append( wsc );
		}

		m_workshops.insert( ws->ID, ws );
	}

	m_jobs.clear();
	rows = DB::selectRows( "Jobs" );
	for( const auto& row : rows )
	{
		QSharedPointer<DBS::Job> job( new DBS::Job );
		job->ID = row.value( "ID" ).toString();
		job->ConstructionType = row.value( "ConstructionType" ).toString();
		job->MayTrapGnome = row.value( "MayTrapGnome" ).toBool();
		job->RequiredToolItemID = row.value( "RequiredToolItemID" ).toString();
		job->RequiredToolLevel = row.value( "RequiredToolLevel" ).toString();
		job->SkillGain = row.value( "SkillGain" ).toString();
		job->SkillID = row.value( "SkillID" ).toString();
		job->TechGain = row.value( "TechGain" ).toString();
		for( auto spos : row.value( "WorkPosition" ).toString().split( "|" ) )
		{
			job->WorkPositions.append( Position( spos ) );
		}
		auto trows = DB::selectRows( "Jobs_Tasks" );
		for( const auto& trow : trows )
		{
			DBS::Job_Task jt;
			jt.ConstructionID = trow.value( "ConstructionID" ).toString();
			jt.Duration = trow.value( "Duration" ).toInt();
			jt.Material = trow.value( "Material" ).toString();
			jt.Offset = Position( trow.value( "Offset" ) );
			jt.Task = trow.value( "Task" ).toString();
			job->tasks.append( jt );
		}
		auto srows = DB::selectRows( "Jobs_SpriteID" );
		for( const auto& srow : srows )
		{
			DBS::Job_SpriteID js;
			js.Offset = Position( srow.value( "Offset" ) );
			js.Rotate = srow.value( "Rotate" ).toBool();
			js.SpriteID = srow.value( "SpriteID" ).toString();
			js.Type = srow.value( "Type" ).toString();
			job->sprites.append( js );
		}
		m_jobs.insert( job->ID, job );
	}
}

/** @brief Get or create a per-thread QSqlDatabase connection.
 *
 *  Uses QThread::currentThreadId() as a key into m_connections. If no
 *  connection exists for the current thread, a new QSQLITE connection is
 *  created with shared-cache mode pointing at the in-memory database.
 *
 *  @return A reference to the QSqlDatabase connection for the calling thread.
 */
QSqlDatabase& DB::getDB()
{
	auto thread = QThread::currentThreadId();
	if ( !m_connections.contains( thread ) )
	{
		auto db = QSqlDatabase::addDatabase( "QSQLITE", QString::number( reinterpret_cast<long long>( thread ) ) );
		db.setConnectOptions( "QSQLITE_OPEN_URI;QSQLITE_ENABLE_SHARED_CACHE" );
		db.setDatabaseName( "file:game?mode=memory&cache=shared" );
		if ( !db.open() )
		{
			qDebug() << "Error: create in memory db";
			abort();
		}
		else
		{
			qDebug() << "Memory DB: connection ok";
		}
		m_connections[thread] = db;
	}
	return m_connections[thread];
}

/** @brief Get and reset the database access counter.
 *
 *  Returns the current value of the access counter and resets it to zero.
 *  Used for profiling how many DB accesses occur per frame or time period.
 *
 *  @return The number of query accesses since the last call to this method.
 */
int DB::getAccessCounter()
{
	int tmp       = accessCounter;
	accessCounter = 0;
	return tmp;
}

/** @brief Execute a SQL query and return the first column value of the first row.
 *
 *  Thread-safe. Increments the access counter and records the query string
 *  in the query counter for profiling.
 *
 *  @param queryString The SQL query string to execute.
 *  @return The first column value of the first result row, or an invalid QVariant if no results.
 */
QVariant DB::execQuery( QString queryString )
{
	QMutexLocker lock( &DB::m_mutex );
	++accessCounter;
	QSqlQuery query( getDB() );
	if ( query.exec( queryString ) )
	{
		m_counter.add( query.lastQuery() );
		if ( query.next() )
		{
			return query.value( 0 );
		}
	}
	else
	{
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << queryString;
	}

	return QVariant();
}

/** @brief Execute a SQL query and return all first-column values from every row.
 *
 *  Thread-safe. Increments the access counter and records the query string
 *  in the query counter for profiling.
 *
 *  @param queryString The SQL query string to execute.
 *  @return A list of first-column values from all result rows.
 */
QVariantList DB::execQuery2( QString queryString )
{
	QMutexLocker lock( &DB::m_mutex );
	++accessCounter;
	QSqlQuery query( getDB() );
	QVariantList out;
	if ( query.exec( queryString ) )
	{
		m_counter.add( query.lastQuery() );
		while ( query.next() )
		{
			out.append( query.value( 0 ) );
		}
	}
	else
	{
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << queryString;
	}

	return out;
}

/** @brief Execute a SQL query and return the full QSqlQuery result object.
 *
 *  Thread-safe. Increments the access counter and records the query string
 *  in the query counter for profiling. The caller can iterate through the
 *  returned QSqlQuery to access all columns and rows.
 *
 *  @param queryString The SQL query string to execute.
 *  @param[out] ok Set to true if the query executed successfully, false otherwise.
 *  @return The QSqlQuery object after execution (caller can iterate rows).
 */
QSqlQuery DB::execQuery3( QString queryString, bool& ok )
{
	QMutexLocker lock( &DB::m_mutex );
	++accessCounter;
	QSqlQuery query( getDB() );
	if ( query.exec( queryString ) )
	{
		m_counter.add( query.lastQuery() );
		ok = true;
		return query;
	}
	else
	{
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << queryString;
	}

	ok = false;
	return query;
}

/** @brief Select a single column value from a table row matched by its ID column.
 *
 *  Executes: SELECT "selectCol" FROM table WHERE ID = "whereVal"
 *
 *  @param selectCol The column name to retrieve.
 *  @param table The table name to query.
 *  @param whereVal The string value to match against the ID column.
 *  @return The requested column value, or an invalid QVariant if not found.
 */
QVariant DB::select( QString selectCol, QString table, QString whereVal )
{
	QMutexLocker lock( &DB::m_mutex );
	++accessCounter;
	QSqlQuery query( getDB() );
	if ( query.exec( "SELECT \"" + selectCol + "\" FROM " + table + " WHERE ID = " + "\"" + whereVal + "\"" ) )
	{
		m_counter.add( query.lastQuery() );
		if ( query.next() )
		{
			return query.value( 0 );
		}
	}
	else
	{
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << "SELECT \"" + selectCol + "\" FROM " + table + " WHERE ID = " + "\"" + whereVal + "\"";
	}

	return QVariant();
}

/** @brief Select a single column value from a table row matched by its rowid.
 *
 *  Executes: SELECT "selectCol" FROM table WHERE rowid = "whereVal"
 *
 *  @param selectCol The column name to retrieve.
 *  @param table The table name to query.
 *  @param whereVal The integer rowid to match.
 *  @return The requested column value, or an invalid QVariant if not found.
 */
QVariant DB::select( QString selectCol, QString table, int whereVal )
{
	QMutexLocker lock( &DB::m_mutex );
	++accessCounter;
	QSqlQuery query( getDB() );
	if ( query.exec( "SELECT \"" + selectCol + "\" FROM " + table + " WHERE rowid = " + "\"" + QString::number( whereVal ) + "\"" ) )
	{
		m_counter.add( query.lastQuery() );
		if ( query.next() )
		{
			return query.value( 0 );
		}
	}
	else
	{
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << "SELECT \"" + selectCol + "\" FROM " + table + " WHERE rowid = " + "\"" + QString::number( whereVal ) + "\"";
	}

	return QVariant();
}

/** @brief Select all values of a column from rows matching an arbitrary WHERE condition (string value).
 *
 *  Executes: SELECT "selectCol" FROM table WHERE "whereCol" = "whereVal"
 *
 *  @param selectCol The column name to retrieve.
 *  @param table The table name to query.
 *  @param whereCol The column name to filter on.
 *  @param whereVal The string value to match in the WHERE clause.
 *  @return A list of matching column values.
 */
QVariantList DB::select2( QString selectCol, QString table, QString whereCol, QString whereVal )
{
	QMutexLocker lock( &DB::m_mutex );
	++accessCounter;
	QSqlQuery query( getDB() );
	QVariantList out;
	if ( query.exec( "SELECT \"" + selectCol + "\" FROM " + table + " WHERE \"" + whereCol + "\" = \"" + whereVal + "\"" ) )
	{
		m_counter.add( query.lastQuery() );
		while ( query.next() )
		{
			out.append( query.value( 0 ) );
		}
	}
	else
	{
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << "SELECT \"" + selectCol + "\" FROM " + table + " WHERE \"" + whereCol + "\" = \"" + whereVal + "\"";
	}

	return out;
}

/** @brief Select all values of a column from rows matching an arbitrary WHERE condition (int value).
 *
 *  Delegates to the string overload after converting whereVal to a string.
 *
 *  @param selectCol The column name to retrieve.
 *  @param table The table name to query.
 *  @param whereCol The column name to filter on.
 *  @param whereVal The integer value to match in the WHERE clause.
 *  @return A list of matching column values.
 */
QVariantList DB::select2( QString selectCol, QString table, QString whereCol, int whereVal )
{
	return select2( selectCol, table, whereCol, QString::number( whereVal ) );
}

/** @brief Select all values of a column from rows matching an arbitrary WHERE condition (float value).
 *
 *  Delegates to the string overload after converting whereVal to a string.
 *
 *  @param selectCol The column name to retrieve.
 *  @param table The table name to query.
 *  @param whereCol The column name to filter on.
 *  @param whereVal The float value to match in the WHERE clause.
 *  @return A list of matching column values.
 */
QVariantList DB::select2( QString selectCol, QString table, QString whereCol, float whereVal )
{
	return select2( selectCol, table, whereCol, QString::number( whereVal ) );
}

/** @brief Select a single column value from a row matching two WHERE conditions.
 *
 *  Executes: SELECT "selectCol" FROM table WHERE "whereCol" = "whereVal" AND "whereCol2" = "whereVal2"
 *
 *  @param selectCol The column name to retrieve.
 *  @param table The table name to query.
 *  @param whereCol The first WHERE column name.
 *  @param whereVal The first WHERE column value.
 *  @param whereCol2 The second WHERE column name.
 *  @param whereVal2 The second WHERE column value.
 *  @return The requested column value, or an invalid QVariant if not found.
 */
QVariant DB::select3( QString selectCol, QString table, QString whereCol, QString whereVal, QString whereCol2, QString whereVal2 )
{
	QMutexLocker lock( &DB::m_mutex );
	++accessCounter;
	QSqlQuery query( getDB() );
	QVariant out;
	if ( query.exec( "SELECT \"" + selectCol + "\" FROM " + table + " WHERE \"" + whereCol + "\" = \"" + whereVal + "\" AND \"" + whereCol2 + "\" = \"" + whereVal2 + "\"" ) )
	{
		m_counter.add( query.lastQuery() );
		if ( query.next() )
		{
			return query.value( 0 );
		}
	}
	else
	{
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << "SELECT \"" + selectCol + "\" FROM " + table + " WHERE \"" + whereCol + "\" = \"" + whereVal + "\" AND \"" + whereCol2 + "\" = \"" + whereVal2 + "\"";
	}

	return out;
}

/** @brief Retrieve all ID values from a table.
 *
 *  Executes: SELECT ID FROM table
 *
 *  @param table The table name to query.
 *  @return A list of all ID column values as strings.
 */
QStringList DB::ids( QString table )
{
	QMutexLocker lock( &DB::m_mutex );
	++accessCounter;
	QStringList out;

	QSqlQuery query( getDB() );
	if ( query.exec( "SELECT ID FROM " + table ) )
	{
		m_counter.add( query.lastQuery() );
		while ( query.next() )
		{
			out.append( query.value( 0 ).toString() );
		}
	}
	else
	{
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << "SELECT ID FROM " + table;
	}

	return out;
}

/** @brief Retrieve all ID values from a table matching a WHERE condition.
 *
 *  Executes: SELECT ID FROM table WHERE "whereCol" = "whereVal"
 *
 *  @param table The table name to query.
 *  @param whereCol The column name to filter on.
 *  @param whereVal The value to match in the WHERE clause.
 *  @return A list of matching ID column values as strings.
 */
QStringList DB::ids( QString table, QString whereCol, QString whereVal )
{
	QMutexLocker lock( &DB::m_mutex );
	++accessCounter;
	QStringList out;

	QSqlQuery query( getDB() );
	if ( query.exec( "SELECT ID FROM " + table + " WHERE \"" + whereCol + "\" = \"" + whereVal + "\"" ) )
	{
		m_counter.add( query.lastQuery() );
		while ( query.next() )
		{
			out.append( query.value( 0 ).toString() );
		}
	}
	else
	{
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << getDB().tables();
		qDebug() << "SELECT ID FROM " + table;
	}

	return out;
}

/** @brief Count the total number of rows in a table.
 *
 *  Executes: SELECT COUNT(*) FROM table
 *
 *  @param table The table name to query.
 *  @return The row count, or 0 on error.
 */
int DB::numRows( QString table )
{
	QMutexLocker lock( &DB::m_mutex );
	++accessCounter;
	QSqlQuery query( getDB() );
	if ( query.exec( "SELECT COUNT(*) FROM " + table ) )
	{
		m_counter.add( query.lastQuery() );
		if ( query.next() )
		{
			return query.value( 0 ).toInt();
		}
	}
	else
	{
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << "SELECT COUNT(*) FROM " + table;
	}
	return 0;
}

/** @brief Count the number of rows in a table where the ID column matches the given value.
 *
 *  Executes: SELECT COUNT(*) FROM table WHERE ID = "id"
 *
 *  @param table The table name to query.
 *  @param id The ID value to match.
 *  @return The matching row count, or 0 on error.
 */
int DB::numRows( QString table, QString id )
{
	QMutexLocker lock( &DB::m_mutex );
	++accessCounter;
	QSqlQuery query( getDB() );
	if ( query.exec( "SELECT COUNT(*) FROM " + table + " WHERE ID = \"" + id + "\"" ) )
	{
		m_counter.add( query.lastQuery() );
		if ( query.next() )
		{
			return query.value( 0 ).toInt();
		}
	}
	else
	{
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << "SELECT COUNT(*) FROM " + table + " WHERE ID = \"" + id + "\"";
	}
	return 0;
}

/** @brief Count the number of rows in a table where the BaseSprite column matches the given value.
 *
 *  Executes: SELECT COUNT(*) FROM table WHERE BaseSprite = "id"
 *
 *  @param table The table name to query.
 *  @param id The BaseSprite value to match.
 *  @return The matching row count, or 0 on error.
 */
int DB::numRows2( QString table, QString id )
{
	QMutexLocker lock( &DB::m_mutex );
	++accessCounter;
	QSqlQuery query( getDB() );
	if ( query.exec( "SELECT COUNT(*) FROM " + table + " WHERE BaseSprite = \"" + id + "\"" ) )
	{
		m_counter.add( query.lastQuery() );
		if ( query.next() )
		{
			return query.value( 0 ).toInt();
		}
	}
	else
	{
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << "SELECT COUNT(*) FROM " + table + " WHERE BaseSprite = \"" + id + "\"";
	}
	return 0;
}

/** @brief Select a single row from a table by its ID, returned as a column-name-to-value map.
 *
 *  Executes: SELECT * FROM table WHERE ID = "whereVal"
 *
 *  @param table The table name to query.
 *  @param whereVal The ID value to match.
 *  @return A QVariantMap mapping column names to values, or an empty map if not found.
 */
QVariantMap DB::selectRow( QString table, QString whereVal )
{
	QMutexLocker lock( &DB::m_mutex );
	++accessCounter;
	QSqlQuery query( getDB() );
	if ( query.exec( "SELECT * FROM " + table + " WHERE ID = \"" + whereVal + "\"" ) )
	{
		m_counter.add( query.lastQuery() );
		if ( query.next() )
		{
			auto record = getDB().record( table );
			int count   = record.count();

			QVariantMap out;
			for ( int i = 0; i < count; ++i )
			{
				out.insert( record.field( i ).name(), query.value( record.field( i ).name() ) );
			}
			return out;
		}
	}
	else
	{
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << "SELECT * FROM " + table + " WHERE ID = \"" + whereVal + "\"";
	}

	return QVariantMap();
}

/** @brief Select multiple rows from a table matching a single WHERE condition.
 *
 *  Executes: SELECT * FROM table WHERE "whereCol" = "whereVal"
 *
 *  @param table The table name to query.
 *  @param whereCol The column name to filter on.
 *  @param whereVal The value to match in the WHERE clause.
 *  @return A list of QVariantMaps, each mapping column names to values.
 */
QList<QVariantMap> DB::selectRows( QString table, QString whereCol, QString whereVal )
{
	QMutexLocker lock( &DB::m_mutex );
	++accessCounter;
	QList<QVariantMap> out;
	QSqlQuery query( getDB() );
	if ( query.exec( "SELECT * FROM " + table + " WHERE \"" + whereCol + "\" = \"" + whereVal + "\"" ) )
	{
		m_counter.add( query.lastQuery() );
		while ( query.next() )
		{
			QVariantMap result;
			auto record = getDB().record( table );
			int count   = record.count();

			for ( int i = 0; i < count; ++i )
			{
				result.insert( record.field( i ).name(), query.value( record.field( i ).name() ) );
			}
			out.append( result );
		}
	}
	else
	{
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << "SELECT * FROM " + table + " WHERE \"" + whereCol + "\" = \"" + whereVal + "\"";
	}
	return out;
}

/** @brief Select multiple rows from a table matching two WHERE conditions (AND).
 *
 *  Executes: SELECT * FROM table WHERE "whereCol" = "whereVal" AND "whereCol2" = "whereVal2"
 *
 *  @param table The table name to query.
 *  @param whereCol The first WHERE column name.
 *  @param whereVal The first WHERE column value.
 *  @param whereCol2 The second WHERE column name.
 *  @param whereVal2 The second WHERE column value.
 *  @return A list of QVariantMaps, each mapping column names to values.
 */
QList<QVariantMap> DB::selectRows( QString table, QString whereCol, QString whereVal, QString whereCol2, QString whereVal2 )
{
	QMutexLocker lock( &DB::m_mutex );
	++accessCounter;
	QSqlQuery query( getDB() );
	QList<QVariantMap> out;
	if ( query.exec( "SELECT * FROM " + table + " WHERE \"" + whereCol + "\" = \"" + whereVal + "\" AND \"" + whereCol2 + "\" = \"" + whereVal2 + "\"" ) )
	{
		m_counter.add( query.lastQuery() );
		while ( query.next() )
		{
			QVariantMap result;
			auto record = getDB().record( table );
			int count   = record.count();

			for ( int i = 0; i < count; ++i )
			{
				result.insert( record.field( i ).name(), query.value( record.field( i ).name() ) );
			}
			out.append( result );
		}
	}
	else
	{
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << "SELECT * FROM " + table + " WHERE \"" + whereCol + "\" = \"" + whereVal + "\" AND \"" + whereCol2 + "\" = \"" + whereVal2 + "\"";
	}

	return out;
}


/** @brief Select all rows from a table with no filtering.
 *
 *  Executes: SELECT * FROM table
 *
 *  @param table The table name to query.
 *  @return A list of QVariantMaps, each mapping column names to values.
 */
QList<QVariantMap> DB::selectRows( QString table )
{
	QMutexLocker lock( &DB::m_mutex );
	++accessCounter;
	QList<QVariantMap> out;
	QSqlQuery query( getDB() );
	if ( query.exec( "SELECT * FROM " + table ) )
	{
		m_counter.add( query.lastQuery() );
		while ( query.next() )
		{
			QVariantMap result;
			auto record = getDB().record( table );
			int count   = record.count();

			for ( int i = 0; i < count; ++i )
			{
				result.insert( record.field( i ).name(), query.value( record.field( i ).name() ) );
			}
			out.append( result );
		}
	}
	else
	{
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << "SELECT * FROM " + table;
	}
	return out;
}

/** @brief Select all rows from a table where the ID column matches the given value.
 *
 *  Executes: SELECT * FROM table WHERE ID = "id"
 *
 *  @param table The table name to query.
 *  @param id The ID value to match.
 *  @return A list of QVariantMaps, each mapping column names to values.
 */
QList<QVariantMap> DB::selectRows( QString table, QString id )
{
	QMutexLocker lock( &DB::m_mutex );
	++accessCounter;
	QList<QVariantMap> out;
	QSqlQuery query( getDB() );
	if ( query.exec( "SELECT * FROM " + table + " WHERE ID = \"" + id + "\"" ) )
	{
		m_counter.add( query.lastQuery() );
		auto record = query.record();
		int count   = record.count();
		while ( query.next() )
		{
			QVariantMap result;
			for ( int i = 0; i < count; ++i )
			{
				result.insert( record.field( i ).name(), query.value( record.field( i ).name() ) );
			}
			out.append( result );
		}
	}
	else
	{
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << "SELECT * FROM " + table + " WHERE ID = \"" + id + "\"";
	}
	return out;
}

/** @brief Get a reference to the per-query-string hit counter.
 *  @return A reference to the Counter object tracking query string frequencies.
 */
Counter<QString>& DB::getQueryCounter()
{
	return m_counter;
}

/** @brief Get the list of all table names in the database.
 *  @return A QStringList of table names.
 */
QStringList DB::tables()
{
	return getDB().tables();
}

/** @brief Update an existing row in a table.
 *
 *  The "Mod" key is removed from values before building the UPDATE statement.
 *  The row is identified by the "ID" key in the values map.
 *
 *  @param table The table name to update.
 *  @param values A map of column names to new values; must contain an "ID" key.
 *  @return True if the update succeeded, false otherwise.
 */
bool DB::updateRow( QString table, QVariantMap values )
{
	values.remove( "Mod" );
	QString id = values.value( "ID" ).toString();

	QString query = "UPDATE " + table + " SET ";
	for ( auto key : values.keys() )
	{
		if ( key != "ID" )
		{
			query += key + " = '";
			query += values[key].toString();
			query += "', ";
		}
	}
	query.chop( 2 );
	query += " WHERE ID = ";
	query += "'";
	query += values.value( "ID" ).toString();
	query += "', ";

	query.chop( 2 );

	bool ok = false;
	DB::execQuery3( query, ok );
	return ok;
}

/** @brief Insert a new row into a table.
 *
 *  The "Mod" key is removed from values before building the INSERT statement.
 *
 *  @param table The table name to insert into.
 *  @param values A map of column names to values for the new row.
 *  @return True if the insertion succeeded, false otherwise.
 */
bool DB::addRow( QString table, QVariantMap values )
{
	values.remove( "Mod" );
	QString query = "INSERT INTO " + table + " ( ";

	QString query2 = "VALUES ( ";
	for ( auto key : values.keys() )
	{
		query += key;
		query += ", ";

		query2 += "'";
		query2 += values[key].toString();
		query2 += "', ";
	}
	query.chop( 2 );
	query += " ) ";
	query2.chop( 2 );
	query2 += " )";

	//qDebug() << query + query2;

	bool ok = false;
	DB::execQuery3( query + query2, ok );
	return ok;
}

/** @brief Delete all rows from a table where the ID matches the given value.
 *
 *  Executes: DELETE FROM table WHERE ID = id
 *
 *  @param table The table name to delete from.
 *  @param id The ID value identifying which rows to remove.
 *  @return True if the deletion succeeded, false otherwise.
 */
bool DB::removeRows( QString table, QString id )
{
	QString query = "DELETE FROM $TABLE WHERE ID = $ID";
	query.replace( "$TABLE", table );
	query.replace( "$ID", id );

	bool ok = false;
	DB::execQuery3( query, ok );
	return ok;
}

/** @brief Insert a new row into the Translation table.
 *
 *  Executes: INSERT INTO Translation ( ID, Text ) VALUES ( "id", "text" )
 *
 *  @param id The translation identifier string.
 *  @param text The translated text.
 *  @return True if the insertion succeeded, false otherwise.
 */
bool DB::addTranslation( QString id, QString text )
{
	QString query = "INSERT INTO Translation ( ID, Text ) VALUES ( \"%1\", \"%2\" )";
	query         = query.arg( id, text );
	bool ok;
	DB::execQuery3( query, ok );
	return ok;
}

/** @brief Look up a cached Workshop struct by its ID.
 *  @param id The workshop identifier string.
 *  @return A shared pointer to the Workshop data, or nullptr if not found.
 */
QSharedPointer<DBS::Workshop> DB::workshop( QString id )
{
	if( DB::m_workshops.contains( id ) )
	{
		return DB::m_workshops.value( id );
	}
	return nullptr;
}

/** @brief Look up a cached Job struct by its ID.
 *  @param id The job identifier string.
 *  @return A shared pointer to the Job data, or nullptr if not found.
 */
QSharedPointer<DBS::Job> DB::job( QString id )
{
	if( DB::m_jobs.contains( id ) )
	{
		return DB::m_jobs.value( id );
	}
	return nullptr;
}

/** @brief Get the list of all cached Job IDs.
 *  @return A list of job identifier strings from the cached jobs map.
 */
QList<QString> DB::jobIds()
{
	return DB::m_jobs.keys();
}
