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
/** @file neighbormanager.cpp
 *  @brief Neighboring kingdom management: procedural generation, diplomacy, raids, trading, and off-map missions.
 */
#include "neighbormanager.h"
#include "game.h"

#include "../base/gamestate.h"
#include "../base/util.h"
#include "../game/eventmanager.h"
#include "../gui/strings.h"

#include <QDebug>

/// @brief Constructs the neighbor manager and populates neighboring kingdoms.
///        In peaceful mode: 10 gnome kingdoms. Otherwise: 5 gnome + 5 goblin kingdoms.
/// @param parent Owning Game instance.
NeighborManager::NeighborManager( Game* parent ) :
	g( parent ),
	QObject( parent )
{
	m_kingdoms.clear();

	if ( GameState::peaceful )
	{
		for ( int i = 0; i < 10; ++i )
		{
			addRandomKingdom( KingdomType::GNOME );
		}
	}
	else
	{
		for ( int i = 0; i < 5; ++i )
		{
			addRandomKingdom( KingdomType::GNOME );
		}

		for ( int i = 0; i < 5; ++i )
		{
			addRandomKingdom( KingdomType::GOBLIN );
		}
	}
}

/// @brief Destructor.
NeighborManager::~NeighborManager()
{
}

/// @brief Serialises this kingdom's state into a QVariantMap.
/// @return Map with keys ID, Discovered, DiscoverMission, Distance, Name, Type, Attitude,
///         Wealth, Economy, Military, NextRaid, NextTrader.
QVariantMap NeighborKingdom::serialize()
{
	QVariantMap out;

	out.insert( "ID", id );
	out.insert( "Discovered", discovered );
	out.insert( "DiscoverMission", discoverMission );
	out.insert( "Distance", distance );
	out.insert( "Name", name );
	out.insert( "Type", (int)type );
	out.insert( "Attitude", attitude );
	out.insert( "Wealth", (int)wealth );
	out.insert( "Economy", (int)economy );
	out.insert( "Military", (int)military );
	out.insert( "NextRaid", nextRaid );
	out.insert( "NextTrader", nextTrader );

	return out;
}

/// @brief Restores kingdom state from a previously serialised map.
/// @param in Map produced by NeighborKingdom::serialize().
void NeighborKingdom::deserialize( QVariantMap in )
{
	id              = in.value( "ID" ).toUInt();
	discovered      = in.value( "Discovered" ).toBool();
	discoverMission = in.value( "DiscoverMission" ).toBool();
	distance        = in.value( "Distance" ).toInt();
	name            = in.value( "Name" ).toString();
	type            = (KingdomType)in.value( "Type" ).toInt();
	attitude        = in.value( "Attitude" ).toFloat();
	wealth          = (KingdomWealth)in.value( "Wealth" ).toInt();
	economy         = (KingdomEconomy)in.value( "Economy" ).toInt();
	military        = (KingdomMilitary)in.value( "Military" ).toInt();
	nextRaid        = in.value( "NextRaid" ).value<quint64>();
	nextTrader      = in.value( "NextTrader" ).value<quint64>();
}

/// @brief Serialises all kingdoms into a QVariantList.
/// @return List of QVariantMaps, one per kingdom.
QVariantList NeighborManager::serialize()
{
	QVariantList out;

	for ( auto neigh : m_kingdoms )
	{
		out.append( neigh.serialize() );
	}

	return out;
}

/// @brief Restores all kingdoms from a saved QVariantList.
/// @param in List produced by NeighborManager::serialize().
void NeighborManager::deserialize( QVariantList in )
{
	m_kingdoms.clear();
	for ( auto vk : in )
	{
		NeighborKingdom nk;
		nk.deserialize( vk.toMap() );
		m_kingdoms.append( nk );
	}
}

/// @brief Generates and appends a randomly configured neighboring kingdom of the given type.
///        Gnome kingdoms get a random positive attitude; goblin kingdoms get a negative attitude
///        and a first raid scheduled within ~10 days.
/// @param type KingdomType::GNOME or KingdomType::GOBLIN.
void NeighborManager::addRandomKingdom( KingdomType type )
{
	srand( std::chrono::system_clock::now().time_since_epoch().count() );

	NeighborKingdom nk;
	nk.id = GameState::createID();

	nk.discovered = false;
	nk.name       = S::gi().randomKingdomName();
	nk.type       = type;
	nk.distance   = rand() % 180 + 72;
	nk.wealth     = ( KingdomWealth )( rand() % ( (int)KingdomWealth::VERYRICH + 1 ) );
	nk.economy    = ( KingdomEconomy )( rand() % ( (int)KingdomEconomy::ANIMALBREEDING + 1 ) );
	nk.military   = ( KingdomMilitary )( rand() % ( (int)KingdomMilitary::VERYSTRONG + 1 ) );

	nk.attitude = 0;
	switch ( type )
	{
		case KingdomType::GNOME:
			nk.attitude = rand() % 100;
			break;
		case KingdomType::GOBLIN:
			nk.attitude = -( rand() % 60 + 40 );
			nk.nextRaid = GameState::tick + 60 * Global::util->ticksPerDayRandomized( 10 );
			break;
	}

	m_kingdoms.append( nk );
}

/// @brief Per-tick update: triggers raid events for goblin kingdoms whose nextRaid tick has passed.
/// @param tickNumber    Current game tick.
/// @param seasonChanged Unused.
/// @param dayChanged    Unused.
/// @param hourChanged   Unused.
/// @param minuteChanged Unused.
void NeighborManager::onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	for ( auto& kingdom : m_kingdoms )
	{
		if ( kingdom.type == KingdomType::GOBLIN && GameState::tick >= kingdom.nextRaid )
		{
			g->m_eventManager->addRaidEvent( kingdom );
			kingdom.nextRaid = GameState::tick + Global::util->ticksPerDayRandomized( 10 ) * Global::util->daysPerSeason * 4;
		}
	}
}

/// @brief Returns a mutable reference to the list of all neighboring kingdoms.
/// @return Reference to the kingdoms list.
QList<NeighborKingdom>& NeighborManager::kingdoms()
{
	return m_kingdoms;
}

/// @brief Returns the number of kingdoms that have been discovered by the player.
/// @return Count of discovered kingdoms.
int NeighborManager::countDiscovered()
{
	int out = 0;
	for ( auto k : m_kingdoms )
	{
		if ( k.discovered )
			++out;
	}
	return out;
}

/// @brief Marks the kingdom with the given ID as discovered.
/// @param id UID of the kingdom to reveal.
void NeighborManager::discoverKingdom( unsigned int id )
{
	for ( auto& k : m_kingdoms )
	{
		if ( k.id == id )
		{
			k.discovered = true;
			break;
		}
	}
}

/// @brief Returns the travel distance (in days) to the kingdom with the given ID.
/// @param kingdomID UID of the kingdom to query.
/// @return Distance value, or 0 if the kingdom is not found.
int NeighborManager::distance( unsigned int kingdomID )
{
	for ( auto& k : m_kingdoms )
	{
		if ( k.id == kingdomID )
		{
			return k.distance;
		}
	}
	return 0;
}

/// @brief Resolves a spy mission against the target kingdom (75% success chance).
///        On success: stores the target's nextRaid tick in mission results.
///        On failure: reduces the kingdom's attitude by 10.
/// @param mission Pointer to the mission; result map is populated in-place.
void NeighborManager::spy( Mission* mission )
{
	auto kingdomID = mission->target;
	for ( auto& k : m_kingdoms )
	{
		if ( k.id == kingdomID )
		{
			srand( std::chrono::system_clock::now().time_since_epoch().count() );
			int chance   = 75;
			bool success = ( rand() % 100 ) < chance;
			if ( success )
			{
				qDebug() << "spy success";
				mission->result.insert( "Success", true );
				mission->result.insert( "NextRaid", k.nextRaid );
			}
			else
			{
				qDebug() << "spy failure";
				mission->result.insert( "Success", false );
				k.attitude -= 10;
			}
		}
	}
}

/// @brief Resolves a sabotage mission (base 50% success, +10% per gnome beyond two).
///        On success: delays the target's next raid by 2–5 days and reduces attitude by 20.
///        On failure: records failure in mission results.
/// @param mission Pointer to the mission; result map is populated in-place.
void NeighborManager::sabotage( Mission* mission )
{
	auto kingdomID = mission->target;
	for ( auto& k : m_kingdoms )
	{
		if ( k.id == kingdomID )
		{
			srand( std::chrono::system_clock::now().time_since_epoch().count() );

			int chance = 50;
			chance += ( mission->gnomes.size() - 2 ) * 10;
			bool success = ( rand() % 100 ) < chance;

			if ( success )
			{
				qDebug() << "sabotage success";
				mission->result.insert( "Success", true );
				int delay = qMax( 2, rand() % 6 );
				mission->result.insert( "Delay", delay );

				k.nextRaid += delay * Global::util->ticksPerDay;
				k.attitude -= 20;
			}
			else
			{
				qDebug() << "sabotage failure";
				mission->result.insert( "Success", false );
			}
		}
	}
}

/// @brief Resolves a gnome raid mission against the target kingdom (base 50% success, +10% per gnome beyond two).
///        Both outcomes reduce the target kingdom's attitude (success −20, failure −10).
/// @param mission Pointer to the mission; result map is populated in-place.
void NeighborManager::raid( Mission* mission )
{
	auto kingdomID = mission->target;
	for ( auto& k : m_kingdoms )
	{
		if ( k.id == kingdomID )
		{
			srand( std::chrono::system_clock::now().time_since_epoch().count() );

			int chance = 50;
			chance += ( mission->gnomes.size() - 2 ) * 10;
			bool success = ( rand() % 100 ) < chance;

			if ( success )
			{
				qDebug() << "raid success";
				mission->result.insert( "Success", true );

				k.attitude -= 20;
			}
			else
			{
				qDebug() << "raid failure";
				k.attitude -= 10;
				mission->result.insert( "Success", false );
			}
		}
	}
}

/// @brief Resolves an emissary mission: applies the chosen diplomatic action to the target kingdom.
///        IMPROVE: +10 attitude; INSULT: −20 attitude;
///        INVITE_TRADER: schedules a trader event; INVITE_AMBASSADOR: stub (no-op).
/// @param mission Pointer to the mission; result map is populated in-place.
void NeighborManager::emissary( Mission* mission )
{
	auto kingdomID = mission->target;
	for ( auto& k : m_kingdoms )
	{
		if ( k.id == kingdomID )
		{

			switch ( mission->action )
			{
				case MissionAction::IMPROVE:
					k.attitude += 10;
					break;
				case MissionAction::INSULT:
					k.attitude -= 20;
					break;
				case MissionAction::INVITE_TRADER:
					k.nextTrader = GameState::tick + 2 * Global::util->ticksPerDayRandomized( 50 );
					g->m_eventManager->addTraderEvent( k );
					break;
				case MissionAction::INVITE_AMBASSADOR:
					break;
			}
			mission->result.insert( "Success", true );
		}
	}
}