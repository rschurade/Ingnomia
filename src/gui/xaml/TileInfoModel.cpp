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

IngnomiaGUI::TabItem::TabItem( QString name, QString sid )
{
	_name = name.toStdString().c_str();
	_sid  = sid.toStdString().c_str();
}

const char* IngnomiaGUI::TabItem::GetName() const
{
	return _name.Str();
}

const char* IngnomiaGUI::TabItem::GetID() const
{
	return _sid.Str();
}

bool IngnomiaGUI::TabItem::GetChecked() const
{
	return _active;
}

void IngnomiaGUI::TabItem::setActive( bool active )
{
	if ( _active != active )
	{
		_active = active;
		OnPropertyChanged( "Checked" );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TerrainTabItem::TerrainTabItem( QString name, QString sid, QString action1, QString action2 )
{
	_name       = name.toStdString().c_str();
	_sid        = sid.toStdString().c_str();
	_action1    = action1.toStdString().c_str();
	_action2    = action2.toStdString().c_str();
	_hasAction1 = !action1.isEmpty();
	_hasAction2 = !action2.isEmpty();
}

const char* TerrainTabItem::GetName() const
{
	return _name.Str();
}

const char* TerrainTabItem::GetID() const
{
	return _sid.Str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ItemTabItem::ItemTabItem( QString name, unsigned int id )
{
	_name = name.toStdString().c_str();
	_id   = id;
}

const char* ItemTabItem::GetName() const
{
	return _name.Str();
}

const unsigned int ItemTabItem::GetID() const
{
	return _id;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CreatureTabItem::CreatureTabItem( QString name, unsigned int id ) :
	m_name( name.toStdString().c_str() ),
	m_id( id ),
	m_sid( QString::number( id ).toStdString().c_str() )
{
}

const char* CreatureTabItem::GetName() const
{
	return m_name.Str();
}

const char* CreatureTabItem::GetID() const
{
	return m_sid.Str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
TileInfoModel::TileInfoModel()
{
	m_proxy = new ProxyTileInfo;
	m_proxy->setParent( this );

	m_tileIDString = Noesis::String( std::to_string( m_tileID ).c_str() );

	_tabItems = *new ObservableCollection<TabItem>();

	_tabItemElements.tt = MakePtr<TabItem>( "T", "T" );
	_tabItems->Add( _tabItemElements.tt );
	_tabItemElements.tt->setActive( true );

	_tabItemElements.tc = MakePtr<TabItem>( "C", "C" );
	_tabItemElements.ti = MakePtr<TabItem>( "I", "I" );
	_tabItemElements.td = MakePtr<TabItem>( "D", "D" );
	_tabItemElements.tj = MakePtr<TabItem>( "J", "J" );

	_terrainTabItems = *new ObservableCollection<TerrainTabItem>();

	_itemTabItems = *new ObservableCollection<ItemTabItem>();

	_creatureTabItems = *new ObservableCollection<CreatureTabItem>();

	_cmdTab.SetExecuteFunc( MakeDelegate( this, &TileInfoModel::OnCmdTab ) );
	_cmdTerrain.SetExecuteFunc( MakeDelegate( this, &TileInfoModel::OnCmdTerrain ) );
	_cmdManage.SetExecuteFunc( MakeDelegate( this, &TileInfoModel::OnCmdManage ) );
	_miniSPContents = *new ObservableCollection<TabItem>();
}

void TileInfoModel::onUpdateTileInfo( const GuiTileInfo& tileInfo )
{
	m_tileIDString = Noesis::String( std::to_string( tileInfo.tileID ).c_str() );
	m_tileID       = tileInfo.tileID;

	// tt is always present
	uint32_t activeItems = 1;
	if ( tileInfo.numGnomes > 0 || tileInfo.numAnimals > 0 || tileInfo.numMonsters > 0 )
	{
		if ( !_tabItems->Contains( _tabItemElements.tc ) )
		{
			_tabItems->Insert( activeItems, _tabItemElements.tc );
		}
		++activeItems;
	}
	else
	{
		_tabItems->Remove( _tabItemElements.tc );
		_tabItemElements.tc->setActive( false );
	}

	if ( tileInfo.numItems > 0 )
	{
		if ( !_tabItems->Contains( _tabItemElements.ti ) )
		{
			_tabItems->Insert( activeItems, _tabItemElements.ti );
		}
		++activeItems;
	}
	else
	{
		_tabItems->Remove( _tabItemElements.ti );
		_tabItemElements.ti->setActive( false );
	}

	auto flags        = tileInfo.flags;
	m_designationFlag = tileInfo.designationFlag;
	//if ( flags & ( TileFlag::TF_WORKSHOP | TileFlag::TF_STOCKPILE | TileFlag::TF_GROVE | TileFlag::TF_FARM | TileFlag::TF_PASTURE | TileFlag::TF_ROOM ) )
	if ( tileInfo.designationID )
	{
		if ( !_tabItems->Contains( _tabItemElements.td ) )
		{
			_tabItems->Insert( activeItems, _tabItemElements.td );
		}
		++activeItems;
		m_designationName = tileInfo.designationName.toStdString().c_str();

		if ( flags & ( TileFlag::TF_GROVE + TileFlag::TF_FARM + TileFlag::TF_PASTURE + TileFlag::TF_ROOM ) )
		{
			m_proxy->sendManageCommand( m_tileID );
			if( _mode == TileInfoMode::Stockpile )
			{
				_mode = TileInfoMode::Designation;
			}
		}
		else if( flags & TileFlag::TF_STOCKPILE )
		{
			m_proxy->requestStockpileItems( m_tileID );
			if( _mode == TileInfoMode::Designation )
			{
				_mode = TileInfoMode::Stockpile;
			}
		}
		else if( flags & TileFlag::TF_WORKSHOP )
		{
			_mode = TileInfoMode::Terrain;
		}
	}
	else
	{
		_tabItems->Remove( _tabItemElements.td );
		_tabItemElements.td->setActive( false );
	}

	if ( flags & ( TileFlag::TF_JOB_FLOOR + TileFlag::TF_JOB_WALL + TileFlag::TF_JOB_BUSY_WALL + TileFlag::TF_JOB_BUSY_FLOOR ) )
	{
		if ( !_tabItems->Contains( _tabItemElements.tj ) )
		{
			_tabItems->Insert( activeItems, _tabItemElements.tj );
		}
		++activeItems;
	}
	else
	{
		_tabItems->Remove( _tabItemElements.tj );
		_tabItemElements.tj->setActive( false );
	}

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

	_itemTabItems->Clear();
	if ( !tileInfo.items.empty() )
	{
		for ( auto git : tileInfo.items )
		{
			_itemTabItems->Add( MakePtr<ItemTabItem>( git.text, git.id ) );
		}
	}

	_creatureTabItems->Clear();
	if ( !tileInfo.creatures.empty() )
	{
		for ( auto gct : tileInfo.creatures )
		{
			_creatureTabItems->Add( MakePtr<CreatureTabItem>( gct.text, gct.id ) );
		}
	}

	m_jobName      = tileInfo.jobName.toStdString().c_str();
	m_jobWorker    = tileInfo.jobWorker.toStdString().c_str();
	m_requiredTool = tileInfo.requiredTool.toStdString().c_str();

	OnPropertyChanged( "JobName" );
	OnPropertyChanged( "JobWorker" );
	OnPropertyChanged( "RequiredTool" );

	OnPropertyChanged( "TileID" );
	OnPropertyChanged( "TabItems" );
	OnPropertyChanged( "TerrainTabItems" );

	OnPropertyChanged( "ShowTerrain" );
	OnPropertyChanged( "ShowItems" );
	OnPropertyChanged( "ShowCreatures" );
	OnPropertyChanged( "ShowJob" );
	OnPropertyChanged( "ShowMiniStockpile" );

	OnPropertyChanged( "ShowDesignation" );
	OnPropertyChanged( "DesignationName" );
}

const char* TileInfoModel::getTileID() const
{
	return m_tileIDString.Str();
}

Noesis::ObservableCollection<IngnomiaGUI::TabItem>* TileInfoModel::getTabItems() const
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

const NoesisApp::DelegateCommand* TileInfoModel::GetCmdTab() const
{
	return &_cmdTab;
}

void TileInfoModel::OnCmdTab( BaseComponent* param )
{
	QString tab = param->ToString().Str();
	if ( tab == "T" )
		_mode = TileInfoMode::Terrain;
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
				break;
		}
	}
	else if ( tab == "J" )
		_mode = TileInfoMode::Job;

	for ( int i = 0; i < _tabItems->Count(); ++i )
	{
		_tabItems->Get( i )->setActive( tab == _tabItems->Get( i )->GetID() );
	}

	OnPropertyChanged( "ShowTerrain" );
	OnPropertyChanged( "ShowItems" );
	OnPropertyChanged( "ShowCreatures" );
	OnPropertyChanged( "ShowDesignation" );
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
}

const char* TileInfoModel::GetShowTerrain() const
{
	if ( _mode == TileInfoMode::Terrain )
	{
		return "Visible";
	}
	return "Hidden";
}

const char* TileInfoModel::GetShowItems() const
{
	if ( _mode == TileInfoMode::Items )
	{
		return "Visible";
	}
	return "Hidden";
}

const char* TileInfoModel::GetShowCreatures() const
{
	if ( _mode == TileInfoMode::Creatures )
	{
		return "Visible";
	}
	return "Hidden";
}

const char* TileInfoModel::GetShowDesignation() const
{
	if ( _mode == TileInfoMode::Designation )
	{
		return "Visible";
	}
	return "Hidden";
}

const char* TileInfoModel::GetShowJob() const
{
	if ( _mode == TileInfoMode::Job )
	{
		return "Visible";
	}
	return "Hidden";
}

const char* TileInfoModel::GetShowMiniSP() const
{
	if ( _mode == TileInfoMode::Stockpile )
	{
		return "Visible";
	}
	return "Hidden";
}

void TileInfoModel::updateMiniStockpile( const GuiStockpileInfo& info )
{
	m_miniStockpileName = info.name.toStdString().c_str();

	_miniSPContents->Clear();

	for( auto is : info.summary )
	{
		_miniSPContents->Add( MakePtr<IngnomiaGUI::TabItem>( QString::number( is.count ) + " " + is.materialName + " " + is.itemName, "" ) );
	}


	OnPropertyChanged( "MiniStockpileName" );
	OnPropertyChanged( "MiniStockpileContents" );
}

Noesis::ObservableCollection<IngnomiaGUI::TabItem>* TileInfoModel::getMiniSPContents() const
{
	return _miniSPContents;
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
	NsProp( "ShowDesignation", &TileInfoModel::GetShowDesignation );
	NsProp( "ShowJob", &TileInfoModel::GetShowJob );
	NsProp( "ShowMiniStockpile", &TileInfoModel::GetShowMiniSP );

	NsProp( "CmdTab", &TileInfoModel::GetCmdTab );
	NsProp( "CmdTerrain", &TileInfoModel::GetCmdTerrain );
	NsProp( "CmdManage", &TileInfoModel::GetCmdManage );

	NsProp( "TerrainTab", &TileInfoModel::getTerrainTabItems );
	NsProp( "ItemTab", &TileInfoModel::getItemTabItems );
	NsProp( "CreatureTab", &TileInfoModel::getCreatureTabItems );

	NsProp( "JobName", &TileInfoModel::GetJobName );
	NsProp( "JobWorker", &TileInfoModel::GetJobWorker );
	NsProp( "RequiredTool", &TileInfoModel::GetRequiredTool );

	NsProp( "DesignationName", &TileInfoModel::GetDesignationName );

	NsProp( "MiniStockpileName", &TileInfoModel::GetMiniSPName );
	NsProp( "MiniStockpileContents", &TileInfoModel::getMiniSPContents );
}

NS_IMPLEMENT_REFLECTION( IngnomiaGUI::TabItem )
{
	NsProp( "Name", &TabItem::GetName );
	NsProp( "ID", &TabItem::GetID );
	NsProp( "Checked", &TabItem::GetChecked, &TabItem::SetChecked );
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
