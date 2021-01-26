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
#include "gamestate.h"
#include "../base/db.h"

#include <QDebug>
#include <QDir>
#include <QFile>

int GameState::alarm        = 0;
unsigned int GameState::alarmRoomID = 0;

bool GameState::daylight    = true;
bool GameState::initialSave = false;
QVariantList GameState::itemFilter;

QVariantMap GameState::military; // military manager

QVariantList GameState::neighbors; // neighbor manager

Position GameState::origin;
QVariantList GameState::squads;

QVariantList GameState::stockOverlay;

QVariantMap GameState::techs;

QString GameState::version = "uninitialized";

int GameState::flatness = 0;

int GameState::minute = 0;
int GameState::hour   = 0;
int GameState::day    = 1;
int GameState::season = 0; // zero based, 0-numSeasons, typically 4 can be modded
int GameState::year   = 1;

QString GameState::seasonString         = "uninitialized";
QString GameState::currentDayTime       = "uninitialized";
QString GameState::currentYearAndSeason = "uninitialized";

bool GameState::dayChanged    = true;
bool GameState::hourChanged   = true;
bool GameState::minuteChanged = true;
bool GameState::seasonChanged = true;

int GameState::nextSunrise = 0;
int GameState::sunrise     = 0;
int GameState::sunset      = 0;

int GameState::groundLevel     = 100;
QString GameState::kingdomName = "uninitialized";

unsigned int GameState::nextID = 1;

bool GameState::peaceful = false;

quint64 GameState::tick = 1;

int GameState::numAnimals   = 0;
int GameState::numGnomes    = 0;
int GameState::oceanSize    = 0;
int GameState::plantDensity = 0;
int GameState::treeDensity  = 0;
int GameState::riverSize    = 0;
int GameState::rivers       = 0;
QString GameState::seed     = "uninitialized";

QVariantList GameState::startingItems;

unsigned int GameState::startingZone = 0;

int GameState::maxAnimalsPerType = 500;
QVariantMap GameState::allowedAnimals;
QVariantMap GameState::allowedPlants;
QVariantMap GameState::allowedTrees;

QVariantList GameState::addedMaterials;
QVariantMap GameState::addedTranslations;

int GameState::moveX = 0;
int GameState::moveY = 0;
float GameState::scale = 0.0;
int GameState::viewLevel = 100;

QList<GuiWatchedItem> GameState::watchedItemList;

QHash<QString, int> GameState::materialSID2ID;
QHash<int, QString> GameState::materialID2SID;
QHash<QString, int> GameState::itemSID2ID;
QHash<int, QString> GameState::itemID2SID;


bool GameState::init()
{
	GameState::materialSID2ID.clear();
	GameState::materialID2SID.clear();

	GameState::watchedItemList.clear();

	nextID = 1000000;
	return true;
}

void GameState::serialize( QVariantMap& out )
{
	out.insert( "alarm", alarm );
	out.insert( "alarmRoomID", alarmRoomID );
	
	out.insert( "daylight", daylight );
	out.insert( "initialSave", initialSave );
	out.insert( "itemFilter", itemFilter );

	out.insert( "military", military );
	out.insert( "neighbors", neighbors );

	out.insert( "origin", origin.toString() );
	out.insert( "squads", squads );

	out.insert( "stockOverlay", stockOverlay );

	out.insert( "techs", techs );

	out.insert( "version", version );

	out.insert( "flatness", flatness );
	
	out.insert( "day", day );
	out.insert( "hour", hour );
	out.insert( "minute", minute );
	out.insert( "season", season );
	out.insert( "year", year );

	out.insert( "seasonString", seasonString );

	out.insert( "dayChanged", dayChanged );
	out.insert( "hourChanged", hourChanged );
	out.insert( "minuteChanged", minuteChanged );
	out.insert( "seasonChanged", seasonChanged );

	out.insert( "nextSunrise", nextSunrise );
	out.insert( "sunrise", sunrise );
	out.insert( "sunset", sunset );
	
	out.insert( "groundLevel", groundLevel );
	out.insert( "kingdomName", kingdomName );
	
	out.insert( "peaceful", peaceful );

	out.insert( "tick", tick );

	out.insert( "nextID", nextID );

	out.insert( "numAnimals", numAnimals );
	out.insert( "numGnomes", numGnomes );
	out.insert( "oceanSize", oceanSize );
	out.insert( "plantDensity", plantDensity );
	out.insert( "treeDensity", treeDensity );
	out.insert( "riverSize", riverSize );
	out.insert( "rivers", rivers );
	out.insert( "seed", seed );

	out.insert( "startingItems", startingItems );

	out.insert( "startingZone", startingZone );
	
	out.insert( "maxAnimalsPerType", maxAnimalsPerType );
	out.insert( "allowedAnimals", allowedAnimals );
	out.insert( "allowedPlants", allowedPlants );
	out.insert( "allowedTrees", allowedTrees );

	out.insert( "addedMaterials", addedMaterials );
	out.insert( "addedTranslations", addedTranslations );
	
	out.insert( "dimX", Global::dimX );
	out.insert( "dimY", Global::dimY );
	out.insert( "dimZ", Global::dimZ );

	out.insert( "moveX", moveX );
	out.insert( "moveY", moveY );
	out.insert( "scale", scale );
	out.insert( "viewLevel", viewLevel );

	QVariantList qwil;
	for( const auto& gwi : GameState::watchedItemList )
	{
		QVariantMap qwi;
		qwi.insert( "category", gwi.category );
		qwi.insert( "group", gwi.group );
		qwi.insert( "item", gwi.item );
		qwi.insert( "material", gwi.material );
		qwil.append( qwi );
	}
	out.insert( "watchedItems", qwil );

	QVariantMap vmIDs;
	for( auto key : GameState::materialSID2ID.keys() )
	{
		vmIDs.insert( key, materialSID2ID.value( key ) );
	}
	out.insert( "mats2ids", vmIDs );

	QVariantMap viIDs;
	for( auto key : GameState::itemSID2ID.keys() )
	{
		viIDs.insert( key, itemSID2ID.value( key ) );
	}
	out.insert( "items2ids", viIDs );

}

void GameState::load( QVariantMap& vals )
{
	// compatibility with older saves
	QVariantMap tmp;
	for ( auto key : vals.keys() )
	{
		QString newKey = key;
		newKey[0]      = key[0].toLower();

		tmp.insert( newKey, vals.value( key ) );
	}

	Global::dimX = tmp.value( "dimX" ).toInt();
	Global::dimY = tmp.value( "dimY" ).toInt();
	Global::dimZ = tmp.value( "dimZ" ).toInt();

	alarm       = tmp.value( "alarm" ).toInt();
	alarmRoomID = tmp.value( "alarmRoomID" ).toUInt();

	daylight    = tmp.value( "daylight" ).toBool();
	initialSave = tmp.value( "initialSave" ).toBool();
	itemFilter  = tmp.value( "itemFilter" ).toList();

	military  = tmp.value( "military" ).toMap();   // military manager
	neighbors = tmp.value( "neighbors" ).toList(); // neighbor manager

	origin = Position( tmp.value( "origin" ) );
	squads = tmp.value( "squads" ).toList();

	stockOverlay = tmp.value( "stockOverlay" ).toList();

	//TODO old format has them as single entries
	if( tmp.contains( "techWood" ) )
	{
		GameState::techs.clear();
		for( auto t : DB::ids( "Tech" ) )
		{
			GameState::techs.insert( t, tmp.value( t ).toInt() );
		}
	}
	else
	{
		techs = tmp.value( "techs" ).toMap();
	}

	version  = tmp.value( "version" ).toString();
	flatness = tmp.value( "flatness" ).toInt();

	//old version has prefix game_ for the next values
	if ( tmp.contains( "game_day" ) )
	{
		day    = tmp.value( "game_day" ).toInt();
		hour   = tmp.value( "game_hour" ).toInt();
		minute = tmp.value( "game_minute" ).toInt();
		season = tmp.value( "game_season" ).toInt();
		year   = tmp.value( "game_year" ).toInt();

		nextSunrise = tmp.value( "game_nextSunrise" ).toInt();
		sunrise     = tmp.value( "game_sunrise" ).toInt();
		sunset      = tmp.value( "game_sunset" ).toInt();

		dayChanged    = tmp.value( "game_dayChanged" ).toBool();
		hourChanged   = tmp.value( "game_hourChanged" ).toBool();
		minuteChanged = tmp.value( "game_minuteChanged" ).toBool();
		seasonChanged = tmp.value( "game_seasonChanged" ).toBool();

		seasonString = tmp.value( "season" ).toString();
	}
	else
	{
		day    = tmp.value( "day" ).toInt();
		hour   = tmp.value( "hour" ).toInt();
		minute = tmp.value( "minute" ).toInt();
		season = tmp.value( "season" ).toInt();
		year   = tmp.value( "year" ).toInt();

		nextSunrise = tmp.value( "nextSunrise" ).toInt();
		sunrise     = tmp.value( "sunrise" ).toInt();
		sunset      = tmp.value( "sunset" ).toInt();

		dayChanged    = tmp.value( "dayChanged" ).toBool();
		hourChanged   = tmp.value( "hourChanged" ).toBool();
		minuteChanged = tmp.value( "minuteChanged" ).toBool();
		seasonChanged = tmp.value( "seasonChanged" ).toBool();

		seasonString = tmp.value( "seasonString" ).toString();
	}

	groundLevel = tmp.value( "groundLevel" ).toInt();
	kingdomName = tmp.value( "kingdomName" ).toString();

	peaceful = tmp.value( "peaceful" ).toBool();

	tick = tmp.value( "tick" ).value<quint64>();

	numAnimals   = tmp.value( "numAnimals" ).toInt();
	numGnomes    = tmp.value( "numGnomes" ).toInt();
	oceanSize    = tmp.value( "oceanSize" ).toInt();
	plantDensity = tmp.value( "plantDensity" ).toInt();
	treeDensity  = tmp.value( "treeDensity" ).toInt();
	riverSize    = tmp.value( "riverSize" ).toInt();
	rivers       = tmp.value( "rivers" ).toInt();
	seed         = tmp.value( "seed" ).toString();

	startingItems = tmp.value( "startingItems" ).toList();

	startingZone = tmp.value( "startingZone" ).toInt();

	nextID = tmp.value( "nextID" ).toUInt();

	maxAnimalsPerType = tmp.value( "maxAnimalsPerType" ).toInt();
	allowedAnimals    = tmp.value( "allowedAnimals" ).toMap();
	allowedPlants     = tmp.value( "allowedPlants" ).toMap();
	allowedTrees      = tmp.value( "allowedTrees" ).toMap();

	addedMaterials    = tmp.value( "addedMaterials" ).toList();
	addedTranslations = tmp.value( "addedTranslations" ).toMap();

	moveX = tmp.value( "moveX" ).toInt();
	moveY = tmp.value( "moveY" ).toInt();
	scale = qMax( tmp.value( "scale" ).toFloat(), 1.0f );
	viewLevel = qMax( tmp.value( "viewLevel" ).toInt(), 100 );

	GameState::watchedItemList.clear();
	auto qwil = tmp.value( "watchedItems" ).toList();
	for( auto qval : qwil )
	{
		QVariantMap qwi = qval.toMap();
		GuiWatchedItem gwi{ qwi.value( "category" ).toString(), qwi.value( "group" ).toString(), qwi.value( "item" ).toString(), qwi.value( "material" ).toString() };
		GameState::watchedItemList.append( gwi );
	}

	itemSID2ID.clear();
	itemID2SID.clear();
	QVariantMap viIDs= tmp.value( "items2ids" ).toMap();

	if( viIDs.isEmpty() )
	{
		for( auto id : DB::ids( "Items" ) )
		{
			int rowid = DBH::rowID( "Items", id );
			itemID2SID.insert( rowid, id );
			itemSID2ID.insert( id, rowid );
		}
	}
	else
	{
		for( auto key : viIDs.keys() )
		{
			itemID2SID.insert( viIDs.value( key ).toInt(), key );
			itemSID2ID.insert( key, viIDs.value( key ).toInt() );
		}
	}

	materialSID2ID.clear();
	materialID2SID.clear();
	QVariantMap vmIDs= tmp.value( "mats2ids" ).toMap();

	if( vmIDs.isEmpty() )
	{
		for( auto id : DB::ids( "Materials" ) )
		{
			int rowid = DBH::rowID( "Materials", id );
			materialID2SID.insert( rowid, id );
			materialSID2ID.insert( id, rowid );
		}
	}
	else
	{
		for( auto key : vmIDs.keys() )
		{
			materialID2SID.insert( vmIDs.value( key ).toInt(), key );
			materialSID2ID.insert( key, vmIDs.value( key ).toInt() );
		}
	}
}

unsigned int GameState::createID()
{
	return nextID++;
}
