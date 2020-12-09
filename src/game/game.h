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
#ifndef GAME_H_
#define GAME_H_

#include <QObject>

#include "../base/enums.h"

class Config;

class QTimer;

class Inventory; 
class ItemHistory;
class JobManager;
class StockpileManager;
class FarmingManager;
class WorkshopManager;
class RoomManager;
class GnomeManager;
class CreatureManager;
class EventManager;
class MechanismManager;
class FluidManager;
class NeighborManager;
class MilitaryManager;

class Game : public QObject
{
	Q_OBJECT

public:
	Game( World* world, QObject* parent );
	virtual ~Game();

	void save();

	GameSpeed gameSpeed();
	void setGameSpeed( GameSpeed speed );

	bool paused();
	void setPaused( bool value );

	World* world() { return m_world; }

private:
	World* m_world = nullptr;
	PathFinder* m_pathFinder = nullptr;


	QTimer* m_timer = nullptr;

	int m_millisecondsSlow = 50;
	int m_millisecondsFast = 5;

	int m_maxLoopTime = 0;

	void processPlants();

	void sendClock();
	void calcDaylight();
	int timeToInt( QString time );
	QString intToTime( int time );

	void autoSave();

	bool m_paused = true;
	GameSpeed m_gameSpeed = GameSpeed::Normal;

	Inventory* m_inventory = nullptr;
	ItemHistory* m_itemHistory = nullptr;
	JobManager* m_jobManager = nullptr;
	StockpileManager* m_stockpileManager = nullptr;
	FarmingManager* m_farmingManager = nullptr;
	WorkshopManager* m_workshopManager = nullptr;
	RoomManager* m_roomManager = nullptr;
	GnomeManager* m_gnomeManager = nullptr;
	CreatureManager* m_creatureManager = nullptr;
	EventManager* m_eventManager = nullptr;
	MechanismManager* m_mechanismManager = nullptr;
	FluidManager* m_fluidManager = nullptr;
	NeighborManager* m_neighborManager = nullptr;
	MilitaryManager* m_militaryManager = nullptr;
	
	

public slots:
	void loop();
	void start();
	void stop();
	void sendTime();

signals:
	void sendOverlayMessage( int id, QString text );
	void signalTimeAndDate( int minute, int hour, int day, QString season, int year, QString sunStatus );
	void signalKingdomInfo( QString name, QString info1, QString info2, QString info3 );
	void signalPause( bool pause );
	void signalEvent( unsigned int id, QString title, QString msg, bool pause, bool yesno );
	void signalStartAutoSave();
	void signalEndAutoSave();
	void signalUpdateTileInfo( QSet<unsigned int> changeSet );
	void signalUpdateStockpile();
};

#endif /* GAME_H_ */
