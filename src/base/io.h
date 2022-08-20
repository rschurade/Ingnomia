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

#include <filesystem>

namespace fs = std::filesystem;

#include <nlohmann/json.hpp>

using json = nlohmann::json;

class Game;

class IO : public QObject
{
	Q_OBJECT

private:
	QPointer<Game> g;

public:
	IO( Game* g, QObject* parent );
	~IO() noexcept override;

	static fs::path getDataFolder();
	static bool createFolders();
	static bool saveConfig();
	static bool loadOriginalConfig( json& jd );

	bool saveGameExists();

	std::string save( bool autosave = false );
	bool load( const fs::path& folder );

	void sanitize();

	static bool saveCompatible( const fs::path& folder );
	static std::string versionString( const fs::path& folder );
	static int versionInt( const fs::path& folder );

	static bool saveFile( const fs::path& url, const QJsonDocument& jd );
	static bool saveFile( const fs::path& url, const QJsonArray& ja );
	static bool saveFile( const fs::path& url, const QJsonObject& jo );
	static bool saveFile( const fs::path& url, const json& jo );
	static bool loadFile( const fs::path& url, QJsonDocument& ja );
	static bool loadFile( const fs::path& url, json& ja );

	bool saveWorld( const fs::path& folder );
	bool loadWorld( const fs::path& folder );
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
	bool loadMonsters( const fs::path& folder );
	bool loadPlants( const fs::path& folder );
	bool loadItems( const fs::path& folder );
	bool loadItemHistory( QJsonDocument& jd );
	bool loadJobs( QJsonDocument& jd );
	bool loadJobSprites( QJsonDocument& jd );
	bool loadFarms( QJsonDocument& jd );
	bool loadStockpiles( QJsonDocument& jd );
	bool loadAnimals( const fs::path& folder );
	bool loadWorkshops( QJsonDocument& jd );
	bool loadRooms( QJsonDocument& jd );
	bool loadDoors( QJsonDocument& jd );
	bool loadEvents( QJsonDocument& jd );
	bool loadMechanisms( QJsonDocument& jd );
	bool loadPipes( QJsonDocument& jd );

	bool saveItems( const fs::path& folder );
	bool savePlants( const fs::path& folder );
	bool saveAnimals( const fs::path& folder );
	bool saveMonsters( const fs::path& folder );

	int version = 0;

	fs::path getTempFolder();

public: // signals:
	sigslot::signal< const QString & /*text*/> signalStatus;
};
