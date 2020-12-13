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
#include "creaturefactory.h"

#include "../base/config.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../game/game.h"
#include "../game/world.h"

#include <QDebug>
#include <QElapsedTimer>

CreatureFactory::CreatureFactory( Game* game ) :
	g( game )
{
}
CreatureFactory::~CreatureFactory()
{
}

Animal* CreatureFactory::createRandomAnimal( QStringList allowedAnimals )
{
	int dimx = Global::dimX;
	int dimy = Global::dimY;
	int dimZ = Global::dimZ;

	int numTypes = allowedAnimals.size();

	int randomType = rand() % ( numTypes );
	QString type   = allowedAnimals[randomType];

	int x = qMax( 2, ( rand() % dimx ) - 2 );
	int y = qMax( 2, ( rand() % dimy ) - 2 );

	Position pos( x, y, dimZ - 2 );
	g->w()->getFloorLevelBelow( pos, false );

	Animal* animal = new Animal( type, pos, rand() % 2 == 0 ? Gender::MALE : Gender::FEMALE, true, g );

	animal->init();
	return animal;
}

Animal* CreatureFactory::createAnimal( QString type, Position pos, Gender gender, bool adult, bool tame )
{
	Animal* animal = new Animal( type, pos, gender, adult, g );
	animal->init();
	animal->setTame( tame );
	return animal;
}

Animal* CreatureFactory::createAnimal( QVariantMap values )
{
	Animal* animal = new Animal( values, g );
	animal->init();
	return animal;
}

Monster* CreatureFactory::createRandomMonster( QStringList allowedMonsters )
{
	/*
	int dimx = Global::dimX
	int dimy = Global::dimY;
	int dimZ = Global::dimZ;

	Table& ta = Database::getInstance().getTable( "Monsters" );

	int numTypes = allowedMonsters.size();

	int randomType = rand() % ( numTypes );
	QString type = allowedMonsters[randomType];

	int x = qMax( 2, ( rand() % dimx ) - 2 );
	int y = qMax( 2, ( rand() % dimy ) - 2 );

	Position pos( x, y, dimZ - 2 );
	m_world->getFloorLevelBelow( pos, false );

	Monster* monster = new Monster( type, 1, pos, rand() % 2 == 0 ? Gender::MALE : Gender::FEMALE );
	
	monster->init();
	return monster;
	*/
	return nullptr;
}

Monster* CreatureFactory::createMonster( QString type, int level, Position pos, Gender gender )
{
	Monster* monster = new Monster( type, level, pos, gender, g );

	monster->init();

	auto attribs = DB::selectRows( "Attributes" );

	for ( auto row : attribs )
	{
		QString attributeID = row.value( "ID" ).toString();

		monster->addAttribute( attributeID, rand() % 10 + 1 );
	}

	auto skills = DB::selectRows( "Skills" );

	for ( auto skill : skills )
	{
		QString skillID = skill.value( "ID" ).toString();

		auto group = skill.value( "SkillGroup" ).toString();
		if ( group == "Combat" || group == "Defense" )
		{
			monster->addSkill( skillID, rand() % 500 );
			//monster->setSkillActive( skillID, true );
		}
	}

	return monster;
}

Monster* CreatureFactory::createMonster( QVariantMap values )
{
	Monster* monster = new Monster( values, g );
	monster->init();
	return monster;
}