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

QSqlQuery DB::m_itemCreateQuery;

void DB::init()
{
	QMutexLocker lock( &DB::m_mutex );

	QFile file( Config::getInstance().get( "dataPath" ).toString() + "/db/" + "ingnomia.db.sql" );
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

	resetLiveTables();
}

QSqlDatabase& DB::getDB()
{
	auto thread = QThread::currentThreadId();
	if ( !m_connections.contains( thread ) )
	{
		auto db = QSqlDatabase::addDatabase( "QSQLITE", QString::number( reinterpret_cast<long long>( thread ) ) );
		db.setDatabaseName( "file:game?mode=memory&cache=shared" );
		if ( !db.open() )
		{
			qDebug() << "Error: create in memory db";
			exit( 0 );
		}
		else
		{
			qDebug() << "Memory DB: connection ok";
		}
		m_connections[thread] = db;
	}
	return m_connections[thread];
}

void DB::resetLiveTables()
{
	return;
	// drop live tables
	execQuery( "DROP TABLE IF EXISTS \"v_Items\"" );

	//create live tables
	QString qs = "CREATE TABLE v_Items (UID BIGINT, position VARCHAR, spriteID BIGINT, itemUID INTEGER, materialUID INTEGER, itemSID VARCHAR, materialSID VARCHAR, category VARCHAR, \"group\" VARCHAR, pickedUp BOOLEAN, isConstructed BOOLEAN, inStockpile BIGINT, inJob BIGINT, isContainer BOOLEAN, inContainer BIGINT, madeBy BIGINT, quality INTEGER, \"value\" INTEGER, eatValue INTEGER, drinkValue INTEGER, color VARCHAR, hasComponents BOOLEAN, componentOf BIGINT, isTool BOOLEAN, lightIntensity INTEGER, stackSize INTEGER )";
	execQuery( qs );

	//prepare queries
	m_itemCreateQuery = QSqlQuery();

	m_itemCreateQuery.prepare( "INSERT INTO v_Items ( UID, position, spriteID, itemUID, materialUID, itemSID, materialSID, category, \"group\", pickedUp, isConstructed, inStockpile, inJob, isContainer, inContainer, madeBy , quality, \"value\", eatValue, drinkValue, color, hasComponents, componentOf, isTool, lightIntensity, stackSize ) VALUES ( ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ? )" );
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

QVariantMap DB::selectRow( QString table, int whereVal )
{
	QMutexLocker lock( &DB::m_mutex );
	++accessCounter;
	QSqlQuery query( getDB() );
	if ( query.exec( "SELECT * FROM " + table + " WHERE rowid = \"" + QString::number( whereVal ) + "\"" ) )
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
		qDebug() << "SELECT * FROM " + table + " WHERE roid = \"" + whereVal + "\"";
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
		qDebug() << "SELECT rowid FROM " + table + " WHERE \"" + whereCol + "\" = \"" + whereVal + "\"";
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
		qDebug() << "SELECT rowid FROM " + table;
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
		qDebug() << "SELECT rowid FROM " + table + " WHERE ID = \"" + id + "\"";
	}
	return out;
}

QList<QVariantMap> DB::selectRows( QString table, int rowid )
{
	QMutexLocker lock( &DB::m_mutex );
	++accessCounter;
	QList<QVariantMap> out;
	QSqlQuery query( getDB() );
	if ( query.exec( "SELECT * FROM " + table + " WHERE rowid = \"" + QString::number( rowid ) + "\"" ) )
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
		qDebug() << "SELECT rowid FROM " + table + " WHERE rowid = \"" + QString::number( rowid ) + "\"";
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

bool DB::createItem( const Item& item, QString itemSID, QString materialSID )
{
	auto baseItemMap = selectRow( "Items", itemSID );
	int value        = item.value();
	QMutexLocker lock( &DB::m_mutex );
	++accessCounter;

	m_itemCreateQuery.bindValue( 0, QString::number( item.id() ) );                   //UID
	m_itemCreateQuery.bindValue( 1, item.getPos().toString() );                       //position
	m_itemCreateQuery.bindValue( 2, QString::number( item.spriteID() ) );             //spriteID
	m_itemCreateQuery.bindValue( 3, QString::number( item.itemUID() ) );              //itemUID
	m_itemCreateQuery.bindValue( 4, QString::number( item.materialUID() ) );          //materialUID
	m_itemCreateQuery.bindValue( 5, itemSID );                                        //itemSID
	m_itemCreateQuery.bindValue( 6, materialSID );                                    //materialSID
	m_itemCreateQuery.bindValue( 7, baseItemMap.value( "Category" ).toString() );     //category
	m_itemCreateQuery.bindValue( 8, baseItemMap.value( "Group" ).toString() );        // group
	m_itemCreateQuery.bindValue( 9, 0 );                                              //pickedUp
	m_itemCreateQuery.bindValue( 10, 0 );                                             //isConstructed
	m_itemCreateQuery.bindValue( 11, 0 );                                             //inStockpile
	m_itemCreateQuery.bindValue( 12, 0 );                                             // inJob
	m_itemCreateQuery.bindValue( 13, baseItemMap.value( "IsContainer" ).toBool() );   //isContainer
	m_itemCreateQuery.bindValue( 14, 0 );                                             //inContainer
	m_itemCreateQuery.bindValue( 15, 0 );                                             // madeBy
	m_itemCreateQuery.bindValue( 16, 1 );                                             //quality
	m_itemCreateQuery.bindValue( 17, value );                                         //value
	m_itemCreateQuery.bindValue( 18, baseItemMap.value( "EatValue" ).toInt() );       //eatValue
	m_itemCreateQuery.bindValue( 19, baseItemMap.value( "DrinkValue" ).toInt() );     //drinkValue
	m_itemCreateQuery.bindValue( 20, "" );                                            //color
	m_itemCreateQuery.bindValue( 21, baseItemMap.value( "HasComponents" ).toBool() ); //hasComponents
	m_itemCreateQuery.bindValue( 22, 0 );                                             //componentOf
	m_itemCreateQuery.bindValue( 23, baseItemMap.value( "IsTool" ).toBool() );        //istTool
	m_itemCreateQuery.bindValue( 24, baseItemMap.value( "LightIntensity" ).toInt() ); //lightIntensity
	m_itemCreateQuery.bindValue( 25, baseItemMap.value( "StackSize" ).toBool() );

	if ( !m_itemCreateQuery.exec() )
	{
		qDebug() << "sql error:  " << m_itemCreateQuery.lastError();
		//qDebug() << "INSERT INTO v_Items ( \"UID\", \"position\", \"spriteID\" ) VALUES (\"" + QString::number( itemID ) + "\", \"" + pos.toString() + "\", \"" + QString::number( spriteID ) +"\" )";
		return false;
	}

	return true;
}

bool DB::createItem( const QVariantMap& in )
{
	QMutexLocker lock( &DB::m_mutex );
	++accessCounter;

	m_itemCreateQuery.bindValue( 0, in.value( "UID" ) );             //UID
	m_itemCreateQuery.bindValue( 1, in.value( "position" ) );        //position
	m_itemCreateQuery.bindValue( 2, in.value( "spriteID" ) );        //spriteID
	m_itemCreateQuery.bindValue( 3, in.value( "itemUID" ) );         //itemUID
	m_itemCreateQuery.bindValue( 4, in.value( "materialUID" ) );     //materialUID
	m_itemCreateQuery.bindValue( 5, in.value( "itemSID" ) );         //itemSID
	m_itemCreateQuery.bindValue( 6, in.value( "materialSID" ) );     //materialSID
	m_itemCreateQuery.bindValue( 7, in.value( "category" ) );        //category
	m_itemCreateQuery.bindValue( 8, in.value( "group" ) );           // group
	m_itemCreateQuery.bindValue( 9, in.value( "pickedUp" ) );        //pickedUp
	m_itemCreateQuery.bindValue( 10, in.value( "isConstructed" ) );  //isConstructed
	m_itemCreateQuery.bindValue( 11, in.value( "inStockpile" ) );    //inStockpile
	m_itemCreateQuery.bindValue( 12, in.value( "inJob" ) );          // inJob
	m_itemCreateQuery.bindValue( 13, in.value( "isContainer" ) );    //isContainer
	m_itemCreateQuery.bindValue( 14, in.value( "inContainer" ) );    //inContainer
	m_itemCreateQuery.bindValue( 15, in.value( "madeBy" ) );         // madeBy
	m_itemCreateQuery.bindValue( 16, in.value( "quality" ) );        //quality
	m_itemCreateQuery.bindValue( 17, in.value( "value" ) );          //value
	m_itemCreateQuery.bindValue( 18, in.value( "eatValue" ) );       //eatValue
	m_itemCreateQuery.bindValue( 19, in.value( "drinkValue" ) );     //drinkValue
	m_itemCreateQuery.bindValue( 20, in.value( "color" ) );          //color
	m_itemCreateQuery.bindValue( 21, in.value( "hasComponents" ) );  //hasComponents
	m_itemCreateQuery.bindValue( 22, in.value( "componentOf" ) );    //componentOf
	m_itemCreateQuery.bindValue( 23, in.value( "isTool" ) );         //istTool
	m_itemCreateQuery.bindValue( 24, in.value( "lightIntensity" ) ); //lightIntensity
	m_itemCreateQuery.bindValue( 25, in.value( "stackSize" ) );

	if ( !m_itemCreateQuery.exec() )
	{
		qDebug() << "sql error:  " << m_itemCreateQuery.lastError();
		//qDebug() << "INSERT INTO v_Items ( \"UID\", \"position\", \"spriteID\" ) VALUES (\"" + QString::number( itemID ) + "\", \"" + pos.toString() + "\", \"" + QString::number( spriteID ) +"\" )";
		return false;
	}

	return true;
}

bool DB::destroyItem( unsigned int itemID )
{
	QMutexLocker lock( &DB::m_mutex );
	QSqlQuery query = getDB().exec( "DELETE FROM v_Items WHERE UID = \"" + QString::number( itemID ) + "\"" );
	if ( query.isValid() )
	{
		if ( query.numRowsAffected() > 0 )
		{
			return true;
		}
	}
	return false;
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
