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
#include "db.h"

#include "dbstructs.h"

#include "../base/config.h"
#include "../game/item.h"
#include "containersHelper.h"
#include "spdlog/spdlog.h"

#include <QFile>
#include <QMutexLocker>
#include <QSqlError>
#include <QSqlField>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QThread>

#include <filesystem>

#include <range/v3/view.hpp>
#include <range/v3/action/split.hpp>

namespace fs = std::filesystem;

QMutex DB::m_mutex;
int DB::accessCounter = 0;
Counter<QString> DB::m_counter;
absl::btree_map<Qt::HANDLE, QSqlDatabase> DB::m_connections;


absl::flat_hash_map<std::string, std::unique_ptr<DBS::Workshop>> DB::m_workshops;
absl::flat_hash_map<std::string, std::unique_ptr<DBS::Job>> DB::m_jobs;

void DB::init()
{
	QMutexLocker lock( &DB::m_mutex );

	const auto dbPath = fs::path(Global::cfg->get<std::string>( "dataPath" )) / "db" / "ingnomia.db.sql";
	QFile file( QString::fromStdString( dbPath.string() ) );
	file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString sql = file.readAll();
    file.close();

	QSqlQuery destQuery( getDB() );

	auto statements = sql.split( ";" );
	for( auto s: statements )
	{
		if ( !s.trimmed().isEmpty() && !destQuery.exec( s ) )
		{
			spdlog::debug("Init error: '{}' for query '{}'", destQuery.lastError().text().toStdString(), s.toStdString());
		}
	}
}

void DB::initStructs()
{
	m_workshops.clear();
	auto rows = DB::selectRows( "Workshops" );
	for( const auto& row : rows )
	{
		auto ws = std::make_unique<DBS::Workshop>();

		const auto gui = row.value( "GUI" ).toString().toStdString();
		const auto icon = row.value( "Icon" ).toString().toStdString();

		ws->ID = row.value( "ID" ).toString().toStdString();
		ws->Crafts = ranges::actions::split( row.value( "Crafts" ).toString().toStdString(), '|' ) | ranges::to<std::vector<std::string>>();
		ws->GUI = vars::opt(gui);
		ws->InputTile = Position( row.value( "InputTile" ) );
		ws->OutputTile = row.value( "OutputTile" ).toString();
		ws->Size = row.value( "Size" ).toString().toStdString();
		ws->NoAutoGenerate = row.value( "NoAutoGenerate" ).toBool();
		ws->Icon = vars::opt( icon );
		ws->Tab = row.value( "Tab" ).toString().toStdString();

		auto crows = DB::selectRows( "Workshops_Components", QString::fromStdString(ws->ID) );
		for( const auto& crow : crows )
		{
			ws->components.emplace_back( DBS::Workshop_Component {
				.Amount       = crow.value( "Amount" ).toInt(),
				.ItemID       = vars::opt(crow.value( "ItemID" ).toString().toStdString()),
				.MaterialItem = vars::opt(crow.value( "MaterialItem" ).toString().toStdString()),
				.Offset       = Position( crow.value( "Offset" ) ),
				.Required     = vars::opt(crow.value( "Required" ).toString().toStdString()),
				.Forbidden    = vars::opt(crow.value( "Forbidden" ).toString().toStdString()),
				.SpriteID     = vars::opt(crow.value( "SpriteID" ).toString().toStdString()),
				.SpriteID2    = vars::opt(crow.value( "SpriteID2" ).toString().toStdString()),
				.Type         = vars::opt(crow.value( "Type" ).toString().toStdString()),
				.WallRotation = vars::opt(crow.value( "WallRotation" ).toString().toStdString()),
				.IsFloor      = crow.value( "IsFloor" ).toBool() } );
		}

		m_workshops.insert_or_assign( ws->ID, std::move(ws) );
	}

	m_jobs.clear();
	rows = DB::selectRows( "Jobs" );
	for( const auto& row : rows )
	{
		auto job = std::make_unique<DBS::Job>();

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
		m_jobs.insert_or_assign( job->ID.toStdString(), std::move(job) );
	}
}

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
			spdlog::debug("Error: create in memory db");
			abort();
		}
		else
		{
			spdlog::debug("Memory DB: connection ok");
		}
		m_connections[thread] = db;
	}
	return m_connections[thread];
}

int DB::getAccessCounter()
{
	int tmp       = accessCounter;
	accessCounter = 0;
	return tmp;
}

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
		spdlog::debug( "sql error:   {}", query.lastError().text().toStdString() );
		spdlog::debug( "{}", queryString.toStdString() );
	}

	return QVariant();
}

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
		spdlog::debug( "sql error:   {}", query.lastError().text().toStdString() );
		spdlog::debug( "{}", queryString.toStdString() );
	}

	return out;
}

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
		spdlog::debug( "sql error:   {}", query.lastError().text().toStdString() );
		spdlog::debug( "{}", queryString.toStdString() );
	}

	ok = false;
	return query;
}

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
		spdlog::debug( "sql error:   {}", query.lastError().text().toStdString() );
		spdlog::debug( ("SELECT \"" + selectCol + "\" FROM " + table + " WHERE ID = " + "\"" + whereVal + "\"").toStdString() );
	}

	return QVariant();
}

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
		spdlog::debug( "sql error:   {}", query.lastError().text().toStdString() );
		spdlog::debug( ("SELECT \"" + selectCol + "\" FROM " + table + " WHERE rowid = " + "\"" + QString::number( whereVal ) + "\"").toStdString() );
	}

	return QVariant();
}

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
		spdlog::debug( "sql error:   {}", query.lastError().text().toStdString() );
		spdlog::debug( ("SELECT \"" + selectCol + "\" FROM " + table + " WHERE \"" + whereCol + "\" = \"" + whereVal + "\"").toStdString() );
	}

	return out;
}

QVariantList DB::select2( QString selectCol, QString table, QString whereCol, int whereVal )
{
	return select2( selectCol, table, whereCol, QString::number( whereVal ) );
}

QVariantList DB::select2( QString selectCol, QString table, QString whereCol, float whereVal )
{
	return select2( selectCol, table, whereCol, QString::number( whereVal ) );
}

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
		spdlog::debug( "sql error:   {}", query.lastError().text().toStdString() );
		spdlog::debug( ("SELECT \"" + selectCol + "\" FROM " + table + " WHERE \"" + whereCol + "\" = \"" + whereVal + "\" AND \"" + whereCol2 + "\" = \"" + whereVal2 + "\"").toStdString() );
	}

	return out;
}

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
		spdlog::debug( "sql error:   {}", query.lastError().text().toStdString() );
		spdlog::debug( ("SELECT ID FROM " + table).toStdString() );
	}

	return out;
}

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
		spdlog::debug( "sql error:   {}", query.lastError().text().toStdString() );
//		spdlog::debug( "{}", getDB().tables() );
		spdlog::debug( "SELECT ID FROM {}", table.toStdString());
	}

	return out;
}

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
		spdlog::debug( "sql error:   {}", query.lastError().text().toStdString() );
		spdlog::debug( ("SELECT COUNT(*) FROM " + table).toStdString() );
	}
	return 0;
}

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
		spdlog::debug( "sql error:   {}", query.lastError().text().toStdString() );
		spdlog::debug( ("SELECT COUNT(*) FROM " + table + " WHERE ID = \"" + id + "\"").toStdString() );
	}
	return 0;
}

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
		spdlog::debug( "sql error:   {}", query.lastError().text().toStdString() );
		spdlog::debug( ("SELECT COUNT(*) FROM " + table + " WHERE BaseSprite = \"" + id + "\"").toStdString() );
	}
	return 0;
}

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
		spdlog::debug( "sql error:   {}", query.lastError().text().toStdString() );
		spdlog::debug( ("SELECT * FROM " + table + " WHERE ID = \"" + whereVal + "\"").toStdString() );
	}

	return QVariantMap();
}

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
		spdlog::debug( "sql error:   {}", query.lastError().text().toStdString() );
		spdlog::debug( ("SELECT * FROM " + table + " WHERE \"" + whereCol + "\" = \"" + whereVal + "\"").toStdString() );
	}
	return out;
}

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
		spdlog::debug( "sql error:   {}", query.lastError().text().toStdString() );
		spdlog::debug( ("SELECT * FROM " + table + " WHERE \"" + whereCol + "\" = \"" + whereVal + "\" AND \"" + whereCol2 + "\" = \"" + whereVal2 + "\"").toStdString() );
	}

	return out;
}


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
		spdlog::debug( "sql error:   {}", query.lastError().text().toStdString() );
		spdlog::debug( ("SELECT * FROM " + table).toStdString() );
	}
	return out;
}

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
		spdlog::debug( "sql error:   {}", query.lastError().text().toStdString() );
		spdlog::debug( ("SELECT * FROM " + table + " WHERE ID = \"" + id + "\"").toStdString() );
	}
	return out;
}

Counter<QString>& DB::getQueryCounter()
{
	return m_counter;
}

QStringList DB::tables()
{
	return getDB().tables();
}

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

	//spdlog::debug( (query + query2).toStdString() );

	bool ok = false;
	DB::execQuery3( query + query2, ok );
	return ok;
}

bool DB::removeRows( QString table, QString id )
{
	QString query = "DELETE FROM $TABLE WHERE ID = $ID";
	query.replace( "$TABLE", table );
	query.replace( "$ID", id );

	bool ok = false;
	DB::execQuery3( query, ok );
	return ok;
}

bool DB::addTranslation( QString id, QString text )
{
	QString query = "INSERT INTO Translation ( ID, Text ) VALUES ( \"%1\", \"%2\" )";
	query         = query.arg( id, text );
	bool ok;
	DB::execQuery3( query, ok );
	return ok;
}

DBS::Workshop* DB::workshop( const std::string& id )
{
	const auto it = DB::m_workshops.find( id );
	if ( it == DB::m_workshops.end() )
	{
		return nullptr;
	}
	return it->second.get();
}

DBS::Job* DB::job( const std::string& id )
{
	const auto it = DB::m_jobs.find( id );
	if ( it == DB::m_jobs.end() )
	{
		return nullptr;
	}
	return it->second.get();
}

std::vector<std::string> DB::jobIds()
{
	std::vector<std::string> keys;
	for ( const auto& key : DB::m_jobs | ranges::views::keys ) {
		keys.push_back(key);
	}
	return keys;
}