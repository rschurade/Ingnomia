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
/** @file debugmodel.h
 *  @brief Noesis view model for the 3-page Debug GUI: Gnomes (spawn, kill, set needs),
 *         Items (spawn at coordinates), Game (need-decay tuning, window size).
 */
#ifndef __DebugModel_H__
#define __DebugModel_H__

#include "../aggregatordebug.h"

#include <QString>

#include <NsApp/DelegateCommand.h>
#include <NsApp/NotifyPropertyChangedBase.h>
#include <NsCore/Noesis.h>
#include <NsCore/Nullable.h>
#include <NsCore/Ptr.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/ReflectionDeclareEnum.h>
#include <NsCore/String.h>
#include <NsGui/Collection.h>

class DebugProxy;

namespace Noesis
{
template <class T>
class ObservableCollection;
}

namespace IngnomiaGUI
{

/// @brief Which tab of the Debug GUI is currently visible.
enum class DebugPage
{
	Gnomes,  ///< Gnome controls (spawn, kill, set needs).
	Items,   ///< Item spawning by group/item/material.
	Game     ///< Game-tuning controls (need decay, window size).
};

/// @brief Generic (name, id) row used in dropdown lists across the Debug GUI.
struct NameEntry : public Noesis::BaseComponent
{
public:
	NameEntry( const QString& name, unsigned int id );

	Noesis::String m_name;
	unsigned int m_id;

    const char* getName() const;

	NS_DECLARE_REFLECTION( NameEntry, BaseComponent )
};

/// @brief Window-size dropdown entry holding a width × height preset.
struct WSEntry : public Noesis::BaseComponent
{
public:
	WSEntry( int width, int height );

	Noesis::String m_name;
	int m_width;
	int m_height;

    const char* getName() const;

	NS_DECLARE_REFLECTION( WSEntry, BaseComponent )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Debug GUI view model. Talks to the game side via DebugProxy and exposes the
///        per-page properties (gnome list, item dropdowns, need sliders, decay settings).
class DebugModel final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	DebugModel();

	/// @brief Asks the proxy to refresh the gnome dropdown list.
	void updateGnomeList();
	Noesis::ObservableCollection<NameEntry>* getGnomeList() const;

	/// @brief Replaces the item-group dropdown with @p groups.
	void updateItemGroups( const QStringList& groups );
	/// @brief Replaces the item dropdown with @p items (filtered to the current group).
	void updateItems( const QStringList& items );
	/// @brief Replaces the material dropdowns. @p componentCount selects whether one or two
	///        material dropdowns are visible.
	void updateMaterials( int componentCount, const QStringList& mats1, const QStringList& mats2 );

private:
	DebugProxy* m_proxy = nullptr;

	DebugPage m_page = DebugPage::Gnomes;

	// Page visibility
	const char* GetShowGnomes() const;
	const char* GetShowItems() const;
	const char* GetShowGame() const;

	// Page command
	void onPageCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetPageCmd() const { return &m_pageCmd; }
	NoesisApp::DelegateCommand m_pageCmd;

	// Spawn creature command
	void onSpawnCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetSpawnCmd() const { return &m_spawnCmd; }
	NoesisApp::DelegateCommand m_spawnCmd;

	// Gnome selector
	void setSelectedGnome( NameEntry* item );
	NameEntry* getSelectedGnome() const;
	Noesis::Ptr<Noesis::ObservableCollection<NameEntry>> m_gnomeList;
	NameEntry* m_selectedGnome = nullptr;

	// Need values
	float m_hungerValue = 100.0f;
	float m_thirstValue = 100.0f;
	float m_sleepValue  = 100.0f;

	float GetHungerValue() const { return m_hungerValue; }
	void SetHungerValue( float v ) { m_hungerValue = v; OnPropertyChanged( "HungerText" ); }
	float GetThirstValue() const { return m_thirstValue; }
	void SetThirstValue( float v ) { m_thirstValue = v; OnPropertyChanged( "ThirstText" ); }
	float GetSleepValue() const { return m_sleepValue; }
	void SetSleepValue( float v ) { m_sleepValue = v; OnPropertyChanged( "SleepText" ); }

	const char* GetHungerText() const;
	const char* GetThirstText() const;
	const char* GetSleepText() const;
	mutable Noesis::String m_hungerText;
	mutable Noesis::String m_thirstText;
	mutable Noesis::String m_sleepText;

	void onSetHungerCmd( BaseComponent* param );
	void onSetThirstCmd( BaseComponent* param );
	void onSetSleepCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetSetHungerCmd() const { return &m_setHungerCmd; }
	const NoesisApp::DelegateCommand* GetSetThirstCmd() const { return &m_setThirstCmd; }
	const NoesisApp::DelegateCommand* GetSetSleepCmd() const { return &m_setSleepCmd; }
	NoesisApp::DelegateCommand m_setHungerCmd;
	NoesisApp::DelegateCommand m_setThirstCmd;
	NoesisApp::DelegateCommand m_setSleepCmd;

	// Kill gnome
	void onKillGnomeCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetKillGnomeCmd() const { return &m_killGnomeCmd; }
	NoesisApp::DelegateCommand m_killGnomeCmd;

	// Window size
	Noesis::ObservableCollection<WSEntry>* getWindowSizes() const;
	void setWindowSize( WSEntry* item );
	WSEntry* getWindowSize() const;
	Noesis::Ptr<Noesis::ObservableCollection<WSEntry>> m_windowSizes;
    WSEntry* m_selectedWindowSize = nullptr;

	// Spawn item — dropdowns
	Noesis::Ptr<Noesis::ObservableCollection<NameEntry>> m_itemGroupList;
	Noesis::Ptr<Noesis::ObservableCollection<NameEntry>> m_itemList;
	Noesis::Ptr<Noesis::ObservableCollection<NameEntry>> m_material1List;
	Noesis::Ptr<Noesis::ObservableCollection<NameEntry>> m_material2List;

	NameEntry* m_selectedItemGroup = nullptr;
	NameEntry* m_selectedItem = nullptr;
	NameEntry* m_selectedMaterial1 = nullptr;
	NameEntry* m_selectedMaterial2 = nullptr;
	int m_componentCount = 0;

	Noesis::ObservableCollection<NameEntry>* getItemGroupList() const;
	void setSelectedItemGroup( NameEntry* item );
	NameEntry* getSelectedItemGroup() const;

	Noesis::ObservableCollection<NameEntry>* getItemList() const;
	void setSelectedItem( NameEntry* item );
	NameEntry* getSelectedItem() const;

	Noesis::ObservableCollection<NameEntry>* getMaterial1List() const;
	void setSelectedMaterial1( NameEntry* item );
	NameEntry* getSelectedMaterial1() const;

	Noesis::ObservableCollection<NameEntry>* getMaterial2List() const;
	void setSelectedMaterial2( NameEntry* item );
	NameEntry* getSelectedMaterial2() const;

	const char* GetShowMaterial2() const;

	Noesis::String m_spawnCount;
	Noesis::String m_spawnX;
	Noesis::String m_spawnY;
	Noesis::String m_spawnZ;

	const char* GetSpawnCount() const { return m_spawnCount.Str(); }
	void SetSpawnCount( const char* v ) { m_spawnCount = v; }
	const char* GetSpawnX() const { return m_spawnX.Str(); }
	void SetSpawnX( const char* v ) { m_spawnX = v; }
	const char* GetSpawnY() const { return m_spawnY.Str(); }
	void SetSpawnY( const char* v ) { m_spawnY = v; }
	const char* GetSpawnZ() const { return m_spawnZ.Str(); }
	void SetSpawnZ( const char* v ) { m_spawnZ = v; }

	void onSpawnItemCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetSpawnItemCmd() const { return &m_spawnItemCmd; }
	NoesisApp::DelegateCommand m_spawnItemCmd;

	// Game controls
	float m_needDecayMultiplier = 1.0f;
	bool m_disableSleepDecay  = false;
	bool m_disableHungerDecay = false;
	bool m_disableThirstDecay = false;

	float GetNeedDecayMultiplier() const { return m_needDecayMultiplier; }
	void SetNeedDecayMultiplier( float v );
	const char* GetNeedDecayText() const;
	mutable Noesis::String m_needDecayText;
	bool GetDisableSleepDecay() const { return m_disableSleepDecay; }
	void SetDisableSleepDecay( bool v );
	bool GetDisableHungerDecay() const { return m_disableHungerDecay; }
	void SetDisableHungerDecay( bool v );
	bool GetDisableThirstDecay() const { return m_disableThirstDecay; }
	void SetDisableThirstDecay( bool v );

	NS_DECLARE_REFLECTION( DebugModel, NotifyPropertyChangedBase )
};

} // namespace IngnomiaGUI

#endif
