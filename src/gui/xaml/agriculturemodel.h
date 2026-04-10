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
/** @file agriculturemodel.h
 *  @brief Noesis view model and helper component types for the Agriculture XAML window.
 *         Owns observable collections for the plant/animal/tree pickers, the pasture food
 *         allow-list, and the per-pasture animal roster, and exposes Get*/Set* properties
 *         that XAML bindings hook into.
 */
#ifndef __AgricultureModel_H__
#define __AgricultureModel_H__

#include "../aggregatoragri.h"

#include <NsApp/DelegateCommand.h>
#include <NsApp/NotifyPropertyChangedBase.h>
#include <NsCore/Noesis.h>
#include <NsCore/Nullable.h>
#include <NsCore/Ptr.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/ReflectionDeclareEnum.h>
#include <NsCore/String.h>
#include <NsGui/Collection.h>
#include <NsGui/ImageSource.h>
#include <NsGui/BitmapSource.h>
#include <NsGui/BitmapImage.h>

#include <QString>

class AgricultureProxy;

namespace Noesis
{
template <class T>
class ObservableCollection;
}

namespace IngnomiaGUI
{

#pragma region AcPriority
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Single priority entry in the priority dropdown for an agricultural designation.
class AcPriority final : public Noesis::BaseComponent
{
public:
	AcPriority( const char* name );

	const char* GetName() const;

private:
	Noesis::String m_name;

	NS_DECLARE_REFLECTION( AcPriority, Noesis::BaseComponent )
};
#pragma endregion AcPriority

/// @brief One tree species entry in the grove product picker grid.
class TreeSelectEntry : public Noesis::BaseComponent
{
public:
	TreeSelectEntry( const GuiPlant& tree );

	const char* GetName() const;
	const char* GetSID() const;

private:
	Noesis::String m_name;
	Noesis::String m_sid;

	const Noesis::ImageSource* getBitmapSource() const
	{
		return m_bitmapSource;
	}
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapSource;

	NS_DECLARE_REFLECTION( TreeSelectEntry, Noesis::BaseComponent )
};

/// @brief A row of TreeSelectEntry items, used so the picker can be displayed as a grid of rows.
class TreeSelectRow : public Noesis::BaseComponent
{
public:
	TreeSelectRow( QList<GuiPlant> trees );

private:
	Noesis::ObservableCollection<TreeSelectEntry>* GetTrees() const;

	Noesis::Ptr<Noesis::ObservableCollection<TreeSelectEntry>> m_entries;

	NS_DECLARE_REFLECTION( TreeSelectRow, Noesis::BaseComponent )
};

/// @brief One animal species entry in the pasture product picker grid.
class AnimalSelectEntry : public Noesis::BaseComponent
{
public:
	AnimalSelectEntry( const GuiAnimal& Animal );

	const char* GetName() const;
	const char* GetSID() const;

private:
	Noesis::String m_name;
	Noesis::String m_sid;

	const Noesis::ImageSource* getBitmapSource() const
	{
		return m_bitmapSource;
	}
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapSource;

	NS_DECLARE_REFLECTION( AnimalSelectEntry, Noesis::BaseComponent )
};

/// @brief A row of AnimalSelectEntry items in the pasture product grid.
class AnimalSelectRow : public Noesis::BaseComponent
{
public:
	AnimalSelectRow( QList<GuiAnimal> Animals );

private:
	Noesis::ObservableCollection<AnimalSelectEntry>* GetAnimals() const;

	Noesis::Ptr<Noesis::ObservableCollection<AnimalSelectEntry>> m_entries;

	NS_DECLARE_REFLECTION( AnimalSelectRow, Noesis::BaseComponent )
};

/// @brief One animal currently in a pasture, with a butcher checkbox bound to the proxy.
class PastureAnimalEntry : public Noesis::BaseComponent
{
public:
	PastureAnimalEntry( const GuiPastureAnimal& Animal, AgricultureProxy* proxy );

	const char* GetName() const;
	bool GetButchering() const;
	void SetButchering( bool value );

private:
	Noesis::String m_name;
	Noesis::String m_sid;
	unsigned int m_id;
	bool m_toButcher = false;
	bool m_isYoung = false;
	Gender m_gender = Gender::UNDEFINED;

	AgricultureProxy* m_proxy = nullptr;

	NS_DECLARE_REFLECTION( PastureAnimalEntry, Noesis::BaseComponent )
};


/// @brief One crop entry in the farm product picker grid.
class PlantSelectEntry : public Noesis::BaseComponent
{
public:
	PlantSelectEntry( const GuiPlant& plant );

	const char* GetName() const;
	const char* GetSID() const;

private:
	Noesis::String m_name;
	Noesis::String m_sid;

	const Noesis::ImageSource* getBitmapSource() const
	{
		return m_bitmapSource;
	}
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapSource;

	NS_DECLARE_REFLECTION( PlantSelectEntry, Noesis::BaseComponent )
};

/// @brief A row of PlantSelectEntry items in the farm product grid.
class PlantSelectRow : public Noesis::BaseComponent
{
public:
	PlantSelectRow( QList<GuiPlant> plants );

private:
	Noesis::ObservableCollection<PlantSelectEntry>* GetPlants() const;

	Noesis::Ptr<Noesis::ObservableCollection<PlantSelectEntry>> m_entries;

	NS_DECLARE_REFLECTION( PlantSelectRow, Noesis::BaseComponent )
};



/// @brief One (item, material) row in the pasture food allow-list with a checkbox bound to the proxy.
class FoodSelectEntry : public Noesis::BaseComponent
{
public:
	FoodSelectEntry( const GuiPastureFoodItem& item, AgricultureProxy* proxy );

	const char* GetName() const;

	bool getChecked() const;
	void setChecked( bool value );

private:
	Noesis::String m_name;
	QString m_itemSID;
	QString m_materialSID;
	bool m_checked = false;

	AgricultureProxy* m_proxy = nullptr;

	const Noesis::ImageSource* getBitmapSource() const
	{
		return m_bitmapSource;
	}
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapSource;

	NS_DECLARE_REFLECTION( FoodSelectEntry, Noesis::BaseComponent )
};

/// @brief A row of FoodSelectEntry items in the pasture food allow-list grid.
class FoodSelectRow : public Noesis::BaseComponent
{
public:
	FoodSelectRow( QList<GuiPastureFoodItem> plants, AgricultureProxy* proxy );

private:
	Noesis::ObservableCollection<FoodSelectEntry>* GetFoods() const;

	Noesis::Ptr<Noesis::ObservableCollection<FoodSelectEntry>> m_entries;

	NS_DECLARE_REFLECTION( FoodSelectRow, Noesis::BaseComponent )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Top-level Agriculture window view model. Talks to the game side via AgricultureProxy
///        (which in turn forwards to AggregatorAgri) and exposes properties and DelegateCommands
///        consumed by Agriculture.xaml. Hosts the same data for all three designation kinds
///        (farm, pasture, grove) and switches the visible UI segment based on the active type.
class AgricultureModel final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	AgricultureModel();

	/// @brief Updates the common header fields (name, current product, priority, suspended) shared by all three designation kinds.
	void updateStandardInfo( unsigned int ID, AgriType type, QString name, QString product, int priority, int maxPriority, bool suspended );
	/// @brief Refreshes the farm-specific fields from a fresh GuiFarmInfo payload.
	void updateFarmInfo( const GuiFarmInfo& info );
	/// @brief Refreshes the pasture-specific fields (counts, hay/food levels, animal roster, food allow-list).
	void updatePastureInfo( const GuiPastureInfo& info );
	/// @brief Refreshes the grove-specific fields (planted trees, fell/pick/plant flags).
	void updateGroveInfo( const GuiGroveInfo& info );
	/// @brief Replaces the cached global plant catalogue used to populate the farm product grid.
	void updateGlobalPlantInfo( const QList<GuiPlant>& info );
	/// @brief Replaces the cached global animal catalogue used to populate the pasture product grid.
	void updateGlobalAnimalInfo( const QList<GuiAnimal>& info );
	/// @brief Replaces the cached global tree catalogue used to populate the grove product grid.
	void updateGlobalTreeInfo( const QList<GuiPlant>& info );

private:
	const char* GetName() const;
	const char* GetTitle() const;
	void SetName( const char* value );
	bool GetSuspended() const;
	void SetSuspended( bool value );
	bool GetHarvest() const;
	void SetHarvest( bool value );
	bool GetPlantTrees() const;
	void SetPlantTrees( bool value );
	bool GetFellTrees() const;
	void SetFellTrees( bool value );
	bool GetPickFruits() const;
	void SetPickFruits( bool value );
	bool GetHarvestHay() const;
	void SetHarvestHay( bool value );
	bool GetTame() const;
	void SetTame( bool value );

	const char* GetShowFarm() const;
	const char* GetShowPasture() const;
	const char* GetShowGrove() const;
	const char* GetShowPlantSelect() const;
	const char* GetShowAnimalSelect() const;
	const char* GetShowTreeSelect() const;
	const char* GetShowPastureFoodSelect() const;

	const char* GetTilled() const;
	const char* GetPlanted() const;
	const char* GetHarvestReady() const;

	const char* GetNumAnimals() const;
	const char* GetNumMale() const;
	const char* GetNumFemale() const;
	const char* GetMaxMale() const;
	const char* GetMaxFemale() const;
	void SetMaxMale( const char* value );
	void SetMaxFemale( const char* value );
	const char* GetManageAnimalsVisible() const;
	const char* GetManageWindowVis() const;

	const char* GetNumPlots() const;
	const char* GetNumSeeds() const;
	const char* GetNumItems() const;
	const char* GetNumPlants() const;

	const char* GetFoodStatus() const;
	const char* GetHayStatus() const;
	const char* GetMaxHay() const;
	void SetMaxHay( const char* value );

	Noesis::ObservableCollection<AcPriority>* GetPrios() const;
	void SetSelectedPriority( AcPriority* prio );
	AcPriority* GetSelectedPriority() const;
	Noesis::Ptr<Noesis::ObservableCollection<AcPriority>> m_prios;
	AcPriority* m_selectedPrio = nullptr;

	void onCmdSelectProduct( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetCmdSelectProduct() const
	{
		return &m_cmdSelectProduct;
	}
	NoesisApp::DelegateCommand m_cmdSelectProduct;

	void onSelectProduct( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetSelectProduct() const
	{
		return &m_selectProduct;
	}
	NoesisApp::DelegateCommand m_selectProduct;

	void onManageAnimals( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetManageAnimals() const
	{
		return &m_manageAnimalsCmd;
	}
	NoesisApp::DelegateCommand m_manageAnimalsCmd;

	void onManageAnimalsBack( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetManageAnimalsBack() const
	{
		return &m_manageAnimalsBackCmd;
	}
	NoesisApp::DelegateCommand m_manageAnimalsBackCmd;

	void onShowPastureFood( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetShowPastureFoodCmd() const
	{
		return &m_showPastureFoodCmd;
	}
	NoesisApp::DelegateCommand m_showPastureFoodCmd;

	Noesis::ObservableCollection<PlantSelectRow>* GetPlants() const
	{
		return m_plantRows;
	}
	Noesis::Ptr<Noesis::ObservableCollection<PlantSelectRow>> m_plantRows;

	Noesis::ObservableCollection<AnimalSelectRow>* GetAnimals() const
	{
		return m_animalRows;
	}
	Noesis::Ptr<Noesis::ObservableCollection<AnimalSelectRow>> m_animalRows;

	Noesis::ObservableCollection<TreeSelectRow>* GetTrees() const
	{
		return m_treeRows;
	}
	Noesis::Ptr<Noesis::ObservableCollection<TreeSelectRow>> m_treeRows;

	Noesis::ObservableCollection<PastureAnimalEntry>* GetPastureAnimals() const
	{
		return m_pastureAnimals;
	}
	Noesis::Ptr<Noesis::ObservableCollection<PastureAnimalEntry>> m_pastureAnimals;

	Noesis::ObservableCollection<FoodSelectRow>* GetFoods() const
	{
		return m_foodRows;
	}
	Noesis::Ptr<Noesis::ObservableCollection<FoodSelectRow>> m_foodRows;

	AgricultureProxy* m_proxy = nullptr;
	bool m_blockSignals = false;

	void setBasicOptions();

	unsigned int m_id           = 0;
	Noesis::String m_name       = "-Farm-";
	Noesis::String m_title      = "Farm";
	bool m_suspended            = false;
	bool m_harvest              = false;
	bool m_harvestHay           = false;
	bool m_tameWild             = false;
	Noesis::String m_tilled     = "0/0";
	Noesis::String m_planted    = "0/0";
	Noesis::String m_ready      = "0/0";
	Noesis::String m_numPlots   = "0";
	Noesis::String m_numSeeds   = "0";
	Noesis::String m_numItems   = "0";
	Noesis::String m_numPlants  = "0";
	Noesis::String m_numAnimals = "0/0";
	Noesis::String m_numMale    = "0/";
	Noesis::String m_numFemale  = "0/";
	int	m_maxAnimals = 0;
	Noesis::String m_maxMale    = "0";
	Noesis::String m_maxFemale    = "0";
	bool m_plantTrees           = true;
	bool m_fellTrees            = false;
	bool m_pickFruits           = true;
	Noesis::String m_foodStatus = "0/0";
	Noesis::String m_hayStatus  = "0/";
	Noesis::String m_maxHay     = "100";


	bool m_productSelect = false;
	bool m_manageWindow = false;
	bool m_pastureFoodSelect = false;

	QString m_gui;

	AgriType m_type = AgriType::Farm;

	QString m_productID;

	const Noesis::ImageSource* getBitmapSource() const
	{
		return m_bitmapSource;
	}
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapSource;

	NS_DECLARE_REFLECTION( AgricultureModel, NotifyPropertyChangedBase )
};

} // namespace IngnomiaGUI

#endif
