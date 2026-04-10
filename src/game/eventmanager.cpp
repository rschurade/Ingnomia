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
/** @file eventmanager.cpp
 *  @brief Timed game events: invasions, trader arrivals, migrations, and missions.
 */
#include "eventmanager.h"
#include "game.h"

#include "../base/config.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/pathfinder.h"
#include "../base/util.h"
#include "../game/gnome.h"
#include "../game/creaturemanager.h"
#include "../game/gnomemanager.h"
#include "../game/workshop.h"
#include "../game/workshopmanager.h"
#include "../game/world.h"

#include "../gui/eventconnector.h"

#include <QDebug>

/** @brief Serializes this Event into a QVariantMap for save/load.
 *  @return QVariantMap containing the event ID, tick, type, and data.
 */
QVariantMap Event::serialize()
{
	QVariantMap out;

	out.insert( "ID", id );
	out.insert( "Tick", tick );
	out.insert( "Type", (int)type );
	out.insert( "Data", data );

	return out;
}

/** @brief Deserializes an Event from a QVariantMap.
 *  @param in The map containing serialized event data.
 */
void Event::deserialize( QVariantMap in )
{
	id   = in.value( "ID" ).toUInt();
	tick = in.value( "Tick" ).value<quint64>();
	type = (EventType)in.value( "Type" ).toInt();
	data = in.value( "Data" );
}

/** @brief Serializes this Mission into a QVariantMap for save/load.
 *  @return QVariantMap containing mission ID, type, action, step, targets, gnomes, and timing.
 */
QVariantMap Mission::serialize()
{
	QVariantMap out;

	out.insert( "ID", id );
	out.insert( "Type", (int)type );
	out.insert( "Action", (int)action );
	out.insert( "Step", (int)step );
	out.insert( "Target", target );
	out.insert( "Distance", distance );
	out.insert( "Gnomes", Global::util->uintList2Variant( gnomes ) );
	out.insert( "StartTick", startTick );
	out.insert( "NextCheckTick", nextCheckTick );

	out.insert( "LeavePos", leavePos.toString() );
	out.insert( "Result", result );

	return out;
}

/** @brief Constructs a Mission by deserializing from a QVariantMap.
 *  @param in The map containing serialized mission data.
 */
Mission::Mission( QVariantMap in )
{
	id            = in.value( "ID" ).toUInt();
	type          = (MissionType)in.value( "Type" ).toInt();
	action        = (MissionAction)in.value( "Action" ).toInt();
	step          = (MissionStep)in.value( "Step" ).toInt();
	target        = in.value( "Target" ).toUInt();
	distance      = in.value( "Distance" ).toUInt();
	gnomes        = Global::util->variantList2UInt( in.value( "Gnomes" ).toList() );
	startTick     = in.value( "StartTick" ).value<quint64>();
	nextCheckTick = in.value( "NextCheckTick" ).value<quint64>();
	leavePos      = Position( in.value( "LeavePos" ) );
	result        = in.value( "Result" ).toMap();
}

/** @brief Constructs the EventManager and initializes event type and requirement lookup maps.
 *  @param parent The parent Game instance.
 */
EventManager::EventManager( Game* parent ) :
	g( parent ),
	QObject( parent )
{
	m_string2type.insert( "EventTrader", (int)EventType::TRADER );
	m_string2type.insert( "EventMigration", (int)EventType::MIGRATION );
	m_string2type.insert( "EventInvasion", (int)EventType::INVASION );

	m_reqMap.insert( "None", (int)EventRequire::NOREQUIRE );
	m_reqMap.insert( "Query", (int)EventRequire::QUERY );
	m_reqMap.insert( "FreeMarketStall", (int)EventRequire::FREEMARKETSTALL );
}

/** @brief Destructor. */
EventManager::~EventManager()
{
}

/** @brief Serializes all pending events and active missions.
 *  @return QVariantMap containing the event list and missions list.
 */
QVariantMap EventManager::serialize()
{
	QVariantMap out;
	QVariantList vel; // variant event list
	for ( auto entry : m_eventList )
	{
		vel.append( entry.serialize() );
	}
	out.insert( "EventList", vel );

	QVariantList ml;
	for ( auto mission : m_missions )
	{
		ml.append( mission.serialize() );
	}
	out.insert( "Missions", ml );

	return out;
}

/** @brief Deserializes events and missions from a saved QVariantMap.
 *  @param in The map containing serialized event and mission data.
 */
void EventManager::deserialize( QVariantMap in )
{
	m_eventList.clear();
	m_missions.clear();
	auto vel = in.value( "EventList" ).toList();
	for ( auto ve : vel )
	{
		Event event;
		event.deserialize( ve.toMap() );
		m_eventList.append( event );
	}

	auto vml = in.value( "Missions" ).toList();
	for ( auto vm : vml )
	{
		Mission m( vm.toMap() );
		m_missions.append( m );
	}
}

/** @brief Per-tick update: schedules migration events on season change, checks and fires pending events.
 *  @param tickNumber Current game tick.
 *  @param seasonChanged True if the season just changed.
 *  @param dayChanged True if the day just changed.
 *  @param hourChanged True if the hour just changed.
 *  @param minuteChanged True if the minute just changed.
 */
void EventManager::onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	if ( hourChanged )
	{
		for ( auto mission : m_missions )
		{
			emit signalUpdateMission( mission );
		}
	}

	if ( seasonChanged )
	{
		auto ev = createEvent( "EventMigration" );
		srand( std::chrono::system_clock::now().time_since_epoch().count() );
		ev.tick = GameState::tick + Global::util->ticksPerDayRandomized( 50 );
		m_eventList.append( ev );
	}

	for ( auto it = m_eventList.begin(); it != m_eventList.end(); )
	{
		auto& event = *it;
		if ( GameState::tick >= event.tick && checkRequirements( event ) )
		{
			auto em     = event.data.toMap();
			QString msg = em.value( "OnSuccess" ).toMap().value( "Message" ).toString();
			if ( !msg.isEmpty() )
			{
				int amount = em.value( "Amount" ).toInt();
				msg.replace( "$Num", QString::number( amount ) );
				QString title = em.value( "OnSuccess" ).toMap().value( "Title" ).toString();
				Global::eventConnector->onEvent( 0, title, msg, true, false );
			}
			executeEvent( event );
			it = m_eventList.erase( it );
		}
		else
		{
			++it;
		}
	}
}

/** @brief Creates a new Event from the database definition.
 *  @param eventID The string identifier of the event type in the DB (e.g. "EventMigration").
 *  @return The newly created Event with randomized amount.
 */
Event EventManager::createEvent( QString eventID )
{
	Event ev;
	ev.id   = GameState::createID();
	ev.tick = GameState::tick;
	ev.type = (EventType)m_string2type.value( eventID );

	QVariantMap em = DB::selectRow( "Events", eventID );

	auto im = DB::selectRow( "Events_Init", eventID );
	em.insert( "Init", im );

	auto sm = DB::selectRow( "Events_OnSuccess", eventID );
	auto fm = DB::selectRow( "Events_OnFailure", eventID );
	em.insert( "OnSuccess", sm );
	em.insert( "OnFailure", fm );

	int min = im.value( "Min" ).toInt();
	int max = im.value( "Max" ).toInt();
	int num = min;
	if ( min != max )
	{
		srand( std::chrono::system_clock::now().time_since_epoch().count() );
		num = qMax( min, rand() % max + 1 );
	}
	em.insert( "Amount", num );

	ev.data = em;

	return ev;
}

/** @brief Checks whether an event's prerequisites are met (e.g. free market stall, user query).
 *  @param event The event to check.
 *  @return True if requirements are satisfied and the event can execute.
 */
bool EventManager::checkRequirements( Event& event )
{
	auto data = event.data.toMap();
	int num   = data.value( "Amount" ).toInt();

	auto im = data.value( "Init" ).toMap();
	switch ( (EventRequire)m_reqMap.value( im.value( "Require" ).toString() ) )
	{
		case EventRequire::NOREQUIRE:
			return true;
		case EventRequire::QUERY:
		{
			QString msg = im.value( "Message" ).toString();

			if ( !msg.isEmpty() )
			{
				msg.replace( "$Num", QString::number( num ) );

				Global::eventConnector->onEvent( event.id, im.value( "Title" ).toString(), msg, im.value( "Pause" ).toBool(), true );
			}

			if ( data.contains( "Expires" ) )
			{
				auto expireMap = data.value( "Expires" ).toMap();
				auto val       = expireMap.value( "After" ).toInt();
				QString unit   = expireMap.value( "Unit" ).toString();
				if ( unit == "Day" )
				{
					data.insert( "ExpireTick", GameState::tick + val * Global::util->ticksPerDay );
				}
				else if ( unit == "Ticks" )
				{
					data.insert( "ExpireTick", GameState::tick + val );
				}
			}
			return false;
		}
		case EventRequire::FREEMARKETSTALL:
		{
			unsigned int marketStall = 0;
			for ( auto ws : g->m_workshopManager->workshops() )
			{
				if ( ws->type() == "MarketStall" )
				{
					if ( ws->assignedGnome() == 0 )
					{
						marketStall = ws->id();
						break;
					}
				}
			}
			if ( marketStall )
			{
				data.insert( "MarketStall", marketStall );
				event.data = data;
				return true;
			}
			return false;
		}
	}
	return false;
}

/** @brief Executes an event by spawning gnomes, traders, or invaders at the event location.
 *  @param event The event to execute.
 */
void EventManager::executeEvent( Event& event )
{
	//qDebug() << "execute event";
	auto data = event.data.toMap();
	/*
	for( auto key : data.keys() )
	{
		qDebug() << key << data[key];
	}
	*/
	Position location = getEventLocation( data );
	int amount        = data.value( "Amount" ).toInt();

	switch ( event.type )
	{
		case EventType::MIGRATION:
		{
			spawnGnome( location, amount );
		}
		break;
		case EventType::TRADER:
		{
			auto marketStall = data.value( "MarketStall" ).toUInt();
			if ( marketStall )
			{
				spawnTrader( location, marketStall, data );
			}
			else
			{
				qDebug() << "Spawn trader failed, marketStall = 0";
				return;
			}
		}
		break;
		case EventType::INVASION:
		{
			spawnInvasion( location, amount, data );
		}
		break;
	}
	emit signalCenterCamera( location );
}

/** @brief Spawns a number of new gnomes at the given location (migration event).
 *  @param location The world position to spawn gnomes at.
 *  @param amount Number of gnomes to spawn.
 */
void EventManager::spawnGnome( Position location, int amount )
{
	for ( int i = 0; i < amount; ++i )
	{
		g->m_gnomeManager->addGnome( location );
	}
}

/** @brief Spawns hostile creatures at the given location (invasion event).
 *  @param location The world position to spawn monsters at.
 *  @param amount Number of monsters to spawn.
 *  @param data Event data containing the species type.
 */
void EventManager::spawnInvasion( Position location, int amount, QVariantMap data )
{
	QString type = data.value( "Species" ).toString();
	for ( int i = 0; i < amount; ++i )
	{
		g->m_creatureManager->addCreature( CreatureType::MONSTER, type, location, Gender::MALE, true, false, 1 );
	}
}

/** @brief Spawns a trader gnome at the given location and assigns it to a market stall.
 *  @param location The world position to spawn the trader at.
 *  @param marketStall The ID of the market stall workshop to assign.
 *  @param data Event data containing the kingdom economy type for trader selection.
 */
void EventManager::spawnTrader( Position location, unsigned int marketStall, QVariantMap data )
{
	auto ws = g->m_workshopManager->workshop( marketStall );
	// spawn trader"
	QString type;
	KingdomEconomy econ = (KingdomEconomy)data.value( "KingdomEconomy" ).toInt();

	switch ( econ )
	{
		case KingdomEconomy::FARMING:
			type = "SeedTrader";
			break;
		case KingdomEconomy::ANIMALBREEDING:
			type = "AnimalTrader";
			break;
		case KingdomEconomy::MINING:
			type = "OreTrader";
			break;
		case KingdomEconomy::LOGGING:
			type = "WoodTrader";
			break;
		case KingdomEconomy::TRADING:
		default:
		{
			auto types = DB::ids( "Traders" );
			if ( types.size() )
			{
				type = types[rand() % types.size()];
			}
		}
		break;
	}
	unsigned int id = g->m_gnomeManager->addTrader( location, ws->id(), type );
	ws->assignGnome( id );
}

/** @brief Handles the player's response to a query event (accept/decline).
 *  @param id The event ID being answered.
 *  @param answer True if accepted, false if declined.
 */
void EventManager::onAnswer( unsigned int id, bool answer )
{
	for ( int i = 0; i < m_eventList.size(); ++i )
	{
		Event e = m_eventList[i];
		if ( e.id == id )
		{
			if ( answer )
			{
				auto eventMap = e.data.toMap();
				if ( eventMap.contains( "ExpireTick" ) )
				{
					quint64 expireTick = eventMap.value( "ExpireTick" ).value<quint64>();
					if ( GameState::tick > expireTick )
					{
						QString msg   = eventMap.value( "Expires" ).toMap().value( "Message" ).toString();
						QString title = eventMap.value( "Expires" ).toMap().value( "Title" ).toString();
						Global::eventConnector->onEvent( 0, title, msg, false, false );
						m_eventList.removeAt( i );
						return;
					}
				}
				e.data = eventMap;
				m_eventList.removeAt( i );
				executeEvent( e );
				break;
			}
			else
			{
				m_eventList.removeAt( i );
				break;
			}
		}
	}
}

/** @brief Determines the spawn location for an event, typically a random walkable border tile.
 *  @param eventMap The event data map containing location configuration.
 *  @return The world position where the event should occur.
 */
Position EventManager::getEventLocation( QVariantMap& eventMap )
{
	auto im = eventMap.value( "Init" ).toMap();

	QString locationString = im.value( "Location" ).toString();

	Position location;

	if ( locationString == "RandomBorderTile")
	{
		bool found = false;
		location = Global::util->borderPos( found );
		if (!found)
		{
			//TODO Use a different spawn location if no edge position was valid
			location = Position();
		}
	}
	else
	{
		location = Position(locationString);
	}
	if ( !g->w()->isWalkableGnome(location))
	{
		bool found = false;
		location = Global::util->borderPos( found );
		if (!found)
		{
			//TODO Use a different spawn location if no edge position was valid
			location = Position();
		}
	}
	return location;
}

/** @brief Creates and queues a debug event (migration, trader, or invasion) for testing.
 *  @param type The type of event to trigger.
 *  @param args Additional arguments such as amount and species type.
 */
void EventManager::onDebugEvent( EventType type, QVariantMap args )
{
	QString eventID;
	switch ( type )
	{
		case EventType::MIGRATION:
		{
			eventID = "EventMigration";
			break;
		}
		case EventType::TRADER:
		{
			{
			
				NeighborKingdom k;
				k.distance = 0;
				k.economy = KingdomEconomy::ANIMALBREEDING;
				k.nextTrader = GameState::tick;
				k.wealth = KingdomWealth::VERYRICH;
				k.type = KingdomType::GNOME;

				eventID = "EventTrader";

				addTraderEvent( k );

				return;
			}
			break;
		}
		case EventType::INVASION:
		{
			eventID = "EventInvasion";
			break;
		}
	}
	Event e = createEvent( eventID );
	auto em    = e.data.toMap();
	int amount = args.value( "Amount" ).toInt();
	if ( amount > 0 )
	{
		em.insert( "Amount", amount );
	}
	em.insert( "Species", args.value( "Type" ).toString() );
	e.data        = em;
	m_eventList.append( e );
}

/** @brief Returns a reference to the list of active missions, updating elapsed time for each.
 *  @return Reference to the missions list.
 */
QList<Mission>& EventManager::missions()
{
	for( auto& mission : m_missions )
	{
		mission.time = ( GameState::tick - mission.startTick ) / ( Global::util->ticksPerMinute * 60 );
	}

	return m_missions;
}

/** @brief Adds a mission to the active missions list.
 *  @param mission The mission to add.
 */
void EventManager::addMission( Mission mission )
{
	m_missions.append( mission );
}

/** @brief Retrieves a pointer to a mission by its ID.
 *  @param id The mission ID to look up.
 *  @return Pointer to the mission, or nullptr if not found.
 */
Mission* EventManager::getMission( unsigned int id )
{
	for ( auto& mission : m_missions )
	{
		if ( mission.id == id )
		{
			mission.time = ( GameState::tick - mission.startTick ) / ( Global::util->ticksPerMinute * 60 );
			return &mission;
		}
	}
	return nullptr;
}

/** @brief Removes a completed mission from the active missions list.
 *  @param id The mission ID to finish and remove.
 */
void EventManager::finishMission( unsigned int id )
{
	for ( int i = 0; i < m_missions.size(); ++i )
	{
		if ( m_missions[i].id == id )
		{
			m_missions.removeAt( i );
			break;
		}
	}
}

/** @brief Schedules a trader arrival event based on a neighbor kingdom's distance and economy.
 *  @param kingdom The neighbor kingdom sending the trader.
 */
void EventManager::addTraderEvent( NeighborKingdom kingdom )
{
	quint64 leaveTick = kingdom.nextTrader;
	quint64 tick      = leaveTick + kingdom.distance * Global::util->ticksPerMinute * Global::util->minutesPerHour;

	auto e = createEvent( "EventTrader" );
	e.tick = tick;

	auto data = e.data.toMap();
	data.insert( "KingdomType", (int)kingdom.type );
	data.insert( "KingdomWealth", (int)kingdom.wealth );
	data.insert( "KingdomEconomy", (int)kingdom.economy );

	e.data = data;

	m_eventList.append( e );
}

/** @brief Schedules a raid/invasion event from a neighbor kingdom. Skipped if peaceful mode.
 *  @param kingdom The neighbor kingdom launching the raid.
 */
void EventManager::addRaidEvent( NeighborKingdom kingdom )
{
	if ( GameState::peaceful )
	{
		return;
	}

	quint64 leaveTick = GameState::tick;
	quint64 tick      = leaveTick + kingdom.distance * Global::util->ticksPerMinute * Global::util->minutesPerHour;

	auto e = createEvent( "EventInvasion" );
	e.tick = tick;

	auto data = e.data.toMap();
	data.insert( "KingdomType", (int)kingdom.type );
	data.insert( "KingdomWealth", (int)kingdom.wealth );
	data.insert( "KingdomMilitary", (int)kingdom.military );
	data.insert( "Amount", GameState::year + 1 ); //TODO amount depending on factors
	data.insert( "Species", "Goblin" );
	e.data = data;

	m_eventList.append( e );
}

/** @brief Starts a new mission (spy, raid, emissary, etc.) targeting a neighbor kingdom.
 *  @param type The type of mission.
 *  @param action The specific action (e.g. improve relations, sabotage).
 *  @param targetKingdom The ID of the target neighbor kingdom.
 *  @param gnomeID The ID of the gnome assigned to the mission.
 */
void EventManager::startMission( MissionType type, MissionAction action, unsigned int targetKingdom, unsigned int gnomeID )
{
	Mission mission;

	mission.id = GameState::createID();

	mission.type = type;
	mission.step = MissionStep::LEAVE_MAP;
	mission.target = targetKingdom;
	mission.action = action;
	mission.distance = g->m_neighborManager->distance( mission.target );
	mission.gnomes = { gnomeID };
	mission.startTick = GameState::tick;
	mission.nextCheckTick = GameState::tick + Global::util->ticksPerDay;

	addMission( mission );

	g->m_gnomeManager->setInMission( gnomeID, mission.id );
}
