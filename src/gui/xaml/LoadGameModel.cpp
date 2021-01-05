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

#include "loadgameproxy.h"

#include <NsApp/Application.h>
#include <NsCore/Log.h>
#include <NsCore/ReflectionImplement.h>
#include <NsCore/ReflectionImplementEnum.h>
#include <NsGui/ObservableCollection.h>
#include <NsGui/UIElement.h>

#include <QDebug>

using namespace IngnomiaGUI;
using namespace Noesis;
using namespace NoesisApp;

////////////////////////////////////////////////////////////////////////////////////////////////////
SaveItem::SaveItem( QString name, QString path, QString dir, QString version, QString date, bool compatible )
{
	_name       = name.toStdString().c_str();
	_path       = path.toStdString().c_str();
	_dir        = dir.toStdString().c_str();
	_version    = version.toStdString().c_str();
	_date       = date.toStdString().c_str();
	_compatible = compatible;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
LoadGameModel::LoadGameModel() :
	_selectedKingdom( nullptr ),
	_selectedGame( nullptr )
{
	m_proxy = new LoadGameProxy;
	m_proxy->setParent( this );

	_loadGame.SetExecuteFunc( MakeDelegate( this, &LoadGameModel::OnLoadGame ) );

	_savedKingdoms = *new ObservableCollection<SaveItem>();
	_savedGames    = *new ObservableCollection<SaveItem>();

	m_proxy->requestKingdoms();
}

Noesis::ObservableCollection<SaveItem>* LoadGameModel::GetSavedKingdoms() const
{
	return _savedKingdoms;
}

Noesis::ObservableCollection<SaveItem>* LoadGameModel::GetSavedGames() const
{
	return _savedGames;
}

void LoadGameModel::updateSavedKingdoms( const QList<GuiSaveInfo>& kingdoms )
{
	_savedKingdoms->Clear();

	for ( const auto& gsk : kingdoms )
	{
		_savedKingdoms->Add( MakePtr<SaveItem>( gsk.name, gsk.folder, "", gsk.version, gsk.date.toString() ) );
	}

	OnPropertyChanged( "SavedKingdoms" );

	if ( _savedKingdoms->Count() > 0 )
	{
		SetSelectedKingdom( _savedKingdoms->Get( 0 ) );
	}
}

void LoadGameModel::updateSaveGames( const QList<GuiSaveInfo>& saveGames )
{
	_savedGames->Clear();

	for ( const auto& gsi : saveGames )
	{
		_savedGames->Add( MakePtr<SaveItem>( gsi.name, gsi.folder, gsi.dir, gsi.version, gsi.date.toString(), gsi.compatible ) );
	}

	OnPropertyChanged( "SavedGames" );

	if ( _savedGames->Count() > 0 )
	{
		SetSelectedGame( _savedGames->Get( 0 ) );
	}
}

void LoadGameModel::SetSelectedKingdom( SaveItem* item )
{
	if ( _selectedKingdom != item )
	{
		_selectedKingdom = item;
		OnPropertyChanged( "SelectedKingdom" );

		if ( item )
		{
			m_proxy->requestSaveGames( item->_path.Str() );
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
	if ( _selectedGame )
	{
		
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
	NsProp( "Dir", &SaveItem::_dir );
	NsProp( "Version", &SaveItem::_version );
	NsProp( "Date", &SaveItem::_date );
}
