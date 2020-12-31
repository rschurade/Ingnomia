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

	QList<Creature*> creaturesAtPosition( Position& pos );
	QList<Animal*> animalsAtPosition( Position& pos );
	QList<Monster*> monstersAtPosition( Position& pos );

	Creature* creature( unsigned int id );
	Animal* animal( unsigned int id );
	Monster* monster( unsigned int id );

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
