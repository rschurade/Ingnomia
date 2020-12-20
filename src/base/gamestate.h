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

#ifndef GAMESTATE_H_
#define GAMESTATE_H_

#include "../base/position.h"
#include "../gui/aggregatorinventory.h"

#include <QHash>
#include <QString>
#include <QVariant>
#include <QVariantMap>

#include <initializer_list>

class GameState
{
private:
	bool m_acceptChangeSets = false;

public:
	GameState() = delete;
	~GameState() = delete;

	static unsigned int createID();

	static bool init();

	static void serialize( QVariantMap& out );
	static void load( QVariantMap& in );

	static int alarm;
	static unsigned int alarmRoomID;
	
	static bool daylight;
	static bool initialSave;
	static QVariantList itemFilter;

	static QVariantMap military; // military manager
	static QVariantList neighbors; // neighbor manager

	static Position origin;
	static QVariantList squads;

	static QVariantList stockOverlay;

	static QVariantMap techs;

	static QString version;

	static int flatness;
	
	static int day;
	static int hour;
	static int minute;
	static int season;
	static int year;

	static QString seasonString;
	static QString currentDayTime;
	static QString currentYearAndSeason;

	static bool dayChanged;
	static bool hourChanged;
	static bool minuteChanged;
	static bool seasonChanged;

	static int nextSunrise;
	static int sunrise;
	static int sunset;
	
	static int groundLevel;
	static QString kingdomName;
	
	static bool peaceful;

	static quint64 tick;

	static int numAnimals;
	static int numGnomes;
	static int oceanSize;
	static int plantDensity;
	static int treeDensity;
	static int riverSize;
	static int rivers;
	static QString seed;

	static QVariantList startingItems;

	static unsigned int startingZone;
	
	static int maxAnimalsPerType;
	static QVariantMap allowedAnimals;
	static QVariantMap allowedPlants;
	static QVariantMap allowedTrees;

	static QVariantList addedMaterials;
	static QVariantMap addedTranslations;

	static int moveX;
	static int moveY;
	static float scale;
	static int viewLevel;

	static QList<GuiWatchedItem> watchedItemList;

	static QHash<QString, int> materialSID2ID;
	static QHash<int, QString> materialID2SID;
	static QHash<QString, int> itemSID2ID;
	static QHash<int, QString> itemID2SID;

private:
	static unsigned int nextID;

};

#endif /* GAMESTATE_H_ */
