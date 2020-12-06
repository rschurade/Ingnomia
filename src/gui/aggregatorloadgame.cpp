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
#include "AggregatorLoadGame.h"

#include "../base/io.h"

#include <QDebug>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QStandardPaths>


AggregatorLoadGame::AggregatorLoadGame( QObject* parent )
{
	
}

AggregatorLoadGame::~AggregatorLoadGame()
{
}

void AggregatorLoadGame::onRequestKingdoms()
{
    QString sfolder = QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/My Games/Ingnomia/save/";

	m_kingdomList.clear();

	QDir dir( sfolder );
	dir.setFilter( QDir::Dirs | QDir::NoDotAndDotDot );
	dir.setSorting( QDir::Time );

	auto entryList = dir.entryList();

	for ( auto sdir : entryList )
	{
		QString kingdomFolder = sfolder + sdir;

		QDirIterator gdirectories( kingdomFolder, QDir::Dirs | QDir::NoDotAndDotDot );
		QStringList gdirs;
		while ( gdirectories.hasNext() )
		{
			gdirectories.next();
			gdirs.push_back( gdirectories.filePath() );
		}
		if ( !gdirs.empty() )
		{
			QString gdir = gdirs.last();

			QJsonDocument jd;
			IO::loadFile( gdir + "/game.json", jd );

			if ( jd.isArray() )
			{
				QJsonArray ja = jd.array();

				QVariantMap vm = ja.toVariantList().first().toMap();

				QFile file( gdir + "/game.json" );
				QFileInfo fi( file );

				GuiSaveInfo gsk;
				gsk.folder  = kingdomFolder;
				gsk.name    = vm.value( "kingdomName" ).toString();
				gsk.version = vm.value( "Version" ).toString();
				gsk.date    = fi.lastModified().toString();

				m_kingdomList.append( gsk );
			}
		}
	}

	emit signalKingdoms( m_kingdomList );
}

void AggregatorLoadGame::onRequestSaveGames( const QString path )
{
	m_gameList.clear();

	QDir dir( path );
	dir.setFilter( QDir::Dirs | QDir::NoDotAndDotDot );
	dir.setSorting( QDir::Time );

	auto dirs = dir.entryList();

	for ( auto sdir : dirs )
	{
		GuiSaveInfo gsi;

		gsi.folder = path + "/" + sdir;
		gsi.dir = sdir;

		QJsonDocument jd;
		IO::loadFile( gsi.folder + "/game.json", jd );
		QJsonArray ja  = jd.array();
		QVariantMap vm = ja.toVariantList().first().toMap();

		gsi.version = IO::versionString( gsi.folder );
		gsi.compatible = true;

		if ( !IO::saveCompatible( gsi.folder ) )
		{
			gsi.version += " - not compatible - sorry broke save games again";
			gsi.compatible = false;
		}

		gsi.name = vm.value( "kingdomName" ).toString();


		QFile file( gsi.folder + "/game.json" );
		QFileInfo fi( file );
		gsi.date = fi.lastModified().toString();

		m_gameList.append( gsi );
	}

	emit signalSaveGames( m_gameList );
}
