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

#include "../gui/eventconnector.h"

#include <QDebug>

QVariantMap Event::serialize()
{
	QVariantMap out;

	out.insert( "ID", id );
	out.insert( "Tick", tick );
	out.insert( "Type", (int)type );
	out.insert( "Data", data );

	return out;
}

void Event::deserialize( QVariantMap in )
{
	id   = in.value( "ID" ).toUInt();
	tick = in.value( "Tick" ).value<quint64>();
	type = (EventType)in.value( "Type" ).toInt();
	data = in.value( "Data" );
}

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

EventManager::~EventManager()
{
}

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
	Position location;
	if ( data.contains( "Location" ) )
	{
		location = Position( data.value( "Location" ) );
	}
	else 
	{
		location = getEventLocation( data );
	}
	int amount        = data.value( "Amount" ).toInt();

	switch ( event.type )
	{
		case EventType::MIGRATION:
		{
			spawnGnome( location, amount );
			emit signalCenterCamera( location.toString(), 4 );
			return;
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
				qDebug() << "Spawn trader, marketStall = 0";
			}
		}
		break;
		case EventType::INVASION:
		{
			spawnInvasion( location, amount, data );
			emit signalCenterCamera( location.toString(), 4 );
			return;
		}
		break;
	}
}

void EventManager::spawnGnome( Position location, int amount )
{
	for ( int i = 0; i < amount; ++i )
	{
		g->m_gnomeManager->addGnome( location );
	}
}

void EventManager::spawnInvasion( Position location, int amount, QVariantMap data )
{
	QString type = data.value( "Species" ).toString();
	for ( int i = 0; i < amount; ++i )
	{
		g->m_creatureManager->addCreature( CreatureType::MONSTER, type, location, Gender::MALE, true, false, 1 );
	}
}

void EventManager::spawnTrader( Position location, unsigned int marketStall, QVariantMap data )
{
	auto ws = g->m_workshopManager->workshop( marketStall );
	// spawn trader"
	srand( std::chrono::system_clock::now().time_since_epoch().count() );
	QString type;
	KingdomEconomy econ = (KingdomEconomy)data.value( "KingdomEconomy" ).toInt();

	location = getEventLocation( data );

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
				getEventLocation( eventMap );
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

Position EventManager::getEventLocation( QVariantMap& eventMap )
{
	auto im = eventMap.value( "Init" ).toMap();

	QString locationString = im.value( "Location" ).toString();

	Position location;

	if ( locationString == "RandomBorderTile" )
	{
		int tries  = 0;
		bool found = false;
		while ( tries < 20 )
		{
			location = Global::util->borderPos( found );
			if ( found )
			{
				break;
			}
		}
		++tries;
	}
	else 
	{
		int tries  = 0;
		bool found = false;
		while ( tries < 20 )
		{
			location = Global::util->borderPos( found );
			if ( found )
			{
				break;
			}
		}
		++tries;
	}
	eventMap.insert( "Location", location.toString() );
	return location;
}

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
	auto location = getEventLocation( em );
	e.data        = em;
	m_eventList.append( e );

	if ( location.isZero() )
	{
		if ( em.contains( "OnFailure" ) )
		{
			QString msg   = em.value( "OnFailure" ).toMap().value( "Message" ).toString();
			QString title = em.value( "OnFailure" ).toMap().value( "Title" ).toString();
			if ( !msg.isEmpty() )
			{
				Global::eventConnector->onEvent( GameState::createID(), title, msg, false, false );
			}
		}
		m_eventList.removeLast();
		return;
	}

	if ( checkRequirements( e ) )
	{
		QString msg = em.value( "OnSuccess" ).toMap().value( "Message" ).toString();
		if ( !msg.isEmpty() )
		{
			int amount2 = em.value( "Amount" ).toInt();
			msg.replace( "$Num", QString::number( amount2 ) );
			QString title = em.value( "OnSuccess" ).toMap().value( "Title" ).toString();
			Global::eventConnector->onEvent( 0, title, msg, true, false );
		}
		executeEvent( e );
		m_eventList.removeLast();
	}
}

QList<Mission>& EventManager::missions()
{
	for( auto& mission : m_missions )
	{
		mission.time = ( GameState::tick - mission.startTick ) / ( Global::util->ticksPerMinute * 60 );
	}

	return m_missions;
}

void EventManager::addMission( Mission mission )
{
	m_missions.append( mission );
}

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