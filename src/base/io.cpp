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

#include <QCoreApplication>
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

#include <range/v3/view.hpp>

#include <fstream>

#include "spdlog/spdlog.h"

IO::IO( Game* game, QObject* parent ) :
	g( game ),
	QObject( parent )
{
}

IO::~IO()
{
}

bool IO::saveCompatible( const fs::path& folder )
{
	int vi = versionInt( folder );
	if ( vi < 741 )
	{
		return false;
	}
	return true;
}

bool IO::saveGameExists()
{
	const fs::path& folder = getDataFolder() / "save";

	if ( !fs::exists( folder ) || fs::is_empty( folder ) )
	{
		return false;
	}
	return true;
}

template<class> inline constexpr bool always_false_v = false;

// TODO: Remove after QJsonDocument is gone
QVariant toQVariant(ConfigVariant v) {
	return std::visit( []( auto&& arg )
				{
					using T = std::decay_t<decltype(arg)>;
					if constexpr (std::is_same_v<T, std::string>)
						return QVariant(QString::fromStdString(arg));
					else if constexpr (std::is_same_v<T, bool>)
						return QVariant(arg);
					else if constexpr (std::is_same_v<T, int>)
						return QVariant(arg);
					else if constexpr (std::is_same_v<T, float>)
						return QVariant(arg);
					else if constexpr (std::is_same_v<T, double>)
						return QVariant(arg);
					else
						static_assert(always_false_v<T>, "non-exhaustive visitor!"); },
				v );
}

// TODO: Remove after QJsonDocument is gone
QVariantMap toQVariant(ConfigMap map) {
	QVariantMap result;
	for ( const auto& entry : map ) {
		result[QString::fromStdString(entry.first)] = toQVariant(entry.second);
	}
	return result;
}

bool IO::saveConfig()
{
	const fs::path& folder = getDataFolder() / "settings";

	ConfigMap cm   = Global::cfg->object();
	json jd;
	for ( const auto& entry : cm )
	{
		// FIXME: Figure out why ADL is not using this `to_json` automatically
		to_json(jd[entry.first], entry.second);
	}

	IO::saveFile( folder / "config.json", jd );

	return true;
}

fs::path IO::getDataFolder()
{
#ifdef _WIN32
	return QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/My Games/Ingnomia";
#else
	// corresponds to ~/.local/share/<APPNAME> on Linux
	return QStandardPaths::writableLocation( QStandardPaths::AppDataLocation ).toStdString();
#endif
}

fs::path IO::getTempFolder()
{
	return getDataFolder() / "tmp";
}

bool IO::createFolders()
{
	const fs::path& exePath = Global::exePath;

	const fs::path& folder = getDataFolder();
	if ( !fs::exists( folder ) )
	{
		fs::create_directories( folder );
	}
	const auto modFolder = folder / "mods";
	if ( !fs::exists( modFolder ) )
	{
		fs::create_directory( modFolder );
	}
	const auto saveFolder = folder / "save";
	if ( !fs::exists( saveFolder ) )
	{
		fs::create_directory( saveFolder );
	}
	const auto ssFolder = folder / "screenshots";
	if ( !fs::exists( ssFolder ) )
	{
		fs::create_directory( ssFolder );
	}
	const auto settingsFolder = folder / "settings";
	if ( !fs::exists( settingsFolder ) )
	{
		fs::create_directory( settingsFolder );
	}
	const auto tmpFolder = folder / "tmp";
	if ( !fs::exists( tmpFolder ) )
	{
		fs::create_directory( tmpFolder );
	}

	if ( !fs::exists( folder / "settings" / "profs.json" ) && fs::exists( exePath / "content" / "JSON" / "profs.json" ) )
	{
		fs::copy( exePath / "content" / "JSON" / "profs.json", folder / "settings" / "profs.json" );
	}
	if ( !fs::exists( folder / "settings" / "config.json" ) && fs::exists( exePath / "content" / "JSON" / "config.json" ) )
	{
		fs::copy( exePath / "content" / "JSON" / "config.json", folder / "settings" / "config.json" );
	}
	if ( !fs::exists( folder / "settings" / "newgame.json" ) && fs::exists( exePath / "content" / "JSON" / "newgame.json" ) )
	{
		fs::copy( exePath / "content" / "JSON" / "newgame.json", folder / "settings" / "newgame.json" );
	}
	/*
	if ( !fs::exists( folder / "settings" / "keybindings.json" ) && fs::exists( exePath / "content" / "JSON" / "keybindings.json" ) )
	{
		fs::copy( exePath / "content" / "JSON" / "keybindings.json", folder / "settings" / "keybindings.json" );
	}
	*/

	return fs::exists( getDataFolder() / "save" );
}

bool IO::loadOriginalConfig( json& jd )
{
	spdlog::debug("load standard config");
	return IO::loadFile( Global::exePath / "content" / "JSON" / "config.json", jd );
}

std::string IO::save( bool autosave )
{
	QElapsedTimer timer;
	timer.start();

	auto folder = getDataFolder() / "save";

	if ( !fs::exists( folder ) )
	{
		fs::create_directory( folder );
	}
	auto name = GameState::kingdomName;
	name         = name.simplified();
	name.replace( " ", "" );
	folder /= name.toStdString();

	if ( !fs::exists( folder ) )
	{
		fs::create_directory( folder );
	}
	int slot = 0;

	if ( autosave )
	{
		folder /= "autosave";
	}
	else
	{

		QDirIterator directories( QString::fromStdString( folder ), QDir::Dirs | QDir::NoDotAndDotDot );
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
		folder /= std::to_string( slot );
	}

	if ( Global::debugMode )
		spdlog::debug( "{}", folder.string() );
	std::string oldFolder;
	if ( fs::exists( folder ) )
	{
		oldFolder = folder.replace_filename(folder.filename().string() + ".backup");
		fs::rename( folder, oldFolder );
	}
	fs::create_directory( folder );
	
	IO::saveWorld( folder );
	IO::saveFile( folder / "sprites.json", IO::jsonArraySprites() );
	IO::saveFile( folder / "game.json", IO::jsonArrayGame() );
	IO::saveFile( folder / "config.json", IO::jsonArrayConfig() );

	IO::saveItems( folder );

	IO::saveFile( folder / "wallconstructions.json", IO::jsonArrayWallConstructions() );
	IO::saveFile( folder / "floorconstructions.json", IO::jsonArrayFloorConstructions() );

	IO::saveFile( folder / "stockpiles.json", IO::jsonArrayStockpiles() );
	IO::saveFile( folder / "jobs.json", IO::jsonArrayJobs() );
	IO::saveFile( folder / "jobsprites.json", IO::jsonArrayJobSprites() );

	IO::saveFile( folder / "gnomes.json", IO::jsonArrayGnomes() );

	IO::saveMonsters( folder );

	IO::savePlants( folder );
	IO::saveAnimals( folder );

	IO::saveFile( folder / "farms.json", IO::jsonArrayFarms() );
	IO::saveFile( folder / "workshops.json", IO::jsonArrayWorkshops() );

	IO::saveFile( folder / "rooms.json", IO::jsonArrayRooms() );
	IO::saveFile( folder / "doors.json", IO::jsonArrayDoors() );

	IO::saveFile( folder / "itemhistory.json", IO::jsonArrayItemHistory() );
	IO::saveFile( folder / "events.json", IO::jsonArrayEvents() );

	IO::saveFile( folder / "mechanisms.json", IO::jsonArrayMechanisms() );
	IO::saveFile( folder / "pipes.json", IO::jsonArrayPipes() );

	qDebug() << "saving game took: " + QString::number( timer.elapsed() ) + " ms";

	if (!oldFolder.empty())
	{
		fs::remove_all( oldFolder );
		spdlog::debug("Savegame backup removed");
	}

	return folder;
}

bool IO::load( const fs::path& folder )
{
	QElapsedTimer timer;
	timer.start();

	IO::version = versionInt( folder );

	QJsonDocument jd;

	loadFile( folder / "game.json", jd );
	IO::loadGame( jd );

	Global::util->initAllowedInContainer();

	loadFile( folder / "sprites.json", jd );
	IO::loadSprites( jd );
	signalStatus( "Start loading world.." );
	if ( !IO::loadWorld( folder ) )
	{
		return false;
	}
	g->w()->afterLoad();
	signalStatus( "Loading world done" );
	IO::loadItems( folder );
	signalStatus( "Loading items done" );
	loadFile( folder / "floorconstructions.json", jd );
	IO::loadFloorConstructions( jd );
	loadFile( folder / "wallconstructions.json", jd );
	IO::loadWallConstructions( jd );
	signalStatus( "Loading constructions done" );
	loadFile( folder / "jobs.json", jd );
	IO::loadJobs( jd );
	loadFile( folder / "jobsprites.json", jd );
	IO::loadJobSprites( jd );
	signalStatus( "Loading jobs done" );
	loadFile( folder / "workshops.json", jd );
	IO::loadWorkshops( jd );
	loadFile( folder / "farms.json", jd );
	IO::loadFarms( jd );
	loadFile( folder / "stockpiles.json", jd );
	IO::loadStockpiles( jd );
	loadFile( folder / "mechanisms.json", jd );
	IO::loadMechanisms( jd );
	loadFile( folder / "pipes.json", jd );
	IO::loadPipes( jd );
	// anything that has local jobs needs to be loaded before this
	loadFile( folder / "gnomes.json", jd );
	IO::loadGnomes( jd );
	loadFile( folder / "monsters.json", jd );
	IO::loadMonsters( folder );
	IO::loadPlants( folder );
	IO::loadAnimals( folder );
	signalStatus( "Loading gnomes, plants and animals done" );
	loadFile( folder / "rooms.json", jd );
	IO::loadRooms( jd );
	loadFile( folder / "doors.json", jd );
	IO::loadDoors( jd );
	loadFile( folder / "itemhistory.json", jd );
	IO::loadItemHistory( jd );
	loadFile( folder / "events.json", jd );
	IO::loadEvents( jd );

	loadFile( folder / "config.json", jd );
	IO::loadConfig( jd );

	sanitize();

	spdlog::debug( "loading game took: {} ms", timer.elapsed() );
	return true;
}

void IO::sanitize()
{
	// Migration for 0.7.5 games where gnomes had stored their own ID in job field of items
	{
		std::unordered_set<unsigned int> legacyJobs;

		std::unordered_map<unsigned int, unsigned int> carriedItems;
		std::unordered_map<unsigned int, unsigned int> constructedItems;
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
		{
			const auto& constructions = g->w()->wallConstructions();
			for ( auto it = constructions.cbegin(); it != constructions.cend(); ++it )
			{
				const auto& construction = it->second;
				if ( construction.contains( "Item" ) )
				{
					auto itemID = construction["Item"].toInt();
					assert( constructedItems.count( itemID ) == 0 );
					constructedItems[itemID] = 0;
				}
				// Items can be either item type IDs or actual item IDs
				if ( construction.contains( "FromParts" ) )
				{
					for ( auto vItem : construction.value( "Items" ).toList() )
					{
						auto itemID = vItem.toInt();
						assert( constructedItems.count( itemID ) == 0 );
						constructedItems[itemID] = 0;
					}
				}
			}
		}

		{
			const auto& constructions = g->w()->floorConstructions();
			for ( auto it = constructions.cbegin(); it != constructions.cend(); ++it )
			{
				const auto& construction = it->second;
				if ( construction.contains( "Item" ) )
				{
					auto itemID = construction["Item"].toInt();
					assert( constructedItems.count( itemID ) == 0 );
					constructedItems[itemID] = 0;
				}
				// Items can be either item type IDs or actual item IDs
				if ( construction.contains( "FromParts" ) )
				{
					for ( auto vItem : construction.value( "Items" ).toList() )
					{
						auto itemID = vItem.toInt();
						assert( constructedItems.count( itemID ) == 0 );
						constructedItems[itemID] = 0;
					}
				}
			}
		}

		for ( const auto& workshop : g->wsm()->workshops() )
		{
			for ( const auto& item : workshop->sourceItems() )
			{
				constructedItems[item.toInt()] = workshop->id();
			}
		}

		for ( auto& item : g->inv()->allItems() | ranges::views::values )
		{
			const auto job = item.isInJob();
			if ( job && !legacyJobs.count( job ) && !g->jm()->getJob( job ) )
			{
				item.setInJob( 0 );
				spdlog::warn("item {} {} had illegal job", QString::number( item.id() ).toStdString(), item.itemSID().toStdString() );
			}
			const bool carried    = 0 != carriedItems.count( item.id() );
			const bool construced = 0 != constructedItems.count( item.id() );

			if ( carried && construced )
			{
				spdlog::warn("item {} {} is both carried around and installed somewhere simultaniously", QString::number( item.id() ).toStdString(), item.itemSID().toStdString() );
				continue;
			}

			// Items which should be in world
			if ( item.isHeldBy() != 0 && !construced && !carried )
			{
				g->inv()->putDownItem( item.id(), item.getPos() );
				item.setIsConstructed( false );
				spdlog::warn("item {} {} found lost in space", QString::number( item.id() ).toStdString(), item.itemSID().toStdString() );
			}
			if ( carried )
			{
				auto realOwner = carriedItems[item.id()];
				if ( item.isHeldBy() != realOwner )
				{
					g->inv()->pickUpItem( item.id(), realOwner );
				}
				if ( item.isConstructed() )
				{
					g->inv()->setConstructed( item.id(), false );
					spdlog::warn("item {} {} found constructed on a gnome", QString::number( item.id() ).toStdString(), item.itemSID().toStdString() );
				}
			}
			// Items in world
			if ( !carried && !construced )
			{
				if ( item.isConstructed() )
				{
					g->inv()->setConstructed( item.id(), false );
					item.setIsConstructed( false );
					g->inv()->putDownItem( item.id(), item.getPos() );
					spdlog::warn("item {} {} found glued to the floor", QString::number( item.id() ).toStdString(), item.itemSID().toStdString() );
				}
			}
			// Items in construction
			if ( construced )
			{
				auto realOwner = constructedItems[item.id()];
				if ( item.isHeldBy() != realOwner )
				{
					if ( realOwner != 0 )
					{
						g->inv()->pickUpItem( item.id(), realOwner );
					}
					else
					{
						g->inv()->putDownItem( item.id(), item.getPos() );
					}
				}
				if ( !item.isConstructed() )
				{
					g->inv()->setConstructed( item.id(), true );
					spdlog::warn("item {} {} found broken loose", QString::number( item.id() ).toStdString(), item.itemSID().toStdString() );
				}
			}
		}
		for ( auto& ws : g->wsm()->workshops() )
		{
			for ( auto& vitem : ws->sourceItems() )
			{
				g->inv()->pickUpItem( vitem.toUInt(), ws->id() );
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
				( !it->material.isEmpty() && std::find( materials.begin(), materials.end(), it->material ) == std::end(materials) )
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

std::string IO::versionString( const fs::path& folder )
{
	QJsonDocument jd;
	IO::loadFile( folder / "game.json", jd );

	if ( jd.isArray() )
	{
		QJsonArray ja = jd.array();
		auto vl       = ja.toVariantList();
		if ( vl.size() > 0 )
		{
			QVariantMap vm = vl.first().toMap();

			if ( vm.contains( "Version" ) )
			{
				return vm.value( "Version" ).toString().toStdString();
			}
			else
			{
				return vm.value( "version" ).toString().toStdString();
			}
		}
	}
	return ( "0.0.0.0" );
}

int IO::versionInt( const fs::path& folder )
{
	const auto version = versionString( folder );
	const auto vl  = ranges::actions::split( version, "." );
	if ( vl.size() == 4 )
	{
		const auto& v0 = vl[0];
		const auto& v1 = vl[1];
		const auto& v2 = vl[2];
		const auto& v3 = vl[3];
		int vi = std::atoi(v0.c_str()) * 1000 + std::atoi(v1.c_str()) * 100 + std::atoi(v2.c_str()) * 10 + std::atoi(v3.c_str());
		return vi;
		if ( vi < 577 )
		{
			return false;
		}
		return true;
	}
	return 0;
}

QJsonArray IO::jsonArrayConfig()
{
	if ( Global::debugMode )
		spdlog::debug("jsonArrayConfig");
	QJsonArray ja;
	ja.append( QJsonValue::fromVariant( toQVariant(Global::cfg->object()) ) );

	return ja;
}

bool IO::loadConfig( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	for ( const auto& entry : ja.toVariantList() )
	{
		auto map = entry.toMap();
	}

	return true;
}

QJsonArray IO::jsonArrayGame()
{
	if ( Global::debugMode )
		spdlog::debug("jsonArrayGame");
	GameState::version = QString::fromStdString(Global::cfg->get<std::string>( "CurrentVersion" ));

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

bool IO::saveWorld( const fs::path& folder )
{
	if ( Global::debugMode )
		spdlog::debug("saveWorld");
	QFile worldFile( QString::fromStdString( folder / "world.dat" ) );
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

bool IO::loadWorld( const fs::path& folder )
{
	QFile worldFile( QString::fromStdString(folder / "world.dat") );
	if ( worldFile.open( QIODevice::ReadOnly ) )
	{
		QDataStream in( &worldFile );

		loadWorld( in );

		worldFile.close();
		return true;
	}
	return false;
}

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

QJsonArray IO::jsonArrayWallConstructions()
{
	if ( Global::debugMode )
		spdlog::debug("jsonArrayWallConstructions");
	QJsonArray ja;
	for ( const auto& constr : g->w()->wallConstructions() | ranges::views::values )
	{
		QJsonValue jv = QJsonValue::fromVariant( constr );
		ja.append( jv );
	}

	return ja;
}

QJsonArray IO::jsonArrayFloorConstructions()
{
	if ( Global::debugMode )
		spdlog::debug("jsonArrayFloorConstructions");
	QJsonArray ja;
	for ( const auto& constr : g->w()->floorConstructions() | ranges::views::values )
	{
		QJsonValue jv = QJsonValue::fromVariant( constr );
		ja.append( jv );
	}

	return ja;
}

bool IO::loadFloorConstructions( QJsonDocument& jd )
{
	g->w()->loadFloorConstructions( jd.array().toVariantList() );

	return true;
}

bool IO::loadWallConstructions( QJsonDocument& jd )
{
	g->w()->loadWallConstructions( jd.array().toVariantList() );

	return true;
}

QJsonArray IO::jsonArraySprites()
{
	if ( Global::debugMode )
		spdlog::debug("jsonArraySprites");
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

QJsonArray IO::jsonArrayGnomes()
{
	if ( Global::debugMode )
		spdlog::debug("jsonArrayGnomes");
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

QJsonArray IO::jsonArrayMonsters( int startIndex, int amount )
{
	if ( Global::debugMode )
		spdlog::debug("jsonArrayMonsters");
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

bool IO::saveMonsters( const fs::path& folder )
{
	int i          = 1;
	int startIndex = 0;
	while ( true )
	{
		auto ja = jsonArrayMonsters( startIndex, 10000 );
		if ( ja.size() > 0 )
		{
			IO::saveFile( folder / "monsters" / (std::to_string( i ) + ".json"), ja );
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

bool IO::loadGnomes( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	spdlog::debug( "load {} gnomes", ja.toVariantList().size() );
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

bool IO::loadMonsters( const fs::path& folder )
{
	QJsonDocument jd;
	if ( QFileInfo::exists( QString::fromStdString( folder / "monsters.json" ) ) )
	{
		loadFile( folder / "monsters.json", jd );
		QJsonArray ja = jd.array();
		for ( const auto& entry : ja.toVariantList() )
		{
			g->cm()->addCreature( CreatureType::MONSTER, entry.toMap() );
		}
	}
	else
	{
		int i = 1;
		while ( QFileInfo::exists( QString::fromStdString( folder / "monsters" / (std::to_string( i ) + ".json") ) ) )
		{
			loadFile( folder / "monsters" / (std::to_string( i ) + ".json"), jd );
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

QJsonArray IO::jsonArrayPlants( int startIndex, int amount )
{
	if ( Global::debugMode )
		spdlog::debug("jsonArrayAnimals");

	QJsonArray ja;
	auto plants = g->w()->plants();
	auto plantsKeys = plants | ranges::views::keys;

	// FIXME: Original code was also creating a copy of the keys to iterate, but this seems wasteful in any case
	std::vector<unsigned int> keys(plantsKeys.begin(), plantsKeys.end());
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

bool IO::savePlants( const fs::path& folder )
{
	QByteArray out;

	int i          = 1;
	int startIndex = 0;
	while ( true )
	{
		auto ja = jsonArrayPlants( startIndex, 10000 );
		if ( ja.size() > 0 )
		{
			IO::saveFile( folder / "plants" / (std::to_string( i ) + ".json"), ja );
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

bool IO::loadPlants( const fs::path& folder )
{
	QJsonDocument jd;
	if ( QFileInfo::exists( QString::fromStdString(folder / "plants.json") ) )
	{
		loadFile( folder / "plants.json", jd );
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

		while ( QFileInfo::exists( QString::fromStdString(folder / "plants" / (std::to_string( i ) + ".json")) ) )
		{
			loadFile( folder / "plants" / (std::to_string( i ) + ".json"), jd );
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

QJsonArray IO::jsonArrayItems( int startIndex, int amount )
{
	if ( Global::debugMode )
		spdlog::debug("jsonArrayAnimals");

	QJsonArray ja;
	auto& items = g->inv()->allItems();
	const auto &itemsKeys = items | ranges::views::keys;

	// FIXME: Original code was also creating a copy of the keys to iterate, but this seems wasteful in any case
	std::vector<unsigned int> keys(itemsKeys.begin(), itemsKeys.end());

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

bool IO::saveItems( const fs::path& folder )
{
	g->inv()->sanityCheck();

	QByteArray out;

	int i          = 1;
	int startIndex = 0;
	while ( true )
	{
		auto ja = jsonArrayItems( startIndex, 50000 );
		if ( ja.size() > 0 )
		{
			IO::saveFile( folder / "items" / (std::to_string( i ) + ".json"), ja );
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

bool IO::loadItems( const fs::path& folder )
{
	g->inv()->loadFilter();

	QJsonDocument jd;
	if ( QFileInfo::exists( QString::fromStdString(folder / "items.json") ) )
	{
		loadFile( folder / "items.json", jd );
		QJsonArray ja = jd.array();
		int count     = 0;
		for ( const auto& entry : ja.toVariantList() )
		{
			g->inv()->createItem( entry.toMap() );
			++count;
		}
		spdlog::debug( "loaded {} items", count );
	}
	else
	{
		int i     = 1;
		int count = 0;
		while ( QFileInfo::exists(QString::fromStdString( folder / "items" / (std::to_string( i ) + ".json") )) )
		{
			loadFile( folder / "items" / (std::to_string( i ) + ".json"), jd );
			QJsonArray ja = jd.array();

			for ( const auto& entry : ja.toVariantList() )
			{
				g->inv()->createItem( entry.toMap() );
				++count;
			}
			++i;
		}
		spdlog::debug( "loaded {} items", count );
	}

	return true;
}

bool IO::loadItemHistory( QJsonDocument& jd )
{
	g->inv()->itemHistory()->deserialize( jd.toVariant().toMap() );
	return true;
}

QJsonArray IO::jsonArrayJobs()
{
	if ( Global::debugMode )
		spdlog::debug("jsonArrayJobs");
	QJsonArray ja;
	for ( const auto& job : g->jm()->allJobs() | ranges::views::values )
	{
		if ( !job->type().isEmpty() )
		{
			QJsonValue jv = QJsonValue::fromVariant( job->serialize() );
			ja.append( jv );
		}
	}

	return ja;
}

QJsonArray IO::jsonArrayJobSprites()
{
	if ( Global::debugMode )
		spdlog::debug("jsonArrayJobSprites");
	QJsonArray ja;
	auto jobSprites = g->w()->jobSprites();

	for ( const auto& key : jobSprites | ranges::views::keys )
	{
		auto entry = jobSprites[key];
		entry.insert( "PosID", key );
		QJsonValue jv = QJsonValue::fromVariant( entry );
		ja.append( jv );
	}

	return ja;
}

bool IO::loadJobs( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	for ( const auto& entry : ja.toVariantList() )
	{
		g->jm()->addLoadedJob( entry );
	}
	return true;
}

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

QJsonArray IO::jsonArrayFarms()
{
	if ( Global::debugMode )
		spdlog::debug("jsonArrayFarms");
	QJsonArray ja;
	for ( const auto& farm : g->fm()->allFarms() | ranges::views::values )
	{
		QJsonValue jv = QJsonValue::fromVariant( farm->serialize() );
		ja.append( jv );
	}
	for ( const auto& grove : g->fm()->allGroves() | ranges::views::values )
	{
		QJsonValue jv = QJsonValue::fromVariant( grove->serialize() );
		ja.append( jv );
	}
	for ( const auto& pasture : g->fm()->allPastures() | ranges::views::values )
	{
		QJsonValue jv = QJsonValue::fromVariant( pasture->serialize() );
		ja.append( jv );
	}
	for ( const auto& beehive : g->fm()->allBeeHives() | ranges::views::values )
	{
		QVariantMap vm;
		beehive->serialize( vm );
		QJsonValue jv = QJsonValue::fromVariant( vm );
		ja.append( jv );
	}

	return ja;
}

QJsonArray IO::jsonArrayRooms()
{
	if ( Global::debugMode )
		spdlog::debug("jsonArrayRooms");
	QJsonArray ja;
	for ( const auto& room : g->rm()->allRooms() | ranges::views::values )
	{
		QJsonValue jv = QJsonValue::fromVariant( room->serialize() );
		ja.append( jv );
	}

	return ja;
}

QJsonArray IO::jsonArrayDoors()
{
	if ( Global::debugMode )
		spdlog::debug("jsonArrayDoors");
	QJsonArray ja;
	for ( const auto& door : g->rm()->allDoors() | ranges::views::values )
	{
		QVariantMap out;

		out.insert( "Pos", door.pos.toString() );
		out.insert( "Name", door.name );
		out.insert( "ItemUID", door.itemUID );
		out.insert( "MaterialUID", door.materialUID );
		out.insert( "BlockGnomes", door.blockGnomes );
		out.insert( "BlockAnimals", door.blockAnimals );
		out.insert( "BlockMonsters", door.blockMonsters );

		QJsonValue jv = QJsonValue::fromVariant( out );
		ja.append( jv );
	}

	return ja;
}

bool IO::loadFarms( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	for ( const auto& entry : ja.toVariantList() )
	{
		g->fm()->load( entry.toMap() );
	}
	return true;
}

bool IO::loadRooms( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	for ( const auto& entry : ja.toVariantList() )
	{
		g->rm()->load( entry.toMap() );
	}
	return true;
}

bool IO::loadDoors( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	for ( const auto& entry : ja.toVariantList() )
	{
		g->rm()->loadDoor( entry.toMap() );
	}
	return true;
}

QJsonArray IO::jsonArrayStockpiles()
{
	if ( Global::debugMode )
		spdlog::debug("jsonArrayStockpiles");
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

bool IO::loadStockpiles( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	for ( const auto& entry : ja.toVariantList() )
	{
		g->spm()->load( entry.toMap() );
	}
	return true;
}

QJsonArray IO::jsonArrayWorkshops()
{
	if ( Global::debugMode )
		spdlog::debug("jsonArrayWorkshops");
	QJsonArray ja;
	for ( const auto& w : g->wsm()->workshops() )
	{
		QJsonValue jv = QJsonValue::fromVariant( w->serialize() );
		ja.append( jv );
	}
	return ja;
}

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

QJsonArray IO::jsonArrayAnimals( int startIndex, int amount )
{
	if ( Global::debugMode )
		spdlog::debug("jsonArrayAnimals");

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

bool IO::saveAnimals( const fs::path& folder )
{
	int i          = 1;
	int startIndex = 0;
	while ( true )
	{
		auto ja = jsonArrayAnimals( startIndex, 10000 );
		if ( ja.size() > 0 )
		{
			IO::saveFile( folder / "animals" / (std::to_string( i ) + ".json"), ja );
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

bool IO::loadAnimals( const fs::path& folder )
{
	QJsonDocument jd;
	if ( QFileInfo::exists( QString::fromStdString( folder / "animals.json" ) ) )
	{
		loadFile( folder / "animals.json", jd );
		QJsonArray ja = jd.array();
		for ( const auto& entry : ja.toVariantList() )
		{
			g->cm()->addCreature( CreatureType::ANIMAL, entry.toMap() );
		}
	}
	else
	{
		int i = 1;
		while ( QFileInfo::exists( QString::fromStdString(folder / "animals" / (std::to_string( i ) + ".json") )) )
		{
			loadFile( folder / "animals" / (std::to_string( i ) + ".json"), jd );
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

QJsonDocument IO::jsonArrayItemHistory()
{
	if ( Global::debugMode )
		spdlog::debug("jsonArrayItemHistory");
	QVariantMap out;
	g->inv()->itemHistory()->serialize( out );

	QJsonDocument jd = QJsonDocument::fromVariant( out );

	return jd;
}

QJsonDocument IO::jsonArrayEvents()
{
	if ( Global::debugMode )
		spdlog::debug("jsonArrayEvents");
	QVariantMap out = g->em()->serialize();
	QVariantList ol;
	ol.append( out );
	QJsonDocument jd = QJsonDocument::fromVariant( ol );

	return jd;
}

bool IO::loadEvents( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	for ( const auto& entry : ja.toVariantList() )
	{
		g->em()->deserialize( entry.toMap() );
	}
	return true;
}

QJsonDocument IO::jsonArrayMechanisms()
{
	if ( Global::debugMode )
		spdlog::debug("jsonArrayMechanisms");
	QVariantList ol;

	for ( const auto& md : g->mcm()->mechanisms() | ranges::views::values )
	{
		auto vmd = md.serialize();
		ol.append( vmd );
	}

	QJsonDocument jd = QJsonDocument::fromVariant( ol );

	return jd;
}

bool IO::loadMechanisms( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	g->mcm()->loadMechanisms( ja.toVariantList() );
	return true;
}

QJsonDocument IO::jsonArrayPipes()
{
	if ( Global::debugMode )
		spdlog::debug("jsonArrayPipes");
	QVariantList ol;

	for ( const auto& fp : g->flm()->pipes() | ranges::views::values )
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

bool IO::loadPipes( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	g->flm()->loadPipes( ja.toVariantList() );
	return true;
}

bool IO::saveFile( const fs::path& url, const QJsonObject& jo )
{
	QJsonDocument saveDoc( jo );
	return saveFile( url, saveDoc );
}

bool IO::saveFile( const fs::path& url, const QJsonArray& ja )
{
	QJsonDocument saveDoc( ja );
	return saveFile( url, saveDoc );
}

bool IO::saveFile( const fs::path& url, const QJsonDocument& jd )
{
	QFile file( QString::fromStdString(url) );

	if ( !file.open( QIODevice::WriteOnly ) )
	{
		spdlog::warn( "Couldn't open save file." );
		return false;
	}

	file.write( jd.toJson() );
	file.close();
	return true;
}

bool IO::saveFile( const fs::path& url, const json& jd )
{
	std::ofstream fos(url);

	if (!fos.is_open() || fos.bad())
	{
		spdlog::warn( "Couldn't open save file." );
		return false;
	}

	fos << jd.dump(4);

	return true;
}

bool IO::loadFile( const fs::path& url, QJsonDocument& ja )
{
	if (!fs::exists(url)) {
		spdlog::debug( "JSON file '{}' doesn't exist", url.string() );
		return false;
	}

	QFile file( QString::fromStdString(url) );
	file.open( QIODevice::ReadOnly | QIODevice::Text );
	QString val = file.readAll();
	file.close();

	QJsonParseError error;
	const auto& jsonData = val.toUtf8();
	ja = QJsonDocument::fromJson( jsonData, &error );

	if ( error.error == QJsonParseError::NoError )
	{
		return true;
	}

	qDebug() << "json parse error in" << QString::fromStdString(url) << error.offset;

	return false;
}

bool IO::loadFile( const fs::path& url, json& ja )
{
	if (!fs::exists(url)) {
		spdlog::debug( "JSON file '{}' doesn't exist", url.string() );
		return false;
	}

	std::ifstream fis(url);
	try
	{
		ja = json::parse( fis );
		return true;
	} catch (json::parse_error& e) {
		spdlog::debug( "json parse error in {}: {}", url.string(), e.what() );

		return false;
	}
}
