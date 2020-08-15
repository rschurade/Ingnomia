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

class IO : public QObject
{
	Q_OBJECT

public:
	IO( QObject* parent = nullptr );
	~IO();

	static bool createFolders();
	static bool saveConfig();

	static bool saveGameExists();

	static QString save( bool autosave = false );
	bool load( QString folder );

	void sanitize();

	static bool saveCompatible( QString folder );
	static QString versionString( QString folder );
	static int versionInt( QString folder );

	static bool saveFile( QString url, const QJsonDocument& jd );
	static bool saveFile( QString url, const QJsonArray& ja );
	static bool saveFile( QString url, const QJsonObject& jo );
	static bool loadFile( QString url, QJsonDocument& ja );

	static bool saveWorld( QString folder );
	static bool loadWorld( QString folder );
	static void loadWorld( QDataStream& in );

	static QJsonArray jsonArraySprites();
	static QJsonArray jsonArrayGame();
	static QJsonArray jsonArrayConfig();
	static QJsonArray jsonArrayWallConstructions();
	static QJsonArray jsonArrayFloorConstructions();
	static QJsonArray jsonArrayGnomes();


	static QJsonArray jsonArrayMonsters( int startindex, int amount );
	static QJsonArray jsonArrayAnimals( int startindex, int amount );
	
	static QJsonArray jsonArrayPlants( int startindex, int amount );
	static QJsonArray jsonArrayItems( int startindex, int amount );
	static QJsonArray jsonArrayJobs();
	static QJsonArray jsonArrayJobSprites();
	static QJsonArray jsonArrayFarms();
	static QJsonArray jsonArrayRooms();
	static QJsonArray jsonArrayDoors();
	static QJsonArray jsonArrayStockpiles();
	static QJsonArray jsonArrayWorkshops();
	static QJsonDocument jsonArrayItemHistory();
	static QJsonDocument jsonArrayEvents();
	static QJsonDocument jsonArrayMechanisms();
	static QJsonDocument jsonArrayPipes();

	static bool loadSprites( QJsonDocument& jd );
	static bool loadGame( QJsonDocument& jd );
	static bool loadConfig( QJsonDocument& jd );
	static bool loadFloorConstructions( QJsonDocument& jd );
	static bool loadWallConstructions( QJsonDocument& jd );
	static bool loadGnomes( QJsonDocument& jd );
	static bool loadMonsters( QString folder );
	static bool loadPlants( QString folder );
	static bool loadItems( QString folder );
	static bool loadItemHistory( QJsonDocument& jd );
	static bool loadJobs( QJsonDocument& jd );
	static bool loadJobSprites( QJsonDocument& jd );
	static bool loadFarms( QJsonDocument& jd );
	static bool loadStockpiles( QJsonDocument& jd );
	static bool loadAnimals( QString folder );
	static bool loadWorkshops( QJsonDocument& jd );
	static bool loadRooms( QJsonDocument& jd );
	static bool loadDoors( QJsonDocument& jd );
	static bool loadEvents( QJsonDocument& jd );
	static bool loadMechanisms( QJsonDocument& jd );
	static bool loadPipes( QJsonDocument& jd );

	static bool saveItems( QString folder );
	static bool savePlants( QString folder );
	static bool saveAnimals( QString folder );
	static bool saveMonsters( QString folder );

	static int version;

	static QString getTempFolder();

signals:
	void signalStatus( QString text );
};
