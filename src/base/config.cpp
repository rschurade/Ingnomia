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
#include "config.h"

#include "../base/io.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>

Config::Config()
{
}

Config::~Config()
{
}

bool Config::init()
{
	IO::createFolders();

	//check if Ingnomia folder in /Documents/My Games exist
	QString folder = QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/My Games/Ingnomia/";
	bool ok        = true;
	QJsonDocument jd;

	if ( !IO::loadFile( folder + "settings/config.json", jd ) )
	{
		return false;
	}

	m_settings = jd.toVariant().toMap();

	/*
	if ( !IO::loadFile( folder + "settings/keybindings.json", jd ) )
	{
		if ( QFile::exists( "keybindings.json" ) )
		{
			QFile::copy( "keybindings.json", folder + "settings/keybindings.json" );
		}
	}
	*/
	// add values to exisiting confings
	if ( !m_settings.contains( "XpMod" ) || m_settings.value( "XpMod" ).toInt() != 250 )
	{
		m_settings.insert( "XpMod", 250. );
	}

	if ( !m_settings.contains( "fow" ) )
	{
		m_settings.insert( "fow", true );
	}

	if ( !m_settings.contains( "AutoSaveInterval" ) )
	{
		m_settings.insert( "AutoSaveInterval", 3 );
	}
	if ( !m_settings.contains( "GUIScale" ) )
	{
		m_settings.insert( "GUIScale", 1.0 );
	}
	m_settings.insert( "dataPath", QCoreApplication::applicationDirPath() + "/content" );

	return ok;
}

QVariant Config::get( QString key )
{
	if ( m_settings.contains( key ) )
	{
		QVariant out = m_settings[key];
		return out;
	}
	else
	{
		return QVariant();
	}
}

void Config::set( QString key, QVariant value )
{
	m_settings[key] = value;
}
