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


#include "../base/position.h"
#include "../base/priorityqueue.h"
#include "../game/neighbormanager.h"

#include <QMap>
#include <QObject>
#include <QVariantMap>

class Game;

enum class EventRequire
{
	NOREQUIRE,
	QUERY,
	FREEMARKETSTALL
};

enum class EventType
{
	TRADER,
	MIGRATION,
	INVASION,
};

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

enum class MissionAction
{
	NONE,
	IMPROVE,
	INSULT,
	INVITE_TRADER,
	INVITE_AMBASSADOR
};
Q_DECLARE_METATYPE( MissionAction )

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
	void signalCenterCamera( QString pos, int zOffset );
	void signalUpdateMission( const Mission& mission );

public slots:
	void onAnswer( unsigned int id, bool answer );

	void onDebugEvent( EventType type, QVariantMap args );
};
