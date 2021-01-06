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
#include "agriculturemodel.h"

#include "../../base/util.h"
#include "agricultureproxy.h"
#include "../strings.h"

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

#pragma region AcPriority
////////////////////////////////////////////////////////////////////////////////////////////////////
AcPriority::AcPriority( const char* name ) :
	m_name( name )
{
}

const char* AcPriority::GetName() const
{
	return m_name.Str();
}
#pragma endregion AcPriority

#pragma region Trees
////////////////////////////////////////////////////////////////////////////////////////////////////
TreeSelectEntry::TreeSelectEntry( const GuiPlant& tree )
{
	m_name = tree.name.toStdString().c_str();
	m_sid  = tree.plantID.toStdString().c_str();

	std::vector<unsigned char> buffer;
	Global::util->createBufferForNoesisImage( tree.sprite, buffer );
	m_bitmapSource = BitmapImage::Create( tree.sprite.width(), tree.sprite.height(), 96, 96, buffer.data(), tree.sprite.width() * 4, BitmapSource::Format::Format_RGBA8 );
}

const char* TreeSelectEntry::GetName() const
{
	return m_name.Str();
}

const char* TreeSelectEntry::GetSID() const
{
	return m_sid.Str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
TreeSelectRow::TreeSelectRow( QList<GuiPlant> Trees )
{
	m_entries = *new ObservableCollection<TreeSelectEntry>();

	for ( auto Tree : Trees )
	{
		m_entries->Add( MakePtr<TreeSelectEntry>( Tree ) );
	}
}

Noesis::ObservableCollection<TreeSelectEntry>* TreeSelectRow::GetTrees() const
{
	return m_entries;
}
#pragma endregion Trees

#pragma region Animals
////////////////////////////////////////////////////////////////////////////////////////////////////
AnimalSelectEntry::AnimalSelectEntry( const GuiAnimal& animal )
{
	m_name = animal.name.toStdString().c_str();
	m_sid  = animal.animalID.toStdString().c_str();

	std::vector<unsigned char> buffer;
	Global::util->createBufferForNoesisImage( animal.sprite, buffer );

	m_bitmapSource = BitmapImage::Create( animal.sprite.width(), animal.sprite.height(), 96, 96, buffer.data(), animal.sprite.width() * 4, BitmapSource::Format::Format_RGBA8 );
}

const char* AnimalSelectEntry::GetName() const
{
	return m_name.Str();
}

const char* AnimalSelectEntry::GetSID() const
{
	return m_sid.Str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AnimalSelectRow::AnimalSelectRow( QList<GuiAnimal> animals )
{
	m_entries = *new ObservableCollection<AnimalSelectEntry>();

	for ( auto animal : animals )
	{
		m_entries->Add( MakePtr<AnimalSelectEntry>( animal ) );
	}
}

Noesis::ObservableCollection<AnimalSelectEntry>* AnimalSelectRow::GetAnimals() const
{
	return m_entries;
}
#pragma endregion Animals

#pragma region Plants
////////////////////////////////////////////////////////////////////////////////////////////////////
PlantSelectEntry::PlantSelectEntry( const GuiPlant& plant )
{
	m_name = plant.name.toStdString().c_str();
	m_sid  = plant.plantID.toStdString().c_str();

	std::vector<unsigned char> buffer;
	Global::util->createBufferForNoesisImage( plant.sprite, buffer );
	m_bitmapSource = BitmapImage::Create( plant.sprite.width(), plant.sprite.height(), 96, 96, buffer.data(), plant.sprite.width() * 4, BitmapSource::Format::Format_RGBA8 );
}

const char* PlantSelectEntry::GetName() const
{
	return m_name.Str();
}

const char* PlantSelectEntry::GetSID() const
{
	return m_sid.Str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
PlantSelectRow::PlantSelectRow( QList<GuiPlant> plants )
{
	m_entries = *new ObservableCollection<PlantSelectEntry>();

	for ( auto plant : plants )
	{
		m_entries->Add( MakePtr<PlantSelectEntry>( plant ) );
	}
}

Noesis::ObservableCollection<PlantSelectEntry>* PlantSelectRow::GetPlants() const
{
	return m_entries;
}
#pragma endregion Plants


#pragma region Pasture
////////////////////////////////////////////////////////////////////////////////////////////////////
PastureAnimalEntry::PastureAnimalEntry( const GuiPastureAnimal& animal, AgricultureProxy* proxy ) :
	m_proxy( proxy )
{
	
	m_sid       = animal.animalID.toStdString().c_str();
	m_id        = animal.id;
	m_toButcher = animal.toButcher;
	m_isYoung = animal.isYoung;
	m_gender = animal.gender;

	QString qName;
	if( m_gender == Gender::MALE )
	{
		qName += "Male ";
	}
	else if( m_gender == Gender::FEMALE )
	{
		qName += "Female ";
	}
	qName += animal.name;
	if( m_isYoung )
	{
		qName += " (young)";
	}
	m_name      = qName.toStdString().c_str();
}

const char* PastureAnimalEntry::GetName() const
{
	return m_name.Str();
}

bool PastureAnimalEntry::GetButchering() const
{
	return m_toButcher;
}

void PastureAnimalEntry::SetButchering( bool value )
{
	if ( m_toButcher != value )
	{
		m_toButcher = value;
		if( m_proxy )
		{
			m_proxy->setButchering( m_id, value );
		}
	}
}
#pragma endregion Pasture

#pragma region PastureFood
////////////////////////////////////////////////////////////////////////////////////////////////////
FoodSelectEntry::FoodSelectEntry( const GuiPastureFoodItem& food, AgricultureProxy* proxy ) :
	m_proxy( proxy )
{
	m_name = food.name.toStdString().c_str();
	
	m_itemSID  = food.itemSID;
	m_materialSID = food.materialSID;
	m_checked = food.checked;

	std::vector<unsigned char> buffer;
	Global::util->createBufferForNoesisImage( food.sprite, buffer );
	m_bitmapSource = BitmapImage::Create( food.sprite.width(), food.sprite.height(), 96, 96, buffer.data(), food.sprite.width() * 4, BitmapSource::Format::Format_RGBA8 );
}

const char* FoodSelectEntry::GetName() const
{
	return m_name.Str();
}

bool FoodSelectEntry::getChecked() const
{
	return m_checked;
}
	
void FoodSelectEntry::setChecked( bool value )
{
	if( m_checked != value )
	{
		m_checked = value;
		m_proxy->setFoodItemChecked( m_itemSID, m_materialSID, m_checked );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
FoodSelectRow::FoodSelectRow( QList<GuiPastureFoodItem> foods, AgricultureProxy* proxy )
{
	m_entries = *new ObservableCollection<FoodSelectEntry>();

	for ( auto food : foods )
	{
		m_entries->Add( MakePtr<FoodSelectEntry>( food, proxy ) );
	}
}

Noesis::ObservableCollection<FoodSelectEntry>* FoodSelectRow::GetFoods() const
{
	return m_entries;
}
#pragma endregion PastureFood


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
AgricultureModel::AgricultureModel()
{
	m_proxy = new AgricultureProxy;
	m_proxy->setParent( this );

	m_prios = *new ObservableCollection<AcPriority>();
	for ( int i = 0; i < 2; ++i )
	{
		m_prios->Add( MakePtr<AcPriority>( ( QString( "Priority " ) + QString::number( i + 1 ) ).toStdString().c_str() ) );
	}
	m_selectedPrio = m_prios->Get( 0 );

	m_plantRows      = *new ObservableCollection<PlantSelectRow>();
	m_animalRows     = *new ObservableCollection<AnimalSelectRow>();
	m_treeRows       = *new ObservableCollection<TreeSelectRow>();
	m_pastureAnimals = *new ObservableCollection<PastureAnimalEntry>();
	m_foodRows       = *new ObservableCollection<FoodSelectRow>();

	m_cmdSelectProduct.SetExecuteFunc( MakeDelegate( this, &AgricultureModel::onCmdSelectProduct ) );
	m_selectProduct.SetExecuteFunc( MakeDelegate( this, &AgricultureModel::onSelectProduct ) );
	m_manageAnimalsCmd.SetExecuteFunc( MakeDelegate( this, &AgricultureModel::onManageAnimals ) );
	m_manageAnimalsBackCmd.SetExecuteFunc( MakeDelegate( this, &AgricultureModel::onManageAnimalsBack ) );
	m_showPastureFoodCmd.SetExecuteFunc( MakeDelegate( this, &AgricultureModel::onShowPastureFood ) );
}

void AgricultureModel::updateGlobalPlantInfo( const QList<GuiPlant>& info )
{
	m_plantRows->Clear();
	int plantDone = 0;
	while ( plantDone < info.size() )
	{
		QList<GuiPlant> plantRow;
		while ( plantRow.size() < 8 && plantDone < info.size() )
		{
			plantRow.append( info[plantDone] );
			++plantDone;
		}
		m_plantRows->Add( MakePtr<PlantSelectRow>( plantRow ) );
	}

	OnPropertyChanged( "PlantRows" );
}

void AgricultureModel::updateGlobalAnimalInfo( const QList<GuiAnimal>& info )
{
	m_animalRows->Clear();
	int animalDone = 0;
	while ( animalDone < info.size() )
	{
		QList<GuiAnimal> animalRow;
		while ( animalRow.size() < 8 && animalDone < info.size() )
		{
			animalRow.append( info[animalDone] );
			++animalDone;
		}
		m_animalRows->Add( MakePtr<AnimalSelectRow>( animalRow ) );
	}

	OnPropertyChanged( "AnimalRows" );
}

void AgricultureModel::updateGlobalTreeInfo( const QList<GuiPlant>& info )
{
	m_treeRows->Clear();
	int plantDone = 0;
	while ( plantDone < info.size() )
	{
		QList<GuiPlant> treeRow;
		while ( treeRow.size() < 8 && plantDone < info.size() )
		{
			treeRow.append( info[plantDone] );
			++plantDone;
		}
		m_treeRows->Add( MakePtr<TreeSelectRow>( treeRow ) );
	}

	OnPropertyChanged( "TreeRows" );
}

void AgricultureModel::updateStandardInfo( unsigned int ID, AgriType type, QString name, QString product, int priority, int maxPriority, bool suspended )
{
	m_blockSignals = true;
	bool isSame = ( m_id == ID );
	m_id        = ID;
	m_type      = type;

	if ( m_name != name.toStdString().c_str() )
	{
		m_name = name.toStdString().c_str();
		OnPropertyChanged( "Name" );
	}
	if ( m_suspended != suspended )
	{
		m_suspended = suspended;
		OnPropertyChanged( "Suspended" );
	}

	if ( m_prios->Count() != maxPriority )
	{
		m_prios->Clear();
		for ( int i = 0; i < maxPriority; ++i )
		{
			m_prios->Add( MakePtr<AcPriority>( ( QString( "Priority " ) + QString::number( i + 1 ) ).toStdString().c_str() ) );
		}
		OnPropertyChanged( "Priorities" );
	}
	auto newPrio = m_prios->Get( qMin( qMax( 0, priority ), m_prios->Count() ) );
	if ( m_selectedPrio != newPrio )
	{
		m_selectedPrio = newPrio;
		OnPropertyChanged( "SelectedPrio" );
	}

	if ( m_productID != product || product.isEmpty() )
	{
		m_productID = product;

		if ( !m_productID.isEmpty() )
		{
			m_title = ( name + " (" + S::s( "$MaterialName_" + m_productID ) + ")" ).toStdString().c_str();
		}
		else
		{
			m_title = m_name;
		}
		OnPropertyChanged( "Title" );
	}
	if ( !isSame )
	{
		m_manageWindow = false;
		m_productSelect = false;

		m_pastureAnimals->Clear();
		OnPropertyChanged( "PastureAnimals" );

		OnPropertyChanged( "ShowFarm" );
		OnPropertyChanged( "ShowPasture" );
		OnPropertyChanged( "ShowGrove" );

		OnPropertyChanged( "ShowPlantSelect" );
		OnPropertyChanged( "ShowAnimalSelect" );
		OnPropertyChanged( "ShowTreeSelect" );
		OnPropertyChanged( "ShowPastureFoodSelect" );

		OnPropertyChanged( "ShowManageWindow" );
	}
	m_blockSignals = false;
}

void AgricultureModel::updateFarmInfo( const GuiFarmInfo& info )
{
	updateStandardInfo( info.ID, AgriType::Farm, info.name, info.plantType, info.priority, info.maxPriority, info.suspended );

	if ( m_harvest != info.harvest )
	{
		m_harvest = info.harvest;
		OnPropertyChanged( "Harvest" );
	}

	m_tilled  = ( QString::number( info.tilled ) + "/" + QString::number( info.numPlots ) ).toStdString().c_str();
	m_planted = ( QString::number( info.planted ) + "/" + QString::number( info.numPlots ) ).toStdString().c_str();
	m_ready   = ( QString::number( info.cropReady ) + "/" + QString::number( info.numPlots ) ).toStdString().c_str();

	OnPropertyChanged( "Tilled" );
	OnPropertyChanged( "Planted" );
	OnPropertyChanged( "HarvestReady" );

	m_bitmapSource = nullptr;
	if ( !info.product.plantID.isEmpty() )
	{
		std::vector<unsigned char> buffer;
		Global::util->createBufferForNoesisImage( info.product.sprite, buffer );
		m_bitmapSource = BitmapImage::Create( info.product.sprite.width(), info.product.sprite.height(), 96, 96, buffer.data(), info.product.sprite.width() * 4, BitmapSource::Format::Format_RGBA8 );

		m_title = ( QString( m_name.Str() ) + " (" + S::s( "$MaterialName_" + info.product.plantID ) + ")" ).toStdString().c_str();
	}
	else
	{
		m_title = m_name;
	}
	OnPropertyChanged( "Image" );
	OnPropertyChanged( "Title" );

	m_numSeeds  = QString::number( info.product.seedCount ).toStdString().c_str();
	m_numItems  = QString::number( info.product.itemCount ).toStdString().c_str();
	m_numPlants = QString::number( info.product.plantCount ).toStdString().c_str();

	OnPropertyChanged( "NumSeeds" );
	OnPropertyChanged( "NumItems" );
	OnPropertyChanged( "NumPlants" );
}

void AgricultureModel::updatePastureInfo( const GuiPastureInfo& info )
{
	updateStandardInfo( info.ID, AgriType::Pasture, info.name, info.animalType, info.priority, info.maxPriority, info.suspended );

	if ( m_harvest != info.harvest )
	{
		m_harvest = info.harvest;
		OnPropertyChanged( "Harvest" );
	}
	if ( m_harvestHay != info.harvestHay )
	{
		m_harvestHay = info.harvestHay;
		OnPropertyChanged( "HarvestHay" );
	}

	m_maxAnimals = info.maxNumber;
	m_numAnimals = ( QString::number( info.numMale + info.numFemale ) + "/" + QString::number( info.maxNumber ) ).toStdString().c_str();
	m_numMale    = ( QString::number( info.numMale ) + "/" ).toStdString().c_str();
	m_numFemale  = ( QString::number( info.numFemale ) + "/" ).toStdString().c_str();
	m_maxMale    = ( QString::number( info.maxMale ) ).toStdString().c_str();
	m_maxFemale  = ( QString::number( info.maxFemale ) ).toStdString().c_str();
	m_foodStatus = ( QString::number( info.foodCurrent ) + "/" + QString::number( info.foodMax ) ).toStdString().c_str();
	m_hayStatus  = ( QString::number( info.hayCurrent ) + "/" ).toStdString().c_str();
	m_maxHay     = ( QString::number( info.hayMax ) ).toStdString().c_str();


	m_pastureAnimals->Clear();

	for ( const auto& pa : info.animals )
	{
		m_pastureAnimals->Add( MakePtr<PastureAnimalEntry>( pa, m_proxy ) );
	}

	OnPropertyChanged( "NumAnimals" );
	OnPropertyChanged( "NumMale" );
	OnPropertyChanged( "NumFemale" );
	OnPropertyChanged( "MaxMale" );
	OnPropertyChanged( "MaxFemale" );
	OnPropertyChanged( "VisManageAnimals" );
	OnPropertyChanged( "PastureAnimals" );
	OnPropertyChanged( "ShowManageWindow" );
	OnPropertyChanged( "MaxHay" );
	OnPropertyChanged( "HayStatus" );
	OnPropertyChanged( "FoodStatus" );

	m_bitmapSource = nullptr;
	if ( !info.product.animalID.isEmpty() )
	{
		std::vector<unsigned char> buffer;
		Global::util->createBufferForNoesisImage( info.product.sprite, buffer );
		m_bitmapSource = BitmapImage::Create( info.product.sprite.width(), info.product.sprite.height(), 96, 96, buffer.data(), info.product.sprite.width() * 4, BitmapSource::Format::Format_RGBA8 );

		m_title = ( QString( m_name.Str() ) + " (" + S::s( "$CreatureName_" + info.product.animalID ) + ")" ).toStdString().c_str();
	}
	else
	{
		m_title = m_name;
	}
	OnPropertyChanged( "Image" );
	OnPropertyChanged( "Title" );

	m_foodRows->Clear();
	if( !info.food.isEmpty() )
	{	
		int foodDone = 0;
		while ( foodDone < info.food.size() )
		{
			QList<GuiPastureFoodItem> foodRow;
			while ( foodRow.size() < 8 && foodDone < info.food.size() )
			{
				foodRow.append( info.food[foodDone] );
				++foodDone;
			}
			m_foodRows->Add( MakePtr<FoodSelectRow>( foodRow, m_proxy ) );
		}
	}
	OnPropertyChanged( "FoodRows" );
}

void AgricultureModel::updateGroveInfo( const GuiGroveInfo& info )
{
	updateStandardInfo( info.ID, AgriType::Grove, info.name, info.treeType, info.priority, info.maxPriority, info.suspended );

	m_plantTrees = info.plantTrees;
	m_fellTrees  = info.fellTrees;
	m_pickFruits = info.pickFruits;
	m_numPlants  = ( "Planted trees: " + QString::number( info.planted ) ).toStdString().c_str();
	m_numPlots   = ( "Num plots: " + QString::number( info.numPlots ) ).toStdString().c_str();

	OnPropertyChanged( "PlantTrees" );
	OnPropertyChanged( "FellTrees" );
	OnPropertyChanged( "PickFruits" );
	OnPropertyChanged( "NumPlants" );
	OnPropertyChanged( "NumPlots" );

	m_bitmapSource = nullptr;
	if ( !info.product.plantID.isEmpty() )
	{
		std::vector<unsigned char> buffer;
		Global::util->createBufferForNoesisImage( info.product.sprite, buffer );

		m_bitmapSource = BitmapImage::Create( info.product.sprite.width(), info.product.sprite.height(), 96, 96, buffer.data(), info.product.sprite.width() * 4, BitmapSource::Format::Format_RGBA8 );

		m_title = ( QString( m_name.Str() ) + " (" + S::s( "$ItemName_" + info.product.plantID ) + ")" ).toStdString().c_str();
	}
	else
	{
		m_title = m_name;
	}
	OnPropertyChanged( "Image" );
	OnPropertyChanged( "Title" );
}

void AgricultureModel::setBasicOptions()
{
	if( !m_blockSignals )
	{
		m_proxy->setBasicOptions( m_id, m_name.Str(), m_prios->IndexOf( m_selectedPrio ), m_suspended );
	}
}

const char* AgricultureModel::GetName() const
{
	return m_name.Str();
}

const char* AgricultureModel::GetTitle() const
{
	return m_title.Str();
}

void AgricultureModel::SetName( const char* value )
{
	m_name = value;
	setBasicOptions();
	OnPropertyChanged( "Name" );
}

bool AgricultureModel::GetSuspended() const
{
	return m_suspended;
}

void AgricultureModel::SetSuspended( bool value )
{
	if ( m_suspended != value )
	{
		m_suspended = value;
		setBasicOptions();
		OnPropertyChanged( "Suspended" );
	}
}

bool AgricultureModel::GetHarvest() const
{
	return m_harvest;
}

void AgricultureModel::SetHarvest( bool value )
{
	if ( m_harvest != value )
	{
		m_harvest = value;
		m_proxy->setHarvestOptions( m_id, m_harvest, m_harvestHay, m_tameWild );
		OnPropertyChanged( "Harvest" );
	}
}

bool AgricultureModel::GetTame() const
{
	return m_tameWild;
}

void AgricultureModel::SetTame( bool value )
{
	if ( m_tameWild != value )
	{
		m_tameWild = value;
		m_proxy->setHarvestOptions( m_id, m_harvest, m_harvestHay, m_tameWild );
		OnPropertyChanged( "Tame" );
	}
}

bool AgricultureModel::GetHarvestHay() const
{
	return m_harvestHay;
}

void AgricultureModel::SetHarvestHay( bool value )
{
	if ( m_harvestHay != value )
	{
		m_harvestHay = value;
		m_proxy->setHarvestOptions( m_id, m_harvest, m_harvestHay, m_tameWild );
		OnPropertyChanged( "HarvestHay" );
	}
}

bool AgricultureModel::GetPlantTrees() const
{
	return m_plantTrees;
}

void AgricultureModel::SetPlantTrees( bool value )
{
	if ( m_plantTrees != value )
	{
		m_plantTrees = value;
		m_proxy->setGroveOptions( m_id, m_pickFruits, m_plantTrees, m_fellTrees );
		OnPropertyChanged( "PlantTrees" );
	}
}

bool AgricultureModel::GetFellTrees() const
{
	return m_fellTrees;
}

void AgricultureModel::SetFellTrees( bool value )
{
	if ( m_fellTrees != value )
	{
		m_fellTrees = value;
		m_proxy->setGroveOptions( m_id, m_pickFruits, m_plantTrees, m_fellTrees );
		OnPropertyChanged( "FellTrees" );
	}
}

bool AgricultureModel::GetPickFruits() const
{
	return m_pickFruits;
}

void AgricultureModel::SetPickFruits( bool value )
{
	if ( m_pickFruits != value )
	{
		m_pickFruits = value;
		m_proxy->setGroveOptions( m_id, m_pickFruits, m_plantTrees, m_fellTrees );
		OnPropertyChanged( "PickFruits" );
	}
}

Noesis::ObservableCollection<AcPriority>* AgricultureModel::GetPrios() const
{
	return m_prios;
}

void AgricultureModel::SetSelectedPriority( AcPriority* prio )
{
	if ( m_selectedPrio && prio && m_selectedPrio != prio )
	{
		m_selectedPrio = prio;
		setBasicOptions();
		OnPropertyChanged( "SelectedPrio" );
	}
}

AcPriority* AgricultureModel::GetSelectedPriority() const
{
	return m_selectedPrio;
}

const char* AgricultureModel::GetShowFarm() const
{
	if ( m_type == AgriType::Farm && !m_productSelect )
	{
		return "Visible";
	}
	return "Hidden";
}

const char* AgricultureModel::GetShowPasture() const
{
	if ( m_type == AgriType::Pasture && !m_productSelect && !m_manageWindow && !m_pastureFoodSelect )
	{
		return "Visible";
	}
	return "Hidden";
}

const char* AgricultureModel::GetShowGrove() const
{
	if ( m_type == AgriType::Grove && !m_productSelect )
	{
		return "Visible";
	}
	return "Hidden";
}

const char* AgricultureModel::GetShowPlantSelect() const
{
	if ( m_type == AgriType::Farm && m_productSelect )
	{
		return "Visible";
	}
	return "Hidden";
}

const char* AgricultureModel::GetShowAnimalSelect() const
{
	if ( m_type == AgriType::Pasture && m_productSelect )
	{
		return "Visible";
	}
	return "Hidden";
}

const char* AgricultureModel::GetShowTreeSelect() const
{
	if ( m_type == AgriType::Grove && m_productSelect )
	{
		return "Visible";
	}
	return "Hidden";
}

const char* AgricultureModel::GetShowPastureFoodSelect() const
{
	if ( m_type == AgriType::Pasture && m_pastureFoodSelect )
	{
		return "Visible";
	}
	return "Hidden";
}

void AgricultureModel::onCmdSelectProduct( BaseComponent* param )
{
	QString qParam( param->ToString().Str() );
	m_productSelect = ( qParam == "Open" );
	OnPropertyChanged( "ShowFarm" );
	OnPropertyChanged( "ShowPasture" );
	OnPropertyChanged( "ShowGrove" );
	OnPropertyChanged( "ShowPlantSelect" );
	OnPropertyChanged( "ShowAnimalSelect" );
	OnPropertyChanged( "ShowTreeSelect" );

	switch ( m_type )
	{
		case AgriType::Farm:
			if ( m_plantRows->Count() == 0 )
			{
				m_proxy->requestGlobalPlantInfo();
			}
			break;
		case AgriType::Pasture:
			if ( m_animalRows->Count() == 0 )
			{
				m_proxy->requestGlobalAnimalInfo();
			}
			break;
		case AgriType::Grove:
			if ( m_treeRows->Count() == 0 )
			{
				m_proxy->requestGlobalTreeInfo();
			}
			break;
		default:

			break;
	}
}

void AgricultureModel::onSelectProduct( BaseComponent* param )
{
	m_productSelect = false;
	m_proxy->selectProduct( m_id, param->ToString().Str() );
	OnPropertyChanged( "ShowFarm" );
	OnPropertyChanged( "ShowPasture" );
	OnPropertyChanged( "ShowGrove" );
	OnPropertyChanged( "ShowPlantSelect" );
	OnPropertyChanged( "ShowAnimalSelect" );
	OnPropertyChanged( "ShowTreeSelect" );
}

const char* AgricultureModel::GetTilled() const
{
	return m_tilled.Str();
}

const char* AgricultureModel::GetPlanted() const
{
	return m_planted.Str();
}

const char* AgricultureModel::GetHarvestReady() const
{
	return m_ready.Str();
}

const char* AgricultureModel::GetNumSeeds() const
{
	return m_numSeeds.Str();
}

const char* AgricultureModel::GetNumItems() const
{
	return m_numItems.Str();
}

const char* AgricultureModel::GetNumPlots() const
{
	return m_numPlots.Str();
}

const char* AgricultureModel::GetNumPlants() const
{
	return m_numPlants.Str();
}

const char* AgricultureModel::GetNumAnimals() const
{
	return m_numAnimals.Str();
}

const char* AgricultureModel::GetNumMale() const
{
	return m_numMale.Str();
}

const char* AgricultureModel::GetNumFemale() const
{
	return m_numFemale.Str();
}

const char* AgricultureModel::GetMaxMale() const
{
	return m_maxMale.Str();
}

const char* AgricultureModel::GetMaxFemale() const
{
	return m_maxFemale.Str();
}

void AgricultureModel::SetMaxMale( const char* value )
{
	bool ok;
	QString ns( value );
	int max = ns.toInt( &ok );
	if ( ok )
	{
		if ( max <= m_maxAnimals && max >= 0 )
		{
			m_maxMale = ( QString::number( max ) ).toStdString().c_str();
			m_proxy->setMaxMale( m_id, max );
		}
		else
		{
			m_maxMale = ( QString::number( m_maxAnimals ) ).toStdString().c_str();
			OnPropertyChanged( "MaxMale" );
		}
	}
	else
	{
		m_maxMale = ( QString::number( m_maxAnimals ) ).toStdString().c_str();
		OnPropertyChanged( "MaxMale" );
	}
}

void AgricultureModel::SetMaxFemale( const char* value )
{
	bool ok;
	QString ns( value );
	int max = ns.toInt( &ok );
	if ( ok )
	{
		if ( max <= m_maxAnimals && max >= 0 )
		{
			m_maxFemale = ( QString::number( max ) ).toStdString().c_str();
			m_proxy->signalSetMaxFemale( m_id, max );
		}
		else
		{
			m_maxFemale = ( QString::number( m_maxAnimals ) ).toStdString().c_str();
			OnPropertyChanged( "MaxFemale" );
		}
	}
	else
	{
		m_maxFemale = ( QString::number( m_maxAnimals ) ).toStdString().c_str();
		OnPropertyChanged( "MaxFemale" );
	}
}

const char* AgricultureModel::GetManageAnimalsVisible() const
{
	if ( m_maxAnimals > 0 ) //has an animal type set
	{
		return "Visible";
	}
	else
	{
		return "Hidden";
	}
}

void AgricultureModel::onManageAnimals( BaseComponent* param )
{
	m_proxy->requestPastureAnimalInfo();

	m_manageWindow  = true;
	m_productSelect = false;
	m_pastureFoodSelect = false;
	OnPropertyChanged( "ShowManageWindow" );
	OnPropertyChanged( "ShowPasture" );
}

void AgricultureModel::onManageAnimalsBack( BaseComponent* param )
{
	m_manageWindow  = false;
	m_productSelect = false;
	m_pastureFoodSelect = false;
	OnPropertyChanged( "ShowManageWindow" );
	OnPropertyChanged( "ShowPasture" );
}

const char* AgricultureModel::GetManageWindowVis() const
{
	if ( m_type == AgriType::Pasture && m_manageWindow )
	{
		return "Visible";
	}
	return "Hidden";
}

void AgricultureModel::onShowPastureFood( BaseComponent* param )
{
	if( param )
	{
		QString qParam = param->ToString().Str();
		if( qParam == "Back" )
		{
			m_pastureFoodSelect = false;
		}
		else if( qParam == "Show" )
		{
			m_proxy->requestPastureFoodInfo();
			m_pastureFoodSelect = true;
		}
	}
	OnPropertyChanged( "ShowPastureFoodSelect" );
	OnPropertyChanged( "ShowPasture" );
}

const char* AgricultureModel::GetFoodStatus() const
{
	return m_foodStatus.Str();
}

const char* AgricultureModel::GetHayStatus() const
{
	return m_hayStatus.Str();
}

const char* AgricultureModel::GetMaxHay() const
{
	return m_maxHay.Str();
}

void AgricultureModel::SetMaxHay( const char* value )
{
	if( value )
	{
		QString qValue = value;
		bool ok;
		int val = qValue.toInt( &ok );
		if( ok )
		{
			m_maxHay = value;
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION( AgricultureModel, "IngnomiaGUI.AgricultureModel" )
{
	NsProp( "Name", &AgricultureModel::GetName, &AgricultureModel::SetName );
	NsProp( "Title", &AgricultureModel::GetTitle );
	NsProp( "Suspended", &AgricultureModel::GetSuspended, &AgricultureModel::SetSuspended );
	NsProp( "Harvest", &AgricultureModel::GetHarvest, &AgricultureModel::SetHarvest );
	NsProp( "Priorities", &AgricultureModel::GetPrios );
	NsProp( "SelectedPrio", &AgricultureModel::GetSelectedPriority, &AgricultureModel::SetSelectedPriority );

	NsProp( "ShowFarm", &AgricultureModel::GetShowFarm );
	NsProp( "ShowPasture", &AgricultureModel::GetShowPasture );
	NsProp( "ShowGrove", &AgricultureModel::GetShowGrove );
	NsProp( "ShowPlantSelect", &AgricultureModel::GetShowPlantSelect );
	NsProp( "ShowAnimalSelect", &AgricultureModel::GetShowAnimalSelect );
	NsProp( "ShowTreeSelect", &AgricultureModel::GetShowTreeSelect );
	NsProp( "ShowManageWindow", &AgricultureModel::GetManageWindowVis );
	NsProp( "ShowPastureFoodSelect", &AgricultureModel::GetShowPastureFoodSelect );
	NsProp( "CmdShowPastureFood", &AgricultureModel::GetShowPastureFoodCmd );

	NsProp( "PlantRows", &AgricultureModel::GetPlants );
	NsProp( "AnimalRows", &AgricultureModel::GetAnimals );
	NsProp( "PastureAnimals", &AgricultureModel::GetPastureAnimals );
	NsProp( "TreeRows", &AgricultureModel::GetTrees );
	NsProp( "FoodRows", &AgricultureModel::GetFoods );

	NsProp( "ProductSelection", &AgricultureModel::GetCmdSelectProduct );
	NsProp( "SelectProduct", &AgricultureModel::GetSelectProduct );

	NsProp( "NumPlots", &AgricultureModel::GetNumPlots );

	NsProp( "Tilled", &AgricultureModel::GetTilled );
	NsProp( "Planted", &AgricultureModel::GetPlanted );
	NsProp( "HarvestReady", &AgricultureModel::GetHarvestReady );

	NsProp( "NumSeeds", &AgricultureModel::GetNumSeeds );
	NsProp( "NumItems", &AgricultureModel::GetNumItems );
	NsProp( "NumPlants", &AgricultureModel::GetNumPlants );

	NsProp( "NumAnimals", &AgricultureModel::GetNumAnimals );
	NsProp( "NumMale", &AgricultureModel::GetNumMale );
	NsProp( "NumFemale", &AgricultureModel::GetNumFemale );
	NsProp( "MaxMale", &AgricultureModel::GetMaxMale, &AgricultureModel::SetMaxMale );
	NsProp( "MaxFemale", &AgricultureModel::GetMaxFemale, &AgricultureModel::SetMaxFemale );
	NsProp( "VisManageAnimals", &AgricultureModel::GetManageAnimalsVisible );
	NsProp( "ManageAnimals", &AgricultureModel::GetManageAnimals );
	NsProp( "ManageAnimalsBack", &AgricultureModel::GetManageAnimalsBack );
	NsProp( "HarvestHay", &AgricultureModel::GetHarvestHay, &AgricultureModel::SetHarvestHay );
	NsProp( "Tame", &AgricultureModel::GetTame, &AgricultureModel::SetTame );

	NsProp( "FoodStatus", &AgricultureModel::GetFoodStatus );
	NsProp( "HayStatus", &AgricultureModel::GetHayStatus );
	NsProp( "MaxHay", &AgricultureModel::GetMaxHay, &AgricultureModel::SetMaxHay );

	NsProp( "PlantTrees", &AgricultureModel::GetPlantTrees, &AgricultureModel::SetPlantTrees );
	NsProp( "FellTrees", &AgricultureModel::GetFellTrees, &AgricultureModel::SetFellTrees );
	NsProp( "PickFruits", &AgricultureModel::GetPickFruits, &AgricultureModel::SetPickFruits );

	NsProp( "Image", &AgricultureModel::getBitmapSource );
}

NS_IMPLEMENT_REFLECTION( AcPriority )
{
	NsProp( "Name", &AcPriority::GetName );
}

NS_IMPLEMENT_REFLECTION( PlantSelectEntry )
{
	NsProp( "Name", &PlantSelectEntry::GetName );
	NsProp( "ID", &PlantSelectEntry::GetSID );
	NsProp( "Image", &PlantSelectEntry::getBitmapSource );
}

NS_IMPLEMENT_REFLECTION( PlantSelectRow )
{
	NsProp( "Plants", &PlantSelectRow::GetPlants );
}

NS_IMPLEMENT_REFLECTION( AnimalSelectEntry )
{
	NsProp( "Name", &AnimalSelectEntry::GetName );
	NsProp( "ID", &AnimalSelectEntry::GetSID );
	NsProp( "Image", &AnimalSelectEntry::getBitmapSource );
}

NS_IMPLEMENT_REFLECTION( AnimalSelectRow )
{
	NsProp( "Animals", &AnimalSelectRow::GetAnimals );
}

NS_IMPLEMENT_REFLECTION( TreeSelectEntry )
{
	NsProp( "Name", &TreeSelectEntry::GetName );
	NsProp( "ID", &TreeSelectEntry::GetSID );
	NsProp( "Image", &TreeSelectEntry::getBitmapSource );
}

NS_IMPLEMENT_REFLECTION( TreeSelectRow )
{
	NsProp( "Trees", &TreeSelectRow::GetTrees );
}

NS_IMPLEMENT_REFLECTION( PastureAnimalEntry )
{
	NsProp( "Name", &PastureAnimalEntry::GetName );
	NsProp( "ToButcher", &PastureAnimalEntry::GetButchering, &PastureAnimalEntry::SetButchering );
}

NS_IMPLEMENT_REFLECTION( FoodSelectEntry )
{
	NsProp( "Name", &FoodSelectEntry::GetName );
	NsProp( "Image", &FoodSelectEntry::getBitmapSource );
	NsProp( "Checked", &FoodSelectEntry::getChecked, &FoodSelectEntry::setChecked );
}

NS_IMPLEMENT_REFLECTION( FoodSelectRow )
{
	NsProp( "Foods", &FoodSelectRow::GetFoods );
}