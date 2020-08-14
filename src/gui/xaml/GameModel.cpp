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
#include "GameModel.h"

#include "../../base/config.h"
#include "../../base/db.h"
#include "../../base/global.h"
#include "../../base/selection.h"
#include "../../base/util.h"
#include "../../game/gamemanager.h"
#include "../../game/inventory.h"
#include "../eventconnector.h"
#include "../strings.h"
#include "ProxyGameView.h"

#include <NsApp/Application.h>
#include <NsCore/Log.h>
#include <NsCore/ReflectionImplement.h>
#include <NsCore/ReflectionImplementEnum.h>
#include <NsGui/ObservableCollection.h>
#include <NsGui/UIElement.h>

#include <QDebug>
#include <QImage>
#include <QPixmap>

#include <functional>

using namespace IngnomiaGUI;
using namespace Noesis;
using namespace NoesisApp;

#pragma region DataItems
////////////////////////////////////////////////////////////////////////////////////////////////////
CommandButton::CommandButton( QString name, QString sid )
{
	_name = name.toStdString().c_str();
	_sid  = sid.toStdString().c_str();
}

const char* CommandButton::GetName() const
{
	return _name.Str();
}

const char* CommandButton::GetID() const
{
	return _sid.Str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
BuildButton::BuildButton( QString name, QString sid, QString image )
{
	_name  = name.toStdString().c_str();
	_sid   = sid.toStdString().c_str();
	_image = image.toStdString().c_str();
}

const char* BuildButton::GetName() const
{
	return _name.Str();
}

const char* BuildButton::GetID() const
{
	return _sid.Str();
}

const char* BuildButton::GetImage() const
{
	return _image.Str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
BuildItem::BuildItem( QString name, QString sid, BuildItemType type )
{
	_name = name.toStdString().c_str();
	_sid  = sid;
	_type = type;

	_cmdBuild.SetExecuteFunc( MakeDelegate( this, &BuildItem::onCmdBuild ) );

	_requiredItems = *new Noesis::ObservableCollection<NRequiredItem>();

	switch ( type )
	{
		case BuildItemType::Workshop:
		{
			for ( auto row : DB::selectRows( "Workshops_Components", sid ) )
			{
				if ( !row.value( "ItemID" ).toString().isEmpty() )
				{
					auto item = MakePtr<NRequiredItem>( row.value( "ItemID" ).toString(), row.value( "Amount" ).toInt() );

					_requiredItems->Add( item );
				}
			}

			QStringList mats;
			for ( int i = 0; i < 25; ++i )
				mats.push_back( "None" );

			QPixmap pm = Util::createWorkshopImage( sid, mats );

			std::vector<unsigned char> buffer;

			Util::createBufferForNoesisImage( pm, buffer );

			_bitmapSource = BitmapImage::Create( pm.width(), pm.height(), 96, 96, buffer.data(), pm.width() * 4, BitmapSource::Format::Format_RGBA8 );
		}
		break;
		case BuildItemType::Terrain:
		{
			for ( auto row : DB::selectRows( "Constructions_Components", sid ) )
			{
				_requiredItems->Add( MakePtr<NRequiredItem>( row.value( "ItemID" ).toString(), row.value( "Amount" ).toInt() ) );
			}

			QStringList mats;
			for ( int i = 0; i < 25; ++i )
				mats.push_back( "None" );

			QPixmap pm = Util::createConstructionImage( sid, mats );

			std::vector<unsigned char> buffer;

			Util::createBufferForNoesisImage( pm, buffer );

			_bitmapSource = BitmapImage::Create( pm.width(), pm.height(), 96, 96, buffer.data(), pm.width() * 4, BitmapSource::Format::Format_RGBA8 );
		}
		break;
		case BuildItemType::Item:
		{
			auto rows = DB::selectRows( "Constructions_Components", sid );

			if( rows.size() )
			{
				for ( auto row : rows )
				{
					_requiredItems->Add( MakePtr<NRequiredItem>( row.value( "ItemID" ).toString(), row.value( "Amount" ).toInt() ) );
				}
			}
			else
			{
				_requiredItems->Add( MakePtr<NRequiredItem>( sid, 1 ) );
			}


			QStringList mats;
			for ( int i = 0; i < 25; ++i )
				mats.push_back( "None" );

			QPixmap pm = Util::createItemImage( sid, mats );

			std::vector<unsigned char> buffer;

			Util::createBufferForNoesisImage( pm, buffer );

			_bitmapSource = BitmapImage::Create( pm.width(), pm.height(), 96, 96, buffer.data(), pm.width() * 4, BitmapSource::Format::Format_RGBA8 );
		}
		break;
	}
}

const char* BuildItem::GetName() const
{
	return _name.Str();
}

QString BuildItem::sid() const
{
	return _sid;
}

Noesis::ObservableCollection<NRequiredItem>* BuildItem::requiredItems() const
{
	return _requiredItems;
}

const NoesisApp::DelegateCommand* BuildItem::GetCmdBuild() const
{
	return &_cmdBuild;
}

const ImageSource* BuildItem::getBitmapSource() const
{
	return _bitmapSource;
}

void BuildItem::onCmdBuild( BaseComponent* param )
{
	QStringList mats;
	for ( int i = 0; i < _requiredItems->Count(); ++i )
	{
		auto item = _requiredItems->Get( i );
		auto mat  = item->GetSelectedMaterial();
		mats.append( mat->sid() );
	}

	Selection::getInstance().setMaterials( mats );
	Selection::getInstance().setItemID( _sid );

	switch ( _type )
	{
		case BuildItemType::Workshop:
		{
			Selection::getInstance().setAction( "BuildWorkshop" );
		}
		break;
		case BuildItemType::Terrain:
		{
			QString type = DB::select( "Type", "Constructions", _sid ).toString();

			Selection::getInstance().setAction( "Build" + type );
		}
		break;
		case BuildItemType::Item:
		{
			Selection::getInstance().setAction( "BuildItem" );
		}
		break;
	}
	EventConnector::getInstance().onBuild();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NRequiredItem::NRequiredItem( QString sid, int amount )
{
	_name   = S::s( "$ItemName_" + sid ).toStdString().c_str();
	_sid    = sid;
	_amount = QString::number( amount ).toStdString().c_str();

	_availableMaterials = *new Noesis::ObservableCollection<AvailableMaterial>();

	auto mats = Global::inv().materialCountsForItem( sid );

	_availableMaterials->Add( MakePtr<AvailableMaterial>( "any", mats["any"], _sid ) );
	for ( auto key : mats.keys() )
	{
		if ( key != "any" )
		{
			_availableMaterials->Add( MakePtr<AvailableMaterial>( key, mats[key], _sid ) );
		}
	}

	SetSelectedMaterial( _availableMaterials->Get( 0 ) );
}

const char* NRequiredItem::GetName() const
{
	return _name.Str();
}

const QString NRequiredItem::sid()
{
	return _sid;
}

const char* NRequiredItem::amount() const
{
	return _amount.Str();
}

Noesis::ObservableCollection<AvailableMaterial>* NRequiredItem::availableMaterials() const
{
	return _availableMaterials;
}

void NRequiredItem::SetSelectedMaterial( AvailableMaterial* mat )
{
	if ( _selectedMaterial != mat )
	{
		_selectedMaterial = mat;
		OnPropertyChanged( "SelectedMaterial" );
	}
}

AvailableMaterial* NRequiredItem::GetSelectedMaterial() const
{
	return _selectedMaterial;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AvailableMaterial::AvailableMaterial( QString sid, int amount, QString item )
{
	_name   = ( S::s( "$MaterialName_" + sid ) + " " + S::s( "$ItemName_" + item ) ).toStdString().c_str();
	_sid    = sid.toStdString().c_str();
	_amount = QString::number( amount ).toStdString().c_str();
}

const char* AvailableMaterial::GetName() const
{
	return _name.Str();
}

const char* AvailableMaterial::sid() const
{
	return _sid.Str();
}

const char* AvailableMaterial::amount() const
{
	return _amount.Str();
}
#pragma endregion Buttons

////////////////////////////////////////////////////////////////////////////////////////////////////
GameModel::GameModel()
{
	_cmdButtonCommand.SetExecuteFunc( MakeDelegate( this, &GameModel::OnCmdButtonCommand ) );
	_cmdCategory.SetExecuteFunc( MakeDelegate( this, &GameModel::OnCmdCategory ) );

	_cmdLeftCommandButton.SetExecuteFunc( MakeDelegate( this, &GameModel::CmdLeftCommandButton ) );
	_cmdRightCommandButton.SetExecuteFunc( MakeDelegate( this, &GameModel::CmdRightCommandButton ) );
	_cmdBack.SetExecuteFunc( MakeDelegate( this, &GameModel::OnCmdBack ) );
	_cmdSimple.SetExecuteFunc( MakeDelegate( this, &GameModel::OnCmdSimple ) );

	m_closeWindowCmd.SetExecuteFunc( MakeDelegate( this, &GameModel::onCloseWindowCmd ) );
	m_openGnomeDetailsCmd.SetExecuteFunc( MakeDelegate( this, &GameModel::onOpenGnomeDetailsCmd ) );

	m_messageButtonCmd.SetExecuteFunc( MakeDelegate( this, &GameModel::onMessageButtonCmd ) );

	m_messageHeader = "Message header";
	m_messageText = "Message text Message text Message text Message text Message text Message text Message text Message text Message text Message text Message text Message text Message text";

	m_proxy = new ProxyGameView;
	m_proxy->setParent( this );

	_commandButtons = *new ObservableCollection<CommandButton>();
	_buildButtons   = *new ObservableCollection<BuildButton>();
	_buildItems     = *new ObservableCollection<BuildItem>();

	_year  = "*Year*";
	_day   = "*Day*";
	_time  = "*Time*";
	_level = "*Level*";

	setShowTileInfo( 0 );
}

void GameModel::setTimeAndDate( int minute, int hour, int day, QString season, int year, QString sunStatus )
{
	_year = ( "Year " + QString::number( year ) ).toStdString().c_str();

	QString dayString;

	switch ( day )
	{
		case 1:
			dayString = "1st day of ";
			break;
		case 2:
			dayString = "2nd day of ";
			break;
		case 3:
			dayString = "3rd day of ";
			break;
		default:
			dayString = QString::number( day ) + "th day of ";
			break;
	}
	_day = ( dayString + season ).toStdString().c_str();

	_time = ( QString( "%1" ).arg( hour, 2, 10, QChar( '0' ) ) + ":" + QString( "%1" ).arg( minute, 2, 10, QChar( '0' ) ) ).toStdString().c_str();

	_sun = sunStatus.toStdString().c_str();

	OnPropertyChanged( "Year" );
	OnPropertyChanged( "Day" );
	OnPropertyChanged( "Time" );
	OnPropertyChanged( "Sun" );

	QString path = "Images/clock/";
	if( season == "Spring" ) path += "s";
	else if( season == "Summer" ) path += "u";
	else if( season == "Autumn" ) path += "v";
	else path += "w";

	hour /= 2;
	path += QStringLiteral( "%1" ).arg( hour, 2, 10, QLatin1Char( '0' ) );
	path += ".png";


	_timeImagePath = path.toStdString().c_str();
	OnPropertyChanged( "TimeImagePath" );

}

void GameModel::setViewLevel( int level )
{
	_level = ( "Level: " + QString::number( level ) ).toStdString().c_str();
	OnPropertyChanged( "Level" );
}

void GameModel::updatePause()
{
	OnPropertyChanged( "Paused" );
}

void GameModel::updateGameSpeed()
{
	OnPropertyChanged( "NormalSpeed" );
	OnPropertyChanged( "FastSpeed" );
}

void GameModel::onBuild()
{
	//m_selectedButtons = ButtonSelection::None;
	m_buildSelection = BuildSelection::None;
	//OnPropertyChanged( "ShowCommandButtons" );
	OnPropertyChanged( "ShowCategoryButtons" );
}

void GameModel::onShowTileInfo( unsigned int tileID )
{
	if ( m_shownInfo == ShownInfo::None || m_shownInfo == ShownInfo::TileInfo )
	{
		setShowTileInfo( tileID );
	}
}

void GameModel::onShowStockpileInfo( unsigned int stockpileID )
{
	setShowStockpile( stockpileID );
}

void GameModel::onShowWorkshopInfo( unsigned int workshopID )
{
	setShowWorkshop( workshopID );
}

void GameModel::onShowAgriculture( unsigned id )
{
	setShowAgriculture( id );
}

const char* GameModel::getYear() const
{
	return _year.Str();
}

const char* GameModel::getDay() const
{
	return _day.Str();
}

const char* GameModel::getTime() const
{
	return _time.Str();
}

const char* GameModel::getLevel() const
{
	return _level.Str();
}

const char* GameModel::getSun() const
{
	return _sun.Str();
}

const char* GameModel::getTimeImagePath() const
{
	return _timeImagePath.Str();
}

const char* GameModel::showCommandButtons() const
{
	if ( m_selectedButtons != ButtonSelection::None )
	{
		return "Visible";
	}
	return "Hidden";
}

const char* GameModel::showCategoryButtons() const
{
	if ( m_buildSelection != BuildSelection::None )
	{
		return "Visible";
	}
	return "Hidden";
}

const char* GameModel::getShowTileInfo() const
{
	if ( m_shownInfo == ShownInfo::TileInfo && m_tileInfoID != 0 )
	{
		return "Visible";
	}
	return "Hidden";
}

void GameModel::setShowTileInfo( unsigned int tileID )
{
	if ( m_shownInfo != ShownInfo::TileInfo || m_tileInfoID != tileID )
	{
		if ( tileID == 0 )
		{
			setShownInfo( ShownInfo::None );
		}
		else
		{
			setShownInfo( ShownInfo::TileInfo );
			m_prevTileInfoID = tileID;
		}
		m_tileInfoID = tileID;
		OnPropertyChanged( "ShowTileInfo" );
	}
}

const char* GameModel::getShowStockpile() const
{
	if ( m_shownInfo == ShownInfo::Stockpile && m_stockpileID != 0 )
	{
		return "Visible";
	}
	return "Hidden";
}

void GameModel::setShowStockpile( unsigned int stockpileID )
{
	if ( m_shownInfo != ShownInfo::Stockpile || m_stockpileID != stockpileID )
	{
		if ( stockpileID == 0 )
		{
			setShownInfo( ShownInfo::None );
			m_proxy->closeStockpileWindow();
		}
		else
		{
			setShowTileInfo( 0 );
			setShownInfo( ShownInfo::Stockpile );
		}
		m_stockpileID = stockpileID;
		OnPropertyChanged( "ShowStockpile" );
	}
}

const char* GameModel::getShowAgriculture() const
{
	if ( m_shownInfo == ShownInfo::Agriculture && m_agricultureID != 0 )
	{
		return "Visible";
	}
	return "Hidden";
}

void GameModel::setShowAgriculture( unsigned int id )
{
	if ( m_shownInfo != ShownInfo::Agriculture || m_agricultureID != id )
	{
		if ( id == 0 )
		{
			setShownInfo( ShownInfo::None );
			m_proxy->closeAgricultureWindow();
		}
		else
		{
			setShowTileInfo( 0 );
			setShownInfo( ShownInfo::Agriculture );
		}
		m_agricultureID = id;
		OnPropertyChanged( "ShowAgriculture" );
	}
}

const char* GameModel::getShowWorkshop() const
{
	if ( m_shownInfo == ShownInfo::Workshop && m_workshopID != 0 )
	{
		return "Visible";
	}
	return "Hidden";
}

void GameModel::setShowWorkshop( unsigned int workshopID )
{
	if ( m_shownInfo != ShownInfo::Workshop || m_workshopID != workshopID )
	{
		if ( workshopID == 0 )
		{
			setShownInfo( ShownInfo::None );
			m_proxy->closeWorkshopWindow();
		}
		else
		{
			setShowTileInfo( 0 );
			setShownInfo( ShownInfo::Workshop );
		}
		m_workshopID = workshopID;
		OnPropertyChanged( "ShowWorkshop" );
	}
}

const char* GameModel::getShowPopulation() const
{
	if ( m_shownInfo == ShownInfo::Population )
	{
		return "Visible";
	}
	return "Hidden";
}
	
void GameModel::setShowPopulation( bool value )
{
	if( value )
	{
		if( m_shownInfo != ShownInfo::Population )
		{
			setShownInfo( ShownInfo::Population );
		}
	}
	else
	{
		setShownInfo( ShownInfo::None );
	}
}

	
const char* GameModel::getShowDebug() const
{
	if ( m_shownInfo == ShownInfo::Debug )
	{
		return "Visible";
	}
	return "Hidden";
}

void GameModel::setShowDebug( bool value )
{
	if( value )
	{
		if( m_shownInfo != ShownInfo::Debug )
		{
			setShownInfo( ShownInfo::Debug );
		}
	}
	else
	{
		setShownInfo( ShownInfo::None );
	}
}

const char* GameModel::getShowNeighbors() const
{
	if ( m_shownInfo == ShownInfo::Neighbors )
	{
		return "Visible";
	}
	return "Hidden";
}

void GameModel::setShowNeighbors( bool value )
{
	if( value )
	{
		if( m_shownInfo != ShownInfo::Neighbors )
		{
			setShownInfo( ShownInfo::Neighbors );
		}
	}
	else
	{
		setShownInfo( ShownInfo::None );
	}
}

const char* GameModel::getShowMilitary() const
{
	if ( m_shownInfo == ShownInfo::Military )
	{
		return "Visible";
	}
	return "Hidden";
}

void GameModel::setShowMilitary( bool value )
{
	if( value )
	{
		if( m_shownInfo != ShownInfo::Military )
		{
			setShownInfo( ShownInfo::Military );
		}
	}
	else
	{
		setShownInfo( ShownInfo::None );
	}
}


void GameModel::setGameSpeed( GameSpeed value )
{
	if ( GameManager::getInstance().gameSpeed() != value )
	{
		GameManager::getInstance().setGameSpeed( value );
	}
	GameManager::getInstance().setPaused( false );
	OnPropertyChanged( "Paused" );
	OnPropertyChanged( "NormalSpeed" );
	OnPropertyChanged( "FastSpeed" );
}

bool GameModel::getPaused() const
{
	return GameManager::getInstance().paused();
}

void GameModel::setPaused( bool value )
{
	if( value )
	{
		GameManager::getInstance().setPaused( value );
		OnPropertyChanged( "Paused" );
		OnPropertyChanged( "NormalSpeed" );
		OnPropertyChanged( "FastSpeed" );
	}
}

bool GameModel::getNormalSpeed() const
{
	return ( GameManager::getInstance().gameSpeed() == GameSpeed::Normal && !GameManager::getInstance().paused() );
}

void GameModel::setNormalSpeed( bool value )
{
	if( value )
	{
		setGameSpeed( GameSpeed::Normal );
	}
}

bool GameModel::getFastSpeed() const
{
	return ( GameManager::getInstance().gameSpeed() == GameSpeed::Fast && !GameManager::getInstance().paused() );
}

void GameModel::setFastSpeed( bool value )
{
	if( value )
	{
		setGameSpeed( GameSpeed::Fast );
	}
}

void GameModel::OnCmdCategory( BaseComponent* param )
{
	setCategory( param->ToString().Str() );
}

void GameModel::setCategory( const char* cats )
{
	QString cat( cats );
	_buildItems->Clear();

	switch ( m_buildSelection )
	{
		case BuildSelection::Wall:
		{
			auto rows = DB::selectRows( "Constructions", "Category", cat );
			for ( auto row : rows )
			{
				if ( row.value( "Type" ).toString() == "Wall" )
				{
					_buildItems->Add( MakePtr<BuildItem>( S::s( "$ConstructionName_" + row.value( "ID" ).toString() ), row.value( "ID" ).toString(), BuildItemType::Terrain ) );
				}
			}
			break;
		}
		case BuildSelection::Floor:
		{
			auto rows = DB::selectRows( "Constructions", "Category", cat );
			for ( auto row : rows )
			{
				if ( row.value( "Type" ).toString() == "Floor" )
				{
					_buildItems->Add( MakePtr<BuildItem>( S::s( "$ConstructionName_" + row.value( "ID" ).toString() ), row.value( "ID" ).toString(), BuildItemType::Terrain ) );
				}
			}
			break;
		}
		case BuildSelection::Stairs:
		{
			auto rows = DB::selectRows( "Constructions", "Category", cat );
			for ( auto row : rows )
			{
				if ( row.value( "Type" ).toString() == "Stairs" )
				{
					_buildItems->Add( MakePtr<BuildItem>( S::s( "$ConstructionName_" + row.value( "ID" ).toString() ), row.value( "ID" ).toString(), BuildItemType::Terrain ) );
				}
			}
			break;
		}
		case BuildSelection::Ramps:
		{
			auto rows = DB::selectRows( "Constructions", "Category", cat );
			for ( auto row : rows )
			{
				if ( row.value( "Type" ).toString() == "Ramp" )
				{
					_buildItems->Add( MakePtr<BuildItem>( S::s( "$ConstructionName_" + row.value( "ID" ).toString() ), row.value( "ID" ).toString(), BuildItemType::Terrain ) );
				}
			}
			break;
		}
		case BuildSelection::Fence:
		{
			auto rows = DB::selectRows( "Constructions", "Category", cat );
			for ( auto row : rows )
			{
				if ( row.value( "Type" ).toString() == "Fence" )
				{
					_buildItems->Add( MakePtr<BuildItem>( S::s( "$ConstructionName_" + row.value( "ID" ).toString() ), row.value( "ID" ).toString(), BuildItemType::Terrain ) );
				}
			}
			break;
		}
		case BuildSelection::Workshop:
		{
			auto rows = DB::selectRows( "Workshops", "Tab", cat );

			for ( auto row : rows )
			{
				_buildItems->Add( MakePtr<BuildItem>( S::s( "$WorkshopName_" + row.value( "ID" ).toString() ), row.value( "ID" ).toString(), BuildItemType::Workshop ) );
			}
			break;
		}
		case BuildSelection::Containers:
		{
			auto rows = DB::selectRows( "Containers" );
			for ( auto row : rows )
			{
				if ( row.value( "Buildable" ).toBool() )
				{
					_buildItems->Add( MakePtr<BuildItem>( S::s( "$ItemName_" + row.value( "ID" ).toString() ), row.value( "ID" ).toString(), BuildItemType::Item ) );
				}
			}
			break;
		}
		case BuildSelection::Furniture:
		{
			auto rows = DB::selectRows( "Items", "Category", "Furniture" );
			for ( auto row : rows )
			{
				if ( row.value( "ItemGroup" ).toString() == cat )
				{
					_buildItems->Add( MakePtr<BuildItem>( S::s( "$ItemName_" + row.value( "ID" ).toString() ), row.value( "ID" ).toString(), BuildItemType::Item ) );
				}
			}
			break;
		}
		case BuildSelection::Utility:
		{
			qDebug() << cat;
			auto rows = DB::selectRows( "Items", "Category", "Utility" );
			for ( auto row : rows )
			{
				if ( row.value( "ItemGroup" ).toString() == cat )
				{
					_buildItems->Add( MakePtr<BuildItem>( S::s( "$ItemName_" + row.value( "ID" ).toString() ), row.value( "ID" ).toString(), BuildItemType::Item ) );
				}
			}
			break;
		}
	}

	OnPropertyChanged( "BuildItems" );
}

void GameModel::OnCmdButtonCommand( BaseComponent* param )
{
	QString cmd( param->ToString().Str() );
	qDebug() << cmd;

	switch ( m_selectedButtons )
	{
		case ButtonSelection::Build:
			if ( cmd == "Deconstruct" )
			{
				m_buildSelection = BuildSelection::None;
				OnPropertyChanged( "BuildButtons" );
				OnPropertyChanged( "ShowCategoryButtons" );
				OnCmdSimple( param );
				return;
			}
			break;
		case ButtonSelection::Mine:
		case ButtonSelection::Agriculture:
		case ButtonSelection::Designation:
		case ButtonSelection::Job:
		case ButtonSelection::Magic:
			OnCmdSimple( param );
			return;
		case ButtonSelection::None:
		default:
			break;
	}

	BuildSelection bs = BuildSelection::None;
	if ( cmd == "Workshop" )
		bs = BuildSelection::Workshop;
	else if ( cmd == "Wall" )
		bs = BuildSelection::Wall;
	else if ( cmd == "Floor" )
		bs = BuildSelection::Floor;
	else if ( cmd == "Stairs" )
		bs = BuildSelection::Stairs;
	else if ( cmd == "Containers" )
		bs = BuildSelection::Containers;
	else if ( cmd == "Ramp" )
		bs = BuildSelection::Ramps;
	else if ( cmd == "Fence" )
		bs = BuildSelection::Fence;
	else if ( cmd == "Furniture" )
		bs = BuildSelection::Furniture;
	else if ( cmd == "Utility" )
		bs = BuildSelection::Utility;

	if ( bs == m_buildSelection )
	{
		m_buildSelection = BuildSelection::None;
	}
	else
	{
		m_buildSelection = bs;
	}

	_buildButtons->Clear();

	switch ( m_buildSelection )
	{
		case BuildSelection::Wall:
		case BuildSelection::Floor:
		case BuildSelection::Stairs:
		case BuildSelection::Ramps:
			_buildButtons->Add( MakePtr<BuildButton>( "Wood", "Wood", "buttons/wood.png" ) );
			_buildButtons->Add( MakePtr<BuildButton>( "Soil", "Soil", "buttons/build-all.png" ) );
			_buildButtons->Add( MakePtr<BuildButton>( "Stone", "Stone", "buttons/stone.png" ) );
			_buildButtons->Add( MakePtr<BuildButton>( "Metal", "Metal", "buttons/metal.png" ) );
			_buildButtons->Add( MakePtr<BuildButton>( "Other", "Other", "buttons/build-all.png" ) );
			setCategory( "Wood" );
			break;
		case BuildSelection::Fence:
			_buildButtons->Add( MakePtr<BuildButton>( "Wood", "Wood", "buttons/wood.png" ) );
			_buildButtons->Add( MakePtr<BuildButton>( "Stone", "Stone", "buttons/stone.png" ) );
			setCategory( "Wood" );
			break;

		case BuildSelection::Workshop:
			_buildButtons->Add( MakePtr<BuildButton>( "Wood", "Wood", "buttons/woodwork.png" ) );
			_buildButtons->Add( MakePtr<BuildButton>( "Stone", "Stone", "buttons/stonework.png" ) );
			_buildButtons->Add( MakePtr<BuildButton>( "Metal", "Metal", "buttons/metalwork.png" ) );
			_buildButtons->Add( MakePtr<BuildButton>( "Food", "Food", "buttons/food.png" ) );
			_buildButtons->Add( MakePtr<BuildButton>( "Crafts", "Craft", "buttons/crafts.png" ) );
			_buildButtons->Add( MakePtr<BuildButton>( "Mechanics", "Mechanics", "buttons/mechanics.png" ) );
			_buildButtons->Add( MakePtr<BuildButton>( "Misc", "Misc", "buttons/crafts.png" ) );
			setCategory( "Wood" );
			break;
		case BuildSelection::Containers:
			_buildButtons->Add( MakePtr<BuildButton>( "All", "Containers", "buttons/stockpile.png" ) );
			setCategory( "Containers" );
			break;
		case BuildSelection::Furniture:
			_buildButtons->Add( MakePtr<BuildButton>( "Chairs", "Chairs", "buttons/chair.png" ) );
			_buildButtons->Add( MakePtr<BuildButton>( "Tables", "Tables", "buttons/table.png" ) );
			_buildButtons->Add( MakePtr<BuildButton>( "Beds", "Beds", "buttons/bed.png" ) );
			_buildButtons->Add( MakePtr<BuildButton>( "Cabinets", "Cabinets", "buttons/cabinet.png" ) );
			_buildButtons->Add( MakePtr<BuildButton>( "Misc", "Misc", "buttons/woodwork.png" ) );
			setCategory( "Chairs" );
			break;
		case BuildSelection::Utility:
			_buildButtons->Add( MakePtr<BuildButton>( "Doors", "Doors", "buttons/door.png" ) );
			_buildButtons->Add( MakePtr<BuildButton>( "Lights", "Lights", "buttons/light.png" ) );
			_buildButtons->Add( MakePtr<BuildButton>( "Farm", "Farm", "buttons/farm.png" ) );
			_buildButtons->Add( MakePtr<BuildButton>( "Mechanism", "Mechanism", "buttons/mechanism.png" ) );
			_buildButtons->Add( MakePtr<BuildButton>( "Hydraulics", "Hydraulics", "buttons/hydro.png" ) );
			_buildButtons->Add( MakePtr<BuildButton>( "Other", "Other", "buttons/woodwork.png" ) );
			setCategory( "Doors" );
			break;
	}

	OnPropertyChanged( "BuildButtons" );
	OnPropertyChanged( "ShowCategoryButtons" );
}

Noesis::ObservableCollection<CommandButton>* GameModel::GetCommandButtons() const
{
	return _commandButtons;
}

Noesis::ObservableCollection<BuildButton>* GameModel::GetBuildButtons() const
{
	return _buildButtons;
}

Noesis::ObservableCollection<BuildItem>* GameModel::GetBuildItems() const
{
	return _buildItems;
}

const NoesisApp::DelegateCommand* GameModel::GetCmdButtonCommand() const
{
	return &_cmdButtonCommand;
}

const NoesisApp::DelegateCommand* GameModel::GetCmdCategory() const
{
	return &_cmdCategory;
}

const NoesisApp::DelegateCommand* GameModel::GetCmdBack() const
{
	return &_cmdBack;
}

const NoesisApp::DelegateCommand* GameModel::GetSimpleCommand() const
{
	return &_cmdSimple;
}

void GameModel::OnCmdBack( BaseComponent* param )
{
	if ( m_shownInfo != ShownInfo::None && m_shownInfo != ShownInfo::TileInfo )
	{
		setShowStockpile( 0 );
		setShowWorkshop( 0 );
		setShowAgriculture( 0 );
		setShowTileInfo( m_prevTileInfoID );
		setShowPopulation( false );
		return;
	}
	else if ( m_shownInfo == ShownInfo::TileInfo )
	{
		setShowStockpile( 0 );
		setShowWorkshop( 0 );
		setShowAgriculture( 0 );
		setShowTileInfo( 0 );
		return;
	}

	if ( m_buildSelection != BuildSelection::None )
	{
		m_buildSelection = BuildSelection::None;
		OnPropertyChanged( "ShowCategoryButtons" );
		return;
	}
	if ( m_selectedButtons != ButtonSelection::None )
	{
		m_selectedButtons = ButtonSelection::None;
		OnPropertyChanged( "ShowCommandButtons" );
		return;
	}

	EventConnector::getInstance().onKeyEsc();
}

void GameModel::CmdRightCommandButton( BaseComponent* param )
{
	QString cmd( param->ToString().Str() );
	
	if ( cmd == "Population" )
	{
		setShowPopulation( true );
		m_proxy->requestPopulationUpdate();
	}
	else if ( cmd == "Kingdom" )
	{
	}
	else if ( cmd == "Military" )
	{
		setShowMilitary( true );
		m_proxy->requestMilitaryUpdate();
	}
	else if ( cmd == "Missions" )
	{
		setShowNeighbors( true );
		m_proxy->requestNeighborsUpdate();
		m_proxy->requestMissionsUpdate();
	}
	else if ( cmd == "Debug" )
	{
		setShowDebug( true );
	}
}

void GameModel::CmdLeftCommandButton( BaseComponent* param )
{
	QString cmd( param->ToString().Str() );
	ButtonSelection bs = ButtonSelection::None;
	if ( cmd == "Build" )
		bs = ButtonSelection::Build;
	else if ( cmd == "Mine" )
		bs = ButtonSelection::Mine;
	else if ( cmd == "Agri" )
		bs = ButtonSelection::Agriculture;
	else if ( cmd == "Designation" )
		bs = ButtonSelection::Designation;
	else if ( cmd == "Job" )
		bs = ButtonSelection::Job;
	else if ( cmd == "Magic" )
		bs = ButtonSelection::Magic;

	m_buildSelection = BuildSelection::None;

	if ( bs == m_selectedButtons )
	{
		m_selectedButtons = ButtonSelection::None;
	}
	else
	{
		m_selectedButtons = bs;
	}

	_commandButtons->Clear();
	switch ( m_selectedButtons )
	{
		case ButtonSelection::Build:
			_commandButtons->Add( MakePtr<CommandButton>( "Workshop", "Workshop" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Wall", "Wall" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Floor", "Floor" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Stairs", "Stairs" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Ramp", "Ramp" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Fence", "Fence" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Containers", "Containers" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Furniture", "Furniture" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Utilities", "Utility" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Deconstruct", "Deconstruct" ) );
			break;
		case ButtonSelection::Mine:
			_commandButtons->Add( MakePtr<CommandButton>( "Mine", "Mine" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Explorative Mine", "ExplorativeMine" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Remove floor", "RemoveFloor" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Stairs down", "DigStairsDown" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Stairs up", "MineStairsUp" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Ramp", "DigRampDown" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Hole", "DigHole" ) );
			break;
		case ButtonSelection::Agriculture:
			_commandButtons->Add( MakePtr<CommandButton>( "Cut tree", "FellTree" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Plant tree", "" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Harvest tree", "HarvestTree" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Forage", "Forage" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Remove plant", "RemovePlant" ) );
			break;
		case ButtonSelection::Designation:
			_commandButtons->Add( MakePtr<CommandButton>( "Farm", "CreateFarm" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Grove", "CreateGrove" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Pasture", "CreatePasture" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Room", "CreateRoom" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Stockpile", "CreateStockpile" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Forbidden", "CreateNoPass" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Guard", "CreateGuardArea" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Remove", "RemoveDesignation" ) );
			break;
		case ButtonSelection::Job:
			_commandButtons->Add( MakePtr<CommandButton>( "Suspend", "SuspendJob" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Resume", "ResumeJob" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Cancel", "CancelJob" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Lower priority", "LowerPrio" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Raise priority", "RaisePrio" ) );
			break;
		case ButtonSelection::Magic:
			break;
		case ButtonSelection::None:
		default:
			break;
	}

	OnPropertyChanged( "ShowCommandButtons" );
	OnPropertyChanged( "ShowCategoryButtons" );
}

const NoesisApp::DelegateCommand* GameModel::GetCmdLeftCommandButton() const
{
	return &_cmdLeftCommandButton;
}

const NoesisApp::DelegateCommand* GameModel::GetCmdRightCommandButton() const
{
	return &_cmdRightCommandButton;
}


void GameModel::OnCmdSimple( BaseComponent* param )
{
	Selection::getInstance().setAction( param->ToString().Str() );
}

void GameModel::onCloseWindowCmd( BaseComponent* param )
{
	setShownInfo( ShownInfo::None );
}

void GameModel::onOpenGnomeDetailsCmd( BaseComponent* param )
{
	if( param )
	{
		QString qParam( param->ToString().Str() );
		m_proxy->requestCreatureUpdate( qParam.toUInt() );
		setShownInfo( ShownInfo::CreatureInfo );
	}
}

const char* GameModel::getShowCreatureInfo() const
{
	if ( m_shownInfo == ShownInfo::CreatureInfo )
	{
		return "Visible";
	}
	return "Hidden";
}

void GameModel::setShownInfo( ShownInfo info )
{
	m_shownInfo = info;

	if( m_shownInfo != ShownInfo::CreatureInfo )
	{
		m_proxy->requestCreatureUpdate( 0 );
	}

	OnPropertyChanged( "ShowPopulation" );
	OnPropertyChanged( "ShowWorkshop" );
	OnPropertyChanged( "ShowAgriculture" );
	OnPropertyChanged( "ShowStockpile" );
	OnPropertyChanged( "ShowTileInfo" );
	OnPropertyChanged( "ShowCreatureInfo" );
	OnPropertyChanged( "ShowDebug" );
	OnPropertyChanged( "ShowNeighbors" );
	OnPropertyChanged( "ShowMilitary" );
	
	OnPropertyChanged( "ShowMessage" );
}


const char* GameModel::getShowMessage() const
{
	if( m_showMessageWindow && m_shownInfo == ShownInfo::None )
	{
		return "Visible";
	}
	return "Hidden";
}

const char* GameModel::getShowMessageButtonOk() const
{
	if( m_showMessageButtonOk )
	{
		return "Visible";
	}
	return "Hidden";
}
	
const char* GameModel::getShowMessageButtonYesNo() const
{
	if( m_showMessageButtonYesNo )
	{
		return "Visible";
	}
	return "Hidden";
}

const char* GameModel::getMessageHeader() const
{
	return m_messageHeader.Str();
}

const char* GameModel::getMessageText() const
{
	return m_messageText.Str();
}

void GameModel::onMessageButtonCmd( BaseComponent* param )
{
	if( param )
	{
		QString qParam = param->ToString().Str();
		if( qParam != "ok" )
		{
			m_proxy->sendEventAnswer( m_messageID, qParam == "yes" );
		}
		m_showMessageWindow = false;
		if( !m_messageQueue.isEmpty() )
		{
			auto gme = m_messageQueue.takeFirst();
			eventMessage( gme.id, gme.title, gme.msg, gme.pause, gme.yesno );
		}
		else
		{
			OnPropertyChanged( "ShowMessage" );
		}
	}
}

void GameModel::eventMessage( unsigned int id, QString title, QString msg, bool pause, bool yesno )
{
	if( m_showMessageWindow )
	{
		m_messageQueue.append( { id, title, msg, pause, yesno } );
	}
	else
	{
		m_messageID = id;
		m_messageHeader = title.toStdString().c_str();
		m_messageText = msg.toStdString().c_str();
		m_showMessageWindow = true;

		m_showMessageButtonOk = !yesno;
		m_showMessageButtonYesNo = yesno;

		if( pause )
		{
			setPaused( true );
		}
		
		OnPropertyChanged( "ShowMessageButtonOk" );
		OnPropertyChanged( "ShowMessageButtonYesNo" );
		OnPropertyChanged( "MessageText" );
		OnPropertyChanged( "MessageHeader" );

		setShownInfo( ShownInfo::None );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION( GameModel, "IngnomiaGUI.GameModel" )
{
	NsProp( "Paused", &GameModel::getPaused, &GameModel::setPaused );
	NsProp( "NormalSpeed", &GameModel::getNormalSpeed, &GameModel::setNormalSpeed );
	NsProp( "FastSpeed", &GameModel::getFastSpeed, &GameModel::setFastSpeed );

	NsProp( "Year", &GameModel::getYear );
	NsProp( "Day", &GameModel::getDay );
	NsProp( "Time", &GameModel::getTime );
	NsProp( "Level", &GameModel::getLevel );
	NsProp( "Sun", &GameModel::getSun );
	NsProp( "TimeImagePath", &GameModel::getTimeImagePath );

	NsProp( "CmdButtonCommand", &GameModel::GetCmdButtonCommand );
	NsProp( "CmdCategory", &GameModel::GetCmdCategory );

	NsProp( "CmdBack", &GameModel::GetCmdBack );
	NsProp( "CmdSimple", &GameModel::GetSimpleCommand );

	NsProp( "CmdLeftCommandButton", &GameModel::GetCmdLeftCommandButton );
	NsProp( "CmdRightCommandButton", &GameModel::GetCmdRightCommandButton );

	NsProp( "CommandButtons", &GameModel::GetCommandButtons );
	NsProp( "BuildButtons", &GameModel::GetBuildButtons );
	NsProp( "BuildItems", &GameModel::GetBuildItems );

	NsProp( "ShowCommandButtons", &GameModel::showCommandButtons );
	NsProp( "ShowCategoryButtons", &GameModel::showCategoryButtons );
	NsProp( "ShowTileInfo", &GameModel::getShowTileInfo );
	NsProp( "ShowStockpile", &GameModel::getShowStockpile );
	NsProp( "ShowWorkshop", &GameModel::getShowWorkshop );
	NsProp( "ShowAgriculture", &GameModel::getShowAgriculture );
	NsProp( "ShowPopulation", &GameModel::getShowPopulation );
	NsProp( "ShowDebug", &GameModel::getShowDebug );
	NsProp( "ShowCreatureInfo", &GameModel::getShowCreatureInfo );
	NsProp( "ShowNeighbors", &GameModel::getShowNeighbors );
	NsProp( "ShowMilitary", &GameModel::getShowMilitary );
	
	NsProp( "ShowMessage", &GameModel::getShowMessage );
	NsProp( "ShowMessageButtonOk", &GameModel::getShowMessageButtonOk );
	NsProp( "ShowMessageButtonYesNo", &GameModel::getShowMessageButtonYesNo );
	NsProp( "MessageHeader", &GameModel::getMessageHeader );
	NsProp( "MessageText", &GameModel::getMessageText );
	NsProp( "MessageCmd", &GameModel::GetMessageButtonCmd );

	NsProp( "CloseWindowCmd", &GameModel::GetCloseWindowCmd );
	NsProp( "OpenCreatureDetailsCmd", &GameModel::GetOpenGnomeDetailsCmd );
}

NS_IMPLEMENT_REFLECTION( CommandButton )
{
	NsProp( "Name", &BuildButton::GetName );
	NsProp( "ID", &BuildButton::GetID );
}

NS_IMPLEMENT_REFLECTION( BuildButton )
{
	NsProp( "Name", &BuildButton::GetName );
	NsProp( "ID", &BuildButton::GetID );
	NsProp( "Image", &BuildButton::GetImage );
}

NS_IMPLEMENT_REFLECTION( BuildItem )
{
	NsProp( "Name", &BuildItem::GetName );
	NsProp( "RequiredItems", &BuildItem::requiredItems );
	NsProp( "Build", &BuildItem::GetCmdBuild );
	NsProp( "Image", &BuildItem::getBitmapSource );
}

NS_IMPLEMENT_REFLECTION( NRequiredItem )
{
	NsProp( "Name", &NRequiredItem::GetName );
	NsProp( "Amount", &NRequiredItem::amount );
	NsProp( "Materials", &NRequiredItem::availableMaterials );
	NsProp( "SelectedMaterial", &NRequiredItem::GetSelectedMaterial, &NRequiredItem::SetSelectedMaterial );
}

NS_IMPLEMENT_REFLECTION( AvailableMaterial )
{
	NsProp( "Name", &AvailableMaterial::GetName );
	NsProp( "Amount", &AvailableMaterial::amount );
}