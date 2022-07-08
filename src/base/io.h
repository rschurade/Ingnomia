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
#include <QPointer>

#include <sigslot/signal.hpp>

class Game;

class IO : public QObject
{
	Q_OBJECT

private:
	QPointer<Game> g;

public:
	IO( Game* g, QObject* parent );
	~IO();

	// TODO: Return fs::path instead
	static std::string getDataFolder();
	static bool createFolders();
	static bool saveConfig();
	static bool loadOriginalConfig( QJsonDocument& jd );

	bool saveGameExists();

	std::string save( bool autosave = false );
	bool load( std::string folder );

	void sanitize();

	static bool saveCompatible( const std::string& folder );
	static std::string versionString( const std::string& folder );
	static int versionInt( const std::string& folder );

	static bool saveFile( const std::string& url, const QJsonDocument& jd );
	static bool saveFile( const std::string& url, const QJsonArray& ja );
	static bool saveFile( const std::string& url, const QJsonObject& jo );
	static bool loadFile( const std::string& url, QJsonDocument& ja );

	bool saveWorld( const std::string& folder );
	bool loadWorld( const std::string& folder );
	void loadWorld( QDataStream& in );

	QJsonArray jsonArraySprites();
	QJsonArray jsonArrayGame();
	QJsonArray jsonArrayConfig();
	QJsonArray jsonArrayWallConstructions();
	QJsonArray jsonArrayFloorConstructions();
	QJsonArray jsonArrayGnomes();


	QJsonArray jsonArrayMonsters( int startindex, int amount );
	QJsonArray jsonArrayAnimals( int startindex, int amount );
	
	QJsonArray jsonArrayPlants( int startindex, int amount );
	QJsonArray jsonArrayItems( int startindex, int amount );
	QJsonArray jsonArrayJobs();
	QJsonArray jsonArrayJobSprites();
	QJsonArray jsonArrayFarms();
	QJsonArray jsonArrayRooms();
	QJsonArray jsonArrayDoors();
	QJsonArray jsonArrayStockpiles();
	QJsonArray jsonArrayWorkshops();
	QJsonDocument jsonArrayItemHistory();
	QJsonDocument jsonArrayEvents();
	QJsonDocument jsonArrayMechanisms();
	QJsonDocument jsonArrayPipes();

	bool loadSprites( QJsonDocument& jd );
	bool loadGame( QJsonDocument& jd );
	bool loadConfig( QJsonDocument& jd );
	bool loadFloorConstructions( QJsonDocument& jd );
	bool loadWallConstructions( QJsonDocument& jd );
	bool loadGnomes( QJsonDocument& jd );
	bool loadMonsters( const std::string& folder );
	bool loadPlants( const std::string& folder );
	bool loadItems( const std::string& folder );
	bool loadItemHistory( QJsonDocument& jd );
	bool loadJobs( QJsonDocument& jd );
	bool loadJobSprites( QJsonDocument& jd );
	bool loadFarms( QJsonDocument& jd );
	bool loadStockpiles( QJsonDocument& jd );
	bool loadAnimals( const std::string& folder );
	bool loadWorkshops( QJsonDocument& jd );
	bool loadRooms( QJsonDocument& jd );
	bool loadDoors( QJsonDocument& jd );
	bool loadEvents( QJsonDocument& jd );
	bool loadMechanisms( QJsonDocument& jd );
	bool loadPipes( QJsonDocument& jd );

	bool saveItems( const std::string& folder );
	bool savePlants( const std::string& folder );
	bool saveAnimals( const std::string& folder );
	bool saveMonsters( const std::string& folder );

	int version = 0;

	// TODO: Return fs::path instead
	std::string getTempFolder();

public: // signals:
	sigslot::signal< const QString & /*text*/> signalStatus;
};
