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

#include "../game/animal.h"
#include "../game/monster.h"

class Game;

class CreatureManager : public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY_MOVE( CreatureManager )
public:
	CreatureManager( Game* parent );
	~CreatureManager();

	void onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged );

	int countWildAnimals();

	unsigned int addCreature( CreatureType ct, QString type, Position pos, Gender gender, bool adult, bool tame, int level = 1 );
	unsigned int addCreature( CreatureType ct, QVariantMap vals );

	void removeCreature( unsigned int id );

	/**
	 * Generic Method to get a creature by Id and automatically cast to a type. 
	 * Be careful when using this method as passing in the wrong type combination will cause a crash.
	 * Recommend to use methods CreatureManager::animal, CreatureManager::animal, and 
	 * CreatureManager::monster as these are safer and use this function under the hood.
	 * 
	 * Below is a good and bad example of usage.
	 * ex) 
	 *  //GOOD Animal matches with CreatureType::Animal
	 *  getCreature<Animal>(local_id, CreatureType::ANIMAL)
	 * 
	 * 	//BAD Animal does not match with CreatureType::Monster
	 * 	getCreature<Animal>(local_id, CreatureType::MONSTER)
	 * 
	 * Eventually this should use QTs MetaTypes so passing the CreatureType would
	 * be unnecessary and a call would look like getCreature<Animal>(id).
	 * 
	 * @param id - Creature id
	 * @param type - CreatureType to look for
	 * @return nullptr or creature cast to given type
	 */
	template<typename TCreatureClass>
	TCreatureClass* getCreature(const unsigned int id, const CreatureType type) const{
		static_assert(!std::is_convertible_v<TCreatureClass,Creature>, "TCreatureClass must be convertable to Creature");
		
		if(m_creaturesByID.contains(id)){
			//Compile time if to skip a type check when TCreatureClass = Creature
			if constexpr (std::is_same_v<TCreatureClass,Creature>){
				return m_creaturesByID[id];
			}else{
				auto creature = m_creaturesByID[id];
				if(creature->type() == type){
					return static_cast<TCreatureClass*>(creature);
				}
			}
		}
		
		return nullptr;
	}

	/**
	 * @param id - Creature id
	 * @return nullptr or Creature
	 */
	Creature* creature(const unsigned int id) const{
		return getCreature<Creature>(id, CreatureType::UNDEFINED);
	}

	/**
	 * @param id - Creature id
	 * @return nullptr or Animal
	 */
	Animal* animal(const unsigned int id) const{
		return getCreature<Animal>(id, CreatureType::ANIMAL);
	}

	/**
	 * @param id - Creature id
	 * @return nullptr or monster
	 */
	Monster* monster(const unsigned int id) const{
		return getCreature<Monster>(id, CreatureType::MONSTER);
	}

	/**
	 * Generic Method to get a list of creatures at a position and automatically cast to a type. 
	 * Be careful when using this method as passing in the wrong type combination will cause a crash.
	 * Recommend to use methods CreatureManager::creaturesAtPosition, CreatureManager::animalsAtPosition, 
	 * and CreatureManager::monstersAtPosition as these are safer and use this function under the hood.
	 * 
	 * Below is a good and bad example of usage.
	 * ex) 
	 *  //GOOD Animal matches with CreatureType::Animal
	 *  getCreaturesAtPosition<Animal>(local_pos, CreatureType::ANIMAL)
	 * 
	 * 	//BAD Animal does not match with CreatureType::Monster
	 * 	getCreaturesAtPosition<Animal>(local_id, CreatureType::MONSTER)
	 * 
	 * Eventually this should use QTs MetaTypes so passing the CreatureType would
	 * be unnecessary and a call would look like getCreaturesAtPosition<Animal>(pos).
	 * 
	 * @param pos - Position to search
	 * @param type - CreatureType to look for
	 * @return QList<TCreatureClass>
	 */
	template<typename TCreatureClass>
	QList<TCreatureClass*> getCreaturesAtPosition(const Position& pos, const CreatureType type) const{
		static_assert(!std::is_convertible_v<TCreatureClass,Creature>, "TCreatureClass must be convertable to Creature");

		QList<TCreatureClass*> out;
		for(Creature* creature : m_creatures){
			//Compile time if to skip a type check when TCreatureClass = Creature
			if constexpr (std::is_same_v<TCreatureClass,Creature>){
				out.push_back(creature);
			}else{
				if(creature->type() == type && creature->getPos() == pos){
					out.push_back(static_cast<TCreatureClass*>(creature));
				}
			}
		}
		return out;
	}

	/**
	 * @param pos - position to search
	 * @return list of Creatures
	 */
	QList<Creature*> creaturesAtPosition(const Position& pos) const{
		return getCreaturesAtPosition<Creature>(pos, CreatureType::UNDEFINED);
	}

		/**
	 * @param pos - position to search
	 * @return list of Animals
	 */
	QList<Animal*> animalsAtPosition(const Position& pos ) const{
		return getCreaturesAtPosition<Animal>(pos, CreatureType::ANIMAL);
	}

	/**
	 * @param pos - position to search
	 * @return list of Monsters
	 */
	QList<Monster*> monstersAtPosition(const Position& pos ) const{
		return getCreaturesAtPosition<Monster>(pos, CreatureType::MONSTER);
	}

	int count( QString type );

	int count();

	Animal* getClosestAnimal( Position pos, QString type );

	QList<Creature*>& creatures()
	{
		return m_creatures;
	}
	QList<Animal*>& animals();
	QList<Monster*>& monsters();
	

	PriorityQueue<Animal*, int> animalsByDistance( Position pos, QString type );
	
	void forceMoveAnimals( const Position& from, const Position& to );

	QList<QString> types();
	QList<unsigned int> animalsByType( QString type );

	bool hasPathTo( const Position& pos, unsigned int creatureID );
	bool hasLineOfSightTo( const Position& pos, unsigned int creatureID );

private:
	QPointer<Game> g;
	QList<Creature*> m_creatures;
	QList<Animal*> m_animals;
	QList<Monster*> m_monsters;

	QHash<unsigned int, Creature*> m_creaturesByID;

	QMap<QString, unsigned int> m_countPerType;
	QMap<QString, QList<unsigned int>> m_creaturesPerType;

	int m_startIndex = 0;

	bool m_dirty = true;

	void updateLists();

signals:
	void signalCreatureDeath( unsigned int id );
	void signalCreatureRemove( unsigned int id );

	void signalAddMonster( unsigned int monsterID );
	void signalRemoveMonster( unsigned int monsterID );
};
