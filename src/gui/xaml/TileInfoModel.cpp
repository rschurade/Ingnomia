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

#include "TileInfoModel.h"

#include "../../base/tile.h"
#include "ProxyTileInfo.h"

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

IngnomiaGUI::TITabItem::TITabItem( QString name, QString sid ) :
	m_name( name.toStdString().c_str() ),
	m_sid( sid.toStdString().c_str() )
{
}

bool IngnomiaGUI::TITabItem::GetChecked() const
{
	return _active;
}

void IngnomiaGUI::TITabItem::setActive( bool active )
{
	if ( _active != active )
	{
		_active = active;
		OnPropertyChanged( "Checked" );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TerrainTabItem::TerrainTabItem( QString name, QString sid, QString action1, QString action2 ) :
	m_name( name.toStdString().c_str() ),
	m_sid( sid.toStdString().c_str() )
{
	_action1    = action1.toStdString().c_str();
	_action2    = action2.toStdString().c_str();
	_hasAction1 = !action1.isEmpty();
	_hasAction2 = !action2.isEmpty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ItemTabItem::ItemTabItem( QString name, unsigned int id ) :
	m_name( name.toStdString().c_str() ),
	m_id( id ),
	m_sid( QString::number( id ).toStdString().c_str() )
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CreatureTabItem::CreatureTabItem( QString name, unsigned int id ) :
	m_name( name.toStdString().c_str() ),
	m_id( id ),
	m_sid( QString::number( id ).toStdString().c_str() )
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////

AutomatonTabItem::AutomatonTabItem( QString name, unsigned int id, bool refuel, QString coreItem, ProxyTileInfo* proxy ) :
	m_name( name.toStdString().c_str() ),
	m_id( id ),
	m_sid( QString::number( id ).toStdString().c_str() ),
	m_refuel( refuel ),
	m_proxy( proxy )
{
	m_coreItems = *new ObservableCollection<TITabItem>();
	m_coreItems->Add( MakePtr<TITabItem>( "None", "" ) );
	m_coreItems->Add( MakePtr<TITabItem>( "Automaton Core MK1", "AutomatonCoreMark1" ) );
	m_coreItems->Add( MakePtr<TITabItem>( "Automaton Core MK2", "AutomatonCoreMark2" ) );

	if( coreItem == "AutomatonCoreMark1" )
	{
		m_selectedCoreItem = m_coreItems->Get( 1 );
	}
	else if( coreItem == "AutomatonCoreMark2" )
	{
		m_selectedCoreItem = m_coreItems->Get( 2 );
	}
	else
	{
		m_selectedCoreItem = m_coreItems->Get( 0 );
	}
}

Noesis::ObservableCollection<TITabItem>* AutomatonTabItem::getCoreItems() const
{
	return m_coreItems;
}

void AutomatonTabItem::setSelectedCore( TITabItem* ci )
{
	if( ci && ci != m_selectedCoreItem )
	{
		m_selectedCoreItem = ci;
		m_proxy->setAutomatonCore( m_id, ci->GetID() );
	}
}
	
TITabItem* AutomatonTabItem::getSelectedCore() const
{
	return m_selectedCoreItem;
}

bool AutomatonTabItem::getRefuel() const
{
	return m_refuel;
}

void AutomatonTabItem::setRefuel( bool value )
{
	if( value != m_refuel )
	{
		m_refuel = value;
		m_proxy->setAutomatonRefuel( m_id, m_refuel );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
TileInfoModel::TileInfoModel()
{
	m_proxy = new ProxyTileInfo;
	m_proxy->setParent( this );

	m_tileIDString = Noesis::String( std::to_string( m_tileID ).c_str() );

	_tabItems = *new ObservableCollection<TITabItem>();

	_tabItemElements.tt = MakePtr<TITabItem>( "T", "T" );
	_tabItems->Add( _tabItemElements.tt );
	_tabItemElements.tt->setActive( true );

	_tabItemElements.tc = MakePtr<TITabItem>( "C", "C" );
	_tabItemElements.ti = MakePtr<TITabItem>( "I", "I" );
	_tabItemElements.td = MakePtr<TITabItem>( "D", "D" );
	_tabItemElements.tj = MakePtr<TITabItem>( "J", "J" );

	_terrainTabItems = *new ObservableCollection<TerrainTabItem>();

	_itemTabItems = *new ObservableCollection<ItemTabItem>();

	_creatureTabItems = *new ObservableCollection<CreatureTabItem>();
	_automatonTabItems = *new ObservableCollection<AutomatonTabItem>();

	_cmdTab.SetExecuteFunc( MakeDelegate( this, &TileInfoModel::OnCmdTab ) );
	_cmdTerrain.SetExecuteFunc( MakeDelegate( this, &TileInfoModel::OnCmdTerrain ) );
	_cmdManage.SetExecuteFunc( MakeDelegate( this, &TileInfoModel::OnCmdManage ) );
	_miniSPContents   = *new ObservableCollection<TITabItem>();
	_possibleTennants = *new ObservableCollection<CreatureTabItem>();

	_jobTabRequiredItems = *new Noesis::ObservableCollection<NRequiredItem>();

	_cmdMechToggleActive.SetExecuteFunc( MakeDelegate( this, &TileInfoModel::OnCmdMechToggleActive ) );
	_cmdMechToggleInvert.SetExecuteFunc( MakeDelegate( this, &TileInfoModel::OnCmdMechToggleInvert ) );
}

void TileInfoModel::onUpdateTileInfo( const GuiTileInfo& tileInfo )
{
	m_tileIDString  = Noesis::String( std::to_string( tileInfo.tileID ).c_str() );
	m_tileID        = tileInfo.tileID;
	m_designationID = tileInfo.designationID;

	// tt is always present
	uint32_t activeItems = 1;

	auto flags        = tileInfo.flags;
	m_designationFlag = tileInfo.designationFlag;
	
	_tabItems->Remove( _tabItemElements.td );
	_tabItemElements.td->setActive( false );

	if ( tileInfo.designationID )
	{
		m_designationName = tileInfo.designationName.toStdString().c_str();

		if ( flags & ( TileFlag::TF_GROVE + TileFlag::TF_FARM + TileFlag::TF_PASTURE ) )
		{
			m_proxy->sendManageCommand( m_tileID );
		}
		else if ( flags & TileFlag::TF_STOCKPILE )
		{
			m_proxy->requestStockpileItems( m_tileID );
		}
		else if ( flags & TileFlag::TF_WORKSHOP )
		{
		}
		else if ( flags & TileFlag::TF_ROOM )
		{
			m_roomType     = tileInfo.roomType;
			m_hasRoof      = tileInfo.hasRoof;
			m_isEnclosed   = tileInfo.isEnclosed;
			m_hasAlarmBell = tileInfo.hasAlarmBell;
			m_beds         = tileInfo.beds.toStdString().c_str();
			m_alarm        = tileInfo.alarm;
			m_roomValue	   = QString::number( tileInfo.roomValue ).toStdString().c_str();
		}
	}
	else
	{
		_mode = TileInfoMode::Terrain;
	}

	m_hasJob = ( flags & ( TileFlag::TF_JOB_FLOOR + TileFlag::TF_JOB_WALL + TileFlag::TF_JOB_BUSY_WALL + TileFlag::TF_JOB_BUSY_FLOOR ) );
	
	bool anyChecked = false;
	for ( int i = 0; i < _tabItems->Count(); ++i )
	{
		anyChecked |= _tabItems->Get( i )->GetChecked();
	}
	if ( !anyChecked )
	{
		_tabItems->Get( 0 )->setActive( true );
		_mode = TileInfoMode::Terrain;
	}

	QString action1;
	QString action2;

	_terrainTabItems->Clear();
	if ( !tileInfo.wall.isEmpty() )
	{
		if ( tileInfo.jobName.isEmpty() )
		{
			action1 = "Mine";
			action2 = "Replace";
		}
		_terrainTabItems->Add( MakePtr<TerrainTabItem>( tileInfo.wall, "Wall", action1, action2 ) );
	}
	action1.clear();
	action2.clear();
	if ( !tileInfo.floor.isEmpty() )
	{
		if ( tileInfo.plant.isEmpty() && tileInfo.wall.isEmpty() && tileInfo.jobName.isEmpty() )
		{
			action1 = "Remove";
			action2 = "Replace";
		}
		_terrainTabItems->Add( MakePtr<TerrainTabItem>( tileInfo.floor, "Floor", action1, action2 ) );
	}
	if ( !tileInfo.embedded.isEmpty() )
	{
		_terrainTabItems->Add( MakePtr<TerrainTabItem>( tileInfo.embedded, "Embedded" ) );
	}
	action1.clear();
	action2.clear();
	if ( !tileInfo.plant.isEmpty() )
	{
		// need to distinguish between plant and tree and check if harvestable
		if ( tileInfo.jobName.isEmpty() )
		{
			if ( tileInfo.plantIsHarvestable )
			{
				action1 = "Harvest";
			}
			action2 = "Destroy";
			if ( tileInfo.plantIsTree )
			{
				action2 = "Fell";
			}
		}

		_terrainTabItems->Add( MakePtr<TerrainTabItem>( tileInfo.plant, "Plant", action1, action2 ) );
	}
	if ( !tileInfo.water.isEmpty() )
		_terrainTabItems->Add( MakePtr<TerrainTabItem>( tileInfo.water, "Water" ) );

	switch ( m_designationFlag )
	{
		case TileFlag::TF_WORKSHOP:
			m_designationTitle = "Workshop";
			break;
		case TileFlag::TF_STOCKPILE:
			m_designationTitle = "Stockpile";
			break;
		case TileFlag::TF_GROVE:
			m_designationTitle = "Grove";
			break;
		case TileFlag::TF_FARM:
			m_designationTitle = "Farm";
			break;
		case TileFlag::TF_PASTURE:
			m_designationTitle = "Pasture";
			break;
		case TileFlag::TF_ROOM:
			m_designationTitle = "Room";
			break;
	}


	_itemTabItems->Clear();
	if ( !tileInfo.items.empty() )
	{
		for ( auto git : tileInfo.items )
		{
			_itemTabItems->Add( MakePtr<ItemTabItem>( git.text, git.id ) );
		}
	}

	_creatureTabItems->Clear();
	_automatonTabItems->Clear();
	if ( !tileInfo.creatures.empty() )
	{
		for ( auto gct : tileInfo.creatures )
		{
			if( gct.type == CreatureType::AUTOMATON )
			{
				_automatonTabItems->Add( MakePtr<AutomatonTabItem>( gct.text, gct.id, gct.refuel, gct.coreItem, m_proxy ) );
			}
			else
			{
				_creatureTabItems->Add( MakePtr<CreatureTabItem>( gct.text, gct.id ) );
			}
		}
	}

	_possibleTennants->Clear();
	m_tennant = nullptr;
	if ( !tileInfo.possibleTennants.empty() )
	{
		_possibleTennants->Add( MakePtr<CreatureTabItem>( "unassigned", 0 ) );
		for ( const auto& gnome : tileInfo.possibleTennants )
		{
			_possibleTennants->Add( MakePtr<CreatureTabItem>( gnome.text, gnome.id ) );
			if ( tileInfo.tennant == gnome.id )
			{
				m_tennant = _possibleTennants->Get( _possibleTennants->Count() - 1 );
			}
		}
		if ( !m_tennant )
		{
			m_tennant = _possibleTennants->Get( 0 );
		}
	}

	_jobTabRequiredItems->Clear();
	for ( auto ri : tileInfo.requiredItems )
	{
		auto item = MakePtr<NRequiredItem>( ri.text, ri.count );

		_jobTabRequiredItems->Add( item );
	}

	m_jobName               = tileInfo.jobName.toStdString().c_str();
	m_jobWorker             = tileInfo.jobWorker.toStdString().c_str();
	m_jobPriority           = tileInfo.jobPriority.toStdString().c_str();
	m_requiredSkill         = tileInfo.requiredSkill.toStdString().c_str();
	m_requiredTool          = tileInfo.requiredTool.toStdString().c_str();
	m_requiredToolAvailable = tileInfo.requiredToolAvailable.toStdString().c_str();
	m_workablePosition      = tileInfo.workPositions.toStdString().c_str();

	if( tileInfo.mechInfo.itemID != 0 && !tileInfo.mechInfo.gui.isEmpty() )
	{
		m_mechanismID = tileInfo.mechInfo.itemID;
		m_hasMechanism = true;
		m_mechGui = tileInfo.mechInfo.gui;
		m_mechFuel = ( QString::number( tileInfo.mechInfo.fuel ) + "/" +
			QString::number( tileInfo.mechInfo.maxFuel ) ).toStdString().c_str();
		m_mechActive = tileInfo.mechInfo.active ? "Active" : "Inactive";
		m_mechInvert = tileInfo.mechInfo.inverted ? "Inverted" : "Not inverted";

		m_mechName = tileInfo.mechInfo.name.toStdString().c_str();
	}
	else
	{
		m_hasMechanism = false;
	}


	OnPropertyChanged( "JobName" );
	OnPropertyChanged( "JobWorker" );
	OnPropertyChanged( "JobPriority" );
	OnPropertyChanged( "RequiredSkill" );
	OnPropertyChanged( "RequiredTool" );
	OnPropertyChanged( "RequiredToolAvailable" );
	OnPropertyChanged( "RequiredItems" );
	OnPropertyChanged( "WorkablePosition" );

	OnPropertyChanged( "TileID" );
	OnPropertyChanged( "TabItems" );
	OnPropertyChanged( "TerrainTabItems" );
	OnPropertyChanged( "PossibleTennnants" );
	OnPropertyChanged( "Tennant" );
	OnPropertyChanged( "Alarm" );

	OnPropertyChanged( "VisRoomAssign" );
	OnPropertyChanged( "VisRoomValue" );
	OnPropertyChanged( "VisBeds" );
	OnPropertyChanged( "VisAlarm" );
	OnPropertyChanged( "RoomValue" );
	OnPropertyChanged( "Beds" );
	OnPropertyChanged( "Enclosed" );
	OnPropertyChanged( "Roofed" );

	OnPropertyChanged( "ShowTerrain" );
	OnPropertyChanged( "ShowItems" );
	OnPropertyChanged( "ShowCreatures" );
	OnPropertyChanged( "ShowAutomatons" );
	OnPropertyChanged( "ShowJob" );
	OnPropertyChanged( "ShowMiniStockpile" );

	OnPropertyChanged( "ShowDesignation" );
	OnPropertyChanged( "ShowDesignationSimple" );
	OnPropertyChanged( "ShowDesignationRoom" );
	OnPropertyChanged( "DesignationName" );
	OnPropertyChanged( "DesignationTitle" );

	OnPropertyChanged( "ShowMechanism" );
	OnPropertyChanged( "ShowMechActive" );
	OnPropertyChanged( "ShowMechFuel" );
	OnPropertyChanged( "ShowMechInvert" );
	OnPropertyChanged( "MechActive" );
	OnPropertyChanged( "MechFuel" );
	OnPropertyChanged( "MechInvert" );
	OnPropertyChanged( "MechName" );
}

const char* TileInfoModel::getTileID() const
{
	return m_tileIDString.Str();
}

Noesis::ObservableCollection<IngnomiaGUI::TITabItem>* TileInfoModel::getTabItems() const
{
	return _tabItems;
}

Noesis::ObservableCollection<TerrainTabItem>* TileInfoModel::getTerrainTabItems() const
{
	return _terrainTabItems;
}

Noesis::ObservableCollection<ItemTabItem>* TileInfoModel::getItemTabItems() const
{
	return _itemTabItems;
}

Noesis::ObservableCollection<CreatureTabItem>* TileInfoModel::getCreatureTabItems() const
{
	return _creatureTabItems;
}

Noesis::ObservableCollection<AutomatonTabItem>* TileInfoModel::getAutomatonTabItems() const
{
	return _automatonTabItems;
}

Noesis::ObservableCollection<CreatureTabItem>* TileInfoModel::getPossibleTennants() const
{
	return _possibleTennants;
}

Noesis::ObservableCollection<NRequiredItem>* TileInfoModel::GetJobRequiredItems() const
{
	return _jobTabRequiredItems;
}

const NoesisApp::DelegateCommand* TileInfoModel::GetCmdTab() const
{
	return &_cmdTab;
}

void TileInfoModel::OnCmdTab( BaseComponent* param )
{
	QString tab = param->ToString().Str();
	if ( tab == "T" )
		_mode = TileInfoMode::Terrain;
	/*
	else if ( tab == "I" )
		_mode = TileInfoMode::Items;
	else if ( tab == "C" )
		_mode = TileInfoMode::Creatures;
	else if ( tab == "D" )
	{
		// if designation is workshop or stock pile, open window
		switch ( m_designationFlag )
		{
			case TileFlag::TF_WORKSHOP:
				m_proxy->sendManageCommand( m_tileID );
				break;
			case TileFlag::TF_STOCKPILE:
				m_proxy->requestStockpileItems( m_tileID );
				_mode = TileInfoMode::Stockpile;
				break;
			case TileFlag::TF_GROVE:
			case TileFlag::TF_FARM:
			case TileFlag::TF_PASTURE:
				_mode = TileInfoMode::Designation;
				m_proxy->sendManageCommand( m_tileID );
				break;
			case TileFlag::TF_ROOM:
				_mode = TileInfoMode::Room;
				break;
		}
	}
	else if ( tab == "J" )
		_mode = TileInfoMode::Job;

	for ( int i = 0; i < _tabItems->Count(); ++i )
	{
		_tabItems->Get( i )->setActive( tab == _tabItems->Get( i )->GetID() );
	}
	*/
	OnPropertyChanged( "ShowTerrain" );
	OnPropertyChanged( "ShowItems" );
	OnPropertyChanged( "ShowCreatures" );
	OnPropertyChanged( "ShowDesignation" );
	OnPropertyChanged( "ShowDesignationRoom" );
	OnPropertyChanged( "ShowJob" );
	OnPropertyChanged( "ShowMiniStockpile" );
}

void TileInfoModel::OnCmdTerrain( BaseComponent* param )
{
	if ( m_tileID )
	{
		m_proxy->sendTerrainCommand( m_tileID, param->ToString().Str() );
	}
}

void TileInfoModel::OnCmdManage( BaseComponent* param )
{
	m_proxy->sendManageCommand( m_tileID );

	if ( m_designationFlag & ( TileFlag::TF_GROVE + TileFlag::TF_FARM + TileFlag::TF_PASTURE ) )
	{
		_mode = TileInfoMode::Designation;
	}
	OnPropertyChanged( "ShowTerrain" );
	OnPropertyChanged( "ShowDesignation" );
}

const char* TileInfoModel::GetShowTerrain() const
{
	if ( _mode == TileInfoMode::Terrain )
	{
		return "Visible";
	}
	return "Collapsed";
}

const char* TileInfoModel::GetShowItems() const
{
	if ( _itemTabItems->Count() > 0 )
	{
		return "Visible";
	}
	return "Collapsed";
}

const char* TileInfoModel::GetShowCreatures() const
{
	if ( _creatureTabItems->Count() > 0 )
	{
		return "Visible";
	}
	return "Collapsed";
}

const char* TileInfoModel::GetShowAutomatons() const
{
	if ( _automatonTabItems->Count() > 0 )
	{
		return "Visible";
	}
	return "Collapsed";
}

const char* TileInfoModel::GetShowDesignation() const
{
	if ( _mode == TileInfoMode::Designation )
	{
		return "Visible";
	}
	return "Collapsed";
}

const char* TileInfoModel::GetShowDesignationSimple() const
{
	if ( m_designationFlag & ( TileFlag::TF_GROVE + TileFlag::TF_FARM + TileFlag::TF_PASTURE + TileFlag::TF_WORKSHOP ) )
	{
		return "Visible";
	}
	return "Collapsed";
}


const char* TileInfoModel::GetShowDesignationRoom() const
{
	if ( m_designationFlag & TileFlag::TF_ROOM )
	{
		return "Visible";
	}
	return "Collapsed";
}

const char* TileInfoModel::GetShowJob() const
{
	if ( m_hasJob )
	{
		return "Visible";
	}
	return "Collapsed";
}

const char* TileInfoModel::GetShowMiniSP() const
{
	if ( m_designationFlag & TileFlag::TF_STOCKPILE )
	{
		return "Visible";
	}
	return "Collapsed";
}

void TileInfoModel::updateMiniStockpile( const GuiStockpileInfo& info )
{
	m_miniStockpileName = info.name.toStdString().c_str();

	m_capacity = ( "Capacity: " + QString::number( info.capacity ) ).toStdString().c_str();
	m_itemCount = ( "Used: " + QString::number( info.itemCount ).toStdString() ).c_str();
	m_reserved = ( "Reserved: " + QString::number( info.reserved ).toStdString() ).c_str();

	_miniSPContents->Clear();

	for ( auto is : info.summary )
	{
		_miniSPContents->Add( MakePtr<IngnomiaGUI::TITabItem>( QString::number( is.count ) + " " + is.materialName + " " + is.itemName, "" ) );
	}

	OnPropertyChanged( "MiniStockpileName" );
	OnPropertyChanged( "MiniStockpileContents" );
	OnPropertyChanged( "Capacity" );
	OnPropertyChanged( "ItemCount" );
	OnPropertyChanged( "Reserved" );
}

Noesis::ObservableCollection<IngnomiaGUI::TITabItem>* TileInfoModel::getMiniSPContents() const
{
	return _miniSPContents;
}

void TileInfoModel::SetTennant( CreatureTabItem* tennant )
{
	if ( tennant && m_tennant != tennant )
	{
		m_tennant = tennant;
		m_proxy->setTennant( m_designationID, tennant->uid() );
	}
}

CreatureTabItem* TileInfoModel::GetTennant() const
{
	return m_tennant;
}

const char* TileInfoModel::GetVisRoomAssign() const
{
	if ( m_roomType == RoomType::PersonalRoom )
	{
		return "Visible";
	}
	return "Collapsed";
}

const char* TileInfoModel::GetVisRoomValue() const
{
	if ( m_roomType == RoomType::PersonalRoom || m_roomType == RoomType::Dining )
	{
		return "Visible";
	}
	return "Collapsed";
}

const char* TileInfoModel::GetVisBeds() const
{
	if ( m_roomType == RoomType::Dorm || m_roomType == RoomType::Hospital )
	{
		return "Visible";
	}
	return "Collapsed";
}

const char* TileInfoModel::GetVisAlarm() const
{
	if ( m_roomType == RoomType::Dining && m_hasAlarmBell )
	{
		return "Visible";
	}
	return "Collapsed";
}

const char* TileInfoModel::GetRoomValue() const
{
	return m_roomValue.Str();
}

const char* TileInfoModel::GetBeds() const
{
	return m_beds.Str();
}

const char* TileInfoModel::GetEnclosed() const
{
	if ( m_isEnclosed )
	{
		return "yes";
	}
	return "no";
}

const char* TileInfoModel::GetRoofed() const
{
	if ( m_hasRoof )
	{
		return "yes";
	}
	return "no";
}

bool TileInfoModel::GetAlarm() const
{
	return m_alarm;
}

void TileInfoModel::SetAlarm( bool value )
{
	if ( m_alarm != value )
	{
		m_alarm = value;
		m_proxy->setAlarm( m_designationID, value );
	}
}

const char* TileInfoModel::GetCapacity() const
{
	return m_capacity.Str();
}
	
const char* TileInfoModel::GetItemCount() const
{
	return m_itemCount.Str();
}

const char* TileInfoModel::GetReserved() const
{
	return m_reserved.Str();
}


const char* TileInfoModel::GetShowMechanism() const
{
	if ( m_hasMechanism )
	{
		return "Visible";
	}
	return "Collapsed";
}
	
const char* TileInfoModel::GetShowMechActive() const
{
	if ( m_mechGui.contains( "Active" ) )
	{
		return "Visible";
	}
	return "Collapsed";
}

const char* TileInfoModel::GetShowMechFuel() const
{
	if ( m_mechGui.contains( "Fuel" ) )
	{
		return "Visible";
	}
	return "Collapsed";
}

const char* TileInfoModel::GetShowMechInvert() const
{
	if ( m_mechGui.contains( "Invert" ) )
	{
		return "Visible";
	}
	return "Collapsed";
}

const char* TileInfoModel::GetMechActive() const
{
	return m_mechActive.Str();
}

const char* TileInfoModel::GetMechFuel() const
{
	return m_mechFuel.Str();
}

const char* TileInfoModel::GetMechInvert() const
{
	return m_mechInvert.Str();
}

const char* TileInfoModel::GetMechName() const
{
	return m_mechName.Str();
}

void TileInfoModel::OnCmdMechToggleActive( BaseComponent* param )
{
	m_proxy->toggleMechActive( m_mechanismID );
}

void TileInfoModel::OnCmdMechToggleInvert( BaseComponent* param )
{
	m_proxy->toggleMechInvert( m_mechanismID );
}


////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION( TileInfoModel, "IngnomiaGUI.TileInfoModel" )
{
	NsProp( "TileID", &TileInfoModel::getTileID );
	NsProp( "TabItems", &TileInfoModel::getTabItems );
	NsProp( "ShowTerrain", &TileInfoModel::GetShowTerrain );
	NsProp( "ShowItems", &TileInfoModel::GetShowItems );
	NsProp( "ShowCreatures", &TileInfoModel::GetShowCreatures );
	NsProp( "ShowAutomatons", &TileInfoModel::GetShowAutomatons );
	NsProp( "ShowDesignation", &TileInfoModel::GetShowDesignation );
	NsProp( "ShowDesignationSimple", &TileInfoModel::GetShowDesignationSimple );
	NsProp( "ShowDesignationRoom", &TileInfoModel::GetShowDesignationRoom );
	NsProp( "ShowJob", &TileInfoModel::GetShowJob );
	NsProp( "ShowMiniStockpile", &TileInfoModel::GetShowMiniSP );
	
	NsProp( "CmdTab", &TileInfoModel::GetCmdTab );
	NsProp( "CmdTerrain", &TileInfoModel::GetCmdTerrain );
	NsProp( "CmdManage", &TileInfoModel::GetCmdManage );

	NsProp( "TerrainTab", &TileInfoModel::getTerrainTabItems );
	NsProp( "ItemTab", &TileInfoModel::getItemTabItems );
	NsProp( "CreatureTab", &TileInfoModel::getCreatureTabItems );
	NsProp( "AutomatonTab", &TileInfoModel::getAutomatonTabItems );

	NsProp( "JobName", &TileInfoModel::GetJobName );
	NsProp( "JobWorker", &TileInfoModel::GetJobWorker );
	NsProp( "JobPriority", &TileInfoModel::GetJobPriority );
	NsProp( "RequiredSkill", &TileInfoModel::GetRequiredSkill );
	NsProp( "RequiredTool", &TileInfoModel::GetRequiredTool );
	NsProp( "RequiredToolAvailable", &TileInfoModel::GetRequiredToolAvailable );
	NsProp( "RequiredItems", &TileInfoModel::GetJobRequiredItems );
	NsProp( "WorkablePosition", &TileInfoModel::GetWorkablePosition );

	NsProp( "DesignationName", &TileInfoModel::GetDesignationName );
	NsProp( "DesignationTitle", &TileInfoModel::GetDesignationTitle );

	NsProp( "MiniStockpileName", &TileInfoModel::GetMiniSPName );
	NsProp( "MiniStockpileContents", &TileInfoModel::getMiniSPContents );
	NsProp( "Capacity", &TileInfoModel::GetCapacity );
	NsProp( "ItemCount", &TileInfoModel::GetItemCount );
	NsProp( "Reserved", &TileInfoModel::GetReserved );

	NsProp( "PossibleTennants", &TileInfoModel::getPossibleTennants );
	NsProp( "Tennant", &TileInfoModel::GetTennant, &TileInfoModel::SetTennant );
	NsProp( "VisRoomAssign", &TileInfoModel::GetVisRoomAssign );
	NsProp( "VisRoomValue", &TileInfoModel::GetVisRoomValue );
	NsProp( "VisBeds", &TileInfoModel::GetVisBeds );
	NsProp( "VisAlarm", &TileInfoModel::GetVisAlarm );
	NsProp( "RoomValue", &TileInfoModel::GetRoomValue );
	NsProp( "Beds", &TileInfoModel::GetBeds );
	NsProp( "Enclosed", &TileInfoModel::GetEnclosed );
	NsProp( "Roofed", &TileInfoModel::GetRoofed );
	NsProp( "Alarm", &TileInfoModel::GetAlarm, &TileInfoModel::SetAlarm );

	NsProp( "ShowMechanism", &TileInfoModel::GetShowMechanism );
	NsProp( "ShowMechActive", &TileInfoModel::GetShowMechActive );
	NsProp( "ShowMechFuel", &TileInfoModel::GetShowMechFuel );
	NsProp( "ShowMechInvert", &TileInfoModel::GetShowMechInvert );
	NsProp( "MechActive", &TileInfoModel::GetMechActive );
	NsProp( "MechFuel", &TileInfoModel::GetMechFuel );
	NsProp( "MechInvert", &TileInfoModel::GetMechInvert );
	NsProp( "MechName", &TileInfoModel::GetMechName );
	NsProp( "CmdMechToggleActive", &TileInfoModel::GetCmdMechToggleActive );
	NsProp( "CmdMechToggleInvert", &TileInfoModel::GetCmdMechToggleInvert );
}

NS_IMPLEMENT_REFLECTION( IngnomiaGUI::TITabItem )
{
	NsProp( "Name", &TITabItem::GetName );
	NsProp( "ID", &TITabItem::GetID );
	NsProp( "Checked", &TITabItem::GetChecked, &TITabItem::SetChecked );
}

NS_IMPLEMENT_REFLECTION( TerrainTabItem )
{
	NsProp( "Name", &TerrainTabItem::GetName );
	NsProp( "ID", &TerrainTabItem::GetID );
	NsProp( "Action1Text", &TerrainTabItem::GetAction1 );
	NsProp( "Action2Text", &TerrainTabItem::GetAction2 );
	NsProp( "Action1Visible", &TerrainTabItem::Action1Visible );
	NsProp( "Action2Visible", &TerrainTabItem::Action2Visible );
}

NS_IMPLEMENT_REFLECTION( ItemTabItem )
{
	NsProp( "Name", &ItemTabItem::GetName );
}

NS_IMPLEMENT_REFLECTION( CreatureTabItem )
{
	NsProp( "Name", &CreatureTabItem::GetName );
	NsProp( "ID", &CreatureTabItem::GetID );
}

NS_IMPLEMENT_REFLECTION( AutomatonTabItem )
{
	NsProp( "Name", &AutomatonTabItem::GetName );
	NsProp( "ID", &AutomatonTabItem::GetID );
	NsProp( "Cores", &AutomatonTabItem::getCoreItems );
	NsProp( "SelectedCore", &AutomatonTabItem::getSelectedCore, &AutomatonTabItem::setSelectedCore );
	NsProp( "Refuel", &AutomatonTabItem::getRefuel, &AutomatonTabItem::setRefuel );
}
