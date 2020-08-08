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

#include "animal.h"
#include "monster.h"

#include <QString>
#include <QVariantMap>

class CreatureFactory
{
private:
	// Private Constructor
	CreatureFactory();
	// Stop the compiler generating methods of copy the object
	CreatureFactory( CreatureFactory const& copy );            // Not Implemented
	CreatureFactory& operator=( CreatureFactory const& copy ); // Not Implemented

	int m_numWoodLice;

public:
	~CreatureFactory();

	static CreatureFactory& getInstance()
	{
		// The only instance
		// Guaranteed to be lazy initialized
		// Guaranteed that it will be destroyed correctly
		static CreatureFactory instance;
		return instance;
	}

	bool init();

	Animal* createRandomAnimal( QStringList allowedAnimals );
	Animal* createAnimal( QString type, Position pos, Gender gender, bool adult, bool tame );
	Animal* createAnimal( QVariantMap values );

	Monster* createRandomMonster( QStringList allowedMonsters );
	Monster* createMonster( QString type, int level, Position pos, Gender gender );
	Monster* createMonster( QVariantMap values );
};
