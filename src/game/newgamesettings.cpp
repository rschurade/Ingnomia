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
/** @file newgamesettings.cpp
 *  @brief New-game configuration: world parameters, starting items/animals, embark presets, and flora/fauna allow-lists.
 */
#include "newgamesettings.h"

#include "../base/config.h"
#include "../base/db.h"
#include "../base/io.h"
#include "../gui/strings.h"

#include <QDebug>
#include <QJsonDocument>
#include <QStandardPaths>

#include <random>

/// @brief Constructs the settings object: loads the embark map and presets, then randomises the seed and kingdom name.
/// @param parent Qt parent object.
NewGameSettings::NewGameSettings( QObject* parent ) :
	QObject( parent )
{
	loadEmbarkMap();
	loadPresets();

	setRandomSeed();
	setRandomName();
}

/// @brief Destructor.
NewGameSettings::~NewGameSettings()
{
}

/// @brief Persists all current settings to newgame.json in the user data folder and saves the active preset.
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

	for( const auto& ci : m_checkableItems )
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


	QVariantList startItems;
	collectStartItems( startItems );

	embarkMap.insert( "startingItems", startItems );

	QJsonDocument sd = QJsonDocument::fromVariant( embarkMap );
	IO::saveFile( IO::getDataFolder() + "/settings/newgame.json", sd );

	savePreset( startItems );
}

/// @brief Builds a serialised starting-items list from the current m_startingItems and m_startingAnimals.
///        Simple items produce a Type=Item entry; two-component items produce a Type=CombinedItem entry with
///        a Components sub-list; animals produce a Type=Animal entry.
/// @param sil Output list to populate.
void NewGameSettings::collectStartItems( QVariantList& sil )
{
	for( const auto& si : m_startingItems )
	{
		QVariantMap sim;
		if( si.mat2.isEmpty() )
		{
			sim.insert( "Amount", si.amount );
			sim.insert( "ItemID", si.itemSID );
			sim.insert( "MaterialID", si.mat1 );
			sim.insert( "Type", "Item" );
		}
		else
		{
			auto rows = DB::selectRows( "Items_Components", si.itemSID );
			if( rows.size() > 1 )
			{
				sim.insert( "Amount", si.amount );
				sim.insert( "ItemID", si.itemSID );

				QVariantList comps;
				QVariantMap cm1;
				QVariantMap cm2;
			
				cm1.insert( "ItemID", rows[0].value( "ItemID" ).toString() );
				cm1.insert( "MaterialID", si.mat1 );
				cm2.insert( "ItemID", rows[1].value( "ItemID" ).toString() );
				cm2.insert( "MaterialID", si.mat2 );

				comps.append( cm1 );
				comps.append( cm2 );
				sim.insert( "Components", comps );
				sim.insert( "Type", "CombinedItem" );
			}
		}
		sil.append( sim );
	}
	for( const auto& si : m_startingAnimals )
	{
		QVariantMap sim;
		sim.insert( "Amount", si.amount );
		sim.insert( "ItemID", si.type );
		sim.insert( "Gender", si.gender );
		sim.insert( "Type", "Animal" );
		sil.append( sim );
	}
}

/// @brief Loads embark configuration from newgame.json (user data folder), falling back to the bundled JSON.
///        Populates checkable tree/plant/animal lists and all scalar world-generation parameters.
void NewGameSettings::loadEmbarkMap()
{
	m_checkableItems.clear();

	QJsonDocument sd;
	if ( !IO::loadFile( IO::getDataFolder() + "/settings/newgame.json", sd ) )
	{
		// if it doesn't exist get from /content/JSON
		if ( IO::loadFile( Global::cfg->get( "dataPath" ).toString() + "/JSON/newgame.json", sd ) )
		{
			IO::saveFile( IO::getDataFolder() + "/settings/newgame.json", sd );
		}
		else
		{
			qDebug() << "Unable to find new game config!";
			abort();
		}
	}
	auto embarkMap = sd.toVariant().toMap();

	QVariantList out;
	QStringList trees = DB::ids( "Plants", "Type", "Tree" );

	for ( auto id : trees )
	{
		CheckableItem ci { id, S::s( "$ItemName_" + id ), "Tree", embarkMap.value( "allowedTrees" ).toMap().value( id ).toBool(), 0 };
		m_checkableItems.insert( id, ci );
	}

	QStringList allplants = DB::ids( "Plants", "Type", "Plant" );
	allplants.sort();
	for ( auto id : allplants )
	{
		if ( DB::select( "AllowInWild", "Plants", id ).toBool() )
		{
			CheckableItem ci { id, S::s( "$MaterialName_" + id ), "Plant", embarkMap.value( "allowedPlants" ).toMap().value( id ).toBool(), 0 };
			m_checkableItems.insert( id, ci );
		}
	}

	QStringList allAnimals = DB::ids( "Animals" );
	allAnimals.sort();
	auto allowedAnimals = embarkMap.value( "allowedAnimals" ).toMap();

	for ( auto id : allAnimals )
	{
		if ( DB::select( "AllowInWild", "Animals", id ).toBool() && DB::select( "Biome", "Animals", id ).toString().isEmpty() )
		{
			CheckableItem ci { id, S::s( "$CreatureName_" + id ), "Animal", allowedAnimals.value( id ).toBool(), allowedAnimals.value( id ).toInt() };
			m_checkableItems.insert( id, ci );
		}
	}

	auto sil = embarkMap.value( "startingItems" ).toList();
	setStartingItems( sil );

	// QString m_kingdomName not in embark map, we want to change it on every embark
	m_seed = embarkMap.value( "seed" ).toString();

	m_worldSize    = embarkMap.value( "dimX" ).toInt();
	m_zLevels      = embarkMap.value( "dimZ" ).toInt();
	m_ground       = embarkMap.value( "groundLevel" ).toInt();
	m_flatness     = embarkMap.value( "flatness" ).toInt();
	m_oceanSize    = embarkMap.value( "oceanSize" ).toInt();
	m_rivers       = embarkMap.value( "rivers" ).toInt();
	m_riverSize    = embarkMap.value( "riverSize" ).toInt();
	m_numGnomes    = embarkMap.value( "numGnomes" ).toInt();
	m_startZone    = embarkMap.value( "startingZone" ).toInt();
	m_treeDensity  = embarkMap.value( "treeDensity" ).toInt();
	m_plantDensity = embarkMap.value( "plantDensity" ).toInt();
	m_isPeaceful   = embarkMap.value( "peaceful" ).toBool();

	m_maxPerType     = embarkMap.value( "maxPerType" ).toInt();
	m_numWildAnimals = embarkMap.value( "numAnimals" ).toInt();
}

/// @brief Assigns a random kingdom name from the localisation string table.
void NewGameSettings::setRandomName()
{
	m_kingdomName = S::gi().randomKingdomName();
}

/// @brief Generates a cryptographically-seeded random integer and stores it as the world seed string.
void NewGameSettings::setRandomSeed()
{
	std::random_device rd;                                // only used once to initialise (seed) engine
	std::mt19937 rng( rd() );                             // random-number engine used (Mersenne-Twister in this case)
	std::uniform_int_distribution<int> uni( 0, INT_MAX ); // guaranteed unbiased

	auto random_integer = uni( rng );

	m_seed = QString::number( random_integer );
}

/// @brief Sets the kingdom name if it differs from the current value.
/// @param value New kingdom name.
/// @return true if the value changed.
bool NewGameSettings::setKingdomName( QString value )
{
	if ( m_kingdomName != value )
	{
		m_kingdomName = value;
		return true;
	}
	return false;
}

/// @brief Sets the world generation seed string if it differs from the current value.
/// @param value New seed string.
/// @return true if the value changed.
bool NewGameSettings::setSeed( QString value )
{
	if ( m_seed != value )
	{
		m_seed = value;
		return true;
	}
	return false;
}

/// @brief Sets the world X/Y dimension if changed. @param value New size. @return true if changed.
bool NewGameSettings::setWorldSize( int value )
{
	if ( m_worldSize != value )
	{
		m_worldSize = value;
		return true;
	}
	return false;
}
/// @brief Sets the number of Z levels if changed. @param value New Z level count. @return true if changed.
bool NewGameSettings::setZLevels( int value )
{
	if ( m_zLevels != value )
	{
		m_zLevels = value;
		return true;
	}
	return false;
}
/// @brief Sets the ground level if changed. @param value New ground level. @return true if changed.
bool NewGameSettings::setGround( int value )
{
	if ( m_ground != value )
	{
		m_ground = value;
		return true;
	}
	return false;
}
/// @brief Sets terrain flatness if changed. @param value New flatness value. @return true if changed.
bool NewGameSettings::setFlatness( int value )
{
	if ( m_flatness != value )
	{
		m_flatness = value;
		return true;
	}
	return false;
}
/// @brief Sets ocean size if changed. @param value New ocean size. @return true if changed.
bool NewGameSettings::setOceanSize( int value )
{
	if ( m_oceanSize != value )
	{
		m_oceanSize = value;
		return true;
	}
	return false;
}
/// @brief Sets the number of rivers if changed. @param value New river count. @return true if changed.
bool NewGameSettings::setRivers( int value )
{
	if ( m_rivers != value )
	{
		m_rivers = value;
		return true;
	}
	return false;
}
/// @brief Sets river width if changed. @param value New river size. @return true if changed.
bool NewGameSettings::setRiverSize( int value )
{
	if ( m_riverSize != value )
	{
		m_riverSize = value;
		return true;
	}
	return false;
}
/// @brief Sets the starting gnome count if changed. @param value New gnome count. @return true if changed.
bool NewGameSettings::setNumGnomes( int value )
{
	if ( m_numGnomes != value )
	{
		m_numGnomes = value;
		return true;
	}
	return false;
}

/// @brief Sets the starting zone index if changed. @param value New zone. @return true if changed.
bool NewGameSettings::setStartZone( int value )
{
	if ( m_startZone != value )
	{
		m_startZone = value;
		return true;
	}
	return false;
}

/// @brief Sets tree density if changed. @param value New density (0–100). @return true if changed.
bool NewGameSettings::setTreeDensity( int value )
{
	if ( m_treeDensity != value )
	{
		m_treeDensity = value;
		return true;
	}
	return false;
}

/// @brief Sets plant density if changed. @param value New density (0–100). @return true if changed.
bool NewGameSettings::setPlantDensity( int value )
{
	if ( m_plantDensity != value )
	{
		m_plantDensity = value;
		return true;
	}
	return false;
}

/// @brief Populates @p mats1 (and optionally @p mats2) with valid material SIDs for @p item.
///        For two-component items the first component's materials go to mats1 and the second to mats2.
///        For simple items all materials go to mats1 and mats2 remains empty.
/// @param item  Item SID to look up.
/// @param mats1 Output: valid materials for the item (or its first component).
/// @param mats2 Output: valid materials for the second component (empty for simple items).
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

/// @brief Returns the list of allowed material SIDs for @p itemSID from AllowedMaterials and AllowedMaterialTypes DB columns.
/// @param itemSID Item SID to look up.
/// @return List of material SIDs.
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

/// @brief Adds or increments a starting item entry. If a matching item+material combination already exists,
///        its amount is incremented; otherwise a new entry is appended.
/// @param itemSID Item string ID.
/// @param mat1    Primary material SID.
/// @param mat2    Secondary material SID (empty for simple items).
/// @param amount  Quantity to add (clamped to minimum 1).
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

/// @brief Removes the starting item identified by @p tag (format: "ItemSID_mat1" or "ItemSID_mat1_mat2").
/// @param tag Underscore-separated tag string produced by the GUI.
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

/// @brief Adds or increments a starting animal entry. Increments an existing type+gender entry or appends a new one.
/// @param type   Animal type SID.
/// @param gender "Male" or "Female".
/// @param amount Quantity to add (clamped to minimum 1).
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

/// @brief Removes the starting animal identified by @p tag (format: "type_gender").
/// @param tag Underscore-separated tag string produced by the GUI.
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

/// @brief Loads standard presets from content/JSON/embarkpresets.json and user presets from the data folder.
void NewGameSettings::loadPresets()
{
	QJsonDocument sd;

	QString exePath = QCoreApplication::applicationDirPath();

	bool ok = IO::loadFile( exePath + "/content/JSON/embarkpresets.json", sd );
	if ( ok )
	{
		m_standardPresets = sd.toVariant().toList();
	}

	ok = IO::loadFile( IO::getDataFolder() + "/settings/userpresets.json", sd );
	if ( ok )
	{
		m_userPresets.clear();
		for( auto vp : sd.toVariant().toList() )
		{
			m_userPresets.append( vp.toMap() );
		}
	}
}

/// @brief Writes the current user preset list to userpresets.json in the data folder.
void NewGameSettings::saveUserPresets()
{
	QVariantList up;
	for( auto pm : m_userPresets )
	{
		up.append( pm );
	}
	QJsonDocument sd = QJsonDocument::fromVariant( up );
	IO::saveFile( IO::getDataFolder() + "/settings/userpresets.json", sd );
}

/// @brief Activates the named preset, replacing the current starting-item list with its contents.
///        Searches standard presets first, then user presets.
/// @param name Preset name to select.
void NewGameSettings::setPreset( QString name )
{
	m_selectedPreset = name;
	for ( auto p : m_standardPresets )
	{
		auto pm = p.toMap();
		if ( pm.value( "Name" ).toString() == name )
		{
			auto sil = pm.value( "startingItems" ).toList();

			setStartingItems( sil );

			return;
		}
	}
	for ( auto pm : m_userPresets )
	{
		if ( pm.value( "Name" ).toString() == name )
		{
			auto sil = pm.value( "startingItems" ).toList();

			setStartingItems( sil );

			return;
		}
	}
}

/// @brief Creates a new user preset with an auto-generated "Custom N" name, cloned from the currently selected
///        preset (or empty if none is selected). Saves immediately.
/// @return The new preset's name, or an empty string on failure.
QString NewGameSettings::addPreset()
{
	// get next user name
	int index       = 1;
	QString newName = "Custom " + QString::number( index );
	while ( true )
	{
		bool conflict = false;
		for ( auto pm : m_userPresets )
		{
			auto name = pm.value( "Name" ).toString();
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
		newName = "Custom " + QString::number( index );
	}

	if ( !m_selectedPreset.isEmpty() )
	{
		for ( auto p : m_standardPresets )
		{
			auto pm = p.toMap();
			if ( pm.value( "Name" ).toString() == m_selectedPreset )
			{
				pm.insert( "Name", newName );
				m_userPresets.append( pm );
				saveUserPresets();
				return newName;
			}
		}
		for ( auto pm : m_userPresets )
		{
			if ( pm.value( "Name" ).toString() == m_selectedPreset )
			{
				pm.insert( "Name", newName );
				m_userPresets.append( pm );
				saveUserPresets();
				return newName;
			}
		}
	}
	else
	{
		QVariantMap pm;
		pm.insert( "Name", newName );
		pm.insert( "startingItems", QVariantList() );
		m_userPresets.append( pm );
		saveUserPresets();
		return newName;
	}
	return "";
}

/// @brief Saves the given item list into the currently selected user preset and persists to disk.
/// @param items Serialised starting-items list to store.
/// @return true if the selected preset was found and updated, false otherwise.
bool NewGameSettings::savePreset(  QVariantList items )
{
	for ( auto& pm : m_userPresets )
	{
		if ( pm.value( "Name" ).toString() == m_selectedPreset )
		{
			pm.insert( "startingItems", items );
		
			saveUserPresets();
			return true;
		}
	}
	return false;
}

/// @brief Collects the current starting items and saves them into the selected preset.
/// @return true if the preset was saved successfully.
bool NewGameSettings::onSavePreset()
{
	QVariantList sil;
	collectStartItems( sil );
	return savePreset( sil );
}

/// @brief Removes the user preset with the given name and persists the change.
/// @param name Preset name to remove.
/// @return true if found and removed, false if the preset was not in the user list.
bool NewGameSettings::removePreset( QString name )
{
	for ( int i = 0; i < m_userPresets.size(); ++i )
	{
		auto pm = m_userPresets[i];
		if ( pm.value( "Name" ).toString() == name )
		{
			m_selectedPreset = "";
			m_userPresets.removeAt( i );
			saveUserPresets();
			return true;
		}
	}
	return false;
}

/// @brief Replaces the current starting-item and starting-animal lists by parsing a serialised QVariantList.
///        Handles Type=Item, Type=CombinedItem, and Type=Animal entries.
/// @param sil Serialised list produced by collectStartItems() or loaded from a preset.
void NewGameSettings::setStartingItems( QVariantList sil )
{
	m_startingItems.clear();
	m_startingAnimals.clear();

	for ( auto vsi : sil )
	{
		auto sipm = vsi.toMap();
		if ( sipm.value( "Type" ).toString() == "Item" )
		{
			QString itemSID = sipm.value( "ItemID" ).toString();
			QString mat1    = sipm.value( "MaterialID" ).toString();
			int amount      = sipm.value( "Amount" ).toInt();
			StartingItem si { itemSID, mat1, "", amount };
			m_startingItems.append( si );
		}
		else if ( sipm.value( "Type" ).toString() == "CombinedItem" )
		{
			QString itemSID = sipm.value( "ItemID" ).toString();
			int amount      = sipm.value( "Amount" ).toInt();
			auto components = sipm.value( "Components" ).toList();
			if ( components.size() == 2 )
			{
				auto vmc1 = components[0].toMap();
				auto vmc2 = components[0].toMap();

				QString mat1 = vmc1.value( "MaterialID" ).toString();
				QString mat2 = vmc2.value( "MaterialID" ).toString();

				StartingItem si { itemSID, mat1, mat2, amount };
				m_startingItems.append( si );
			}
		}
		if ( sipm.value( "Type" ).toString() == "Animal" )
		{
			QString type   = sipm.value( "ItemID" ).toString();
			QString gender = sipm.value( "Gender" ).toString();
			gender.replace( 0, 1, gender[0].toUpper() );
			int amount = sipm.value( "Amount" ).toInt();
			StartingAnimal si { type, gender, amount };
			m_startingAnimals.append( si );
		}
	}
}

/// @brief Returns the names of all available presets (standard presets first, then user presets).
/// @return Combined list of preset names.
QStringList NewGameSettings::presetNames()
{
	QStringList out;
	for ( auto p : m_standardPresets )
	{
		out.append( p.toMap().value( "Name" ).toString() );
	}
	for ( auto pm : m_userPresets )
	{
		out.append( pm.value( "Name" ).toString() );
	}
	return out;
}

/// @brief Returns a list of all tree types as variant maps with ID, Name, and Allowed fields.
/// @return QVariantList of tree checkable items.
QVariantList NewGameSettings::trees()
{
	QVariantList out;
	for ( auto ci : m_checkableItems )
	{
		if ( ci.type == "Tree" )
		{
			QVariantMap entry;
			entry.insert( "ID", ci.sid );
			entry.insert( "Name", ci.name );
			entry.insert( "Allowed", ci.isChecked );
			out.append( entry );
		}
	}
	return out;
}

/// @brief Returns a list of all plant types as variant maps with ID, Name, and Allowed fields.
/// @return QVariantList of plant checkable items.
QVariantList NewGameSettings::plants()
{
	QVariantList out;
	for ( auto ci : m_checkableItems )
	{
		if ( ci.type == "Plant" )
		{
			QVariantMap entry;
			entry.insert( "ID", ci.sid );
			entry.insert( "Name", ci.name );
			entry.insert( "Allowed", ci.isChecked );
			out.append( entry );
		}
	}
	return out;
}

/// @brief Returns a list of all wild animal types as variant maps with ID, Name, Allowed, and Amount fields.
/// @return QVariantList of animal checkable items.
QVariantList NewGameSettings::animals()
{
	QVariantList out;
	for ( auto ci : m_checkableItems )
	{
		if ( ci.type == "Animal" )
		{
			QVariantMap entry;
			entry.insert( "ID", ci.sid );
			entry.insert( "Name", ci.name );
			entry.insert( "Allowed", ci.isChecked );
			entry.insert( "Amount", ci.max );
			out.append( entry );
		}
	}
	return out;
}

/// @brief Returns whether the tree/plant/animal with the given SID is allowed in this world.
/// @param sid String ID to query.
/// @return true if checked/allowed, false if unchecked or not found.
bool NewGameSettings::isChecked( QString sid )
{
	if ( m_checkableItems.contains( sid ) )
	{
		CheckableItem& ci = m_checkableItems[sid];
		return ci.isChecked;
	}
	return false;
}

/// @brief Sets the allowed/checked state for the tree/plant/animal with the given SID.
/// @param sid   String ID to update.
/// @param value true to allow, false to disallow.
void NewGameSettings::setChecked( QString sid, bool value )
{
	if ( m_checkableItems.contains( sid ) )
	{
		auto& ci     = m_checkableItems[sid];
		ci.isChecked = value;
	}
}

/// @brief Sets the peaceful-mode flag if changed. @param value true for peaceful mode. @return true if changed.
bool NewGameSettings::setPeaceful( bool value )
{
	if ( m_isPeaceful != value )
	{
		m_isPeaceful = value;
		return true;
	}
	return false;
}

/// @brief Sets the max-animals-per-type cap if changed. @param value New cap. @return true if changed.
bool NewGameSettings::setMaxPerType( int value )
{
	if ( m_maxPerType != value )
	{
		m_maxPerType = value;
		return true;
	}
	return false;
}

/// @brief Sets the total wild-animal count if changed. @param value New count. @return true if changed.
bool NewGameSettings::setNumWildAnimals( int value )
{
	if ( m_numWildAnimals != value )
	{
		m_numWildAnimals = value;
		return true;
	}
	return false;
}

/// @brief Returns the maximum spawn count configured for the given animal type.
/// @param type Animal type SID.
/// @return Max count from the checkable-items map, or 0 if not found.
int NewGameSettings::maxAnimalsPerType( QString type )
{
	if( m_checkableItems.contains( type ) )
	{
		const auto& ci = m_checkableItems.value( type );
		return ci.max;
	}
	return 0;
}

/// @brief Sets the max-spawn amount for the checkable item with the given SID.
/// @param sid   String ID of the tree/plant/animal.
/// @param value New maximum amount.
void NewGameSettings::setAmount( QString sid, int value )
{
	if ( m_checkableItems.contains( sid ) )
	{
		auto& ci     = m_checkableItems[sid];
		ci.max = value;
	}
}
