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
		if ( !destQuery.exec( s ) )
		{
			qDebug() << destQuery.lastError();
		}
	}
}

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
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << queryString;
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
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << queryString;
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
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << queryString;
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
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << "SELECT \"" + selectCol + "\" FROM " + table + " WHERE ID = " + "\"" + whereVal + "\"";
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
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << "SELECT \"" + selectCol + "\" FROM " + table + " WHERE rowid = " + "\"" + QString::number( whereVal ) + "\"";
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
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << "SELECT \"" + selectCol + "\" FROM " + table + " WHERE \"" + whereCol + "\" = \"" + whereVal + "\"";
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
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << "SELECT \"" + selectCol + "\" FROM " + table + " WHERE \"" + whereCol + "\" = \"" + whereVal + "\" AND \"" + whereCol2 + "\" = \"" + whereVal2 + "\"";
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
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << "SELECT ID FROM " + table;
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
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << getDB().tables();
		qDebug() << "SELECT ID FROM " + table;
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
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << "SELECT COUNT(*) FROM " + table;
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
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << "SELECT COUNT(*) FROM " + table + " WHERE ID = \"" + id + "\"";
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
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << "SELECT COUNT(*) FROM " + table + " WHERE BaseSprite = \"" + id + "\"";
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
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << "SELECT * FROM " + table + " WHERE ID = \"" + whereVal + "\"";
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
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << "SELECT * FROM " + table + " WHERE \"" + whereCol + "\" = \"" + whereVal + "\"";
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
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << "SELECT * FROM " + table + " WHERE \"" + whereCol + "\" = \"" + whereVal + "\" AND \"" + whereCol2 + "\" = \"" + whereVal2 + "\"";
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
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << "SELECT * FROM " + table;
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
		qDebug() << "sql error:  " << query.lastError();
		qDebug() << "SELECT * FROM " + table + " WHERE ID = \"" + id + "\"";
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

	//qDebug() << query + query2;

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

QSharedPointer<DBS::Workshop> DB::workshop( QString id )
{
	if( DB::m_workshops.contains( id ) )
	{
		return DB::m_workshops.value( id );
	}
	return nullptr;
}

QSharedPointer<DBS::Job> DB::job( QString id )
{
	if( DB::m_jobs.contains( id ) )
	{
		return DB::m_jobs.value( id );
	}
	return nullptr;
}

QList<QString> DB::jobIds()
{
	return DB::m_jobs.keys();
}