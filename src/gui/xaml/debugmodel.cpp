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
#include "debugmodel.h"
#include "debugproxy.h"

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
NameEntry::NameEntry( const QString& name, unsigned int id ) :
	m_id( id )
{
	m_name = name.toStdString().c_str();
}

const char* NameEntry::getName() const
{
	return m_name.Str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
WSEntry::WSEntry( int width, int height ) :
	m_width( width ),
	m_height( height )
{
	m_name = ( QString::number( width ) + "x" + QString::number( height ) ).toStdString().c_str();
}

const char* WSEntry::getName() const
{
	return m_name.Str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DebugModel::DebugModel()
{
	m_proxy = new DebugProxy;
	m_proxy->setParent( this );

	m_pageCmd.SetExecuteFunc( MakeDelegate( this, &DebugModel::onPageCmd ) );
	m_spawnCmd.SetExecuteFunc( MakeDelegate( this, &DebugModel::onSpawnCmd ) );
	m_setHungerCmd.SetExecuteFunc( MakeDelegate( this, &DebugModel::onSetHungerCmd ) );
	m_setThirstCmd.SetExecuteFunc( MakeDelegate( this, &DebugModel::onSetThirstCmd ) );
	m_setSleepCmd.SetExecuteFunc( MakeDelegate( this, &DebugModel::onSetSleepCmd ) );
	m_killGnomeCmd.SetExecuteFunc( MakeDelegate( this, &DebugModel::onKillGnomeCmd ) );
	m_spawnItemCmd.SetExecuteFunc( MakeDelegate( this, &DebugModel::onSpawnItemCmd ) );

	m_gnomeList = *new ObservableCollection<NameEntry>();
	m_gnomeList->Add( MakePtr<NameEntry>( "All Gnomes", 0 ) );

	m_windowSizes = *new ObservableCollection<WSEntry>();
	m_windowSizes->Add( MakePtr<WSEntry>( 1920, 1080 ) );
	m_windowSizes->Add( MakePtr<WSEntry>( 1360, 768 ) );

	m_itemGroupList = *new ObservableCollection<NameEntry>();
	m_itemList = *new ObservableCollection<NameEntry>();
	m_material1List = *new ObservableCollection<NameEntry>();
	m_material2List = *new ObservableCollection<NameEntry>();

	m_spawnCount = "1";
	m_spawnX = "50";
	m_spawnY = "50";
	m_spawnZ = "100";
}

void DebugModel::updateGnomeList()
{
	m_gnomeList->Clear();
	m_gnomeList->Add( MakePtr<NameEntry>( "All Gnomes", 0 ) );
	m_proxy->requestGnomeList();
}

// Page visibility
const char* DebugModel::GetShowGnomes() const
{
	return m_page == DebugPage::Gnomes ? "Visible" : "Hidden";
}

const char* DebugModel::GetShowItems() const
{
	return m_page == DebugPage::Items ? "Visible" : "Hidden";
}

const char* DebugModel::GetShowGame() const
{
	return m_page == DebugPage::Game ? "Visible" : "Hidden";
}

void DebugModel::onPageCmd( BaseComponent* param )
{
	if ( param->ToString() == "Gnomes" )
	{
		m_page = DebugPage::Gnomes;
		updateGnomeList();
	}
	else if ( param->ToString() == "Items" )
	{
		m_page = DebugPage::Items;
		if ( m_itemGroupList->Count() == 0 )
		{
			m_proxy->requestItemGroups();
		}
	}
	else
	{
		m_page = DebugPage::Game;
	}

	OnPropertyChanged( "ShowGnomes" );
	OnPropertyChanged( "ShowItems" );
	OnPropertyChanged( "ShowGame" );
}

void DebugModel::onSpawnCmd( BaseComponent* param )
{
	m_proxy->spawnCreature( param->ToString().Str() );
}

// Gnome selector
Noesis::ObservableCollection<NameEntry>* DebugModel::getGnomeList() const
{
	return m_gnomeList;
}

void DebugModel::setSelectedGnome( NameEntry* item )
{
	m_selectedGnome = item;
	// If list only has "All Gnomes", refresh it
	if ( m_gnomeList->Count() <= 1 )
	{
		updateGnomeList();
	}
}

NameEntry* DebugModel::getSelectedGnome() const
{
	return m_selectedGnome;
}

const char* DebugModel::GetHungerText() const
{
	m_hungerText = QString::number( (int)m_hungerValue ).toStdString().c_str();
	return m_hungerText.Str();
}

const char* DebugModel::GetThirstText() const
{
	m_thirstText = QString::number( (int)m_thirstValue ).toStdString().c_str();
	return m_thirstText.Str();
}

const char* DebugModel::GetSleepText() const
{
	m_sleepText = QString::number( (int)m_sleepValue ).toStdString().c_str();
	return m_sleepText.Str();
}

// Need commands
void DebugModel::onSetHungerCmd( BaseComponent* )
{
	unsigned int gnomeID = m_selectedGnome ? m_selectedGnome->m_id : 0;
	m_proxy->setNeed( gnomeID, "Hunger", m_hungerValue );
}

void DebugModel::onSetThirstCmd( BaseComponent* )
{
	unsigned int gnomeID = m_selectedGnome ? m_selectedGnome->m_id : 0;
	m_proxy->setNeed( gnomeID, "Thirst", m_thirstValue );
}

void DebugModel::onSetSleepCmd( BaseComponent* )
{
	unsigned int gnomeID = m_selectedGnome ? m_selectedGnome->m_id : 0;
	m_proxy->setNeed( gnomeID, "Sleep", m_sleepValue );
}

void DebugModel::onKillGnomeCmd( BaseComponent* )
{
	if ( m_selectedGnome && m_selectedGnome->m_id != 0 )
	{
		m_proxy->killGnome( m_selectedGnome->m_id );
	}
}

// Spawn item
void DebugModel::onSpawnItemCmd( BaseComponent* )
{
	if ( !m_selectedItem || !m_selectedMaterial1 ) return;

	QString itemSID = QString::fromUtf8( m_selectedItem->m_name.Str() );
	int count = QString::fromUtf8( m_spawnCount.Str() ).toInt();
	int x = QString::fromUtf8( m_spawnX.Str() ).toInt();
	int y = QString::fromUtf8( m_spawnY.Str() ).toInt();
	int z = QString::fromUtf8( m_spawnZ.Str() ).toInt();
	if ( count < 1 ) count = 1;

	if ( m_componentCount >= 2 && m_selectedMaterial2 )
	{
		QStringList mats;
		mats.append( QString::fromUtf8( m_selectedMaterial1->m_name.Str() ) );
		mats.append( QString::fromUtf8( m_selectedMaterial2->m_name.Str() ) );
		m_proxy->spawnCompositeItem( itemSID, mats, count, x, y, z );
	}
	else
	{
		QString matSID = QString::fromUtf8( m_selectedMaterial1->m_name.Str() );
		m_proxy->spawnItem( itemSID, matSID, count, x, y, z );
	}
}

// Item dropdown accessors
Noesis::ObservableCollection<NameEntry>* DebugModel::getItemGroupList() const
{
	return m_itemGroupList;
}

void DebugModel::setSelectedItemGroup( NameEntry* item )
{
	if ( item && m_selectedItemGroup != item )
	{
		m_selectedItemGroup = item;
		m_proxy->requestItems( QString::fromUtf8( item->m_name.Str() ) );
	}
}

NameEntry* DebugModel::getSelectedItemGroup() const
{
	return m_selectedItemGroup;
}

Noesis::ObservableCollection<NameEntry>* DebugModel::getItemList() const
{
	return m_itemList;
}

void DebugModel::setSelectedItem( NameEntry* item )
{
	if ( item && m_selectedItem != item )
	{
		m_selectedItem = item;
		m_proxy->requestMaterials( QString::fromUtf8( item->m_name.Str() ) );
	}
}

NameEntry* DebugModel::getSelectedItem() const
{
	return m_selectedItem;
}

Noesis::ObservableCollection<NameEntry>* DebugModel::getMaterial1List() const
{
	return m_material1List;
}

void DebugModel::setSelectedMaterial1( NameEntry* item )
{
	m_selectedMaterial1 = item;
}

NameEntry* DebugModel::getSelectedMaterial1() const
{
	return m_selectedMaterial1;
}

Noesis::ObservableCollection<NameEntry>* DebugModel::getMaterial2List() const
{
	return m_material2List;
}

void DebugModel::setSelectedMaterial2( NameEntry* item )
{
	m_selectedMaterial2 = item;
}

NameEntry* DebugModel::getSelectedMaterial2() const
{
	return m_selectedMaterial2;
}

const char* DebugModel::GetShowMaterial2() const
{
	return m_componentCount >= 2 ? "Visible" : "Collapsed";
}

void DebugModel::updateItemGroups( const QStringList& groups )
{
	m_itemGroupList->Clear();
	for ( const auto& g : groups )
	{
		m_itemGroupList->Add( MakePtr<NameEntry>( g, 0 ) );
	}
}

void DebugModel::updateItems( const QStringList& items )
{
	m_itemList->Clear();
	m_selectedItem = nullptr;
	m_material1List->Clear();
	m_material2List->Clear();
	m_selectedMaterial1 = nullptr;
	m_selectedMaterial2 = nullptr;
	m_componentCount = 0;
	OnPropertyChanged( "ShowMaterial2" );

	for ( const auto& item : items )
	{
		m_itemList->Add( MakePtr<NameEntry>( item, 0 ) );
	}
}

void DebugModel::updateMaterials( int componentCount, const QStringList& mats1, const QStringList& mats2 )
{
	m_componentCount = componentCount;
	m_selectedMaterial1 = nullptr;
	m_selectedMaterial2 = nullptr;

	m_material1List->Clear();
	for ( const auto& m : mats1 )
	{
		m_material1List->Add( MakePtr<NameEntry>( m, 0 ) );
	}

	m_material2List->Clear();
	for ( const auto& m : mats2 )
	{
		m_material2List->Add( MakePtr<NameEntry>( m, 0 ) );
	}

	OnPropertyChanged( "ShowMaterial2" );
}

// Window size
Noesis::ObservableCollection<WSEntry>* DebugModel::getWindowSizes() const
{
    return m_windowSizes;
}

void DebugModel::setWindowSize( WSEntry* item )
{
    if( item && m_selectedWindowSize != item )
	{
        m_selectedWindowSize = item;
        m_proxy->setWindowSize( item->m_width, item->m_height );
	}
}

WSEntry* DebugModel::getWindowSize() const
{
    return m_selectedWindowSize;
}

// Game controls
void DebugModel::SetNeedDecayMultiplier( float v )
{
	m_needDecayMultiplier = v;
	m_proxy->setNeedDecayMultiplier( v );
	OnPropertyChanged( "NeedDecayText" );
}

const char* DebugModel::GetNeedDecayText() const
{
	m_needDecayText = QString::number( m_needDecayMultiplier, 'f', 1 ).toStdString().c_str();
	return m_needDecayText.Str();
}

void DebugModel::SetDisableSleepDecay( bool v )
{
	m_disableSleepDecay = v;
	m_proxy->setDisableNeedDecay( "Sleep", v );
}

void DebugModel::SetDisableHungerDecay( bool v )
{
	m_disableHungerDecay = v;
	m_proxy->setDisableNeedDecay( "Hunger", v );
}

void DebugModel::SetDisableThirstDecay( bool v )
{
	m_disableThirstDecay = v;
	m_proxy->setDisableNeedDecay( "Thirst", v );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION( DebugModel, "IngnomiaGUI.DebugModel" )
{
	NsProp( "PageCmd", &DebugModel::GetPageCmd );
	NsProp( "ShowGnomes", &DebugModel::GetShowGnomes );
	NsProp( "ShowItems", &DebugModel::GetShowItems );
	NsProp( "ShowGame", &DebugModel::GetShowGame );

	NsProp( "SpawnCmd", &DebugModel::GetSpawnCmd );

	NsProp( "GnomeList", &DebugModel::getGnomeList );
	NsProp( "SelectedGnome", &DebugModel::getSelectedGnome, &DebugModel::setSelectedGnome );

	NsProp( "HungerValue", &DebugModel::GetHungerValue, &DebugModel::SetHungerValue );
	NsProp( "ThirstValue", &DebugModel::GetThirstValue, &DebugModel::SetThirstValue );
	NsProp( "SleepValue", &DebugModel::GetSleepValue, &DebugModel::SetSleepValue );
	NsProp( "HungerText", &DebugModel::GetHungerText );
	NsProp( "ThirstText", &DebugModel::GetThirstText );
	NsProp( "SleepText", &DebugModel::GetSleepText );
	NsProp( "SetHungerCmd", &DebugModel::GetSetHungerCmd );
	NsProp( "SetThirstCmd", &DebugModel::GetSetThirstCmd );
	NsProp( "SetSleepCmd", &DebugModel::GetSetSleepCmd );

	NsProp( "KillGnomeCmd", &DebugModel::GetKillGnomeCmd );

	NsProp( "WindowSizes", &DebugModel::getWindowSizes );
	NsProp( "SelectedWindowSize", &DebugModel::getWindowSize, &DebugModel::setWindowSize );

	NsProp( "ItemGroupList", &DebugModel::getItemGroupList );
	NsProp( "SelectedItemGroup", &DebugModel::getSelectedItemGroup, &DebugModel::setSelectedItemGroup );
	NsProp( "ItemList", &DebugModel::getItemList );
	NsProp( "SelectedItem", &DebugModel::getSelectedItem, &DebugModel::setSelectedItem );
	NsProp( "Material1List", &DebugModel::getMaterial1List );
	NsProp( "SelectedMaterial1", &DebugModel::getSelectedMaterial1, &DebugModel::setSelectedMaterial1 );
	NsProp( "Material2List", &DebugModel::getMaterial2List );
	NsProp( "SelectedMaterial2", &DebugModel::getSelectedMaterial2, &DebugModel::setSelectedMaterial2 );
	NsProp( "ShowMaterial2", &DebugModel::GetShowMaterial2 );
	NsProp( "SpawnCount", &DebugModel::GetSpawnCount, &DebugModel::SetSpawnCount );
	NsProp( "SpawnX", &DebugModel::GetSpawnX, &DebugModel::SetSpawnX );
	NsProp( "SpawnY", &DebugModel::GetSpawnY, &DebugModel::SetSpawnY );
	NsProp( "SpawnZ", &DebugModel::GetSpawnZ, &DebugModel::SetSpawnZ );
	NsProp( "SpawnItemCmd", &DebugModel::GetSpawnItemCmd );

	NsProp( "NeedDecayMultiplier", &DebugModel::GetNeedDecayMultiplier, &DebugModel::SetNeedDecayMultiplier );
	NsProp( "NeedDecayText", &DebugModel::GetNeedDecayText );
	NsProp( "DisableSleepDecay", &DebugModel::GetDisableSleepDecay, &DebugModel::SetDisableSleepDecay );
	NsProp( "DisableHungerDecay", &DebugModel::GetDisableHungerDecay, &DebugModel::SetDisableHungerDecay );
	NsProp( "DisableThirstDecay", &DebugModel::GetDisableThirstDecay, &DebugModel::SetDisableThirstDecay );
}

NS_IMPLEMENT_REFLECTION( NameEntry )
{
	NsProp( "Name", &NameEntry::getName );
}

NS_IMPLEMENT_REFLECTION( WSEntry )
{
	NsProp( "Name", &WSEntry::getName );
}
