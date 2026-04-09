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
/** @file creaturemanager.cpp
 *  Implementation of CreatureManager for ticking, spawning, and removing animals and monsters.
 */
#include "creaturemanager.h"
#include "game.h"

#include "../base/db.h"
#include "../base/global.h"
#include "../base/regionmap.h"
#include "../game/world.h"
#include "../game/creaturefactory.h"
#include "../game/farmingmanager.h"
#include "../game/inventory.h"
#include "../game/jobmanager.h"
#include "../game/newgamesettings.h"

#include <QDebug>
#include <QElapsedTimer>

/** @brief Constructs the creature manager.
 *  @param parent Pointer to the owning Game instance.
 */
CreatureManager::CreatureManager( Game* parent ) :
	g( parent ),
	QObject( parent )
{
}

/** @brief Destructor. Deletes all owned creature instances. */
CreatureManager::~CreatureManager()
{
	for ( const auto& c : m_creatures )
	{
		delete c;
	}
}

/** @brief Advances all creatures by one game tick. Handles death, corpse creation, and cleanup.
 *  @param tickNumber Current game tick number.
 *  @param seasonChanged Whether the season changed this tick.
 *  @param dayChanged Whether the day changed this tick.
 *  @param hourChanged Whether the hour changed this tick.
 *  @param minuteChanged Whether the minute changed this tick.
 */
void CreatureManager::onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	QElapsedTimer timer;
	timer.start();

	//if ( m_startIndex >= m_creatures.size() )
	{
		m_startIndex = 0;
	}
	int oldStartIndex = m_startIndex;
	QList<unsigned int> toDestroy;
	for ( int i = m_startIndex; i < m_creatures.size(); ++i )
	{
		Creature* creature = m_creatures[i];

		CreatureTickResult ctr = creature->onTick( tickNumber, seasonChanged, dayChanged, hourChanged, minuteChanged );

		switch ( ctr )
		{
			case CreatureTickResult::TODESTROY:
				toDestroy.append( creature->id() );
				break;
			case CreatureTickResult::DEAD:
				if( creature->type() == CreatureType::ANIMAL )
				{
					auto a = dynamic_cast<Animal*>( creature );
					g->inv()->createItem( a->getPos(), "AnimalCorpse", { a->species() } );
				}
				toDestroy.append( creature->id() );
				break;
		}

		m_startIndex = i + 1;
		//if( jobChanged ) emit signalGnomeActivity( g.id(), g.getActivity() );
		//if ( timer.elapsed() > 2 )
		//	break;
	}

	if ( toDestroy.size() )
	{
		for ( auto aid : toDestroy )
		{
			for ( int i = 0; i < m_creatures.size(); ++i )
			{
				if ( aid == m_creatures[i]->id() )
				{
					removeCreature( aid );
					break;
				}
			}
		}
	}

	if ( hourChanged )
	{
		
	}
}

/** @brief Returns all creatures located at the given position.
 *  @param pos The world position to query.
 *  @return List of creature pointers at that position.
 */
QList<Creature*> CreatureManager::creaturesAtPosition( Position& pos )
{
	QList<Creature*> out;
	for ( int i = 0; i < m_creatures.size(); ++i )
	{
		if ( m_creatures[i]->getPos() == pos )
		{
			out.push_back( m_creatures[i] );
		}
	}
	return out;
}

/** @brief Returns all animals located at the given position.
 *  @param pos The world position to query.
 *  @return List of animal pointers at that position.
 */
QList<Animal*> CreatureManager::animalsAtPosition( Position& pos )
{
	QList<Animal*> out;
	for ( int i = 0; i < m_creatures.size(); ++i )
	{
		auto c = m_creatures[i];

		if ( c->isAnimal() && c->getPos() == pos )
		{
			out.push_back( dynamic_cast<Animal*>( c ) );
		}
	}
	return out;
}
	
/** @brief Returns all monsters located at the given position.
 *  @param pos The world position to query.
 *  @return List of monster pointers at that position.
 */
QList<Monster*> CreatureManager::monstersAtPosition( Position& pos )
{
	QList<Monster*> out;
	for ( int i = 0; i < m_creatures.size(); ++i )
	{
		auto c = m_creatures[i];

		if ( c->isMonster() && c->getPos() == pos )
		{
			out.push_back( dynamic_cast<Monster*>( c ) );
		}
	}
	return out;
}

/** @brief Returns the total number of managed creatures (animals and monsters).
 *  @return Total creature count.
 */
int CreatureManager::countWildAnimals()
{
	return m_creatures.size();
}

/** @brief Creates and registers a new creature with the given parameters.
 *  @param ct The creature type (ANIMAL or MONSTER).
 *  @param type Species/type ID from the database.
 *  @param pos World position to spawn the creature.
 *  @param gender Gender of the creature.
 *  @param adult Whether the creature is an adult (animals only).
 *  @param tame Whether the creature is tame (animals only).
 *  @param level Difficulty level (monsters only, defaults to 1).
 *  @return The unique ID of the created creature, or 0 on failure.
 */
unsigned int CreatureManager::addCreature( CreatureType ct, QString type, Position pos, Gender gender, bool adult, bool tame, int level )
{
	Creature* creature = nullptr;
	CreatureFactory cf( g );
	switch ( ct )
	{
		case CreatureType::ANIMAL:
			creature = cf.createAnimal( type, pos, gender, adult, tame );
			break;
		case CreatureType::MONSTER:
			creature = cf.createMonster( type, level, pos, gender );
			break;
	}

	if( creature )
	{
		m_creatures.append( creature );
		unsigned int id = m_creatures.last()->id();
		m_creaturesByID.insert( id, m_creatures.last() );

		int count = m_countPerType.value( type );
	
		++count;
		m_countPerType.insert( type, count );

		auto& list = m_creaturesPerType[type];
		list.append( id );

		m_dirty = true;

		if( creature->hasTransparency() )
		{
			g->m_world->setTileFlag( creature->getPos(), TileFlag::TF_TRANSPARENT );
		}

		return id;
	}
	return 0;
}

/** @brief Creates and registers a creature restored from serialized save data.
 *  @param ct The creature type (ANIMAL or MONSTER).
 *  @param vals QVariantMap containing the saved creature state.
 *  @return The unique ID of the restored creature, or 0 on failure.
 */
unsigned int CreatureManager::addCreature( CreatureType ct, QVariantMap vals )
{
	Creature* creature = nullptr;
	CreatureFactory cf( g );
	switch ( ct )
	{
		case CreatureType::ANIMAL:
			creature = cf.createAnimal( vals );
			break;
		case CreatureType::MONSTER:
			creature = cf.createMonster( vals );
			break;
	}
	if( creature )
	{
		m_creatures.append( creature );
		unsigned int id = m_creatures.last()->id();
		m_creaturesByID.insert( id, m_creatures.last() );

		QString type = creature->species();
		int count    = m_countPerType.value( type );
		++count;
		m_countPerType.insert( type, count );

		auto& list = m_creaturesPerType[type];
		list.append( id );

		m_dirty = true;

		if( creature->hasTransparency() )
		{
			g->m_world->setTileFlag( creature->getPos(), TileFlag::TF_TRANSPARENT );
		}

		return id;
	}
	return 0;
}

/** @brief Looks up a creature by its unique ID.
 *  @param id The creature's unique ID.
 *  @return Pointer to the creature, or nullptr if not found.
 */
Creature* CreatureManager::creature( unsigned int id )
{
	if ( m_creaturesByID.contains( id ) )
	{
		return m_creaturesByID[id];
	}
	return nullptr;
}

/** @brief Looks up an animal by its unique ID.
 *  @param id The creature's unique ID.
 *  @return Pointer to the Animal, or nullptr if not found or not an animal.
 */
Animal* CreatureManager::animal( unsigned int id )
{
	if ( m_creaturesByID.contains( id ) )
	{
		auto c = m_creaturesByID[id];
		if( c->isAnimal() )
		{
			return dynamic_cast<Animal*>( c );
		}
	}
	return nullptr;
}
	
/** @brief Looks up a monster by its unique ID.
 *  @param id The creature's unique ID.
 *  @return Pointer to the Monster, or nullptr if not found or not a monster.
 */
Monster* CreatureManager::monster( unsigned int id )
{
	if ( m_creaturesByID.contains( id ) )
	{
		auto c = m_creaturesByID[id];
		if( c->isMonster() )
		{
			return dynamic_cast<Monster*>( c );
		}
	}
	return nullptr;
}

/** @brief Returns the total number of managed creatures.
 *  @return Total creature count.
 */
int CreatureManager::count()
{
	return m_creatures.size();
}

/** @brief Returns the number of creatures of a specific species type.
 *  @param type Species/type ID to count.
 *  @return Count of creatures of that type.
 */
int CreatureManager::count( QString type )
{
	return m_countPerType.value( type );
}

/** @brief Removes and deletes a creature by ID. Also cleans up pasture assignments for animals.
 *  @param id The unique ID of the creature to remove.
 */
void CreatureManager::removeCreature( unsigned int id )
{
	if( m_creaturesByID.contains( id ) )
	{
		auto creature = m_creaturesByID[id];

		switch( creature->type() )
		{
			case CreatureType::ANIMAL:
			{
				Animal* a = dynamic_cast<Animal*>( creature );
				if ( a )
				{
					if ( a->pastureID() )
					{
						qDebug() << "remove animal from pasture";
						auto pasture = g->m_farmingManager->getPasture( a->pastureID() );
						if ( pasture )
						{
							pasture->removeAnimal( a->id() );
						}
					}
				}
			}
			break;
		}

		auto& perTypeList = m_creaturesPerType[creature->species()];
		perTypeList.removeAll( id );

		m_creaturesByID.remove( id );
		m_creatures.removeAll( creature );

		int count = m_countPerType.value( creature->species() );
		--count;
		m_countPerType.insert( creature->species(), count );

		delete creature;

		m_dirty = true;

		emit signalCreatureRemove( id );
	}
}

/** @brief Finds the closest reachable animal of a given type to a position.
 *  @param pos Reference position to measure distance from.
 *  @param type Species/type ID to search for.
 *  @return Pointer to the closest Animal, or nullptr if none found.
 */
Animal* CreatureManager::getClosestAnimal( Position pos, QString type )
{
	auto distanceQueue = animalsByDistance( pos, type );

	if ( !distanceQueue.empty() )
	{
		return distanceQueue.get();
	}
	return nullptr;
}

/** @brief Returns a priority queue of reachable, available animals of a type sorted by distance.
 *  @param pos Reference position to measure distance from.
 *  @param type Species/type ID to filter by.
 *  @return Priority queue of animals ordered by squared distance (closest first).
 */
PriorityQueue<Animal*, int> CreatureManager::animalsByDistance( Position pos, QString type )
{
	auto distanceQueue = PriorityQueue<Animal*, int>();
	for ( auto id : m_creaturesPerType[type] )
	{
		Creature* creature = m_creaturesByID[id];
		if( creature->isAnimal() )
		{
			Animal* a = dynamic_cast<Animal*>( creature );
			if ( a && !a->inJob() && !a->isDead() && !a->toDestroy() )
			{
				Position targetPos = a->getPos();
				if ( g->m_pf->checkConnectedRegions( pos, targetPos ) )
				{
					distanceQueue.put( a, pos.distSquare( targetPos ) );
				}
			}
		}
	}
	return distanceQueue;
}

/** @brief Returns the list of creature IDs for a given species type.
 *  @param type Species/type ID to query.
 *  @return List of creature IDs of that type.
 */
QList<unsigned int> CreatureManager::animalsByType( QString type )
{
	return m_creaturesPerType.value( type );
}

/** @brief Forces all creatures at a given position to move to a new position.
 *  @param from The source position.
 *  @param to The destination position.
 */
void CreatureManager::forceMoveAnimals( const Position& from, const Position& to )
{
	for ( auto& a : m_creatures )
	{
		// check gnome position
		if ( a->getPos().toInt() == from.toInt() )
		{
			//qDebug() << "force move gnome from " << from.toString() << " to " << to.toString();
			// move gnome
			a->forceMove( to );
			// abort job if he has one
		}
	}
}

/** @brief Returns a list of all creature species types currently tracked.
 *  @return List of type ID strings.
 */
QList<QString> CreatureManager::types()
{
	return m_countPerType.keys();
}

	
/** @brief Returns the cached list of all animals. Rebuilds the cache if dirty.
 *  @return Reference to the list of Animal pointers.
 */
QList<Animal*>& CreatureManager::animals()
{
	if( m_dirty )
	{
		updateLists();
	}

	return m_animals;
}

/** @brief Returns the cached list of all monsters. Rebuilds the cache if dirty.
 *  @return Reference to the list of Monster pointers.
 */
QList<Monster*>& CreatureManager::monsters()
{
	if( m_dirty )
	{
		updateLists();
	}

	return m_monsters;
}

/** @brief Rebuilds the separate animal and monster lists from the unified creature list. */
void CreatureManager::updateLists()
{
	m_animals.clear();
	m_monsters.clear();

	for( const auto& c : m_creatures )
	{
		if( c->isAnimal() )
		{
			m_animals.append( dynamic_cast<Animal*>( c ) );
		}
		else
		{
			m_monsters.append( dynamic_cast<Monster*>( c ) );
		}
	}
	m_dirty = false;
}

/** @brief Checks whether a creature can reach a given position via connected regions.
 *  @param pos Target position.
 *  @param creatureID ID of the creature to check from.
 *  @return True if the creature's region is connected to the target position.
 */
bool CreatureManager::hasPathTo( const Position& pos, unsigned int creatureID )
{
	if( m_creaturesByID.contains( creatureID ) )
	{
		auto creature = m_creaturesByID[creatureID];
		if( creature )
		{
			return g->m_world->regionMap().checkConnectedRegions( pos, creature->getPos() );
		}
	}
	return false;
}

/** @brief Checks whether a creature has line of sight to a given position.
 *  @param pos Target position.
 *  @param creatureID ID of the creature to check from.
 *  @return True if line of sight exists between the creature and the target.
 */
bool CreatureManager::hasLineOfSightTo( const Position& pos, unsigned int creatureID )
{
	if ( m_creaturesByID.contains( creatureID ) )
	{
		auto creature = m_creaturesByID[creatureID];
		if ( creature )
		{
			return g->m_world->isLineOfSight( pos, creature->getPos() );
		}
	}
	return false;
}
