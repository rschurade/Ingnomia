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

/** @file io.cpp
 *  @brief Implementation of the IO class for save/load operations.
 *
 *  Handles all serialization and deserialization of game state, including
 *  the world grid (binary), entities (JSON), configuration, and chunked
 *  save/load for large collections (items, plants, animals, monsters).
 *  Also provides static file utilities and version compatibility checks.
 */

#include "io.h"

#include "../base/config.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/position.h"
#include "../base/util.h"
#include "../game/creaturemanager.h"
#include "../game/eventmanager.h"
#include "../game/farmingmanager.h"
#include "../game/fluidmanager.h"
#include "../game/game.h"
#include "../game/gnomefactory.h"
#include "../game/gnomemanager.h"
#include "../game/inventory.h"
#include "../game/itemhistory.h"
#include "../game/jobmanager.h"
#include "../game/mechanismmanager.h"
#include "../game/militarymanager.h"
#include "../game/neighbormanager.h"
#include "../game/plant.h"
#include "../game/room.h"
#include "../game/roommanager.h"
#include "../game/stockpile.h"
#include "../game/stockpilemanager.h"
#include "../game/workshopmanager.h"
#include "../game/world.h"
#include "../gfx/sprite.h"
#include "../gfx/spritefactory.h"
#include "../gui/strings.h"

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSaveFile>
#include <QStandardPaths>

#include <unordered_set>

/** @brief Constructs the IO handler.
 *  @param game Pointer to the Game instance that owns all managers.
 *  @param parent Optional QObject parent for Qt memory management.
 */
IO::IO( Game* game, QObject* parent ) :
	g( game ),
	QObject( parent )
{
}

/** @brief Destructor. */
IO::~IO()
{
}

/** @brief Checks whether a save game in the given folder is compatible with the current version.
 *  @param folder Path to the save game folder.
 *  @return True if the save version is >= 0.7.4.1 (version int 741), false otherwise.
 */
bool IO::saveCompatible( QString folder )
{
	int vi = versionInt( folder );
	if ( vi < 741 )
	{
		return false;
	}
	return true;
}

/** @brief Checks whether any save game exists in the user data save folder.
 *  @return True if the save directory exists and is not empty.
 */
bool IO::saveGameExists()
{
	QString folder = getDataFolder() + "/save/";

	if ( !QDir( folder ).exists() || QDir( folder ).isEmpty() )
	{
		return false;
	}
	return true;
}

/** @brief Saves the current global configuration to config.json in the settings folder.
 *  @return True on success.
 */
bool IO::saveConfig()
{
	QString folder = getDataFolder() + "/settings/";

	QVariantMap cm   = Global::cfg->object();
	QJsonDocument jd = QJsonDocument::fromVariant( cm );

	IO::saveFile( folder + "config.json", jd );

	return true;
}

/** @brief Returns the platform-specific user data folder for Ingnomia.
 *  @return On Windows: Documents/My Games/Ingnomia. On Linux: ~/.local/share/Ingnomia.
 */
QString IO::getDataFolder()
{
#ifdef _WIN32
	return QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/My Games/Ingnomia";
#else
	// corresponds to ~/.local/share/<APPNAME> on Linux
	return QStandardPaths::writableLocation( QStandardPaths::AppDataLocation ) + "/Ingnomia";
#endif
}

/** @brief Returns the path to the temporary folder inside the user data directory.
 *  @return Path to the tmp subfolder.
 */
QString IO::getTempFolder()
{
	return getDataFolder() + "/tmp/";
}

/** @brief Creates all required subdirectories (mods, save, screenshots, settings, tmp) in the
 *         user data folder, and copies default config files from the application content directory
 *         if they do not already exist.
 *  @return True if the save folder exists after creation.
 */
bool IO::createFolders()
{
	QString folder = getDataFolder() + "/";
	if ( !QDir( folder ).exists() )
	{
		QDir().mkpath( folder );
	}
	QString modFolder = folder + "mods/";
	if ( !QDir( modFolder ).exists() )
	{
		QDir().mkdir( modFolder );
	}
	QString saveFolder = folder + "save/";
	if ( !QDir( saveFolder ).exists() )
	{
		QDir().mkdir( saveFolder );
	}
	QString ssFolder = folder + "screenshots/";
	if ( !QDir( ssFolder ).exists() )
	{
		QDir().mkdir( ssFolder );
	}
	QString settingsFolder = folder + "settings/";
	if ( !QDir( settingsFolder ).exists() )
	{
		QDir().mkdir( settingsFolder );
	}
	QString tmpFolder = folder + "tmp/";
	if ( !QDir( tmpFolder ).exists() )
	{
		QDir().mkdir( tmpFolder );
	}

	QString exePath = QCoreApplication::applicationDirPath();

	if ( !QFile::exists( folder + "settings/profs.json" ) && QFile::exists( exePath + "/content/JSON/profs.json" ) )
	{
		QFile::copy( exePath + "/content/JSON/profs.json", folder + "settings/profs.json" );
	}
	if ( !QFile::exists( folder + "settings/config.json" ) && QFile::exists( exePath + "/content/JSON/config.json" ) )
	{
		QFile::copy( exePath + "/content/JSON/config.json", folder + "settings/config.json" );
	}
	if ( !QFile::exists( folder + "settings/newgame.json" ) && QFile::exists( exePath + "/content/JSON/newgame.json" ) )
	{
		QFile::copy( exePath + "/content/JSON/newgame.json", folder + "settings/newgame.json" );
	}
	/*
	if ( !QFile::exists( folder + "settings/keybindings.json" ) && QFile::exists( exePath + "/content/JSON/keybindings.json" ) )
	{
		QFile::copy( exePath + "/content/JSON/keybindings.json", folder + "settings/keybindings.json" );
	}
	*/

	folder = getDataFolder() + "/save/";

	return QDir( folder ).exists();
}

/** @brief Loads the original (shipped) config.json from the application content directory.
 *  @param jd Output parameter receiving the parsed JSON document.
 *  @return True if the file was loaded and parsed successfully.
 */
bool IO::loadOriginalConfig( QJsonDocument& jd )
{
	qDebug() << "load standard config";
	QString exePath = QCoreApplication::applicationDirPath();
	return IO::loadFile( exePath + "/content/JSON/config.json", jd );
}

/** @brief Saves the entire game state to disk.
 *
 *  Creates a save folder named after the kingdom. For manual saves, increments a
 *  numbered slot; for autosaves, uses an "autosave" subfolder. Existing folders are
 *  backed up and removed after a successful save. Serializes world, sprites, game
 *  state, config, items, constructions, stockpiles, jobs, gnomes, monsters, plants,
 *  animals, farms, workshops, rooms, doors, item history, events, mechanisms, and pipes.
 *
 *  @param autosave If true, saves to the "autosave" slot instead of a new numbered slot.
 *  @return The path to the folder where the game was saved.
 */
QString IO::save( bool autosave )
{
	QElapsedTimer timer;
	timer.start();

	QString folder = getDataFolder() + "/save/";

	if ( !QDir( folder ).exists() )
	{
		QDir().mkdir( folder );
	}
	QString name = GameState::kingdomName;
	name         = name.simplified();
	name.replace( " ", "" );
	folder += name;
	folder += "/";

	if ( !QDir( folder ).exists() )
	{
		QDir().mkdir( folder );
	}
	int slot = 0;

	if ( autosave )
	{
		folder += "autosave";
	}
	else
	{

		QDirIterator directories( folder, QDir::Dirs | QDir::NoDotAndDotDot );
		QStringList dirs;
		while ( directories.hasNext() )
		{
			directories.next();
			dirs.push_back( directories.filePath() );
		}

		for ( auto sdir : dirs )
		{
			if ( sdir.endsWith( "/" ) )
			{
				sdir.remove( sdir.size() - 1, 1 );
			}
			QString slotDir = sdir.split( "/" ).last();

			bool ok;
			int num = slotDir.toInt( &ok );
			if ( ok )
			{
				slot = qMax( slot, num );
			}
		}
		++slot;
		folder += QString::number( slot );
	}

	if ( Global::debugMode )
		qDebug() << folder;
	QString oldFolder;
	if ( QDir( folder ).exists() )
	{
		oldFolder = folder + ".backup";
		QDir().rename( folder, oldFolder );
	}
	QDir().mkdir( folder );
	
	folder += "/";

	IO::saveWorld( folder );
	IO::saveFile( folder + "sprites.json", IO::jsonArraySprites() );
	IO::saveFile( folder + "game.json", IO::jsonArrayGame() );
	IO::saveFile( folder + "config.json", IO::jsonArrayConfig() );

	IO::saveItems( folder );

	IO::saveFile( folder + "wallconstructions.json", IO::jsonArrayWallConstructions() );
	IO::saveFile( folder + "floorconstructions.json", IO::jsonArrayFloorConstructions() );

	IO::saveFile( folder + "stockpiles.json", IO::jsonArrayStockpiles() );
	IO::saveFile( folder + "jobs.json", IO::jsonArrayJobs() );
	IO::saveFile( folder + "jobsprites.json", IO::jsonArrayJobSprites() );

	IO::saveFile( folder + "gnomes.json", IO::jsonArrayGnomes() );

	IO::saveMonsters( folder );

	IO::savePlants( folder );
	IO::saveAnimals( folder );

	IO::saveFile( folder + "farms.json", IO::jsonArrayFarms() );
	IO::saveFile( folder + "workshops.json", IO::jsonArrayWorkshops() );

	IO::saveFile( folder + "rooms.json", IO::jsonArrayRooms() );
	IO::saveFile( folder + "doors.json", IO::jsonArrayDoors() );

	IO::saveFile( folder + "itemhistory.json", IO::jsonArrayItemHistory() );
	IO::saveFile( folder + "events.json", IO::jsonArrayEvents() );

	IO::saveFile( folder + "mechanisms.json", IO::jsonArrayMechanisms() );
	IO::saveFile( folder + "pipes.json", IO::jsonArrayPipes() );

	qDebug() << "saving game took: " + QString::number( timer.elapsed() ) + " ms";

	if (!oldFolder.isEmpty())
	{
		QDir( oldFolder ).removeRecursively();
		qDebug() << "Savegame backup removed";
	}

	return folder;
}

/** @brief Loads an entire game state from the given save folder.
 *
 *  Deserializes all game data in dependency order: game state, sprites, world grid,
 *  items, constructions, jobs, workshops, farms, stockpiles, mechanisms, pipes,
 *  gnomes, monsters, plants, animals, rooms, doors, item history, events, and config.
 *  Runs sanitize() at the end to fix up any inconsistencies.
 *
 *  @param folder Path to the save game folder to load from.
 *  @return True if loading succeeded, false if the world file could not be read.
 */
bool IO::load( QString folder )
{
	if ( !folder.endsWith( "/" ) )
	{
		folder += "/";
	}

	QElapsedTimer timer;
	timer.start();

	IO::version = versionInt( folder );

	QJsonDocument jd;

	loadFile( folder + "game.json", jd );
	IO::loadGame( jd );

	Global::util->initAllowedInContainer();

	loadFile( folder + "sprites.json", jd );
	IO::loadSprites( jd );
	emit signalStatus( "Start loading world.." );
	if ( !IO::loadWorld( folder ) )
	{
		return false;
	}
	g->w()->afterLoad();
	emit signalStatus( "Loading world done" );
	IO::loadItems( folder );
	emit signalStatus( "Loading items done" );
	loadFile( folder + "floorconstructions.json", jd );
	IO::loadFloorConstructions( jd );
	loadFile( folder + "wallconstructions.json", jd );
	IO::loadWallConstructions( jd );
	emit signalStatus( "Loading constructions done" );
	loadFile( folder + "jobs.json", jd );
	IO::loadJobs( jd );
	loadFile( folder + "jobsprites.json", jd );
	IO::loadJobSprites( jd );
	emit signalStatus( "Loading jobs done" );
	loadFile( folder + "workshops.json", jd );
	IO::loadWorkshops( jd );
	loadFile( folder + "farms.json", jd );
	IO::loadFarms( jd );
	loadFile( folder + "stockpiles.json", jd );
	IO::loadStockpiles( jd );
	loadFile( folder + "mechanisms.json", jd );
	IO::loadMechanisms( jd );
	loadFile( folder + "pipes.json", jd );
	IO::loadPipes( jd );
	// anything that has local jobs needs to be loaded before this
	loadFile( folder + "gnomes.json", jd );
	IO::loadGnomes( jd );
	loadFile( folder + "monsters.json", jd );
	IO::loadMonsters( folder );
	IO::loadPlants( folder );
	IO::loadAnimals( folder );
	emit signalStatus( "Loading gnomes, plants and animals done" );
	loadFile( folder + "rooms.json", jd );
	IO::loadRooms( jd );
	loadFile( folder + "doors.json", jd );
	IO::loadDoors( jd );
	loadFile( folder + "itemhistory.json", jd );
	IO::loadItemHistory( jd );
	loadFile( folder + "events.json", jd );
	IO::loadEvents( jd );

	loadFile( folder + "config.json", jd );
	IO::loadConfig( jd );

	sanitize();

	qDebug() << "loading game took: " + QString::number( timer.elapsed() ) + " ms";
	return true;
}

/** @brief Performs post-load fixups to correct data inconsistencies.
 *
 *  Handles migration from older save formats (e.g. 0.7.5 gnome ID in item job field),
 *  fixes orphaned item ownership (items claimed to be held but not in any gnome's
 *  inventory), and cleans out watch list entries referencing invalid inventory categories.
 */
void IO::sanitize()
{
	// Migration for 0.7.5 games where gnomes had stored their own ID in job field of items
	{
		std::unordered_set<unsigned int> legacyJobs;

		std::unordered_map<unsigned int, unsigned int> carriedItems;
		for ( const auto& gnome : g->gm()->gnomes() )
		{
			{
				const auto& c = gnome->inventoryItems();
				for ( const auto& itemID : c )
				{
					assert( carriedItems.count( itemID ) == 0 );
					carriedItems[itemID] = gnome->id();
				}
			}
			{
				const auto& c = gnome->carriedItems();
				for ( const auto& itemID : c )
				{
					assert( carriedItems.count( itemID ) == 0 );
					carriedItems[itemID] = gnome->id();
				}
			}
			for ( const auto& claim : gnome->claimedItems() )
			{
				// Special exemption in case this is legacy type reservation
				legacyJobs.emplace( gnome->id() );
			}

			for ( const auto& itemID : gnome->equipment().wornItems() )
			{
				assert( carriedItems.count( itemID ) == 0 );
				carriedItems[itemID] = gnome->id();
			}
		}

		for ( auto& item : g->inv()->allItems() )
		{
			const auto job = item.isInJob();
			if ( job && !legacyJobs.count( job ) && !g->jm()->getJob( job ) )
			{
				item.setInJob( 0 );
				qWarning() << "item " + QString::number( item.id() ) + " " + item.itemSID() + " had illegal job";
			}
			const bool carried = 0 != carriedItems.count( item.id() );

			if ( item.isHeldBy() != 0 && !carried )
			{
				g->inv()->putDownItem( item.id(), item.getPos() );
				qWarning() << "item " + QString::number( item.id() ) + " " + item.itemSID() + " found lost in space";
			}
			if ( carried )
			{
				auto realOwner = carriedItems[item.id()];
				if ( item.isHeldBy() != realOwner )
				{
					g->inv()->pickUpItem( item.id(), realOwner );
				}
			}
		}
	}
	// Clean out orphaned watch list items
	{
		const auto categories = g->inv()->categories();
		for ( auto it = GameState::watchedItemList.begin(); it != GameState::watchedItemList.end(); )
		{
			bool valid = true;

			const auto groups    = g->inv()->groups( it->category );
			const auto items     = g->inv()->items( it->category, it->group );
			const auto materials = g->inv()->materials( it->category, it->group, it->item );

			if (
				!categories.contains( it->category ) ||
				( !it->group.isEmpty() && !groups.contains( it->group ) ) ||
				( !it->item.isEmpty() && !items.contains( it->item ) ) ||
				( !it->material.isEmpty() && !materials.contains( it->material ) )
			)
			{
				it = GameState::watchedItemList.erase( it );
			}
			else
			{
				++it;
			}
		}
	}
}

/** @brief Reads the version string from a save game's game.json file.
 *  @param folder Path to the save game folder.
 *  @return Version string in "major.minor.patch.build" format, or "0.0.0.0" if not found.
 */
QString IO::versionString( QString folder )
{
	QJsonDocument jd;
	IO::loadFile( folder + "/game.json", jd );

	if ( jd.isArray() )
	{
		QJsonArray ja = jd.array();
		auto vl       = ja.toVariantList();
		if ( vl.size() > 0 )
		{
			QVariantMap vm = vl.first().toMap();

			if ( vm.contains( "Version" ) )
			{
				return vm.value( "Version" ).toString();
			}
			else
			{
				return vm.value( "version" ).toString();
			}
		}
	}
	return ( "0.0.0.0" );
}

/** @brief Converts the save game version string to a comparable integer.
 *
 *  Encodes "a.b.c.d" as a*1000 + b*100 + c*10 + d.
 *
 *  @param folder Path to the save game folder.
 *  @return Integer version number, or 0 if the version string is malformed.
 */
int IO::versionInt( QString folder )
{
	QString version = versionString( folder );
	QStringList vl  = version.split( "." );
	if ( vl.size() == 4 )
	{
		int vi = vl[0].toInt() * 1000 + vl[1].toInt() * 100 + vl[2].toInt() * 10 + vl[3].toInt();
		return vi;
		if ( vi < 577 )
		{
			return false;
		}
		return true;
	}
	return 0;
}

/** @brief Serializes the global configuration into a JSON array.
 *  @return QJsonArray containing the configuration as a single object entry.
 */
QJsonArray IO::jsonArrayConfig()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayConfig";
	QJsonArray ja;
	ja.append( QJsonValue::fromVariant( Global::cfg->object() ) );

	return ja;
}

/** @brief Loads configuration from a parsed JSON document.
 *  @param jd JSON document containing the configuration array.
 *  @return True (currently a no-op stub that iterates entries without applying them).
 */
bool IO::loadConfig( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	for ( const auto& entry : ja.toVariantList() )
	{
		auto map = entry.toMap();
	}

	return true;
}

/** @brief Serializes the full game state (version, inventory filters, neighbors, military)
 *         into a JSON array.
 *  @return QJsonArray containing the serialized GameState as a single object entry.
 */
QJsonArray IO::jsonArrayGame()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayGame";
	GameState::version = Global::cfg->get( "CurrentVersion" ).toString();

	GameState::initialSave = true;

	g->inv()->saveFilter();

	GameState::neighbors = g->nm()->serialize();
	GameState::military  = g->mil()->serialize();

	g->mil()->save(); // TODO why this? maybe move it into serialize

	QJsonArray ja;
	QVariantMap out;
	GameState::serialize( out );
	ja.append( QJsonValue::fromVariant( out ) );

	return ja;
}

/** @brief Loads game state from a parsed JSON document.
 *
 *  Deserializes GameState, registers any added materials into the DB,
 *  inserts added translation strings, and deserializes neighbor data.
 *
 *  @param jd JSON document containing the game state array.
 *  @return True on success.
 */
bool IO::loadGame( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	auto doc      = ja.toVariantList();
	if ( doc.size() )
	{
		auto map = doc.first().toMap();
		GameState::load( map );
	}

	for ( const auto& vMat : GameState::addedMaterials )
	{
		DB::addRow( "Materials", vMat.toMap() );
	}

	for ( const auto& key : GameState::addedTranslations.keys() )
	{
		Strings::getInstance().insertString( key, GameState::addedTranslations[key].toString() );
	}

	g->nm()->deserialize( GameState::neighbors );

	return true;
}

/** @brief Saves the world grid to a binary file (world.dat).
 *
 *  Writes every tile's flags, wall/floor type and material, rotations,
 *  fluid level, pressure, flow, vegetation, embedded material, and sprite UIDs
 *  as a compact binary stream.
 *
 *  @param folder Path to the save folder (must end with '/').
 *  @return True on success.
 */
bool IO::saveWorld( QString folder )
{
	if ( Global::debugMode )
		qDebug() << "saveWorld";
	QFile worldFile( folder + "world.dat" );
	if ( worldFile.open( QIODevice::WriteOnly ) )
	{
		QDataStream out( &worldFile );
		std::vector<Tile>& world = g->w()->world();

		for ( const auto& tile : world )
		{
			out << (quint64)tile.flags;
			out << (quint16)tile.wallType;
			out << (quint16)tile.wallMaterial;
			out << (quint16)tile.floorType;
			out << (quint16)tile.floorMaterial;
			out << (quint8)tile.wallRotation;
			out << (quint8)tile.floorRotation;
			out << (quint8)tile.fluidLevel;
			out << (quint8)tile.pressure;
			out << (quint8)tile.flow;
			out << (quint8)tile.vegetationLevel;
			out << (quint16)tile.embeddedMaterial;

			out << (quint32)tile.wallSpriteUID;
			out << (quint32)tile.floorSpriteUID;
			out << (quint32)tile.itemSpriteUID;
#ifdef SAVEREGIONINFO
			out << (quint32)tile.region;
#endif
		}
	}
	worldFile.close();
	return true;
}

/** @brief Loads the world grid from a binary file (world.dat) in the given folder.
 *  @param folder Path to the save folder.
 *  @return True if the file was opened and read successfully, false otherwise.
 */
bool IO::loadWorld( QString folder )
{
	QFile worldFile( folder + "world.dat" );
	if ( worldFile.open( QIODevice::ReadOnly ) )
	{
		QDataStream in( &worldFile );

		loadWorld( in );

		worldFile.close();
		return true;
	}
	return false;
}

/** @brief Reads world tile data from a QDataStream and populates the world grid.
 *
 *  Allocates the world using Global::dimX/Y/Z, then reads each tile's binary
 *  fields until the stream is exhausted.
 *
 *  @param in Input data stream positioned at the start of tile data.
 */
void IO::loadWorld( QDataStream& in )
{
	unsigned short dimX = Global::dimX;
	unsigned short dimY = Global::dimY;
	unsigned short dimZ = Global::dimZ;

	g->setWorld( dimX, dimY, dimZ );
	std::vector<Tile>& world = g->w()->world();
	world.clear();
	world.reserve( dimX * dimY * dimZ );

	while ( !in.atEnd() )
	{
		quint16 wallType;
		quint16 floorType;
		quint64 tileFlags;
		quint8 flow;

		Tile tile;
		in >> tileFlags;
		in >> wallType;
		in >> tile.wallMaterial;
		in >> floorType;
		in >> tile.floorMaterial;
		in >> tile.wallRotation;
		in >> tile.floorRotation;
		in >> tile.fluidLevel;
		in >> tile.pressure;
		in >> flow;
		in >> tile.vegetationLevel;
		in >> tile.embeddedMaterial;
		in >> tile.wallSpriteUID;
		in >> tile.floorSpriteUID;
		in >> tile.itemSpriteUID;
		tile.flags     = (TileFlag)tileFlags;
		tile.wallType  = (WallType)wallType;
		tile.floorType = (FloorType)floorType;
		tile.flow      = (WaterFlow)flow;

		world.push_back( tile );
	}
	world.shrink_to_fit();
}

/** @brief Serializes all wall constructions into a JSON array.
 *  @return QJsonArray of wall construction variant maps.
 */
QJsonArray IO::jsonArrayWallConstructions()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayWallConstructions";
	QJsonArray ja;
	for ( const auto& constr : g->w()->wallConstructions() )
	{
		QJsonValue jv = QJsonValue::fromVariant( constr );
		ja.append( jv );
	}

	return ja;
}

/** @brief Serializes all floor constructions into a JSON array.
 *  @return QJsonArray of floor construction variant maps.
 */
QJsonArray IO::jsonArrayFloorConstructions()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayFloorConstructions";
	QJsonArray ja;
	for ( const auto& constr : g->w()->floorConstructions() )
	{
		QJsonValue jv = QJsonValue::fromVariant( constr );
		ja.append( jv );
	}

	return ja;
}

/** @brief Loads floor constructions from a parsed JSON document into the world.
 *  @param jd JSON document containing the floor constructions array.
 *  @return True on success.
 */
bool IO::loadFloorConstructions( QJsonDocument& jd )
{
	g->w()->loadFloorConstructions( jd.array().toVariantList() );

	return true;
}

/** @brief Loads wall constructions from a parsed JSON document into the world.
 *  @param jd JSON document containing the wall constructions array.
 *  @return True on success.
 */
bool IO::loadWallConstructions( QJsonDocument& jd )
{
	g->w()->loadWallConstructions( jd.array().toVariantList() );

	return true;
}

/** @brief Serializes all sprite creation records into a JSON array.
 *
 *  Each entry contains the item SID, material SIDs, random map, UID,
 *  and optionally a creature ID.
 *
 *  @return QJsonArray of sprite creation records.
 */
QJsonArray IO::jsonArraySprites()
{
	if ( Global::debugMode )
		qDebug() << "jsonArraySprites";
	QJsonArray ja;
	for ( const auto& sc : g->sf()->spriteCreations() )
	{
		QVariantMap vm;
		vm.insert( "ItemSID", sc.itemSID );
		vm.insert( "MaterialSIDs", sc.materialSIDs.join( '_' ) );
		vm.insert( "Random", Global::util->mapJoin( sc.random ) );
		vm.insert( "UID", sc.uID );
		if ( sc.creatureID )
		{
			vm.insert( "CreatureID", sc.creatureID );
		}
		QJsonValue jv = QJsonValue::fromVariant( vm );
		ja.append( jv );
	}

	return ja;
}

/** @brief Loads sprite creation records from a parsed JSON document and recreates sprites.
 *  @param jd JSON document containing the sprites array.
 *  @return True on success.
 */
bool IO::loadSprites( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	QList<SpriteCreation> scl;
	for ( const auto& entry : ja.toVariantList() )
	{
		QVariantMap em = entry.toMap();
		SpriteCreation sc;
		sc.itemSID      = em.value( "ItemSID" ).toString();
		sc.materialSIDs = em.value( "MaterialSIDs" ).toString().split( "_" );
		sc.random       = Global::util->mapSplit( em.value( "Random" ).toString() );
		sc.uID          = em.value( "UID" ).toUInt();
		sc.creatureID   = em.value( "CreatureID" ).toUInt();
		scl.push_back( sc );
	}
	g->sf()->createSprites( scl );
	return true;
}

/** @brief Serializes all gnomes (regular, special, and automatons) into a JSON array.
 *  @return QJsonArray containing serialized gnome data.
 */
QJsonArray IO::jsonArrayGnomes()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayGnomes";
	QJsonArray ja;
	for ( const auto& gnome : g->gm()->gnomes() )
	{
		QVariantMap out;
		gnome->serialize( out );
		QJsonValue jv = QJsonValue::fromVariant( out );
		ja.append( jv );
	}
	for ( const auto& gnome : g->gm()->specialGnomes() )
	{
		QVariantMap out;
		gnome->serialize( out );
		QJsonValue jv = QJsonValue::fromVariant( out );
		ja.append( jv );
	}
	for ( const auto& automaton : g->gm()->automatons() )
	{
		QVariantMap out;
		automaton->serialize( out );
		QJsonValue jv = QJsonValue::fromVariant( out );
		ja.append( jv );
	}

	return ja;
}

/** @brief Serializes a subset of monsters into a JSON array for chunked saving.
 *  @param startIndex Index of the first monster to serialize.
 *  @param amount Maximum number of monsters to serialize in this chunk.
 *  @return QJsonArray of serialized monster data.
 */
QJsonArray IO::jsonArrayMonsters( int startIndex, int amount )
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayMonsters";
	QJsonArray ja;

	int i         = startIndex;
	auto monsters = g->cm()->monsters();

	while ( i < monsters.size() && amount > 0 )
	{
		auto monster = monsters[i];

		QVariantMap out;
		monster->serialize( out );
		QJsonValue jv = QJsonValue::fromVariant( out );
		ja.append( jv );

		++i;
		--monster;
	}

	return ja;
}

/** @brief Saves all monsters to numbered chunk files (monsters1.json, monsters2.json, ...).
 *
 *  Each chunk contains up to 10,000 monsters.
 *
 *  @param folder Path to the save folder.
 *  @return True on success.
 */
bool IO::saveMonsters( QString folder )
{
	int i          = 1;
	int startIndex = 0;
	while ( true )
	{
		auto ja = jsonArrayMonsters( startIndex, 10000 );
		if ( ja.size() > 0 )
		{
			IO::saveFile( folder + "monsters" + QString::number( i ) + ".json", ja );
			++i;
			startIndex += 10000;
		}
		else
		{
			break;
		}
	}
	return true;
}

/** @brief Loads gnomes from a parsed JSON document, dispatching by creature type.
 *
 *  Handles GNOME, GNOME_TRADER, and AUTOMATON types.
 *
 *  @param jd JSON document containing the gnomes array.
 *  @return True on success.
 */
bool IO::loadGnomes( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	qDebug() << "load " << ja.toVariantList().size() << "gnomes";
	for ( const auto& entry : ja.toVariantList() )
	{
		auto em = entry.toMap();
		switch ( (CreatureType)em.value( "Type" ).toInt() )
		{
			case CreatureType::GNOME:
				g->gm()->addGnome( em );
				break;
			case CreatureType::GNOME_TRADER:
				g->gm()->addTrader( em );
				break;
			case CreatureType::AUTOMATON:
				g->gm()->addAutomaton( em );
				break;
		}
	}
	return true;
}

/** @brief Loads monsters from the save folder.
 *
 *  Supports both legacy single-file format (monsters.json) and chunked format
 *  (monsters1.json, monsters2.json, ...).
 *
 *  @param folder Path to the save folder.
 *  @return True on success.
 */
bool IO::loadMonsters( QString folder )
{
	QJsonDocument jd;
	if ( QFileInfo::exists( folder + "monsters.json" ) )
	{
		loadFile( folder + "monsters.json", jd );
		QJsonArray ja = jd.array();
		for ( const auto& entry : ja.toVariantList() )
		{
			g->cm()->addCreature( CreatureType::MONSTER, entry.toMap() );
		}
	}
	else
	{
		int i = 1;
		while ( QFileInfo::exists( folder + "monsters" + QString::number( i ) + ".json" ) )
		{
			loadFile( folder + "monsters" + QString::number( i ) + ".json", jd );
			QJsonArray ja = jd.array();
			for ( const auto& entry : ja.toVariantList() )
			{
				g->cm()->addCreature( CreatureType::MONSTER, entry.toMap() );
			}
			++i;
		}
	}
	return true;
}

/** @brief Serializes a subset of plants into a JSON array for chunked saving.
 *  @param startIndex Index of the first plant to serialize.
 *  @param amount Maximum number of plants to serialize in this chunk.
 *  @return QJsonArray of serialized plant data.
 */
QJsonArray IO::jsonArrayPlants( int startIndex, int amount )
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayAnimals";

	QJsonArray ja;
	auto plants = g->w()->plants();
	auto keys   = plants.keys();
	int i       = startIndex;

	while ( i < keys.size() && amount > 0 )
	{
		const auto& plant = plants[keys[i]];

		QJsonValue jv = QJsonValue::fromVariant( plant.serialize() );
		ja.append( jv );

		++i;
		--amount;
	}
	return ja;
}

/** @brief Saves all plants to numbered chunk files (plants1.json, plants2.json, ...).
 *
 *  Each chunk contains up to 10,000 plants.
 *
 *  @param folder Path to the save folder.
 *  @return True on success.
 */
bool IO::savePlants( QString folder )
{
	QByteArray out;

	int i          = 1;
	int startIndex = 0;
	while ( true )
	{
		auto ja = jsonArrayPlants( startIndex, 10000 );
		if ( ja.size() > 0 )
		{
			IO::saveFile( folder + "plants" + QString::number( i ) + ".json", ja );
			++i;
			startIndex += 10000;
		}
		else
		{
			break;
		}
	}
	return true;
}

/** @brief Loads plants from the save folder.
 *
 *  Supports both legacy single-file format (plants.json) and chunked format
 *  (plants1.json, plants2.json, ...).
 *
 *  @param folder Path to the save folder.
 *  @return True on success.
 */
bool IO::loadPlants( QString folder )
{
	QJsonDocument jd;
	if ( QFileInfo::exists( folder + "plants.json" ) )
	{
		loadFile( folder + "plants.json", jd );
		QJsonArray ja = jd.array();
		for ( const auto& entry : ja.toVariantList() )
		{
			Plant plant( entry.toMap(), g );
			g->w()->addPlant( plant );
		}
	}
	else
	{
		int i = 1;

		while ( QFileInfo::exists( folder + "plants" + QString::number( i ) + ".json" ) )
		{
			loadFile( folder + "plants" + QString::number( i ) + ".json", jd );
			QJsonArray ja = jd.array();

			for ( const auto& entry : ja.toVariantList() )
			{
				Plant plant( entry.toMap(), g );
				g->w()->addPlant( plant );
			}
			++i;
		}
	}
	return true;
}

/** @brief Serializes a subset of items into a JSON array for chunked saving.
 *  @param startIndex Index of the first item to serialize.
 *  @param amount Maximum number of items to serialize in this chunk.
 *  @return QJsonArray of serialized item data.
 */
QJsonArray IO::jsonArrayItems( int startIndex, int amount )
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayAnimals";

	QJsonArray ja;
	auto& items = g->inv()->allItems();
	auto keys   = items.keys();

	int i = startIndex;

	while ( i < keys.size() && amount > 0 )
	{
		auto itemID   = keys[i];
		auto& item    = items[itemID];
		QJsonValue jv = QJsonValue::fromVariant( item.serialize() );
		ja.append( jv );

		++i;
		--amount;
	}
	return ja;
}

/** @brief Saves all items to numbered chunk files (items1.json, items2.json, ...).
 *
 *  Runs a sanity check on the inventory before saving. Each chunk contains
 *  up to 50,000 items.
 *
 *  @param folder Path to the save folder.
 *  @return True on success.
 */
bool IO::saveItems( QString folder )
{
	g->inv()->sanityCheck();

	QByteArray out;

	auto& items = g->inv()->allItems();

	int i          = 1;
	int startIndex = 0;
	while ( true )
	{
		auto ja = jsonArrayItems( startIndex, 50000 );
		if ( ja.size() > 0 )
		{
			IO::saveFile( folder + "items" + QString::number( i ) + ".json", ja );
			++i;
			startIndex += 50000;
		}
		else
		{
			break;
		}
	}

	return true;
}

/** @brief Loads items from the save folder.
 *
 *  Initializes the inventory filter, then loads items from either a single
 *  items.json (legacy) or chunked files (items1.json, items2.json, ...).
 *
 *  @param folder Path to the save folder.
 *  @return True on success.
 */
bool IO::loadItems( QString folder )
{
	g->inv()->loadFilter();

	QJsonDocument jd;
	if ( QFileInfo::exists( folder + "items.json" ) )
	{
		loadFile( folder + "items.json", jd );
		QJsonArray ja = jd.array();
		int count     = 0;
		for ( const auto& entry : ja.toVariantList() )
		{
			g->inv()->createItem( entry.toMap() );
			++count;
		}
		qDebug() << "loaded" << count << "items";
	}
	else
	{
		int i     = 1;
		int count = 0;
		while ( QFileInfo::exists( folder + "items" + QString::number( i ) + ".json" ) )
		{
			loadFile( folder + "items" + QString::number( i ) + ".json", jd );
			QJsonArray ja = jd.array();

			for ( const auto& entry : ja.toVariantList() )
			{
				g->inv()->createItem( entry.toMap() );
				++count;
			}
			++i;
		}
		qDebug() << "loaded" << count << "items";
	}

	return true;
}

/** @brief Loads item history data from a parsed JSON document.
 *  @param jd JSON document containing the item history map.
 *  @return True on success.
 */
bool IO::loadItemHistory( QJsonDocument& jd )
{
	g->inv()->itemHistory()->deserialize( jd.toVariant().toMap() );
	return true;
}

/** @brief Serializes all non-empty jobs into a JSON array.
 *  @return QJsonArray of serialized job data (skips jobs with empty type).
 */
QJsonArray IO::jsonArrayJobs()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayJobs";
	QJsonArray ja;
	for ( const auto& job : g->jm()->allJobs() )
	{
		if ( !job->type().isEmpty() )
		{
			QJsonValue jv = QJsonValue::fromVariant( job->serialize() );
			ja.append( jv );
		}
	}

	return ja;
}

/** @brief Serializes all job sprites (position-keyed sprite overrides) into a JSON array.
 *  @return QJsonArray of job sprite entries, each including a PosID key.
 */
QJsonArray IO::jsonArrayJobSprites()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayJobSprites";
	QJsonArray ja;
	auto jobSprites = g->w()->jobSprites();

	for ( const auto& key : jobSprites.keys() )
	{
		auto entry = jobSprites[key];
		entry.insert( "PosID", key );
		QJsonValue jv = QJsonValue::fromVariant( entry );
		ja.append( jv );
	}

	return ja;
}

/** @brief Loads jobs from a parsed JSON document into the job manager.
 *  @param jd JSON document containing the jobs array.
 *  @return True on success.
 */
bool IO::loadJobs( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	for ( const auto& entry : ja.toVariantList() )
	{
		g->jm()->addLoadedJob( entry );
	}
	return true;
}

/** @brief Loads job sprites from a parsed JSON document into the world.
 *  @param jd JSON document containing the job sprites array.
 *  @return True on success.
 */
bool IO::loadJobSprites( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	for ( const auto& entry : ja.toVariantList() )
	{
		auto em          = entry.toMap();
		unsigned int key = em.value( "JobID" ).toUInt();
		em.remove( "JobID" );
		g->w()->insertLoadedJobSprite( key, em );
	}
	return true;
}

/** @brief Serializes all farms, groves, pastures, and beehives into a JSON array.
 *  @return QJsonArray containing serialized farming entity data.
 */
QJsonArray IO::jsonArrayFarms()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayFarms";
	QJsonArray ja;
	for ( const auto& farm : g->fm()->allFarms() )
	{
		QJsonValue jv = QJsonValue::fromVariant( farm->serialize() );
		ja.append( jv );
	}
	for ( const auto& grove : g->fm()->allGroves() )
	{
		QJsonValue jv = QJsonValue::fromVariant( grove->serialize() );
		ja.append( jv );
	}
	for ( const auto& pasture : g->fm()->allPastures() )
	{
		QJsonValue jv = QJsonValue::fromVariant( pasture->serialize() );
		ja.append( jv );
	}
	for ( const auto& beehive : g->fm()->allBeeHives() )
	{
		QVariantMap vm;
		beehive->serialize( vm );
		QJsonValue jv = QJsonValue::fromVariant( vm );
		ja.append( jv );
	}

	return ja;
}

/** @brief Serializes all rooms into a JSON array.
 *  @return QJsonArray containing serialized room data.
 */
QJsonArray IO::jsonArrayRooms()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayRooms";
	QJsonArray ja;
	for ( const auto& room : g->rm()->allRooms() )
	{
		QJsonValue jv = QJsonValue::fromVariant( room->serialize() );
		ja.append( jv );
	}

	return ja;
}

/** @brief Serializes all doors into a JSON array.
 *
 *  Each door entry includes position, name, source, sprite ID, and blocking flags
 *  for gnomes, animals, and monsters.
 *
 *  @return QJsonArray containing serialized door data.
 */
QJsonArray IO::jsonArrayDoors()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayDoors";
	QJsonArray ja;
	for ( const auto& door : g->rm()->allDoors() )
	{
		QVariantMap out;

		out.insert( "Pos", door.pos.toString() );
		out.insert( "Name", door.name );
		out.insert( "Source", door.source.serialize() );
		out.insert( "SpriteID", door.spriteID );
		out.insert( "BlockGnomes", door.blockGnomes );
		out.insert( "BlockAnimals", door.blockAnimals );
		out.insert( "BlockMonsters", door.blockMonsters );

		QJsonValue jv = QJsonValue::fromVariant( out );
		ja.append( jv );
	}

	return ja;
}

/** @brief Loads farms, groves, pastures, and beehives from a parsed JSON document.
 *  @param jd JSON document containing the farming entities array.
 *  @return True on success.
 */
bool IO::loadFarms( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	for ( const auto& entry : ja.toVariantList() )
	{
		g->fm()->load( entry.toMap() );
	}
	return true;
}

/** @brief Loads rooms from a parsed JSON document into the room manager.
 *  @param jd JSON document containing the rooms array.
 *  @return True on success.
 */
bool IO::loadRooms( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	for ( const auto& entry : ja.toVariantList() )
	{
		g->rm()->load( entry.toMap() );
	}
	return true;
}

/** @brief Loads doors from a parsed JSON document into the room manager.
 *  @param jd JSON document containing the doors array.
 *  @return True on success.
 */
bool IO::loadDoors( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	for ( const auto& entry : ja.toVariantList() )
	{
		g->rm()->loadDoor( entry.toMap() );
	}
	return true;
}

/** @brief Serializes all stockpiles into a JSON array, preserving their order.
 *  @return QJsonArray containing serialized stockpile data.
 */
QJsonArray IO::jsonArrayStockpiles()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayStockpiles";
	QJsonArray ja;
	for ( const auto& stockpileID : g->spm()->allStockpilesOrdered() )
	{
		auto stockpile = g->spm()->getStockpile( stockpileID );
		if ( stockpile )
		{
			QJsonValue jv = QJsonValue::fromVariant( stockpile->serialize() );
			ja.append( jv );
		}
	}

	return ja;
}

/** @brief Loads stockpiles from a parsed JSON document into the stockpile manager.
 *  @param jd JSON document containing the stockpiles array.
 *  @return True on success.
 */
bool IO::loadStockpiles( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	for ( const auto& entry : ja.toVariantList() )
	{
		g->spm()->load( entry.toMap() );
	}
	return true;
}

/** @brief Serializes all workshops into a JSON array.
 *  @return QJsonArray containing serialized workshop data.
 */
QJsonArray IO::jsonArrayWorkshops()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayWorkshops";
	QJsonArray ja;
	for ( const auto& w : g->wsm()->workshops() )
	{
		QJsonValue jv = QJsonValue::fromVariant( w->serialize() );
		ja.append( jv );
	}
	return ja;
}

/** @brief Loads workshops from a parsed JSON document and registers their sprites.
 *  @param jd JSON document containing the workshops array.
 *  @return True on success.
 */
bool IO::loadWorkshops( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	for ( const auto& entry : ja.toVariantList() )
	{
		g->wsm()->addWorkshop( entry.toMap() );
		for ( const auto& s : entry.toMap().value( "Sprites" ).toList() )
		{
			g->w()->addLoadedSprites( s.toMap() );
		}
	}
	return true;
}

/** @brief Serializes a subset of animals into a JSON array for chunked saving.
 *  @param startIndex Index of the first animal to serialize.
 *  @param amount Maximum number of animals to serialize in this chunk.
 *  @return QJsonArray of serialized animal data.
 */
QJsonArray IO::jsonArrayAnimals( int startIndex, int amount )
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayAnimals";

	QJsonArray ja;
	auto animals = g->cm()->animals();

	int i = startIndex;

	while ( i < animals.size() && amount > 0 )
	{
		auto a = animals[i];
		QVariantMap values;
		a->serialize( values );
		QJsonValue jv = QJsonValue::fromVariant( values );
		ja.append( jv );

		++i;
		--amount;
	}
	return ja;
}

/** @brief Saves all animals to numbered chunk files (animals1.json, animals2.json, ...).
 *
 *  Each chunk contains up to 10,000 animals.
 *
 *  @param folder Path to the save folder.
 *  @return True on success.
 */
bool IO::saveAnimals( QString folder )
{
	int i          = 1;
	int startIndex = 0;
	while ( true )
	{
		auto ja = jsonArrayAnimals( startIndex, 10000 );
		if ( ja.size() > 0 )
		{
			IO::saveFile( folder + "animals" + QString::number( i ) + ".json", ja );
			++i;
			startIndex += 10000;
		}
		else
		{
			break;
		}
	}
	return true;
}

/** @brief Loads animals from the save folder.
 *
 *  Supports both legacy single-file format (animals.json) and chunked format
 *  (animals1.json, animals2.json, ...).
 *
 *  @param folder Path to the save folder.
 *  @return True on success.
 */
bool IO::loadAnimals( QString folder )
{
	QJsonDocument jd;
	if ( QFileInfo::exists( folder + "animals.json" ) )
	{
		loadFile( folder + "animals.json", jd );
		QJsonArray ja = jd.array();
		for ( const auto& entry : ja.toVariantList() )
		{
			g->cm()->addCreature( CreatureType::ANIMAL, entry.toMap() );
		}
	}
	else
	{
		int i = 1;
		while ( QFileInfo::exists( folder + "animals" + QString::number( i ) + ".json" ) )
		{
			loadFile( folder + "animals" + QString::number( i ) + ".json", jd );
			QJsonArray ja = jd.array();
			for ( const auto& entry : ja.toVariantList() )
			{
				g->cm()->addCreature( CreatureType::ANIMAL, entry.toMap() );
			}
			++i;
		}
	}
	return true;
}

/** @brief Serializes the item history tracking data into a JSON document.
 *  @return QJsonDocument containing the serialized item history map.
 */
QJsonDocument IO::jsonArrayItemHistory()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayItemHistory";
	QVariantMap out;
	g->inv()->itemHistory()->serialize( out );

	QJsonDocument jd = QJsonDocument::fromVariant( out );

	return jd;
}

/** @brief Serializes all game events into a JSON document.
 *  @return QJsonDocument containing the serialized event data as an array.
 */
QJsonDocument IO::jsonArrayEvents()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayEvents";
	QVariantMap out = g->em()->serialize();
	QVariantList ol;
	ol.append( out );
	QJsonDocument jd = QJsonDocument::fromVariant( ol );

	return jd;
}

/** @brief Loads game events from a parsed JSON document into the event manager.
 *  @param jd JSON document containing the events array.
 *  @return True on success.
 */
bool IO::loadEvents( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	for ( const auto& entry : ja.toVariantList() )
	{
		g->em()->deserialize( entry.toMap() );
	}
	return true;
}

/** @brief Serializes all mechanisms (levers, pressure plates, etc.) into a JSON document.
 *  @return QJsonDocument containing the serialized mechanisms as an array.
 */
QJsonDocument IO::jsonArrayMechanisms()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayMechanisms";
	QVariantList ol;

	for ( const auto& md : g->mcm()->mechanisms() )
	{
		auto vmd = md.serialize();
		ol.append( vmd );
	}

	QJsonDocument jd = QJsonDocument::fromVariant( ol );

	return jd;
}

/** @brief Loads mechanisms from a parsed JSON document into the mechanism manager.
 *  @param jd JSON document containing the mechanisms array.
 *  @return True on success.
 */
bool IO::loadMechanisms( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	g->mcm()->loadMechanisms( ja.toVariantList() );
	return true;
}

/** @brief Serializes all fluid pipes into a JSON document.
 *  @return QJsonDocument containing the serialized pipe data as an array.
 */
QJsonDocument IO::jsonArrayPipes()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayPipes";
	QVariantList ol;

	for ( const auto& fp : g->flm()->pipes() )
	{
		auto vfp = fp.serialize();
		/*
		if( md.jobID )
		{
			QSharedPointer<Job> job = g->mcm()->getJob( md.jobID );
			if( job )
			{
				vmd.insert( "Job", job->serialize() );
			}
			else
			{
				vmd.insert( "JobID", 0 );
			}
		}
		*/
		ol.append( vfp );
	}

	QJsonDocument jd = QJsonDocument::fromVariant( ol );

	return jd;
}

/** @brief Loads fluid pipes from a parsed JSON document into the fluid manager.
 *  @param jd JSON document containing the pipes array.
 *  @return True on success.
 */
bool IO::loadPipes( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	g->flm()->loadPipes( ja.toVariantList() );
	return true;
}

/** @brief Saves a QJsonObject to a file by wrapping it in a QJsonDocument.
 *  @param url File path to write to.
 *  @param jo JSON object to save.
 *  @return True on success, false if the file could not be opened.
 */
bool IO::saveFile( QString url, const QJsonObject& jo )
{
	QJsonDocument saveDoc( jo );
	return saveFile( url, saveDoc );
}

/** @brief Saves a QJsonArray to a file by wrapping it in a QJsonDocument.
 *  @param url File path to write to.
 *  @param ja JSON array to save.
 *  @return True on success, false if the file could not be opened.
 */
bool IO::saveFile( QString url, const QJsonArray& ja )
{
	QJsonDocument saveDoc( ja );
	return saveFile( url, saveDoc );
}

/** @brief Saves a QJsonDocument to a file as formatted JSON text.
 *  @param url File path to write to.
 *  @param jd JSON document to save.
 *  @return True on success, false if the file could not be opened.
 */
bool IO::saveFile( QString url, const QJsonDocument& jd )
{
	QFile file( url );

	if ( !file.open( QIODevice::WriteOnly ) )
	{
		qWarning( "Couldn't open save file." );
		return false;
	}

	file.write( jd.toJson() );
	file.close();
	return true;
}

/** @brief Loads and parses a JSON file from disk.
 *  @param url File path to read from.
 *  @param ja Output parameter receiving the parsed JSON document.
 *  @return True if parsing succeeded, false on JSON parse error (logged to debug output).
 */
bool IO::loadFile( QString url, QJsonDocument& ja )
{
	QFile file( url );
	file.open( QIODevice::ReadOnly | QIODevice::Text );
	QString val = file.readAll();
	file.close();

	QJsonParseError error;
	ja = QJsonDocument::fromJson( val.toUtf8(), &error );

	if ( error.error == QJsonParseError::NoError )
	{
		return true;
	}

	qDebug() << "json parse error in" << url << error.offset;

	return false;
}
