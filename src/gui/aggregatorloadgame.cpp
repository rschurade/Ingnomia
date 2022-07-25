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
#include "aggregatorloadgame.h"

#include "../base/io.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QStandardPaths>

AggregatorLoadGame::AggregatorLoadGame( QObject* parent ) :
	QObject(parent)
{
}

AggregatorLoadGame::~AggregatorLoadGame()
{
}

void AggregatorLoadGame::onRequestKingdoms()
{
	const auto sfolder = IO::getDataFolder() / "save";

	m_kingdomList.clear();

	QDir dir( QString::fromStdString(sfolder.string()) );
	dir.setFilter( QDir::Dirs | QDir::NoDotAndDotDot );

	auto entryList = dir.entryList();

	for ( auto sdir : entryList )
	{
		const auto kingdomFolder = sfolder / sdir.toStdString();

		QDir kdir( QString::fromStdString(kingdomFolder.string()) );
		kdir.setFilter( QDir::Dirs | QDir::NoDotAndDotDot );
		kdir.setSorting( QDir::Time );

		auto gdirs = kdir.entryList();

		if ( !gdirs.empty() )
		{
			for( const auto& gdir : gdirs )
			{
				const auto gameFolder = kingdomFolder / gdir.toStdString();
				QJsonDocument jd;
				IO::loadFile( gameFolder / "game.json", jd );

				if ( jd.isArray() )
				{
					QJsonArray ja = jd.array();
					auto vl = ja.toVariantList();
					if( vl.size() > 0 )
					{
						QVariantMap vm = vl.first().toMap();

						QFile file( QString::fromStdString( ( gameFolder / "game.json" ).string() ) );
						QFileInfo fi( file );

						GuiSaveInfo gsk;
						gsk.folder  = kingdomFolder.string();
						gsk.name    = vm.value( "kingdomName" ).toString();
						gsk.version = vm.value( "Version" ).toString().toStdString();
						gsk.date    = fi.lastModified();

						m_kingdomList.append( gsk );
						break;
					}
				}
			}
		}
	}
	std::sort( m_kingdomList.begin(), m_kingdomList.end(), []( const GuiSaveInfo& a, const GuiSaveInfo& b ) {
		return a.date > b.date;
	} );

	signalKingdoms( m_kingdomList );
}

void AggregatorLoadGame::onRequestSaveGames( const fs::path& path )
{
	m_gameList.clear();

	QDir dir( QString::fromStdString(path.string()) );
	dir.setFilter( QDir::Dirs | QDir::NoDotAndDotDot );

	auto sdirs = dir.entryList();

	for ( auto sdir : sdirs )
	{
		GuiSaveInfo gsi;

		gsi.folder = ( path / sdir.toStdString() ).string();
		gsi.dir    = sdir;

		QJsonDocument jd;
		IO::loadFile( fs::path(gsi.folder) / "game.json", jd );

		if( jd.isArray() )
		{
			QJsonArray ja  = jd.array();
			auto vl = ja.toVariantList();
			if( vl.size() > 0 )
			{
				QVariantMap vm = vl.first().toMap();

				gsi.version    = IO::versionString( gsi.folder );
				gsi.compatible = true;

				if ( !IO::saveCompatible( gsi.folder ) )
				{
					gsi.version += " - not compatible - sorry broke save games again";
					gsi.compatible = false;
				}

				gsi.name = vm.value( "kingdomName" ).toString();

				QFile file( QString::fromStdString( ( fs::path( gsi.folder ) / "game.json" ).string() ) );
				QFileInfo fi( file );
				gsi.date = fi.lastModified();

				m_gameList.append( gsi );
			}
		}
	}

	std::sort( m_gameList.begin(), m_gameList.end(), []( const GuiSaveInfo& a, const GuiSaveInfo& b ) {
		return a.date > b.date;
	} );

	signalSaveGames( m_gameList );
}
