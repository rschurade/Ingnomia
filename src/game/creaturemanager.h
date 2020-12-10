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

#include "managerbase.h"
#include "../game/animal.h"
#include "../game/monster.h"

class CreatureManager : public ManagerBase
{
	Q_OBJECT

public:
	CreatureManager( QObject* parent = nullptr );
	~CreatureManager();

	void reset();

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
	
	void forceMoveAnimals( Position& from, Position& to );

	QList<QString> types();
	QList<unsigned int> animalsByType( QString type );

	bool hasPathTo( Position& pos, unsigned int creatureID );

private:
	QList<Creature*> m_creatures;
	QList<Animal*> m_animals;
	QList<Monster*> m_monsters;

	QHash<unsigned int, Creature*> m_creaturesByID;

	QMap<QString, unsigned int> m_countPerType;
	QMap<QString, QList<unsigned int>> m_creaturesPerType;

	QMutex m_mutex;

	int m_startIndex = 0;

	bool m_dirty = true;

	void updateLists();

signals:
	void signalCreatureDeath( unsigned int id );
	void signalCreatureRemove( unsigned int id );

	void signalAddMonster( unsigned int monsterID );
	void signalRemoveMonster( unsigned int monsterID );
};
