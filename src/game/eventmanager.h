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
/** @file eventmanager.h
 *  @brief Timed game event system for migrations, trader arrivals, invasions, and off-map missions.
 */
#pragma once


#include "../base/position.h"
#include "../base/priorityqueue.h"
#include "../game/neighbormanager.h"

#include <QMap>
#include <QObject>
#include <QVariantMap>

class Game;

/** @brief Prerequisite type that must be met before an event can fire. */
enum class EventRequire
{
	NOREQUIRE,
	QUERY,
	FREEMARKETSTALL
};

/** @brief Category of a scheduled game event. */
enum class EventType
{
	TRADER,
	MIGRATION,
	INVASION,
};

/** @brief A scheduled game event with a target tick, type, and associated data. */
struct Event
{
	unsigned int id = 0;
	quint64 tick    = 0;

	EventType type;

	QVariant data;

	bool operator==( const Event& other )
	{
		return tick == other.tick;
	}
	bool operator!=( const Event& other )
	{
		return tick != other.tick;
	};
	bool operator<( const Event& other )
	{
		return tick < other.tick;
	};

	QVariantMap serialize();
	void deserialize( QVariantMap in );
};

/** @brief Type of off-map mission that gnomes can undertake. */
enum class MissionType
{
	NOMISSION,
	EXPLORE,
	SPY,
	EMISSARY,
	RAID,
	SABOTAGE
};
Q_DECLARE_METATYPE( MissionType )

/** @brief Current phase of an in-progress mission. */
enum class MissionStep
{
	NONE,
	LEAVE_MAP,
	TRAVEL,
	ACTION,
	RETURN,
	RETURNED
};
Q_DECLARE_METATYPE( MissionStep )

/** @brief Specific diplomatic action performed during a mission. */
enum class MissionAction
{
	NONE,
	IMPROVE,
	INSULT,
	INVITE_TRADER,
	INVITE_AMBASSADOR
};
Q_DECLARE_METATYPE( MissionAction )

/** @brief An off-map mission sent to a neighboring kingdom, tracking gnomes, timing, and results. */
struct Mission
{
	unsigned int id = 0;
	MissionType type;
	MissionAction action;
	MissionStep step;
	unsigned int target = 0;
	int distance        = 0;
	QList<unsigned int> gnomes;
	quint64 startTick     = 0;
	quint64 nextCheckTick = 0;
	int time = 0;

	Position leavePos;
	QVariantMap result;

	Mission() {};
	Mission( QVariantMap in );

	QVariantMap serialize();
};
Q_DECLARE_METATYPE( Mission )


/** @brief Manages timed game events (migrations, traders, invasions) and off-map missions.
 *
 *  Maintains a list of pending events that fire when their tick arrives and requirements
 *  are met. Schedules migration events each season, handles trader and raid events from
 *  the neighbor system, spawns gnomes/creatures/traders, and tracks active missions.
 */
class EventManager : public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY_MOVE( EventManager )
public:
	EventManager( Game* parent );
	~EventManager();

	QVariantMap serialize();
	void deserialize( QVariantMap in );

	void onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged );

	void addTraderEvent( NeighborKingdom kingdom );
	void addRaidEvent( NeighborKingdom kingdom );

	QList<Mission>& missions();
	void addMission( Mission mission );
	Mission* getMission( unsigned int id );
	void finishMission( unsigned int id );

	void startMission( MissionType type, MissionAction action, unsigned int targetKingdom, unsigned int gnomeID );

private:
	QPointer<Game> g;

	QMap<QString, int> m_string2type;
	QMap<QString, int> m_reqMap;

	QList<Event> m_eventList;

	Event createEvent( QString eventID );

	bool checkRequirements( Event& event );

	void executeEvent( Event& event );

	void spawnGnome( Position location, int amount );
	void spawnInvasion( Position location, int amount, QVariantMap data );
	void spawnTrader( Position location, unsigned int marketStall, QVariantMap data );

	Position getEventLocation( QVariantMap& eventMap );

	QList<Mission> m_missions;

signals:
	void signalCenterCamera( const Position& location );
	void signalUpdateMission( const Mission& mission );

public slots:
	void onAnswer( unsigned int id, bool answer );

	void onDebugEvent( EventType type, QVariantMap args );
};
