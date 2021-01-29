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

#include "../base/enums.h"

#include <QObject>

class Config;
class NewGameSettings;

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
class SoundManager;

class PathFinder;
class SpriteFactory;
class World;

class Game : public QObject
{
	friend class Inventory;
	friend class SpriteFactory;
	friend class World;

	friend class JobManager;
	friend class StockpileManager;
	friend class FarmingManager;
	friend class WorkshopManager;
	friend class RoomManager;
	friend class GnomeManager;
	friend class CreatureManager;
	friend class EventManager;
	friend class MechanismManager;
	friend class FluidManager;
	friend class NeighborManager;
	friend class MilitaryManager;
	friend class SoundManager;

	friend class Gnome;
	friend class GnomeTrader;
	friend class Automaton;

	Q_OBJECT
	Q_DISABLE_COPY_MOVE( Game )
public:
	Game( QObject* parent );
	Game() = delete;
	virtual ~Game();

	void save();

	GameSpeed gameSpeed();
	void setGameSpeed( GameSpeed speed );

	bool paused();
	void setPaused( bool value );

	void generateWorld( NewGameSettings* ngs );
	void setWorld( int dimX, int dimY, int dimZ );
	World* world();

	Inventory* inv();
	ItemHistory* ih();
	JobManager* jm();
	StockpileManager* spm();
	FarmingManager* fm();
	WorkshopManager* wsm();
	World* w();
	SpriteFactory* sf();
	RoomManager* rm();
	GnomeManager* gm();
	CreatureManager* cm();
	EventManager* em();
	MechanismManager* mcm();
	FluidManager* flm();
	NeighborManager* nm();
	MilitaryManager* mil();
	PathFinder* pf();
	SoundManager* sm();

private:
	QScopedPointer<World> m_world;
	QScopedPointer<SpriteFactory> m_sf;
	QScopedPointer<PathFinder> m_pf;

	QPointer<QTimer> m_timer;

	int m_millisecondsSlow = 50;
	int m_millisecondsFast = 5;

	int m_maxLoopTime = 0;

	void processPlants();

	void sendClock();
	void calcDaylight();
	int timeToInt( QString time );
	QString intToTime( int time );

	void autoSave();

	bool m_paused         = true;
	GameSpeed m_gameSpeed = GameSpeed::Normal;

	QPointer<Inventory> m_inv;

	QPointer<JobManager> m_jobManager;
	QPointer<StockpileManager> m_spm;
	QPointer<FarmingManager> m_farmingManager;
	QPointer<WorkshopManager> m_workshopManager;
	QPointer<RoomManager> m_roomManager;
	QPointer<GnomeManager> m_gnomeManager;
	QPointer<CreatureManager> m_creatureManager;
	QPointer<EventManager> m_eventManager;
	QPointer<MechanismManager> m_mechanismManager;
	QPointer<FluidManager> m_fluidManager;
	QPointer<NeighborManager> m_neighborManager;
	QPointer<MilitaryManager> m_militaryManager;
	QPointer<SoundManager> m_soundManager;

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
