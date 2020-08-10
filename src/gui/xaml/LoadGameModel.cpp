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
#include "LoadGameModel.h"

#include "../../base/io.h"

#include <NsApp/Application.h>
#include <NsCore/Log.h>
#include <NsCore/ReflectionImplement.h>
#include <NsCore/ReflectionImplementEnum.h>
#include <NsGui/ObservableCollection.h>
#include <NsGui/UIElement.h>

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

using namespace IngnomiaGUI;
using namespace Noesis;
using namespace NoesisApp;

////////////////////////////////////////////////////////////////////////////////////////////////////
SaveItem::SaveItem( QString name, QString path, QString version, QString date, bool compatible )
{
	_name       = name.toStdString().c_str();
	_path       = path.toStdString().c_str();
	_version    = version.toStdString().c_str();
	_date       = date.toStdString().c_str();
	_compatible = compatible;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
LoadGameModel::LoadGameModel() :
	_selectedKingdom( nullptr ),
	_selectedGame( nullptr )
{
	_loadGame.SetExecuteFunc( MakeDelegate( this, &LoadGameModel::OnLoadGame ) );

	_savedKingdoms = *new ObservableCollection<SaveItem>();
	_savedGames    = *new ObservableCollection<SaveItem>();

	updateSavedKingdoms();

	if ( _savedKingdoms->Count() > 0 )
	{
		SetSelectedKingdom( _savedKingdoms->Get( 0 ) );
	}
}

Noesis::ObservableCollection<SaveItem>* LoadGameModel::GetSavedKingdoms() const
{
	return _savedKingdoms;
}

Noesis::ObservableCollection<SaveItem>* LoadGameModel::GetSavedGames() const
{
	return _savedGames;
}

void LoadGameModel::updateSavedKingdoms()
{
	_savedKingdoms->Clear();

	QString sfolder = QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/My Games/Ingnomia/save/";

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

				QString name    = vm.value( "kingdomName" ).toString();
				QString version = vm.value( "Version" ).toString();
				QString date    = fi.lastModified().toString();

				_savedKingdoms->Add( MakePtr<SaveItem>( name, kingdomFolder, version, date ) );
			}
		}
	}
	OnPropertyChanged( "SavedKingdoms" );
}

void LoadGameModel::updateSavedGames( const Noesis::String& path )
{
	_savedGames->Clear();
	QString kFolder( path.Str() );
	QDir dir( kFolder );
	dir.setFilter( QDir::Dirs | QDir::NoDotAndDotDot );
	dir.setSorting( QDir::Time );

	auto dirs = dir.entryList();

	for ( auto sdir : dirs )
	{
		QString folder = kFolder + "/" + sdir;

		QJsonDocument jd;
		IO::loadFile( folder + "/game.json", jd );
		QJsonArray ja  = jd.array();
		QVariantMap vm = ja.toVariantList().first().toMap();

		QString version = IO::versionString( folder );
		bool compatible = true;

		if ( !IO::saveCompatible( folder ) )
		{
			version += " - not compatible - sorry broke save games again";
			compatible = false;
		}

		QString name = vm.value( "kingdomName" ).toString();

		QFile file( folder + "/game.json" );
		QFileInfo fi( file );
		QString date( fi.lastModified().toString() );

		_savedGames->Add( MakePtr<SaveItem>( name, folder, version, date, compatible ) );
	}

	if ( _savedGames->Count() > 0 )
	{
		SetSelectedGame( _savedGames->Get( 0 ) );
	}

	OnPropertyChanged( "SavedGames" );
}

void LoadGameModel::SetSelectedKingdom( SaveItem* item )
{
	if ( _selectedKingdom != item )
	{
		//qDebug() << item->_path.c_str();
		_selectedKingdom = item;
		OnPropertyChanged( "SelectedKingdom" );

		if ( item )
		{
			updateSavedGames( item->_path );
		}
	}
}

SaveItem* LoadGameModel::GetSelectedKingdom() const
{
	return _selectedKingdom;
}

void LoadGameModel::SetSelectedGame( SaveItem* item )
{
	if ( _selectedGame != item )
	{
		_selectedGame = item;
		OnPropertyChanged( "SelectedGame" );
	}
}

SaveItem* LoadGameModel::GetSelectedGame() const
{
	return _selectedGame;
}

const NoesisApp::DelegateCommand* LoadGameModel::GetLoadGame() const
{
	return &_loadGame;
}

void LoadGameModel::OnLoadGame( BaseComponent* param )
{
	qDebug() << "OnLoadGame";
	if ( _selectedGame )
	{
		qDebug() << _selectedGame->_path.Str();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION( LoadGameModel, "IngnomiaGUI.LoadGameModel" )
{
	NsProp( "LoadGame", &LoadGameModel::GetLoadGame );

	NsProp( "SavedKingdoms", &LoadGameModel::GetSavedKingdoms );
	NsProp( "SavedGames", &LoadGameModel::GetSavedGames );

	NsProp( "SelectedKingdom", &LoadGameModel::GetSelectedKingdom, &LoadGameModel::SetSelectedKingdom );
	NsProp( "SelectedGame", &LoadGameModel::GetSelectedGame, &LoadGameModel::SetSelectedGame );
}

NS_IMPLEMENT_REFLECTION( SaveItem )
{
	NsProp( "Name", &SaveItem::_name );
	NsProp( "Path", &SaveItem::_path );
	NsProp( "Version", &SaveItem::_version );
	NsProp( "Date", &SaveItem::_date );
}
