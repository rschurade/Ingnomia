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
#include "creaturemanager.h"

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

CreatureManager::CreatureManager()
{
}

CreatureManager::~CreatureManager()
{
}

void CreatureManager::reset()
{
	m_creatures.clear();
	m_creaturesByID.clear();

	m_countPerType.clear();
	m_creaturesPerType.clear();

	// add monster entries for monsters that aren't typically on the map so they can appear in the squad priority list
	auto monsters = DB::ids( "Monsters" );
	for( auto monster : monsters )
	{
		m_countPerType.insert( monster, 0 );
	}

	auto animals = DB::ids( "Animals" );
	for ( auto animal : animals )
	{
		if ( NewGameSettings::getInstance().isChecked( animal ) )
		{
			m_countPerType.insert( animal, 0 );
		}
	}

}

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
				//Global::inv().createItem( a->getPos(), "AnimalCorpse", { a->species() } );
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

int CreatureManager::countWildAnimals()
{
	return m_creatures.size();
}

unsigned int CreatureManager::addCreature( CreatureType ct, QString type, Position pos, Gender gender, bool adult, bool tame, int level )
{
	QMutexLocker l( &m_mutex );

	Creature* creature = nullptr;
	switch ( ct )
	{
		case CreatureType::ANIMAL:
			creature = CreatureFactory::getInstance().createAnimal( type, pos, gender, adult, tame );
			break;
		case CreatureType::MONSTER:
			creature = CreatureFactory::getInstance().createMonster( type, level, pos, gender );
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

		return id;
	}
	return 0;
}

unsigned int CreatureManager::addCreature( CreatureType ct, QVariantMap vals )
{
	Creature* creature = nullptr;
	switch ( ct )
	{
		case CreatureType::ANIMAL:
			creature = CreatureFactory::getInstance().createAnimal( vals );
			break;
		case CreatureType::MONSTER:
			creature = CreatureFactory::getInstance().createMonster( vals );
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

		return id;
	}
	return 0;
}

Creature* CreatureManager::creature( unsigned int id )
{
	if ( m_creaturesByID.contains( id ) )
	{
		return m_creaturesByID[id];
	}
	return nullptr;
}

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

int CreatureManager::count()
{
	QMutexLocker l( &m_mutex );
	return m_creatures.size();
}

int CreatureManager::count( QString type )
{
	QMutexLocker l( &m_mutex );
	return m_countPerType.value( type );
}

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
						auto pasture = Global::fm().getPasture( a->pastureID() );
						if ( pasture )
						{
							pasture->removeAnimal( a->id() );
						}
					}
				}
			}
			break;
		}
		QMutexLocker l( &m_mutex );

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

Animal* CreatureManager::getClosestAnimal( Position pos, QString type )
{
	auto distanceQueue = animalsByDistance( pos, type );

	if ( !distanceQueue.empty() )
	{
		return distanceQueue.get();
	}
	return nullptr;
}

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
				if ( PathFinder::getInstance().checkConnectedRegions( pos, targetPos ) )
				{
					distanceQueue.put( a, pos.distSquare( targetPos ) );
				}
			}
		}
	}
	return distanceQueue;
}

QList<unsigned int> CreatureManager::animalsByType( QString type )
{
	return m_creaturesPerType.value( type );
}

void CreatureManager::forceMoveAnimals( Position& from, Position& to )
{
	for ( auto&& a : m_creatures )
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

QList<QString> CreatureManager::types()
{
	return m_countPerType.keys();
}

	
QList<Animal*>& CreatureManager::animals()
{
	if( m_dirty )
	{
		updateLists();
	}

	return m_animals;
}

QList<Monster*>& CreatureManager::monsters()
{
	if( m_dirty )
	{
		updateLists();
	}

	return m_monsters;
}

void CreatureManager::updateLists()
{
	m_animals.clear();
	m_monsters.clear();

	for( auto c : m_creatures )
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

bool CreatureManager::hasPathTo( Position& pos, unsigned int creatureID )
{
	if( m_creaturesByID.contains( creatureID ) )
	{
		auto creature = m_creaturesByID[creatureID];
		if( creature )
		{
			return Global::w().regionMap().checkConnectedRegions( pos, creature->getPos() );
		}
	}
	return false;
}