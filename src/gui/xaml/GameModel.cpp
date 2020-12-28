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
#include "ProxyGameView.h"

#include "../strings.h"

#include <NsApp/Application.h>
#include <NsCore/Log.h>
#include <NsCore/ReflectionImplement.h>
#include <NsCore/ReflectionImplementEnum.h>
#include <NsGui/ObservableCollection.h>
#include <NsGui/UIElement.h>

#include <QDebug>
#include <QImage>
#include <QPixmap>

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
BuildItem::BuildItem( const GuiBuildItem& gbi, ProxyGameView* proxy )
{
	m_name = gbi.name.toStdString().c_str();
	m_sid  = gbi.id;
	m_type = gbi.biType;
	m_proxy = proxy;

	m_cmdBuild.SetExecuteFunc( MakeDelegate( this, &BuildItem::onCmdBuild ) );

	m_requiredItems = *new Noesis::ObservableCollection<NRequiredItem>();
	m_bitmapSource = BitmapImage::Create( gbi.iconWidth, gbi.iconHeight, 96, 96, gbi.buffer.data(), gbi.iconWidth * 4, BitmapSource::Format::Format_RGBA8 );

	for( auto ri : gbi.requiredItems )
	{
		auto item = MakePtr<NRequiredItem>( ri.itemID, ri.amount, ri.availableMats );
		m_requiredItems->Add( item );
	}
}

const char* BuildItem::GetName() const
{
	return m_name.Str();
}

QString BuildItem::sid() const
{
	return m_sid;
}

Noesis::ObservableCollection<NRequiredItem>* BuildItem::requiredItems() const
{
	return m_requiredItems;
}

const NoesisApp::DelegateCommand* BuildItem::GetCmdBuild() const
{
	return &m_cmdBuild;
}

const char* BuildItem::GetShowReplaceButton() const
{
	if( m_type == BuildItemType::Terrain )
	{
		if( m_sid.endsWith( "Wall" ) || m_sid.endsWith( "Floor" ) || m_sid.startsWith( "FancyFloor" ) ||  m_sid.startsWith( "FancyWall" ) )
		{
			return "Visible";
		}
	}
	return "Hidden";
}

const char* BuildItem::GetShowFillHoleButton() const
{
	if( m_type == BuildItemType::Terrain )
	{
		if( m_sid.endsWith( "Wall" ) || m_sid.startsWith( "FancyWall" ) )
		{
			return "Visible";
		}
	}
	return "Hidden";
}

const ImageSource* BuildItem::getBitmapSource() const
{
	return m_bitmapSource;
}

void BuildItem::onCmdBuild( BaseComponent* param )
{
	QStringList mats;
	for ( int i = 0; i < m_requiredItems->Count(); ++i )
	{
		auto item = m_requiredItems->Get( i );
		auto mat  = item->GetSelectedMaterial();
		mats.append( mat->sid() );
	}
	QString qParam;
	if( param )
	{
		qParam = param->ToString().Str();
	}

	m_proxy->requestCmdBuild( m_type, qParam, m_sid, mats );
}


////////////////////////////////////////////////////////////////////////////////////////////////////
NRequiredItem::NRequiredItem( QString sid, int amount, const QList<QPair<QString, int>>& mats )
{
	_name   = S::s( "$ItemName_" + sid ).toStdString().c_str();
	_sid    = sid;
	_amount = QString::number( amount ).toStdString().c_str();

	_availableMaterials = *new Noesis::ObservableCollection<AvailableMaterial>();

	for( auto mat : mats )
	{
		_availableMaterials->Add( MakePtr<AvailableMaterial>( mat.first, mat.second, _sid ) );
	}

	SetSelectedMaterial( _availableMaterials->Get( 0 ) );
}

NRequiredItem::NRequiredItem( QString sid, int amount )
{
	_name   = S::s( "$ItemName_" + sid ).toStdString().c_str();
	_sid    = sid;
	_amount = QString::number( amount ).toStdString().c_str();
	_availableMaterials = *new Noesis::ObservableCollection<AvailableMaterial>();
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
	m_watchList		= *new Noesis::ObservableCollection<GameItem>();

	m_year  = "*Year*";
	m_day   = "*Day*";
	m_time  = "*Time*";
	m_level = "*Level*";

	m_kingdomName = "*Kingdom Name*";

	setShowTileInfo( 0 );
}

void GameModel::updateKingdomInfo( QString name, QString info1, QString info2, QString info3 )
{
	m_kingdomName = name.toStdString().c_str();
	m_kingdomInfo1 = info1.toStdString().c_str();
	m_kingdomInfo2 = info2.toStdString().c_str();
	m_kingdomInfo3 = info3.toStdString().c_str();
	OnPropertyChanged( "KingdomName" );
	OnPropertyChanged( "KingdomInfo1" );
	OnPropertyChanged( "KingdomInfo2" );
	OnPropertyChanged( "KingdomInfo3" );
}

void GameModel::setTimeAndDate( int minute, int hour, int day, QString season, int year, QString sunStatus )
{
	{
		const auto tmp = ( "Year " + QString::number( year ) ).toStdString();
		if ( tmp.compare( m_year.Str() ) != 0 )
		{
			m_year = tmp.c_str();
			OnPropertyChanged( "Year" );
		}
	}

	{
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
		const auto tmp = ( dayString + season ).toStdString();
		if ( tmp.compare( m_year.Str() ) != 0 )
		{
			m_day = tmp.c_str();
			OnPropertyChanged( "Day" );
		}
	}

	{
		m_time = ( QString( "%1" ).arg( hour, 2, 10, QChar( '0' ) ) + ":" + QString( "%1" ).arg( minute, 2, 10, QChar( '0' ) ) ).toStdString().c_str();
		OnPropertyChanged( "Time" );
	}

	{
		const auto tmp = sunStatus.toStdString();
		if ( tmp.compare( m_sun.Str() ) != 0 )
		{
			m_sun = tmp.c_str();
			OnPropertyChanged( "Sun" );
		}
	}

	{
		QString path = "Images/clock/";
		if ( season == "Spring" )
			path += "s";
		else if ( season == "Summer" )
			path += "u";
		else if ( season == "Autumn" )
			path += "v";
		else
			path += "w";

		hour /= 2;
		path += QStringLiteral( "%1" ).arg( hour, 2, 10, QLatin1Char( '0' ) );
		path += ".png";

		const auto tmp = path.toStdString();
		if ( tmp.compare( m_timeImagePath.Str() ) != 0 )
		{
			m_timeImagePath = tmp.c_str();
			OnPropertyChanged( "TimeImagePath" );
		}
	}

}

void GameModel::setViewLevel( int level )
{
	m_level = ( "Level: " + QString::number( level ) ).toStdString().c_str();
	OnPropertyChanged( "Level" );
}

void GameModel::updatePause( bool paused )
{
	setPaused( paused );
}

void GameModel::updateGameSpeed( GameSpeed speed )
{
	setGameSpeed( speed );
}

void GameModel::updateRenderOptions( bool designation, bool jobs, bool walls, bool axles )
{
	setRenderDesignations( designation );
	setRenderJobs( jobs );
	setRenderWalls( walls );
	setRenderAxles( axles );
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
	return m_year.Str();
}

const char* GameModel::getDay() const
{
	return m_day.Str();
}

const char* GameModel::getTime() const
{
	return m_time.Str();
}

const char* GameModel::getLevel() const
{
	return m_level.Str();
}

const char* GameModel::getSun() const
{
	return m_sun.Str();
}
	
const char* GameModel::getKingdomName() const
{
	return m_kingdomName.Str();
}

const char* GameModel::getKingdomInfo1() const
{
	return m_kingdomInfo1.Str();
}

const char* GameModel::getKingdomInfo2() const
{
	return m_kingdomInfo2.Str();
}

const char* GameModel::getKingdomInfo3() const
{
	return m_kingdomInfo3.Str();
}

const char* GameModel::getTimeImagePath() const
{
	return m_timeImagePath.Str();
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

const char* GameModel::getShowInventory() const
{
	if ( m_shownInfo == ShownInfo::Inventory )
	{
		return "Visible";
	}
	return "Hidden";
}

void GameModel::setShowInventory( bool value )
{
	if( value )
	{
		if( m_shownInfo != ShownInfo::Inventory )
		{
			setShownInfo( ShownInfo::Inventory );
		}
	}
	else
	{
		setShownInfo( ShownInfo::None );
	}
}



void GameModel::setGameSpeed( GameSpeed value )
{
	if( m_showMessageWindow )
	{
		m_proxy->setPaused( true );
	}
	else
	{
		if ( m_gameSpeed != value )
		{
			m_proxy->setGameSpeed( value );
		}
		m_proxy->setPaused( false );
	}
	m_gameSpeed = value;

	OnPropertyChanged( "Paused" );
	OnPropertyChanged( "NormalSpeed" );
	OnPropertyChanged( "FastSpeed" );
}

bool GameModel::getPaused() const
{
	return m_paused;
}

void GameModel::setPaused( bool value )
{
	if( m_showMessageWindow && !value )
	{
		return;
	}
	m_paused = value;
	m_proxy->setPaused( value );
	OnPropertyChanged( "Paused" );
	OnPropertyChanged( "NormalSpeed" );
	OnPropertyChanged( "FastSpeed" );
}

bool GameModel::getNormalSpeed() const
{
	return ( m_gameSpeed == GameSpeed::Normal && !m_paused );
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
	return ( m_gameSpeed == GameSpeed::Fast && !m_paused );
}

void GameModel::setFastSpeed( bool value )
{
	if( value )
	{
		setGameSpeed( GameSpeed::Fast );
	}
}

bool GameModel::getRenderDesignations() const
{
	return m_renderDesignations;
}
void GameModel::setRenderDesignations( bool value )
{
	if( m_renderDesignations != value )
	{
		m_renderDesignations = value;
		m_proxy->setRenderOptions( m_renderDesignations, m_renderJobs, m_wallsLowered, m_renderAxles );
		OnPropertyChanged( "RenderDesignations" );
	}
}
bool GameModel::getRenderJobs() const
{
	return m_renderJobs;
}
void GameModel::setRenderJobs( bool value )
{
	if( m_renderJobs != value )
	{
		m_renderJobs = value;
		m_proxy->setRenderOptions( m_renderDesignations, m_renderJobs, m_wallsLowered, m_renderAxles );
		OnPropertyChanged( "RenderJobs" );
	}
}
bool GameModel::getRenderWalls() const
{
	return m_wallsLowered;
}
void GameModel::setRenderWalls( bool value )
{
	if( m_wallsLowered != value )
	{
		m_wallsLowered = value;
		m_proxy->setRenderOptions( m_renderDesignations, m_renderJobs, m_wallsLowered, m_renderAxles );
		OnPropertyChanged( "RenderWalls" );
	}
}
bool GameModel::getRenderAxles() const
{
	return m_renderAxles;
}
void GameModel::setRenderAxles( bool value )
{
	if( m_renderAxles != value )
	{
		m_renderAxles = value;
		m_proxy->setRenderOptions( m_renderDesignations, m_renderJobs, m_wallsLowered, m_renderAxles );
		OnPropertyChanged( "RenderWalls" );
	}
}

void GameModel::OnCmdCategory( BaseComponent* param )
{
	setCategory( param->ToString().Str() );
}

void GameModel::setCategory( const char* cats )
{
	QString cat( cats );
	m_proxy->requestBuildItems( m_buildSelection, cat );
}

void GameModel::updateBuildItems( const QList<GuiBuildItem>& items )
{
	_buildItems->Clear();

	for( const auto& item : items )
	{
		_buildItems->Add( MakePtr<BuildItem>( item, m_proxy ) );
	}

	OnPropertyChanged( "BuildItems" );
}

void GameModel::OnCmdButtonCommand( BaseComponent* param )
{
	QString cmd( param->ToString().Str() );

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

	m_proxy->propagateEscape();
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
	else if ( cmd == "Inventory" )
	{
		setShowInventory( true );
		m_proxy->requestInventoryUpdate();
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
			_commandButtons->Add( MakePtr<CommandButton>( "Stockpile", "CreateStockpile" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Farm", "CreateFarm" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Grove", "CreateGrove" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Pasture", "CreatePasture" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Personal Room", "CreateRoom" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Dormitory", "CreateDorm" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Dining Hall", "CreateDining" ) );
			_commandButtons->Add( MakePtr<CommandButton>( "Hospital", "CreateHospital" ) );
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
	if( param )
	{
		m_proxy->setSelectionAction( param->ToString().Str() );
	}
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
	OnPropertyChanged( "ShowInventory" );
	
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
	}
	OnPropertyChanged( "ShowMessage" );
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

const char* GameModel::getShowSelection() const
{
	if ( m_showSelection )
	{
		return "Visible";
	}
	return "Hidden";
}
	
void GameModel::setShowSelection( bool value )
{
	if( m_showSelection != value )
	{
		m_showSelection = value;
		OnPropertyChanged( "ShowSelection" );
	}
}

void GameModel::updateWatchList( const QList<GuiWatchedItem>& list )
{
	m_watchList->Clear();
	for( const auto& item : list )
	{
		m_watchList->Add( MakePtr<GameItem>( item.guiString, "" ) );
	}
	OnPropertyChanged( "WatchList" );
}

Noesis::ObservableCollection<GameItem>* GameModel::GetWatchList() const
{
	return m_watchList;
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

	NsProp( "KingdomName", &GameModel::getKingdomName );
	NsProp( "KingdomInfo1", &GameModel::getKingdomInfo1 );
	NsProp( "KingdomInfo2", &GameModel::getKingdomInfo2 );
	NsProp( "KingdomInfo3", &GameModel::getKingdomInfo3 );

	NsProp( "RenderDesignations", &GameModel::getRenderDesignations, &GameModel::setRenderDesignations );
	NsProp( "RenderJobs", &GameModel::getRenderJobs, &GameModel::setRenderJobs );
	NsProp( "RenderWalls", &GameModel::getRenderWalls, &GameModel::setRenderWalls );
	NsProp( "RenderAxles", &GameModel::getRenderAxles, &GameModel::setRenderAxles );

	NsProp( "CmdButtonCommand", &GameModel::GetCmdButtonCommand );
	NsProp( "CmdCategory", &GameModel::GetCmdCategory );

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
	NsProp( "ShowInventory", &GameModel::getShowInventory );
	NsProp( "ShowSelection", &GameModel::getShowSelection );
	
	NsProp( "ShowMessage", &GameModel::getShowMessage );
	NsProp( "ShowMessageButtonOk", &GameModel::getShowMessageButtonOk );
	NsProp( "ShowMessageButtonYesNo", &GameModel::getShowMessageButtonYesNo );
	NsProp( "MessageHeader", &GameModel::getMessageHeader );
	NsProp( "MessageText", &GameModel::getMessageText );
	NsProp( "MessageCmd", &GameModel::GetMessageButtonCmd );

	NsProp( "CloseWindowCmd", &GameModel::GetCloseWindowCmd );
	NsProp( "OpenCreatureDetailsCmd", &GameModel::GetOpenGnomeDetailsCmd );

	NsProp( "WatchList", &GameModel::GetWatchList );

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
	NsProp( "ShowReplaceButton", &BuildItem::GetShowReplaceButton );
	NsProp( "ShowFillHoleButton", &BuildItem::GetShowFillHoleButton );
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
