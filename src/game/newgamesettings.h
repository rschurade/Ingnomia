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
#pragma once

#include <QObject>
#include <QVariantMap>
#include <absl/container/btree_map.h>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct JsonBaseStartingItem {
	int Amount;
	std::string ItemID;
	std::string Type;
};

struct JsonCombinedItemComponent {
	std::string ItemID, MaterialID;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(JsonCombinedItemComponent, ItemID, MaterialID)

struct JsonCombinedItem : public JsonBaseStartingItem {
	std::vector<JsonCombinedItemComponent> Components;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(JsonCombinedItem, Amount, ItemID, Type, Components)

struct JsonAnimalItem : public JsonBaseStartingItem {
	std::string Gender;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(JsonAnimalItem, Amount, ItemID, Type, Gender)

struct JsonNormalItem : public JsonBaseStartingItem {
	std::string MaterialID;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(JsonNormalItem, Amount, ItemID, Type, MaterialID)

using JsonStartingItem = std::variant<JsonCombinedItem, JsonAnimalItem, JsonNormalItem>;

void from_json(const json& j, JsonStartingItem& item);
void to_json(json& j, const JsonStartingItem& item);

struct JsonPreset {
	std::string Name;
	std::vector<JsonStartingItem> startingItems;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(JsonPreset, Name, startingItems)

struct StartingItem
{
	std::string itemSID;
	std::string mat1;
	std::string mat2;
	int amount = 1;
};

struct StartingAnimal
{
	std::string type;
	std::string gender;
	int amount = 1;
};

struct CheckableItem
{
	std::string sid;
	std::string name;
	std::string type;
	bool isChecked;
	int max;
};

class NewGameSettings : public QObject
{
	Q_OBJECT

public:
	NewGameSettings( QObject* parent = 0 );
	~NewGameSettings();

	void save();

	void setRandomName();
	void setRandomSeed();

	bool setKingdomName( const std::string& value );
	bool setSeed( const std::string& value );

	const std::string& kingdomName()
	{
		return m_kingdomName;
	}

	const std::string& seed()
	{
		return m_seed;
	}

	int worldSize()
	{
		return m_worldSize;
	}
	int zLevels()
	{
		return m_zLevels;
	}
	int ground()
	{
		return m_ground;
	}
	int flatness()
	{
		return m_flatness;
	}
	int oceanSize()
	{
		return m_oceanSize;
	}
	int rivers()
	{
		return m_rivers;
	}
	int riverSize()
	{
		return m_riverSize;
	}
	 int numGnomes()
	{
		return m_numGnomes;
	}
	int startZone()
	{
		return m_startZone;
	}
	int treeDensity()
	{
		return m_treeDensity;
	}
	int plantDensity()
	{
		return m_plantDensity;
	}
	bool isPeaceful()
	{
		return m_isPeaceful;
	}
	int globalMaxPerType()
	{
		return m_maxPerType;
	}
	int numWildAnimals()
	{
		return m_numWildAnimals;
	}
	int maxAnimalsPerType( const std::string& type );
	

	bool setWorldSize( int value );
	bool setZLevels( int value );
	bool setGround( int value );
	bool setFlatness( int value );
	bool setOceanSize( int value );
	bool setRivers( int value );
	bool setRiverSize( int value );
	bool setNumGnomes( int value );
	bool setStartZone( int value );
	bool setTreeDensity( int value );
	bool setPlantDensity( int value );
	bool setPeaceful( bool value );
	bool setMaxPerType( int value );
	bool setNumWildAnimals( int value );

	void materialsForItem( const std::string& item, std::vector<std::string>& mats1, std::vector<std::string>& mats2 );
	void addStartingItem( const std::string& itemSID, const std::string& mat1, const std::string& mat2, int amount );
	void removeStartingItem( const std::string& tag );

	void addStartingAnimal( const std::string& type, const std::string& gender, int amount );
	void removeStartingAnimal( const std::string& tag );

	std::vector<StartingItem> startingItems()
	{
		return m_startingItems;
	}
	std::vector<StartingAnimal> startingAnimals()
	{
		return m_startingAnimals;
	}

	std::vector<CheckableItem> trees();
	std::vector<CheckableItem> plants();
	std::vector<CheckableItem> animals();

	void setPreset( const std::string& name );
	std::vector<std::string> presetNames();
	std::string addPreset();
	bool removePreset( const std::string& name );
	bool savePreset( const std::vector<JsonStartingItem>& items );
	bool onSavePreset();

	bool isChecked( const std::string& sid );
	void setChecked( const std::string& sid, bool value );

	void setAmount( const std::string& sid, int value );

private:
	void loadEmbarkMap();
	void loadPresets();
	void saveUserPresets();

	void setStartingItems( const std::vector<JsonStartingItem>& sil );
	void collectStartItems( std::vector<JsonStartingItem>& sil );

	std::vector<CheckableItem> filterCheckableItems( const std::string& itemType );

	std::string m_kingdomName;
	std::string m_seed;

	int m_worldSize      = 100;
	int m_zLevels        = 130;
	int m_ground         = 100;
	int m_flatness       = 0;
	int m_oceanSize      = 0;
	int m_rivers         = 1;
	int m_riverSize      = 3;
	int m_numGnomes      = 8;
	int m_startZone      = 10;
	int m_treeDensity    = 50;
	int m_plantDensity   = 50;
	int m_numWildAnimals = 500;
	int m_maxPerType     = 1000;

	bool m_isPeaceful = true;

	std::vector<StartingItem> m_startingItems;
	std::vector<StartingAnimal> m_startingAnimals;

	std::vector<std::string> materials( const std::string& itemSID );

	std::vector<JsonPreset> m_standardPresets;
	std::vector<JsonPreset> m_userPresets;

	std::string m_selectedPreset;

	absl::btree_map<std::string, CheckableItem> m_checkableItems;
};
