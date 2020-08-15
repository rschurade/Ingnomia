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

int IO::version = 0;

IO::IO( QObject* parent ) :
	QObject( parent )
{
}

IO::~IO()
{
}

bool IO::saveCompatible( QString folder )
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
	QString folder = QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/My Games/Ingnomia/save/";

	if ( !QDir( folder ).exists() || QDir( folder ).isEmpty() )
	{
		return false;
	}
	return true;
}

bool IO::saveConfig()
{
	QString folder = QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/My Games/Ingnomia/settings/";

	QVariantMap cm   = Config::getInstance().object();
	QJsonDocument jd = QJsonDocument::fromVariant( cm );

	IO::saveFile( folder + "config.json", jd );

	return true;
}

QString IO::getTempFolder()
{
	return QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/My Games/Ingnomia/tmp/";
}

bool IO::createFolders()
{
	QString folder = QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/My Games/";
	if ( !QDir( folder ).exists() )
	{
		QDir().mkdir( folder );
	}
	folder += "Ingnomia/";
	if ( !QDir( folder ).exists() )
	{
		QDir().mkdir( folder );
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

	folder = QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/My Games/Ingnomia/save/";

	return QDir( folder ).exists();
}

QString IO::save( bool autosave )
{
	QElapsedTimer timer;
	timer.start();

	QString folder = QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/My Games/Ingnomia/save/";

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
	/*
	if( autosave )
	{
		folder += "_autosave";
	}*/

	folder += "/";
	if ( Global::debugMode )
		qDebug() << folder;
	if ( !QDir( folder ).exists() )
	{
		QDir().mkdir( folder );
	}

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
	return folder;
}

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

	Global::gm().init();
	Global::mil().init();

	PathFinder::getInstance().init();
	Util::initAllowedInContainer();

	loadFile( folder + "sprites.json", jd );
	IO::loadSprites( jd );
	emit signalStatus( "Start loading world..." );
	if ( !IO::loadWorld( folder ) )
	{
		return false;
	}
	Global::w().afterLoad();
	emit signalStatus( "Loading world...done" );
	IO::loadItems( folder );
	emit signalStatus( "Loading items done" );
	loadFile( folder + "floorconstructions.json", jd );
	IO::loadFloorConstructions( jd );
	loadFile( folder + "wallconstructions.json", jd );
	IO::loadWallConstructions( jd );
	emit signalStatus( "Loading constructions done" );
	loadFile( folder + "workshops.json", jd );
	IO::loadWorkshops( jd );
	loadFile( folder + "jobs.json", jd );
	IO::loadJobs( jd );
	loadFile( folder + "jobsprites.json", jd );
	IO::loadJobSprites( jd );
	emit signalStatus( "Loading jobs done" );
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

void IO::sanitize()
{
	// Migration for 0.7.5 games where gnomes had stored their own ID in job field of items
	{
		std::unordered_set<unsigned int> legalJobs;
		for ( const auto& job : Global::jm().allJobs() )
		{
			legalJobs.emplace( job.id() );
		}

		std::unordered_set<unsigned int> carriedItems;
		std::unordered_set<unsigned int> wornItems;
		for ( const auto& gnome : Global::gm().gnomes() )
		{
			{
				const auto& c = gnome->inventoryItems();
				carriedItems.insert( c.cbegin(), c.cend() );
			}
			{
				const auto& c = gnome->carriedItems();
				carriedItems.insert( c.cbegin(), c.cend() );
			}
			for ( const auto& claim : gnome->claimedItems() )
			{
				// Special exemption in case this is legacy type reservation
				legalJobs.emplace( gnome->id() );
			}

			for ( const auto& itemID : gnome->equipment().wornItems() )
			{
				wornItems.insert( itemID );
			}
		}

		for ( auto& item : Global::inv().allItems() )
		{
			const auto job = item.isInJob();
			if ( job && !legalJobs.count( job ) )
			{
				item.setInJob( 0 );
				qWarning() << "item " + QString::number( item.id() ) + " " + item.itemSID() + " had illegal job";
			}
			if ( item.isPickedUp() )
			{
				const bool worn    = 0 != wornItems.count( item.id() );
				const bool carried = 0 != carriedItems.count( item.id() );
				if ( !worn && !carried )
				{
					item.setIsConstructedOrEquipped( false );
					Global::inv().putDownItem( item.id(), item.getPos() );
					qWarning() << "item " + QString::number( item.id() ) + " " + item.itemSID() + " found lost in space";
				}
				else if ( worn xor item.isConstructedOrEquipped() )
				{
					item.setIsConstructedOrEquipped( false );
					Global::inv().putDownItem( item.id(), item.getPos() );
					qWarning() << "item " + QString::number( item.id() ) + " " + item.itemSID() + " found being dragged along";
				}
			}
		}
	}
}

QString IO::versionString( QString folder )
{
	QJsonDocument jd;
	IO::loadFile( folder + "/game.json", jd );

	QJsonArray ja = jd.array();

	QVariantMap vm = ja.toVariantList().first().toMap();

	if ( vm.contains( "Version" ) )
	{
		return vm.value( "Version" ).toString();
	}
	else
	{
		return vm.value( "version" ).toString();
	}
}

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

QJsonArray IO::jsonArrayConfig()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayConfig";
	QJsonArray ja;
	ja.append( QJsonValue::fromVariant( Config::getInstance().object() ) );

	return ja;
}

bool IO::loadConfig( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	for ( auto entry : ja.toVariantList() )
	{
		auto map = entry.toMap();

		Config::getInstance().set( "moveX", map.value( "moveX" ).toInt() );
		Config::getInstance().set( "moveY", map.value( "moveY" ).toInt() );
		Config::getInstance().set( "oldMoveX", map.value( "oldMoveX" ).toInt() );
		Config::getInstance().set( "oldMoveY", map.value( "oldMoveY" ).toInt() );
		Config::getInstance().set( "viewLevel", map.value( "viewLevel" ).toInt() );
		Config::getInstance().set( "scale", map.value( "scale" ).toInt() );
	}

	return true;
}

QJsonArray IO::jsonArrayGame()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayGame";
	GameState::version = Config::getInstance().get( "CurrentVersion" ).toString();

	GameState::initialSave = true;

	Global::inv().saveFilter();

	GameState::neighbors = Global::nm().serialize();
	GameState::military  = Global::mil().serialize();

	Global::mil().save(); // TODO why this? maybe move it into serialize

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

	for ( auto vMat : GameState::addedMaterials )
	{
		DB::addRow( "Materials", vMat.toMap() );
	}

	for ( auto key : GameState::addedTranslations.keys() )
	{
		Strings::getInstance().insertString( key, GameState::addedTranslations[key].toString() );
	}

	Global::nm().deserialize( GameState::neighbors );

	return true;
}

bool IO::saveWorld( QString folder )
{
	if ( Global::debugMode )
		qDebug() << "saveWorld";
	QFile worldFile( folder + "world.dat" );
	if ( worldFile.open( QIODevice::WriteOnly ) )
	{
		QDataStream out( &worldFile );
		std::vector<Tile>& world = Global::w().world();

		for ( auto tile : world )
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

void IO::loadWorld( QDataStream& in )
{
	unsigned short dimX = Global::dimX;
	unsigned short dimY = Global::dimY;
	unsigned short dimZ = Global::dimZ;

	std::vector<Tile>& world = Global::w().world();
	world.clear();
	world.reserve( dimX * dimY * dimZ );

	quint16 wallType;
	quint16 floorType;
	quint8 flow;

#ifdef SAVEREGIONINFO
	quint32 region;
#endif

	while ( !in.atEnd() )
	{
		Tile tile;
		in >> tile.flags;
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
		qDebug() << "jsonArrayWallConstructions";
	QJsonArray ja;
	for ( auto constr : Global::w().wallConstructions() )
	{
		QJsonValue jv = QJsonValue::fromVariant( constr );
		ja.append( jv );
	}

	return ja;
}

QJsonArray IO::jsonArrayFloorConstructions()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayFloorConstructions";
	QJsonArray ja;
	for ( auto constr : Global::w().floorConstructions() )
	{
		QJsonValue jv = QJsonValue::fromVariant( constr );
		ja.append( jv );
	}

	return ja;
}

bool IO::loadFloorConstructions( QJsonDocument& jd )
{
	Global::w().loadFloorConstructions( jd.array().toVariantList() );

	return true;
}

bool IO::loadWallConstructions( QJsonDocument& jd )
{
	Global::w().loadWallConstructions( jd.array().toVariantList() );

	return true;
}

QJsonArray IO::jsonArraySprites()
{
	if ( Global::debugMode )
		qDebug() << "jsonArraySprites";
	QJsonArray ja;
	for ( auto sc : Global::sf().spriteCreations() )
	{
		QVariantMap vm;
		vm.insert( "ItemSID", sc.itemSID );
		vm.insert( "MaterialSIDs", sc.materialSIDs.join( '_' ) );
		vm.insert( "Random", Util::mapJoin( sc.random ) );
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
	for ( auto entry : ja.toVariantList() )
	{
		QVariantMap em = entry.toMap();
		SpriteCreation sc;
		sc.itemSID      = em.value( "ItemSID" ).toString();
		sc.materialSIDs = em.value( "MaterialSIDs" ).toString().split( "_" );
		sc.random       = Util::mapSplit( em.value( "Random" ).toString() );
		sc.uID          = em.value( "UID" ).toUInt();
		sc.creatureID   = em.value( "CreatureID" ).toUInt();
		scl.push_back( sc );
	}
	Global::sf().createSprites( scl );
	return true;
}

QJsonArray IO::jsonArrayGnomes()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayGnomes";
	QJsonArray ja;
	for ( auto gnome : Global::gm().gnomes() )
	{
		QVariantMap out;
		gnome->serialize( out );
		QJsonValue jv = QJsonValue::fromVariant( out );
		ja.append( jv );
	}
	for ( auto gnome : Global::gm().specialGnomes() )
	{
		QVariantMap out;
		gnome->serialize( out );
		QJsonValue jv = QJsonValue::fromVariant( out );
		ja.append( jv );
	}
	for ( auto automaton : Global::gm().automatons() )
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
		qDebug() << "jsonArrayMonsters";
	QJsonArray ja;

	int i         = startIndex;
	auto monsters = Global::cm().monsters();

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

bool IO::loadGnomes( QJsonDocument& jd )
{
	World& world  = Global::w();
	QJsonArray ja = jd.array();
	qDebug() << "load " << ja.toVariantList().size() << "gnomes";
	for ( auto entry : ja.toVariantList() )
	{
		auto em = entry.toMap();
		switch ( (CreatureType)em.value( "Type" ).toInt() )
		{
			case CreatureType::GNOME:
				Global::gm().addGnome( em );
				break;
			case CreatureType::GNOME_TRADER:
				Global::gm().addTrader( em );
				break;
			case CreatureType::AUTOMATON:
				Global::gm().addAutomaton( em );
				break;
		}
	}
	return true;
}

bool IO::loadMonsters( QString folder )
{
	QJsonDocument jd;
	if ( QFileInfo::exists( folder + "monsters.json" ) )
	{
		loadFile( folder + "monsters.json", jd );
		QJsonArray ja = jd.array();
		for ( auto entry : ja.toVariantList() )
		{
			Global::cm().addCreature( CreatureType::MONSTER, entry.toMap() );
		}
	}
	else
	{
		int i = 1;
		while ( QFileInfo::exists( folder + "monsters" + QString::number( i ) + ".json" ) )
		{
			loadFile( folder + "monsters" + QString::number( i ) + ".json", jd );
			QJsonArray ja = jd.array();
			for ( auto entry : ja.toVariantList() )
			{
				Global::cm().addCreature( CreatureType::MONSTER, entry.toMap() );
			}
			++i;
		}
	}
	return true;
}

QJsonArray IO::jsonArrayPlants( int startIndex, int amount )
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayAnimals";

	QJsonArray ja;
	auto plants = Global::w().plants();
	auto keys   = plants.keys();
	int i       = startIndex;

	while ( i < keys.size() && amount > 0 )
	{
		auto plant = plants[keys[i]];

		QJsonValue jv = QJsonValue::fromVariant( plant.serialize() );
		ja.append( jv );

		++i;
		--amount;
	}
	return ja;
}

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

bool IO::loadPlants( QString folder )
{
	World& world = Global::w();

	QJsonDocument jd;
	if ( QFileInfo::exists( folder + "plants.json" ) )
	{
		loadFile( folder + "plants.json", jd );
		QJsonArray ja = jd.array();
		for ( auto entry : ja.toVariantList() )
		{
			Plant plant( entry.toMap() );
			world.addPlant( plant );
		}
	}
	else
	{
		int i = 1;

		while ( QFileInfo::exists( folder + "plants" + QString::number( i ) + ".json" ) )
		{
			loadFile( folder + "plants" + QString::number( i ) + ".json", jd );
			QJsonArray ja = jd.array();

			for ( auto entry : ja.toVariantList() )
			{
				Plant plant( entry.toMap() );
				world.addPlant( plant );
			}
			++i;
		}
	}
	return true;
}

QJsonArray IO::jsonArrayItems( int startIndex, int amount )
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayAnimals";

	QJsonArray ja;
	auto& items = Global::inv().allItems();
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

bool IO::saveItems( QString folder )
{
	Inventory& inv = Global::inv();
	inv.sanityCheck();

	QByteArray out;

	auto& items = Global::inv().allItems();

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

bool IO::loadItems( QString folder )
{
	Inventory& inv = Global::inv();
	inv.loadFilter();

	QJsonDocument jd;
	if ( QFileInfo::exists( folder + "items.json" ) )
	{
		loadFile( folder + "items.json", jd );
		QJsonArray ja = jd.array();
		int count     = 0;
		for ( const auto& entry : ja.toVariantList() )
		{
			inv.createItem( entry.toMap() );
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

			for ( auto entry : ja.toVariantList() )
			{
				inv.createItem( entry.toMap() );
				++count;
			}
			++i;
		}
		qDebug() << "loaded" << count << "items";
	}

	return true;
}

bool IO::loadItemHistory( QJsonDocument& jd )
{
	Global::ih().deserialize( jd.toVariant().toMap() );
	return true;
}

QJsonArray IO::jsonArrayJobs()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayJobs";
	QJsonArray ja;
	for ( auto job : Global::jm().allJobs() )
	{
		if ( !job.type().isEmpty() )
		{
			QJsonValue jv = QJsonValue::fromVariant( job.serialize() );
			ja.append( jv );
		}
	}

	return ja;
}

QJsonArray IO::jsonArrayJobSprites()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayJobSprites";
	QJsonArray ja;
	auto jobSprites = Global::w().jobSprites();

	for ( auto key : jobSprites.keys() )
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
	JobManager& jm = Global::jm();
	QJsonArray ja  = jd.array();
	for ( auto entry : ja.toVariantList() )
	{
		jm.addLoadedJob( entry );
	}
	return true;
}

bool IO::loadJobSprites( QJsonDocument& jd )
{
	World& world  = Global::w();
	QJsonArray ja = jd.array();
	for ( auto entry : ja.toVariantList() )
	{
		auto em          = entry.toMap();
		unsigned int key = em.value( "JobID" ).toUInt();
		em.remove( "JobID" );
		world.insertLoadedJobSprite( key, em );
	}
	return true;
}

QJsonArray IO::jsonArrayFarms()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayFarms";
	QJsonArray ja;
	for ( auto farm : Global::fm().allFarms() )
	{
		QJsonValue jv = QJsonValue::fromVariant( farm.serialize() );
		ja.append( jv );
	}
	for ( auto grove : Global::fm().allGroves() )
	{
		QJsonValue jv = QJsonValue::fromVariant( grove.serialize() );
		ja.append( jv );
	}
	for ( auto pasture : Global::fm().allPastures() )
	{
		QJsonValue jv = QJsonValue::fromVariant( pasture.serialize() );
		ja.append( jv );
	}
	for ( auto beehive : Global::fm().allBeeHives() )
	{
		QVariantMap vm;
		beehive.serialize( vm );
		QJsonValue jv = QJsonValue::fromVariant( vm );
		ja.append( jv );
	}

	return ja;
}

QJsonArray IO::jsonArrayRooms()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayRooms";
	QJsonArray ja;
	for ( auto room : Global::rm().allRooms() )
	{
		QJsonValue jv = QJsonValue::fromVariant( room.serialize() );
		ja.append( jv );
	}

	return ja;
}

QJsonArray IO::jsonArrayDoors()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayDoors";
	QJsonArray ja;
	for ( auto door : Global::rm().allDoors() )
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
	FarmingManager& fm = Global::fm();
	QJsonArray ja      = jd.array();
	for ( auto entry : ja.toVariantList() )
	{
		fm.load( entry.toMap() );
	}
	return true;
}

bool IO::loadRooms( QJsonDocument& jd )
{
	RoomManager& rm = Global::rm();
	QJsonArray ja   = jd.array();
	for ( auto entry : ja.toVariantList() )
	{
		rm.load( entry.toMap() );
	}
	return true;
}

bool IO::loadDoors( QJsonDocument& jd )
{
	RoomManager& rm = Global::rm();
	QJsonArray ja   = jd.array();
	for ( auto entry : ja.toVariantList() )
	{
		rm.loadDoor( entry.toMap() );
	}
	return true;
}

QJsonArray IO::jsonArrayStockpiles()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayStockpiles";
	QJsonArray ja;
	for ( auto stockpileID : Global::spm().allStockpilesOrdered() )
	{
		auto stockpile = Global::spm().getStockpile( stockpileID );
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
	StockpileManager& sm = Global::spm();
	QJsonArray ja        = jd.array();
	for ( auto entry : ja.toVariantList() )
	{
		sm.load( entry.toMap() );
	}
	return true;
}

QJsonArray IO::jsonArrayWorkshops()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayWorkshops";
	QJsonArray ja;
	for ( auto w : Global::wsm().workshops() )
	{
		QJsonValue jv = QJsonValue::fromVariant( w->serialize() );
		ja.append( jv );
	}
	return ja;
}

bool IO::loadWorkshops( QJsonDocument& jd )
{
	WorkshopManager& wm = Global::wsm();

	QJsonArray ja = jd.array();
	for ( auto entry : ja.toVariantList() )
	{
		wm.addWorkshop( entry.toMap() );
		for ( auto s : entry.toMap().value( "Sprites" ).toList() )
		{
			Global::w().addLoadedSprites( s.toMap() );
		}
	}
	return true;
}

QJsonArray IO::jsonArrayAnimals( int startIndex, int amount )
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayAnimals";

	QJsonArray ja;
	auto animals = Global::cm().animals();

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

bool IO::loadAnimals( QString folder )
{
	QJsonDocument jd;
	if ( QFileInfo::exists( folder + "animals.json" ) )
	{
		loadFile( folder + "animals.json", jd );
		QJsonArray ja = jd.array();
		for ( auto entry : ja.toVariantList() )
		{
			Global::cm().addCreature( CreatureType::ANIMAL, entry.toMap() );
		}
	}
	else
	{
		int i = 1;
		while ( QFileInfo::exists( folder + "animals" + QString::number( i ) + ".json" ) )
		{
			loadFile( folder + "animals" + QString::number( i ) + ".json", jd );
			QJsonArray ja = jd.array();
			for ( auto entry : ja.toVariantList() )
			{
				Global::cm().addCreature( CreatureType::ANIMAL, entry.toMap() );
			}
			++i;
		}
	}
	return true;
}

QJsonDocument IO::jsonArrayItemHistory()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayItemHistory";
	QVariantMap out;
	Global::ih().serialize( out );

	QJsonDocument jd = QJsonDocument::fromVariant( out );

	return jd;
}

QJsonDocument IO::jsonArrayEvents()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayEvents";
	QVariantMap out = Global::em().serialize();
	QVariantList ol;
	ol.append( out );
	QJsonDocument jd = QJsonDocument::fromVariant( ol );

	return jd;
}

bool IO::loadEvents( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	for ( auto entry : ja.toVariantList() )
	{
		Global::em().deserialize( entry.toMap() );
	}
	return true;
}

QJsonDocument IO::jsonArrayMechanisms()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayMechanisms";
	QVariantList ol;

	for ( auto md : Global::mcm().mechanisms() )
	{
		auto vmd = md.serialize();

		if ( md.jobID )
		{
			Job* job = Global::mcm().getJob( md.jobID );
			if ( job )
			{
				vmd.insert( "Job", job->serialize() );
			}
			else
			{
				vmd.insert( "JobID", 0 );
			}
		}

		ol.append( vmd );
	}

	QJsonDocument jd = QJsonDocument::fromVariant( ol );

	return jd;
}

bool IO::loadMechanisms( QJsonDocument& jd )
{
	QJsonArray ja = jd.array();
	Global::mcm().loadMechanisms( ja.toVariantList() );
	return true;
}

QJsonDocument IO::jsonArrayPipes()
{
	if ( Global::debugMode )
		qDebug() << "jsonArrayPipes";
	QVariantList ol;

	for ( auto fp : Global::flm().pipes() )
	{
		auto vfp = fp.serialize();
		/*
		if( md.jobID )
		{
			Job* job = Global::mcm().getJob( md.jobID );
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
	Global::flm().loadPipes( ja.toVariantList() );
	return true;
}

bool IO::saveFile( QString url, const QJsonObject& jo )
{
	QJsonDocument saveDoc( jo );
	return saveFile( url, saveDoc );
}

bool IO::saveFile( QString url, const QJsonArray& ja )
{
	QJsonDocument saveDoc( ja );
	return saveFile( url, saveDoc );
}

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
