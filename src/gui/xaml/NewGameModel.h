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

#ifndef __MENU3D_NEWGAMEMODEL_H__
#define __MENU3D_NEWGAMEMODEL_H__

#include <NsApp/DelegateCommand.h>
#include <NsApp/NotifyPropertyChangedBase.h>
#include <NsCore/Noesis.h>
#include <NsCore/Ptr.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/ReflectionDeclareEnum.h>
#include <NsCore/String.h>
#include <NsGui/Collection.h>
#include <NsGui/ObservableCollection.h>

#include <QString>

namespace IngnomiaGUI
{

////////////////////////////////////////////////////////////////////////////////////////////////////
class Preset final : public Noesis::BaseComponent
{
public:
	Preset( const char* name );

	const char* GetName() const;

private:
	Noesis::String _name;

	NS_DECLARE_REFLECTION( Preset, Noesis::BaseComponent )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class GameItem final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	GameItem( QString name, QString sid );
	GameItem( QString name, QString sid, int amount );

	const char* GetName() const;
	bool isChecked() const;
	const char* sid();

	void setChecked( bool value );

	const char* getAmount() const;
	void setAmount( const char* value );

private:
	Noesis::String _name;
	Noesis::String _sid;
	
	Noesis::String m_amountString = "0";
	int m_amount = 0;

	NS_DECLARE_REFLECTION( GameItem, NoesisApp::NotifyPropertyChangedBase )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
struct StartItem : public Noesis::BaseComponent
{
public:
	StartItem( QString name, QString mat1, QString mat2, int amount );

	Noesis::String _tag;
	Noesis::String _name;
	Noesis::String _mat1;
	Noesis::String _mat2;
	Noesis::String _amount;

	NS_DECLARE_REFLECTION( StartItem, BaseComponent )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
struct StartAnimal : public Noesis::BaseComponent
{
public:
	StartAnimal( QString type, QString gender, int amount );

	Noesis::String _tag;
	Noesis::String _type;
	Noesis::String _gender;
	Noesis::String _amount;

	NS_DECLARE_REFLECTION( StartAnimal, BaseComponent )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class NewGameModel final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	NewGameModel();

private:
	const NoesisApp::DelegateCommand* GetRandomKingdomName() const;
	const NoesisApp::DelegateCommand* GetRandomSeed() const;
	const NoesisApp::DelegateCommand* GetNewPreset() const;
	const NoesisApp::DelegateCommand* GetSavePreset() const;
	const NoesisApp::DelegateCommand* GetDeletePreset() const;
	const NoesisApp::DelegateCommand* GetAddItem() const;
	const NoesisApp::DelegateCommand* GetRemoveItem() const;
	const NoesisApp::DelegateCommand* GetAddAnimal() const;
	const NoesisApp::DelegateCommand* GetRemoveAnimal() const;

	void OnRandomKingdomName( BaseComponent* param );
	void OnRandomSeed( BaseComponent* param );
	void OnNewPreset( BaseComponent* param );
	void OnSavePreset( BaseComponent* param );
	void OnDeletePreset( BaseComponent* param );
	void OnAddItem( BaseComponent* param );
	void OnRemoveItem( BaseComponent* param );
	void OnAddAnimal( BaseComponent* param );
	void OnRemoveAnimal( BaseComponent* param );

	int GetWorldSize() const;
	void SetWorldSize( int value );

	int GetZLevels() const;
	void SetZLevels( int value );

	int GetGround() const;
	void SetGround( int value );

	int GetFlatness() const;
	void SetFlatness( int value );

	int GetOceanSize() const;
	void SetOceanSize( int value );

	int GetRivers() const;
	void SetRivers( int value );

	int GetRiverSize() const;
	void SetRiverSize( int value );

	int GetNumGnomes() const;
	void SetNumGnomes( int value );

	int GetStartZone() const;
	void SetStartZone( int value );

	int GetTreeDensity() const;
	void SetTreeDensity( int value );

	int GetPlantDensity() const;
	void SetPlantDensity( int value );

	const char* GetKingdomName() const;
	void SetKingdomName( const char* value );

	const char* GetSeed() const;
	void SetSeed( const char* value );

	const char* GetItemAmount() const;
	void SetItemAmount( const char* value );

	const char* GetAnimalAmount() const;
	void SetAnimalAmount( const char* value );

	bool GetPeaceful() const;
	void SetPeaceful( bool value );

	int GetMaxPerType() const;
	void SetMaxPertype( int value );

	int GetNumWildAnimals() const;
	void SetNumWildAnimals( int value );

	Noesis::ObservableCollection<Preset>* GetPresets() const;
	void SetSelectedPreset( Preset* preset );
	Preset* GetSelectedPreset() const;

	Noesis::ObservableCollection<GameItem>* GetItems() const;
	void SetSelectedItem( GameItem* item );
	GameItem* GetSelectedItem() const;

	Noesis::ObservableCollection<GameItem>* GetMaterials1() const;
	void SetSelectedMaterial1( GameItem* item );
	GameItem* GetSelectedMaterial1() const;

	Noesis::ObservableCollection<GameItem>* GetMaterials2() const;
	void SetSelectedMaterial2( GameItem* item );
	GameItem* GetSelectedMaterial2() const;

	Noesis::ObservableCollection<StartItem>* GetStartingItems() const;
	Noesis::ObservableCollection<StartAnimal>* GetStartingAnimals() const;
	Noesis::ObservableCollection<GameItem>* GetAllowedTrees() const;
	Noesis::ObservableCollection<GameItem>* GetAllowedPlants() const;
	Noesis::ObservableCollection<GameItem>* GetAllowedWildAnimals() const;

	Noesis::ObservableCollection<GameItem>* GetAnimals() const;
	void SetSelectedAnimal( GameItem* item );
	GameItem* GetSelectedAnimal() const;

	Noesis::ObservableCollection<GameItem>* GetGenders() const;
	void SetSelectedGender( GameItem* item );
	GameItem* GetSelectedGender() const;

private:
	NoesisApp::DelegateCommand _randomKingdomName;
	NoesisApp::DelegateCommand _randomSeed;
	NoesisApp::DelegateCommand _newPreset;
	NoesisApp::DelegateCommand _savePreset;
	NoesisApp::DelegateCommand _deletePreset;
	NoesisApp::DelegateCommand _addItem;
	NoesisApp::DelegateCommand _removeItem;
	NoesisApp::DelegateCommand _addAnimal;
	NoesisApp::DelegateCommand _removeAnimal;

	Noesis::String _seed;
	Noesis::String _kingdomName;

	Noesis::String _itemAmount;
	Noesis::String _animalAmount;

	Noesis::Ptr<Noesis::ObservableCollection<Preset>> _presets;
	Preset* _selectedPreset;

	Noesis::Ptr<Noesis::ObservableCollection<GameItem>> _items;
	GameItem* _selectedItem;

	Noesis::Ptr<Noesis::ObservableCollection<GameItem>> _animals;
	GameItem* _selectedAnimal;

	Noesis::Ptr<Noesis::ObservableCollection<GameItem>> _genders;
	GameItem* _selectedGender;

	Noesis::Ptr<Noesis::ObservableCollection<GameItem>> _itemMaterials1;
	GameItem* _selectedItemMaterial1;
	Noesis::Ptr<Noesis::ObservableCollection<GameItem>> _itemMaterials2;
	GameItem* _selectedItemMaterial2;

	Noesis::Ptr<Noesis::ObservableCollection<StartItem>> _startingItems;
	Noesis::Ptr<Noesis::ObservableCollection<StartAnimal>> _startingAnimals;
	Noesis::Ptr<Noesis::ObservableCollection<GameItem>> _allowedTrees;
	Noesis::Ptr<Noesis::ObservableCollection<GameItem>> _allowedPlants;
	Noesis::Ptr<Noesis::ObservableCollection<GameItem>> _allowedWildAnimals;

	void updateStartingItems();
	void updateStartingAnimals();

	NS_DECLARE_REFLECTION( NewGameModel, NotifyPropertyChangedBase )
};

} // namespace IngnomiaGUI

#endif
