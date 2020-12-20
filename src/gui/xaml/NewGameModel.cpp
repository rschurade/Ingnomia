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

#include "NewGameModel.h"

#include "../../base/db.h"
#include "../../game/newgamesettings.h"

#include "../eventconnector.h"
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

////////////////////////////////////////////////////////////////////////////////////////////////////
Preset::Preset( const char* name ) :
	_name( name )
{
}

const char* Preset::GetName() const
{
	return _name.Str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GameItem::GameItem( QString name, QString sid )
{
	_name = name.toStdString().c_str();
	_sid  = sid.toStdString().c_str();
}

GameItem::GameItem( QString name, QString sid, int amount ) :
	_name( name.toStdString().c_str() ),
	_sid( sid.toStdString().c_str() ),
	m_amount( amount )
{
	m_amountString = QString::number( amount ).toStdString().c_str();
}


const char* GameItem::GetName() const
{
	return _name.Str();
}

const char* GameItem::sid()
{
	return _sid.Str();
}
	
const char* GameItem::getAmount() const
{
	return m_amountString.Str();
}

void GameItem::setAmount( const char* value )
{
	if( value )
	{
		m_amountString = value;
		if( m_amountString.Size() > 0 )
		{
			m_amount = std::stoi( value );
		}
		else
		{
			m_amount = 0;
		}
		Global::newGameSettings->setAmount( _sid.Str(), m_amount );

		Global::newGameSettings->setChecked( _sid.Str(), m_amount != 0 );
		
		OnPropertyChanged( "IsChecked" );
	}
}

bool GameItem::isChecked() const
{
	return Global::newGameSettings->isChecked( _sid.Str() );
}

void GameItem::setChecked( bool value )
{
	m_amount = value ? -1 : 0;
	m_amountString = QString::number( m_amount ).toStdString().c_str();

	Global::newGameSettings->setAmount( _sid.Str(), m_amount );
	Global::newGameSettings->setChecked( _sid.Str(), value );

	OnPropertyChanged( "Amount" );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
StartItem::StartItem( QString name, QString mat1, QString mat2, int amount )
{
	_tag = ( name + "_" + mat1 + "_" + mat2 ).toStdString().c_str();

	_name = S::s( "$ItemName_" + name ).toStdString().c_str();
	if ( !mat1.isEmpty() )
	{
		_mat1 = S::s( "$MaterialName_" + mat1 ).toStdString().c_str();
	}
	if ( !mat2.isEmpty() )
	{
		_mat2 = S::s( "$MaterialName_" + mat2 ).toStdString().c_str();
	}
	_amount = QString::number( amount ).toStdString().c_str();
}

StartAnimal::StartAnimal( QString type, QString gender, int amount )
{
	_tag    = ( type + "_" + gender ).toStdString().c_str();
	_type   = S::s( "$CreatureName_" + type ).toStdString().c_str();
	_gender = gender.toStdString().c_str();
	_amount = QString::number( amount ).toStdString().c_str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NewGameModel::NewGameModel() :
	_selectedPreset( nullptr ),
	_selectedItem( nullptr ),
	_selectedAnimal( nullptr ),
	_selectedGender( nullptr ),
	_selectedItemMaterial1( nullptr ),
	_selectedItemMaterial2( nullptr )
{
	_randomKingdomName.SetExecuteFunc( MakeDelegate( this, &NewGameModel::OnRandomKingdomName ) );
	_randomSeed.SetExecuteFunc( MakeDelegate( this, &NewGameModel::OnRandomSeed ) );
	_newPreset.SetExecuteFunc( MakeDelegate( this, &NewGameModel::OnNewPreset ) );
	_savePreset.SetExecuteFunc( MakeDelegate( this, &NewGameModel::OnSavePreset ) );
	_deletePreset.SetExecuteFunc( MakeDelegate( this, &NewGameModel::OnDeletePreset ) );
	_addItem.SetExecuteFunc( MakeDelegate( this, &NewGameModel::OnAddItem ) );
	_removeItem.SetExecuteFunc( MakeDelegate( this, &NewGameModel::OnRemoveItem ) );
	_addAnimal.SetExecuteFunc( MakeDelegate( this, &NewGameModel::OnAddAnimal ) );
	_removeAnimal.SetExecuteFunc( MakeDelegate( this, &NewGameModel::OnRemoveAnimal ) );

	_startingItems   = *new ObservableCollection<StartItem>();
	_startingAnimals = *new ObservableCollection<StartAnimal>();

	_allowedWildAnimals = *new ObservableCollection<GameItem>();
	_allowedPlants      = *new ObservableCollection<GameItem>();
	_allowedTrees       = *new ObservableCollection<GameItem>();

	_kingdomName  = Global::newGameSettings->kingdomName().toStdString().c_str();
	_seed         = Global::newGameSettings->seed().toStdString().c_str();
	_itemAmount   = "1";
	_animalAmount = "1";

	_presets = *new ObservableCollection<Preset>();
	for ( auto name : Global::newGameSettings->presetNames() )
	{
		_presets->Add( MakePtr<Preset>( name.toStdString().c_str() ) );
	}
	updateStartingItems();
	updateStartingAnimals();
	//SetSelectedPreset( _presets->Get( 0 ) );

	_itemMaterials1 = *new ObservableCollection<GameItem>();
	_itemMaterials2 = *new ObservableCollection<GameItem>();

	_items             = *new ObservableCollection<GameItem>();
	QStringList diKeys = DB::ids( "Items" );
	diKeys.sort();
	for ( auto key : diKeys )
	{
		_items->Add( MakePtr<GameItem>( S::s( "$ItemName_" + key ), key ) );
	}
	SetSelectedItem( _items->Get( 0 ) );

	_animals               = *new ObservableCollection<GameItem>();
	QStringList allAnimals = DB::ids( "Animals" );
	allAnimals.sort();
	for ( auto animal : allAnimals )
	{
		if ( DB::select( "Pasture", "Animals", animal ).toBool() || DB::select( "Embark", "Animals", animal ).toBool() )
		{
			_animals->Add( MakePtr<GameItem>( S::s( "$CreatureName_" + animal ), animal ) );
		}
	}
	SetSelectedAnimal( _animals->Get( 0 ) );

	_genders = *new ObservableCollection<GameItem>();
	_genders->Add( MakePtr<GameItem>( "Male", "Male" ) );
	_genders->Add( MakePtr<GameItem>( "Female", "Female" ) );
	SetSelectedGender( _genders->Get( 0 ) );

	for ( auto vt : Global::newGameSettings->trees() )
	{
		auto vm = vt.toMap();
		auto gi = MakePtr<GameItem>( vm.value( "Name" ).toString(), vm.value( "ID" ).toString() );
		//gi->setChecked( vm.value( "Allowed" ).toBool() ? true : false );
		_allowedTrees->Add( gi );
	}

	for ( auto vt : Global::newGameSettings->plants() )
	{
		auto vm = vt.toMap();
		auto gi = MakePtr<GameItem>( vm.value( "Name" ).toString(), vm.value( "ID" ).toString() );
		//gi->setChecked( vm.value( "Allowed" ).toBool() ? true : false );
		_allowedPlants->Add( gi );
	}

	for ( auto vt : Global::newGameSettings->animals() )
	{
		auto vm = vt.toMap();
		auto gi = MakePtr<GameItem>( vm.value( "Name" ).toString(), vm.value( "ID" ).toString(), vm.value( "Amount" ).toInt() );
		_allowedWildAnimals->Add( gi );
	}
}

const DelegateCommand* NewGameModel::GetRandomKingdomName() const
{
	return &_randomKingdomName;
}

const DelegateCommand* NewGameModel::GetRandomSeed() const
{
	return &_randomSeed;
}
const DelegateCommand* NewGameModel::GetNewPreset() const
{
	return &_newPreset;
}

const DelegateCommand* NewGameModel::GetSavePreset() const
{
	return &_savePreset;
}

const DelegateCommand* NewGameModel::GetDeletePreset() const
{
	return &_deletePreset;
}

const DelegateCommand* NewGameModel::GetAddItem() const
{
	return &_addItem;
}

const DelegateCommand* NewGameModel::GetRemoveItem() const
{
	return &_removeItem;
}

const DelegateCommand* NewGameModel::GetAddAnimal() const
{
	return &_addAnimal;
}

const DelegateCommand* NewGameModel::GetRemoveAnimal() const
{
	return &_removeAnimal;
}

void NewGameModel::OnRandomKingdomName( BaseComponent* param )
{
	Global::newGameSettings->setRandomName();
	_kingdomName = Global::newGameSettings->kingdomName().toStdString().c_str();
	OnPropertyChanged( "KingdomName" );
}

void NewGameModel::OnRandomSeed( BaseComponent* param )
{
	Global::newGameSettings->setRandomSeed();
	_seed = Global::newGameSettings->seed().toStdString().c_str();
	OnPropertyChanged( "Seed" );
}

void NewGameModel::OnNewPreset( BaseComponent* param )
{
	QString newName = Global::newGameSettings->addPreset();
	if ( !newName.isEmpty() )
	{
		_presets->Add( MakePtr<Preset>( newName.toStdString().c_str() ) );
	}
	SetSelectedPreset( _presets->Get( _presets->Count() - 1 ) );
}

void NewGameModel::OnSavePreset( BaseComponent* param )
{
	Global::newGameSettings->onSavePreset();
}

void NewGameModel::OnDeletePreset( BaseComponent* param )
{
	//get name from combobox
	auto name = _selectedPreset->GetName();
	if ( Global::newGameSettings->removePreset( name ) )
	{
		SetSelectedPreset( _presets->Get( 0 ) );
		for ( int i = 0; i < _presets->Count(); ++i )
		{
			if ( _presets->Get( i )->GetName() == name )
			{
				_presets->RemoveAt( i );
				OnPropertyChanged( "SelectedPreset" );
				break;
			}
		}
	}
}

void NewGameModel::OnAddItem( BaseComponent* param )
{
	QString itemSID;
	QString mat1;
	QString mat2;
	int amount = QString( _itemAmount.Str() ).toInt();
	if ( _selectedItem )
	{
		itemSID = _selectedItem->sid();
	}
	if ( _selectedItemMaterial1 )
	{
		mat1 = _selectedItemMaterial1->sid();
	}
	if ( _selectedItemMaterial2 )
	{
		mat2 = _selectedItemMaterial2->sid();
	}

	Global::newGameSettings->addStartingItem( itemSID, mat1, mat2, amount );

	updateStartingItems();
}
void NewGameModel::OnRemoveItem( BaseComponent* param )
{
	Global::newGameSettings->removeStartingItem( param->ToString().Str() );
	updateStartingItems();
}

void NewGameModel::OnAddAnimal( BaseComponent* param )
{
	QString type;
	QString gender;
	int amount = QString( _animalAmount.Str() ).toInt();
	if ( _selectedAnimal )
	{
		type = _selectedAnimal->sid();
	}

	if ( _selectedGender )
	{
		gender = _selectedGender->sid();
	}

	Global::newGameSettings->addStartingAnimal( type, gender, amount );

	updateStartingAnimals();
}
void NewGameModel::OnRemoveAnimal( BaseComponent* param )
{
	Global::newGameSettings->removeStartingAnimal( param->ToString().Str() );
	updateStartingAnimals();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int NewGameModel::GetWorldSize() const
{
	return Global::newGameSettings->worldSize();
}
void NewGameModel::SetWorldSize( int size )
{
	if ( Global::newGameSettings->setWorldSize( size ) )
	{
		OnPropertyChanged( "WorldSize" );
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
int NewGameModel::GetZLevels() const
{
	return Global::newGameSettings->zLevels();
}
void NewGameModel::SetZLevels( int value )
{
	if ( Global::newGameSettings->setZLevels( value ) )
	{
		OnPropertyChanged( "ZLevels" );
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
int NewGameModel::GetGround() const
{
	return Global::newGameSettings->ground();
}
void NewGameModel::SetGround( int value )
{
	if ( Global::newGameSettings->setGround( value ) )
	{
		OnPropertyChanged( "Ground" );
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
int NewGameModel::GetFlatness() const
{
	return Global::newGameSettings->flatness();
}
void NewGameModel::SetFlatness( int value )
{
	if ( Global::newGameSettings->setFlatness( value ) )
	{
		OnPropertyChanged( "Flatness" );
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
int NewGameModel::GetOceanSize() const
{
	return Global::newGameSettings->oceanSize();
}
void NewGameModel::SetOceanSize( int value )
{
	if ( Global::newGameSettings->setOceanSize( value ) )
	{
		OnPropertyChanged( "OceanSize" );
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
int NewGameModel::GetRivers() const
{
	return Global::newGameSettings->rivers();
}
void NewGameModel::SetRivers( int value )
{
	if ( Global::newGameSettings->setRivers( value ) )
	{
		OnPropertyChanged( "Rivers" );
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
int NewGameModel::GetRiverSize() const
{
	return Global::newGameSettings->riverSize();
}
void NewGameModel::SetRiverSize( int value )
{
	if ( Global::newGameSettings->setRiverSize( value ) )
	{
		OnPropertyChanged( "RiverSize" );
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
int NewGameModel::GetNumGnomes() const
{
	return Global::newGameSettings->numGnomes();
}
void NewGameModel::SetNumGnomes( int value )
{
	if ( Global::newGameSettings->setNumGnomes( value ) )
	{
		OnPropertyChanged( "NumGnomes" );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int NewGameModel::GetStartZone() const
{
	return Global::newGameSettings->startZone();
}
void NewGameModel::SetStartZone( int value )
{
	if ( Global::newGameSettings->setStartZone( value ) )
	{
		OnPropertyChanged( "StartZone" );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int NewGameModel::GetTreeDensity() const
{
	return Global::newGameSettings->treeDensity();
}
void NewGameModel::SetTreeDensity( int value )
{
	if ( Global::newGameSettings->setTreeDensity( value ) )
	{
		OnPropertyChanged( "TreeDensity" );
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
int NewGameModel::GetPlantDensity() const
{
	return Global::newGameSettings->plantDensity();
}
void NewGameModel::SetPlantDensity( int value )
{
	if ( Global::newGameSettings->setPlantDensity( value ) )
	{
		OnPropertyChanged( "PlantDensity" );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const char* NewGameModel::GetKingdomName() const
{
	return _kingdomName.Str();
}
void NewGameModel::SetKingdomName( const char* value )
{
	if ( Global::newGameSettings->setKingdomName( value ) )
	{
		_kingdomName = Global::newGameSettings->kingdomName().toStdString().c_str();
		OnPropertyChanged( "KingdomName" );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const char* NewGameModel::GetSeed() const
{
	return _seed.Str();
}
void NewGameModel::SetSeed( const char* value )
{
	if ( Global::newGameSettings->setSeed( value ) )
	{
		_seed = Global::newGameSettings->seed().toStdString().c_str();
		OnPropertyChanged( "Seed" );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int NewGameModel::GetMaxPerType() const
{
	return Global::newGameSettings->globalMaxPerType();
}

void NewGameModel::SetMaxPertype( int value )
{
	if ( Global::newGameSettings->setMaxPerType( value ) )
	{
		OnPropertyChanged( "MaxPerType" );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int NewGameModel::GetNumWildAnimals() const
{
	return Global::newGameSettings->numWildAnimals();
}

void NewGameModel::SetNumWildAnimals( int value )
{
	if ( Global::newGameSettings->setNumWildAnimals( value ) )
	{
		OnPropertyChanged( "NumWildAnimals" );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool NewGameModel::GetPeaceful() const
{
	return Global::newGameSettings->isPeaceful();
}

void NewGameModel::SetPeaceful( bool value )
{
	Global::newGameSettings->setPeaceful( value );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const char* NewGameModel::GetItemAmount() const
{
	return _itemAmount.Str();
}
void NewGameModel::SetItemAmount( const char* value )
{
	if ( _itemAmount != value )
	{
		_itemAmount = value;
		OnPropertyChanged( "ItemAmount" );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const char* NewGameModel::GetAnimalAmount() const
{
	return _animalAmount.Str();
}
void NewGameModel::SetAnimalAmount( const char* value )
{
	if ( _animalAmount != value )
	{
		_animalAmount = value;
		OnPropertyChanged( "AnimalAmount" );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ObservableCollection<Preset>* NewGameModel::GetPresets() const
{
	return _presets;
}
void NewGameModel::SetSelectedPreset( Preset* preset )
{
	if ( _selectedPreset != preset )
	{
		Global::newGameSettings->setPreset( preset->GetName() );

		_selectedPreset = preset;
		OnPropertyChanged( "SelectedPreset" );

		updateStartingItems();
		updateStartingAnimals();
	}
}

void NewGameModel::updateStartingItems()
{
	_startingItems->Clear();

	for ( auto si : Global::newGameSettings->startingItems() )
	{
		_startingItems->Add( MakePtr<StartItem>( si.itemSID, si.mat1, si.mat2, si.amount ) );
	}
	OnPropertyChanged( "StartingItems" );
}

void NewGameModel::updateStartingAnimals()
{
	_startingAnimals->Clear();

	for ( auto sa : Global::newGameSettings->startingAnimals() )
	{
		_startingAnimals->Add( MakePtr<StartAnimal>( sa.type, sa.gender, sa.amount ) );
	}
	OnPropertyChanged( "StartingAnimals" );
}

Preset* NewGameModel::GetSelectedPreset() const
{
	return _selectedPreset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ObservableCollection<GameItem>* NewGameModel::GetItems() const
{
	return _items;
}
void NewGameModel::SetSelectedItem( GameItem* item )
{
	if ( _selectedItem != item )
	{
		_selectedItem = item;
		OnPropertyChanged( "SelectedItem" );

		_itemMaterials1->Clear();
		_itemMaterials2->Clear();

		QStringList mats1;
		QStringList mats2;

		Global::newGameSettings->materialsForItem( item->sid(), mats1, mats2 );

		if ( mats1.size() )
		{
			for ( auto mat : mats1 )
			{
				_itemMaterials1->Add( MakePtr<GameItem>( S::s( "$MaterialName_" + mat ), mat ) );
			}
			SetSelectedMaterial1( _itemMaterials1->Get( 0 ) );
		}
		if ( mats2.size() )
		{
			for ( auto mat : mats2 )
			{
				_itemMaterials2->Add( MakePtr<GameItem>( S::s( "$MaterialName_" + mat ), mat ) );
			}
			SetSelectedMaterial2( _itemMaterials2->Get( 0 ) );
		}
	}
}
GameItem* NewGameModel::GetSelectedItem() const
{
	return _selectedItem;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ObservableCollection<GameItem>* NewGameModel::GetAnimals() const
{
	return _animals;
}
void NewGameModel::SetSelectedAnimal( GameItem* item )
{
	if ( _selectedAnimal != item )
	{
		_selectedAnimal = item;
		OnPropertyChanged( "SelectedAnimal" );
	}
}
GameItem* NewGameModel::GetSelectedAnimal() const
{
	return _selectedAnimal;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
ObservableCollection<GameItem>* NewGameModel::GetGenders() const
{
	return _genders;
}
void NewGameModel::SetSelectedGender( GameItem* item )
{
	if ( _selectedGender != item )
	{
		_selectedGender = item;
		OnPropertyChanged( "SelectedGender" );
	}
}
GameItem* NewGameModel::GetSelectedGender() const
{
	return _selectedGender;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ObservableCollection<GameItem>* NewGameModel::GetMaterials1() const
{
	return _itemMaterials1;
}
void NewGameModel::SetSelectedMaterial1( GameItem* mat )
{
	if ( _selectedItemMaterial1 != mat )
	{
		_selectedItemMaterial1 = mat;
		OnPropertyChanged( "SelectedMaterial1" );
	}
}
GameItem* NewGameModel::GetSelectedMaterial1() const
{
	return _selectedItemMaterial1;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
ObservableCollection<GameItem>* NewGameModel::GetMaterials2() const
{
	return _itemMaterials2;
}
void NewGameModel::SetSelectedMaterial2( GameItem* mat )
{
	if ( _selectedItemMaterial2 != mat )
	{
		_selectedItemMaterial2 = mat;
		OnPropertyChanged( "Selectedmaterial2" );
	}
}
GameItem* NewGameModel::GetSelectedMaterial2() const
{
	return _selectedItemMaterial2;
}

ObservableCollection<StartItem>* NewGameModel::GetStartingItems() const
{
	return _startingItems;
}

ObservableCollection<StartAnimal>* NewGameModel::GetStartingAnimals() const
{
	return _startingAnimals;
}

ObservableCollection<GameItem>* NewGameModel::GetAllowedTrees() const
{
	return _allowedTrees;
}

ObservableCollection<GameItem>* NewGameModel::GetAllowedPlants() const
{
	return _allowedPlants;
}

ObservableCollection<GameItem>* NewGameModel::GetAllowedWildAnimals() const
{
	return _allowedWildAnimals;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION( NewGameModel, "IngnomiaGUI.NewGameModel" )
{
	// menu properties
	NsProp( "RandomKingdomName", &NewGameModel::GetRandomKingdomName );
	NsProp( "RandomSeed", &NewGameModel::GetRandomSeed );
	NsProp( "NewPreset", &NewGameModel::GetNewPreset );
	NsProp( "SavePreset", &NewGameModel::GetSavePreset );
	NsProp( "DeletePreset", &NewGameModel::GetDeletePreset );
	NsProp( "AddItem", &NewGameModel::GetAddItem );
	NsProp( "RemoveItem", &NewGameModel::GetRemoveItem );
	NsProp( "AddAnimal", &NewGameModel::GetAddAnimal );
	NsProp( "RemoveAnimal", &NewGameModel::GetRemoveAnimal );

	// New Game properties
	NsProp( "WorldSize", &NewGameModel::GetWorldSize, &NewGameModel::SetWorldSize );
	NsProp( "ZLevels", &NewGameModel::GetZLevels, &NewGameModel::SetZLevels );
	NsProp( "Ground", &NewGameModel::GetGround, &NewGameModel::SetGround );
	NsProp( "Flatness", &NewGameModel::GetFlatness, &NewGameModel::SetFlatness );
	NsProp( "OceanSize", &NewGameModel::GetOceanSize, &NewGameModel::SetOceanSize );
	NsProp( "Rivers", &NewGameModel::GetRivers, &NewGameModel::SetRivers );
	NsProp( "RiverSize", &NewGameModel::GetRiverSize, &NewGameModel::SetRiverSize );
	NsProp( "NumGnomes", &NewGameModel::GetNumGnomes, &NewGameModel::SetNumGnomes );
	NsProp( "StartZone", &NewGameModel::GetStartZone, &NewGameModel::SetStartZone );
	NsProp( "KingdomName", &NewGameModel::GetKingdomName, &NewGameModel::SetKingdomName );
	NsProp( "Seed", &NewGameModel::GetSeed, &NewGameModel::SetSeed );
	NsProp( "ItemAmount", &NewGameModel::GetItemAmount, &NewGameModel::SetItemAmount );
	NsProp( "AnimalAmount", &NewGameModel::GetAnimalAmount, &NewGameModel::SetAnimalAmount );
	NsProp( "MaxPerType", &NewGameModel::GetMaxPerType, &NewGameModel::SetMaxPertype );
	NsProp( "NumWildAnimals", &NewGameModel::GetNumWildAnimals, &NewGameModel::SetNumWildAnimals );
	NsProp( "StartingItems", &NewGameModel::GetStartingItems );
	NsProp( "StartingAnimals", &NewGameModel::GetStartingAnimals );
	NsProp( "AllowedTrees", &NewGameModel::GetAllowedTrees );
	NsProp( "AllowedPlants", &NewGameModel::GetAllowedPlants );
	NsProp( "AllowedWildAnimals", &NewGameModel::GetAllowedWildAnimals );
	NsProp( "TreeDensity", &NewGameModel::GetTreeDensity, &NewGameModel::SetTreeDensity );
	NsProp( "PlantDensity", &NewGameModel::GetPlantDensity, &NewGameModel::SetPlantDensity );
	NsProp( "Peaceful", &NewGameModel::GetPeaceful, &NewGameModel::SetPeaceful );

	NsProp( "Presets", &NewGameModel::GetPresets );
	NsProp( "SelectedPreset", &NewGameModel::GetSelectedPreset, &NewGameModel::SetSelectedPreset );
	NsProp( "Items", &NewGameModel::GetItems );
	NsProp( "SelectedItem", &NewGameModel::GetSelectedItem, &NewGameModel::SetSelectedItem );
	NsProp( "Materials1", &NewGameModel::GetMaterials1 );
	NsProp( "SelectedMaterial1", &NewGameModel::GetSelectedMaterial1, &NewGameModel::SetSelectedMaterial1 );
	NsProp( "Materials2", &NewGameModel::GetMaterials2 );
	NsProp( "SelectedMaterial2", &NewGameModel::GetSelectedMaterial2, &NewGameModel::SetSelectedMaterial2 );
	NsProp( "Animals", &NewGameModel::GetAnimals );
	NsProp( "SelectedAnimal", &NewGameModel::GetSelectedAnimal, &NewGameModel::SetSelectedAnimal );
	NsProp( "Genders", &NewGameModel::GetGenders );
	NsProp( "SelectedGender", &NewGameModel::GetSelectedGender, &NewGameModel::SetSelectedGender );
}

NS_IMPLEMENT_REFLECTION( Preset )
{
	NsProp( "Name", &Preset::GetName );
}

NS_IMPLEMENT_REFLECTION( GameItem )
{
	NsProp( "Name", &GameItem::GetName );
	NsProp( "IsChecked", &GameItem::isChecked, &GameItem::setChecked );
	NsProp( "Amount", &GameItem::getAmount, &GameItem::setAmount );
}

NS_IMPLEMENT_REFLECTION( StartItem )
{
	NsProp( "Tag", &StartItem::_tag );
	NsProp( "Name", &StartItem::_name );
	NsProp( "Material1", &StartItem::_mat1 );
	NsProp( "Material2", &StartItem::_mat2 );
	NsProp( "Amount", &StartItem::_amount );
}

NS_IMPLEMENT_REFLECTION( StartAnimal )
{
	NsProp( "Tag", &StartAnimal::_tag );
	NsProp( "Name", &StartAnimal::_type );
	NsProp( "Gender", &StartAnimal::_gender );
	NsProp( "Amount", &StartAnimal::_amount );
}
