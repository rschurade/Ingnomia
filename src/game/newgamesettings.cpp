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
#include "newgamesettings.h"

#include "../base/config.h"
#include "../base/db.h"
#include "../base/io.h"
#include "../gui/strings.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>

#include <random>
#include <range/v3/view.hpp>

#include "spdlog/spdlog.h"

inline void from_json(const json& j, JsonStartingItem& item) {
	const auto type = j.at("Type").get<std::string>();
	if (type == "Item") {
		JsonNormalItem newItem;
		from_json(j, newItem);
		item = newItem;
	} else if (type == "CombinedItem") {
		JsonCombinedItem newItem;
		from_json(j, newItem);
		item = newItem;
	} else if (type == "Animal") {
		JsonAnimalItem newItem;
		from_json(j, newItem);
		item = newItem;
	} else {
		throw std::runtime_error("Invalid starting item type: " + type);
	}
}

inline void to_json(json& j, const JsonStartingItem& item) {
	if ( const JsonNormalItem* itemPtr = std::get_if<JsonNormalItem>( &item ) )
	{
		j = *itemPtr;
	}
	else if ( const JsonCombinedItem* itemPtr = std::get_if<JsonCombinedItem>( &item ) )
	{
		j = *itemPtr;
	}
	else if ( const JsonAnimalItem* itemPtr = std::get_if<JsonAnimalItem>( &item ) )
	{
		j = *itemPtr;
	}
	else
	{
		throw std::runtime_error( "Cannot convert JsonStartingItem to JSON" );
	}
}

NewGameSettings::NewGameSettings( QObject* parent ) :
	QObject( parent )
{
	loadEmbarkMap();
	loadPresets();

	setRandomSeed();
	setRandomName();
}

NewGameSettings::~NewGameSettings()
{
}

void NewGameSettings::save()
{
	QVariantMap embarkMap;

	embarkMap.insert( "dimX", m_worldSize );
    embarkMap.insert( "dimY", m_worldSize );
    embarkMap.insert( "dimZ", m_zLevels );
    embarkMap.insert( "flatness", m_flatness );
    embarkMap.insert( "groundLevel", m_ground );
    //embarkMap.insert( "hasOcean", m_oc
    embarkMap.insert( "maxPerType", m_maxPerType );
    embarkMap.insert( "numAnimals", m_numWildAnimals );
    embarkMap.insert( "numGnomes", m_numGnomes );
    embarkMap.insert( "oceanSize", m_oceanSize );
    embarkMap.insert( "peaceful", m_isPeaceful );
    embarkMap.insert( "plantDensity", m_plantDensity );
    embarkMap.insert( "riverSize", m_riverSize );
    embarkMap.insert( "rivers", m_rivers );
    embarkMap.insert( "seed", m_seed );
	embarkMap.insert( "startingZone", m_startZone );
    embarkMap.insert( "treeDensity", m_treeDensity );

	QVariantMap allowedAnimals;
	QVariantMap allowedPlants;
	QVariantMap allowedTrees;

	for( const auto& ci : m_checkableItems | ranges::views::values )
	{
		if( ci.type == "Animal" )
		{
			allowedAnimals.insert( ci.sid, ci.max );
		}
		else if( ci.type == "Plant" )
		{
			allowedPlants.insert( ci.sid, ci.isChecked );
		}
		else if( ci.type == "Tree" )
		{
			allowedTrees.insert( ci.sid, ci.isChecked );
		}
	}
	embarkMap.insert( "allowedAnimals", allowedAnimals );
	embarkMap.insert( "allowedPlants", allowedPlants );
	embarkMap.insert( "allowedTrees", allowedTrees );

	std::vector<JsonStartingItem> startItems;
	collectStartItems( startItems );

	// FIXME: Remove this intermediate step after we completely remove QJsonDocument
	json tmp = startItems;
	const auto tmpStr = QString::fromStdString( tmp.dump() );
	QJsonDocument doc = QJsonDocument::fromJson( tmpStr.toUtf8() );
	embarkMap.insert( "startingItems", doc.toVariant().toList() );

	QJsonDocument sd = QJsonDocument::fromVariant( embarkMap );
	IO::saveFile( IO::getDataFolder() / "settings" / "newgame.json", sd );

	savePreset( startItems );
}

void NewGameSettings::collectStartItems( std::vector<JsonStartingItem>& sil )
{
	for( const auto& si : m_startingItems )
	{
		if( si.mat2.isEmpty() )
		{
			sil.emplace_back(JsonNormalItem{
				{ si.amount, si.itemSID.toStdString(), "Item" },
				si.mat1.toStdString()
			});
		}
		else
		{
			auto rows = DB::selectRows( "Items_Components", si.itemSID );
			if( rows.size() > 1 )
			{
				sil.emplace_back(JsonCombinedItem{
					{ si.amount, si.itemSID.toStdString(), "CombinedItem" },
					{
						{rows[0].value( "ItemID" ).toString().toStdString(), si.mat1.toStdString()},
						{rows[1].value( "ItemID" ).toString().toStdString(), si.mat2.toStdString()}
					}
				});
			}
		}
	}
	for( const auto& si : m_startingAnimals )
	{
		sil.emplace_back(JsonAnimalItem{
			{ si.amount, si.type.toStdString(), "Animal" },
			si.gender.toStdString()
		});
	}
}

void NewGameSettings::loadEmbarkMap()
{
	m_checkableItems.clear();

	json embarkMap;
	if ( !IO::loadFile( IO::getDataFolder() / "settings" / "newgame.json", embarkMap ) )
	{
		// if it doesn't exist get from /content/JSON
		if ( IO::loadFile( fs::path(Global::cfg->get<std::string>( "dataPath" )) / "JSON" / "newgame.json", embarkMap ) )
		{
			IO::saveFile( IO::getDataFolder() / "settings" / "newgame.json", embarkMap );
		}
		else
		{
			spdlog::debug("Unable to find new game config!");
			abort();
		}
	}

	QVariantList out;
	const auto& trees = DB::ids( "Plants", "Type", "Tree" );

	const auto& allowedTrees = embarkMap.at( "allowedTrees" );
	for ( const auto& id : trees )
	{
		const auto& key = id.toStdString();
		CheckableItem ci { id, S::s( "$ItemName_" + id ), "Tree", allowedTrees.contains( key ) && allowedTrees.at( key ).get<bool>(), 0 };
		m_checkableItems.insert_or_assign( id, ci );
	}

	auto allplants = DB::ids( "Plants", "Type", "Plant" );
	allplants.sort();
	const auto allowedPlants = embarkMap.at( "allowedPlants" );
	for ( const auto& id : allplants )
	{
		if ( DB::select( "AllowInWild", "Plants", id ).toBool() )
		{
			const auto& key = id.toStdString();
			CheckableItem ci { id, S::s( "$MaterialName_" + id ), "Plant", allowedPlants.contains(key) && allowedPlants.at( key ).get<bool>(), 0 };
			m_checkableItems.insert_or_assign( id, ci );
		}
	}

	QStringList allAnimals = DB::ids( "Animals" );
	allAnimals.sort();
	const auto& allowedAnimals = embarkMap.at( "allowedAnimals" );

	for ( const auto& id : allAnimals )
	{
		if ( DB::select( "AllowInWild", "Animals", id ).toBool() && DB::select( "Biome", "Animals", id ).toString().isEmpty() )
		{
			const auto& key = id.toStdString();
			const auto containsKey = allowedAnimals.contains( key );
			CheckableItem ci { id, S::s( "$CreatureName_" + id ), "Animal", containsKey, containsKey && allowedAnimals.at( key ).get<int>() };
			m_checkableItems.insert_or_assign( id, ci );
		}
	}

	auto sil = embarkMap.at( "startingItems" );
	setStartingItems( sil );

	// QString m_kingdomName not in embark map, we want to change it on every embark
	m_seed = QString::fromStdString( embarkMap.at( "seed" ).get<std::string>() );

	m_worldSize    = embarkMap.at( "dimX" ).get<int>();
	m_zLevels      = embarkMap.at( "dimZ" ).get<int>();
	m_ground       = embarkMap.at( "groundLevel" ).get<int>();
	m_flatness     = embarkMap.at( "flatness" ).get<int>();
	m_oceanSize    = embarkMap.at( "oceanSize" ).get<int>();
	m_rivers       = embarkMap.at( "rivers" ).get<int>();
	m_riverSize    = embarkMap.at( "riverSize" ).get<int>();
	m_numGnomes    = embarkMap.at( "numGnomes" ).get<int>();
	m_startZone    = embarkMap.at( "startingZone" ).get<int>();
	m_treeDensity  = embarkMap.at( "treeDensity" ).get<int>();
	m_plantDensity = embarkMap.at( "plantDensity" ).get<int>();
	m_isPeaceful   = embarkMap.at( "peaceful" ).get<bool>();

	m_maxPerType     = embarkMap.at( "maxPerType" ).get<int>();
	m_numWildAnimals = embarkMap.at( "numAnimals" ).get<int>();
}

void NewGameSettings::setRandomName()
{
	m_kingdomName = S::gi().randomKingdomName();
}

void NewGameSettings::setRandomSeed()
{
	std::random_device rd;                                // only used once to initialise (seed) engine
	std::mt19937 rng( rd() );                             // random-number engine used (Mersenne-Twister in this case)
	std::uniform_int_distribution<int> uni( 0, INT_MAX ); // guaranteed unbiased

	auto random_integer = uni( rng );

	m_seed = QString::number( random_integer );
}

bool NewGameSettings::setKingdomName( QString value )
{
	if ( m_kingdomName != value )
	{
		m_kingdomName = value;
		return true;
	}
	return false;
}

bool NewGameSettings::setSeed( QString value )
{
	if ( m_seed != value )
	{
		m_seed = value;
		return true;
	}
	return false;
}

bool NewGameSettings::setWorldSize( int value )
{
	if ( m_worldSize != value )
	{
		m_worldSize = value;
		return true;
	}
	return false;
}
bool NewGameSettings::setZLevels( int value )
{
	if ( m_zLevels != value )
	{
		m_zLevels = value;
		return true;
	}
	return false;
}
bool NewGameSettings::setGround( int value )
{
	if ( m_ground != value )
	{
		m_ground = value;
		return true;
	}
	return false;
}
bool NewGameSettings::setFlatness( int value )
{
	if ( m_flatness != value )
	{
		m_flatness = value;
		return true;
	}
	return false;
}
bool NewGameSettings::setOceanSize( int value )
{
	if ( m_oceanSize != value )
	{
		m_oceanSize = value;
		return true;
	}
	return false;
}
bool NewGameSettings::setRivers( int value )
{
	if ( m_rivers != value )
	{
		m_rivers = value;
		return true;
	}
	return false;
}
bool NewGameSettings::setRiverSize( int value )
{
	if ( m_riverSize != value )
	{
		m_riverSize = value;
		return true;
	}
	return false;
}
bool NewGameSettings::setNumGnomes( int value )
{
	if ( m_numGnomes != value )
	{
		m_numGnomes = value;
		return true;
	}
	return false;
}

bool NewGameSettings::setStartZone( int value )
{
	if ( m_startZone != value )
	{
		m_startZone = value;
		return true;
	}
	return false;
}

bool NewGameSettings::setTreeDensity( int value )
{
	if ( m_treeDensity != value )
	{
		m_treeDensity = value;
		return true;
	}
	return false;
}

bool NewGameSettings::setPlantDensity( int value )
{
	if ( m_plantDensity != value )
	{
		m_plantDensity = value;
		return true;
	}
	return false;
}

void NewGameSettings::materialsForItem( QString item, QStringList& mats1, QStringList& mats2 )
{
	QVariantMap row = DB::selectRow( "Items", item );

	if ( DB::numRows( "Items_Components", item ) )
	{
		auto comps = DB::selectRows( "Items_Components", item );

		for ( auto mat : materials( comps.first().value( "ItemID" ).toString() ) )
		{
			mats1.append( mat );
		}
		for ( auto mat : materials( comps.last().value( "ItemID" ).toString() ) )
		{
			mats2.append( mat );
		}
	}
	else
	{
		for ( auto mat : materials( item ) )
		{
			mats1.append( mat );
		}
	}
}

QStringList NewGameSettings::materials( QString itemSID )
{
	QVariantMap row = DB::selectRow( "Items", itemSID );

	QStringList out;

	if ( !row.value( "AllowedMaterials" ).toString().isEmpty() )
	{
		for ( auto mat : row.value( "AllowedMaterials" ).toString().split( "|" ) )
		{
			out.append( mat );
		}
	}
	if ( !row.value( "AllowedMaterialTypes" ).toString().isEmpty() )
	{
		for ( auto type : row.value( "AllowedMaterialTypes" ).toString().split( "|" ) )
		{
			for ( auto mat : DB::select2( "ID", "Materials", "Type", type ) )
			{
				out.append( mat.toString() );
			}
		}
	}
	return out;
}

void NewGameSettings::addStartingItem( QString itemSID, QString mat1, QString mat2, int amount )
{
	if ( amount <= 0 )
	{
		amount = 1;
	}
	for ( auto& si : m_startingItems )
	{
		if ( itemSID == si.itemSID && mat1 == si.mat1 && mat2 == si.mat2 )
		{
			si.amount += amount;
			return;
		}
	}
	StartingItem si { itemSID, mat1, mat2, amount };
	m_startingItems.append( si );
}

void NewGameSettings::removeStartingItem( QString tag )
{
	auto tl = tag.split( "_" );
	QString itemSID;
	QString mat1;
	QString mat2;
	if ( tl.size() > 1 )
	{
		itemSID = tl[0];
		mat1    = tl[1];
	}
	if ( tl.size() > 2 )
	{
		mat2 = tl[2];
	}
	for ( int i = 0; i < m_startingItems.size(); ++i )
	{
		auto si = m_startingItems[i];
		if ( itemSID == si.itemSID && mat1 == si.mat1 && mat2 == si.mat2 )
		{
			m_startingItems.removeAt( i );
			return;
		}
	}
}

void NewGameSettings::addStartingAnimal( QString type, QString gender, int amount )
{
	if ( amount <= 0 )
	{
		amount = 1;
	}
	for ( auto& sa : m_startingAnimals )
	{
		if ( type == sa.type && gender == sa.gender )
		{
			sa.amount += amount;
			return;
		}
	}
	StartingAnimal sa { type, gender, amount };
	m_startingAnimals.append( sa );
}

void NewGameSettings::removeStartingAnimal( QString tag )
{
	auto tl = tag.split( "_" );
	QString type;
	QString gender;

	if ( tl.size() > 1 )
	{
		type   = tl[0];
		gender = tl[1];
	}
	for ( int i = 0; i < m_startingAnimals.size(); ++i )
	{
		auto sa = m_startingAnimals[i];
		if ( type == sa.type && gender == sa.gender )
		{
			m_startingAnimals.removeAt( i );
			return;
		}
	}
}

void NewGameSettings::loadPresets()
{
	json sd;

	const auto& exePath = Global::exePath;

	bool ok = IO::loadFile( exePath / "content" / "JSON" / "embarkpresets.json", sd );
	if ( ok )
	{
		m_standardPresets = sd;
	}
	else
	{
		throw std::runtime_error("Cannot load 'embarkPresets.json'");
	}

	ok = IO::loadFile( IO::getDataFolder() / "settings" / "userpresets.json", sd );
	if ( ok )
	{
		m_userPresets = sd;
	}
}

void NewGameSettings::saveUserPresets()
{
	json sd = m_userPresets;
	IO::saveFile( IO::getDataFolder() / "settings" / "userpresets.json", sd );
}

void NewGameSettings::setPreset( const std::string& name )
{
	m_selectedPreset = name;
	for ( const auto& pm : m_standardPresets )
	{
		if ( pm.Name == name )
		{
			setStartingItems( pm.startingItems );

			return;
		}
	}
	for ( const auto& pm : m_userPresets )
	{
		if ( pm.Name == name )
		{
			setStartingItems( pm.startingItems );

			return;
		}
	}
}

std::string NewGameSettings::addPreset()
{
	// get next user name
	int index       = 1;
	auto newName = "Custom " + std::to_string( index );
	while ( true )
	{
		bool conflict = false;
		for ( const auto& pm : m_userPresets )
		{
			auto name = pm.Name;
			if ( name == newName )
			{
				conflict = true;
				break;
			}
		}
		if ( !conflict )
		{
			break;
		}
		++index;
		newName = "Custom " + std::to_string( index );
	}

	if ( !m_selectedPreset.empty() )
	{
		for ( auto pm : m_standardPresets )
		{
			if ( pm.Name == m_selectedPreset )
			{
				pm.Name = newName;
				m_userPresets.push_back( pm );
				saveUserPresets();
				return newName;
			}
		}
		for ( auto pm : m_userPresets )
		{
			if ( pm.Name == m_selectedPreset )
			{
				pm.Name = newName;
				m_userPresets.push_back( pm );
				saveUserPresets();
				return newName;
			}
		}
	}
	else
	{
		JsonPreset pm;
		pm.Name = newName;
		m_userPresets.push_back( pm );
		saveUserPresets();
		return newName;
	}
	return "";
}

bool NewGameSettings::savePreset( const std::vector<JsonStartingItem>& items )
{
	for ( auto& pm : m_userPresets )
	{
		if ( pm.Name == m_selectedPreset )
		{
			pm.startingItems = items;

			saveUserPresets();
			return true;
		}
	}
	return false;
}

bool NewGameSettings::onSavePreset()
{
	std::vector<JsonStartingItem> sil;
	collectStartItems( sil );
	return savePreset( sil );
}

bool NewGameSettings::removePreset( const std::string& name )
{
	for ( auto it = m_userPresets.begin(); it != m_userPresets.end(); it++ )
	{
		if ( it->Name == name )
		{
			m_selectedPreset = "";
			m_userPresets.erase( it );
			saveUserPresets();
			return true;
		}
	}
	return false;
}

void NewGameSettings::setStartingItems( const std::vector<JsonStartingItem>& sil )
{
	m_startingItems.clear();
	m_startingAnimals.clear();

	for ( const auto& sipm : sil )
	{
		if ( const JsonNormalItem* item = std::get_if<JsonNormalItem>( &sipm ) )
		{
			auto itemSID = QString::fromStdString( item->ItemID );
			auto mat1    = QString::fromStdString( item->MaterialID );
			int amount      = item->Amount;
			StartingItem si { itemSID, mat1, "", amount };
			m_startingItems.append( si );
		} else if ( const JsonCombinedItem* item = std::get_if<JsonCombinedItem>( &sipm ) )
		{
			auto itemSID = QString::fromStdString( item->ItemID );
			int amount      = item->Amount;
			auto components = item->Components;
			if ( components.size() == 2 )
			{
				auto mat1 = QString::fromStdString(components[0].MaterialID);
				auto mat2 = QString::fromStdString(components[1].MaterialID);

				StartingItem si { itemSID, mat1, mat2, amount };
				m_startingItems.append( si );
			}
		} else if ( const JsonAnimalItem* item = std::get_if<JsonAnimalItem>( &sipm ) )
		{
			QString type   = QString::fromStdString(item->ItemID);
			QString gender = QString::fromStdString(item->Gender);
			gender.replace( 0, 1, gender[0].toUpper() );
			int amount = item->Amount;
			StartingAnimal si { type, gender, amount };
			m_startingAnimals.append( si );
		}
	}
}

std::vector<std::string> NewGameSettings::presetNames()
{
	std::vector<std::string> out;

	for ( const auto& p : m_standardPresets )
	{
		out.push_back(p.Name);
	}
	for ( const auto& p : m_userPresets )
	{
		out.push_back( p.Name );
	}
	return out;
}

std::vector<CheckableItem> NewGameSettings::trees()
{
	return filterCheckableItems( "Tree" );
}

std::vector<CheckableItem> NewGameSettings::plants()
{
	return filterCheckableItems( "Plant" );
}

std::vector<CheckableItem> NewGameSettings::animals()
{
	return filterCheckableItems( "Animal" );
}

bool NewGameSettings::isChecked( QString sid )
{
	if ( m_checkableItems.contains( sid ) )
	{
		CheckableItem& ci = m_checkableItems[sid];
		return ci.isChecked;
	}
	return false;
}

void NewGameSettings::setChecked( QString sid, bool value )
{
	if ( m_checkableItems.contains( sid ) )
	{
		auto& ci     = m_checkableItems[sid];
		ci.isChecked = value;
	}
}

bool NewGameSettings::setPeaceful( bool value )
{
	if ( m_isPeaceful != value )
	{
		m_isPeaceful = value;
		return true;
	}
	return false;
}

bool NewGameSettings::setMaxPerType( int value )
{
	if ( m_maxPerType != value )
	{
		m_maxPerType = value;
		return true;
	}
	return false;
}

bool NewGameSettings::setNumWildAnimals( int value )
{
	if ( m_numWildAnimals != value )
	{
		m_numWildAnimals = value;
		return true;
	}
	return false;
}

int NewGameSettings::maxAnimalsPerType( QString type )
{
	if( m_checkableItems.contains( type ) )
	{
		const auto& ci = m_checkableItems.at( type );
		return ci.max;
	}
	return 0;
}

void NewGameSettings::setAmount( QString sid, int value )
{
	if ( m_checkableItems.contains( sid ) )
	{
		auto& ci     = m_checkableItems[sid];
		ci.max = value;
	}
}

std::vector<CheckableItem> NewGameSettings::filterCheckableItems( const QString& itemType )
{
	std::vector<CheckableItem> result;
	const auto &values = ranges::views::values(m_checkableItems);
	std::copy_if( values.begin(), values.end(),
				  std::back_inserter( result ),
				  [itemType]( const CheckableItem& item )
				  { return item.type == itemType; } );
	return result;
}
