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

class TreeSelectRow : public Noesis::BaseComponent
{
public:
	TreeSelectRow( QList<GuiPlant> trees );

private:
	Noesis::ObservableCollection<TreeSelectEntry>* GetTrees() const;

	Noesis::Ptr<Noesis::ObservableCollection<TreeSelectEntry>> m_entries;

	NS_DECLARE_REFLECTION( TreeSelectRow, Noesis::BaseComponent )
};

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

class AnimalSelectRow : public Noesis::BaseComponent
{
public:
	AnimalSelectRow( QList<GuiAnimal> Animals );

private:
	Noesis::ObservableCollection<AnimalSelectEntry>* GetAnimals() const;

	Noesis::Ptr<Noesis::ObservableCollection<AnimalSelectEntry>> m_entries;

	NS_DECLARE_REFLECTION( AnimalSelectRow, Noesis::BaseComponent )
};

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

class PlantSelectRow : public Noesis::BaseComponent
{
public:
	PlantSelectRow( QList<GuiPlant> plants );

private:
	Noesis::ObservableCollection<PlantSelectEntry>* GetPlants() const;

	Noesis::Ptr<Noesis::ObservableCollection<PlantSelectEntry>> m_entries;

	NS_DECLARE_REFLECTION( PlantSelectRow, Noesis::BaseComponent )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class AgricultureModel final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	AgricultureModel();

	void updateStandardInfo( unsigned int ID, AgriType type, QString name, QString product, int priority, int maxPriority, bool suspended );
	void updateFarmInfo( const GuiFarmInfo& info );
	void updatePastureInfo( const GuiPastureInfo& info );
	void updateGroveInfo( const GuiGroveInfo& info );
	void updateGlobalPlantInfo( const QList<GuiPlant>& info );
	void updateGlobalAnimalInfo( const QList<GuiAnimal>& info );
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

	const char* GetNumSeeds() const;
	const char* GetNumItems() const;
	const char* GetNumPlants() const;

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

	AgricultureProxy* m_proxy = nullptr;

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
	
	bool m_productSelect = false;
	bool m_manageWindow = false;

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
