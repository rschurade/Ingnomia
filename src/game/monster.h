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

#include "../game/creature.h"

class Monster : public Creature
{
	friend class MonsterWidget;

public:
	Monster( QString species, int level, Position& pos, Gender gender, Game* game );
	Monster( QVariantMap in, Game* game );
	~Monster();

	virtual void init();

	virtual void updateSprite();

	virtual void serialize( QVariantMap& out );

	CreatureTickResult onTick( quint64 tick, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged );

	virtual void updateMoveSpeed();

	bool attack( DamageType dt, AnatomyHeight da, int skill, int strength, Position sourcePos, unsigned int attackerID );

private:
	void initTaskMap();

	void generateAggroList();

	BT_RESULT actionMove( bool halt );
	BT_RESULT actionAttackTarget( bool halt );
	BT_RESULT actionGetTarget( bool halt );

	
	int m_level;
};
