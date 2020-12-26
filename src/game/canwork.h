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
#include "../game/job.h"
#include "../game/jobmanager.h"

class CanWork : public Creature
{
public:
	CanWork( Position& pos, QString name, Gender gender, QString species, Game* game );
	CanWork( QVariantMap in, Game* game );
	~CanWork() {};

	virtual void serialize( QVariantMap& out );

	virtual CreatureTickResult onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged ) = 0;

	bool getSkillActive( QString id );
	void setSkillActive( QString id, bool active );
	void clearAllSkills();
	void setAllSkillsActive( bool active );

protected:
	unsigned int m_jobID = 0;

	PriorityQueue<Position, int> m_workPositionQueue;
	Position m_workPosition;
	QVariantMap m_currentTask;

	unsigned int m_assignedWorkshop = 0;
	int m_totalDurationTicks        = 0;
	quint64 m_taskFinishTick        = 0;
	bool m_repeatJob                = false;
	QList<Position> m_effectedTiles;
	unsigned int m_animal       = 0;
	unsigned int m_itemToPickUp = 0;
	bool m_startedEating        = false;
	bool m_startedDrinking      = false;
	QList<QVariantMap> m_taskList;
	int m_trainCounter            = -1;
	unsigned int m_trainingGround = 0;
	////////////////////////////////////////////////////////////////////////////

	QSharedPointer<Job> m_job;
	bool m_jobChanged = false;

	void resetJobVars();
	void cleanUpJob( bool finished );

	int getDurationTicks( QVariant value, QSharedPointer<Job> job );

	bool dropEquippedItem();
	void equipHand( unsigned int item, QString side );

	void gainTech( QVariant techGain, QSharedPointer<Job> job );
	void gainSkill( QVariant skillGain, QSharedPointer<Job> job );
	void gainSkill( QString skill, int value );
	double parseValue( QVariant v );
	double parseGain( QVariantMap gainMap );

	// Tasks
	bool mineWall();
	bool mineFloor();
	bool digHole();
	bool explorativeMineWall();
	bool removeWall();
	bool removeRamp();
	bool removeFloor();
	bool construct();
	bool constructDugStairs();
	bool constructDugRamp();
	bool constructAnimate();
	bool createItem();
	bool plantTree();
	bool fellTree();
	bool harvest();
	bool harvestHay();
	bool plant();
	bool removePlant();
	bool till();
	bool craft();
	bool deconstruct();
	bool fish();
	bool butcherFish();
	bool butcherCorpse();
	bool prepareSpell();
	bool castSpell();
	bool castSpellAnimate();
	bool finishSpell();
	bool switchMechanism();
	bool invertMechanism();
	bool refuel();
	bool install();
	bool uninstall();
	bool fillTrough();
	bool soundAlarm();
};