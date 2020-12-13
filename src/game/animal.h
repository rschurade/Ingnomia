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

#include "creature.h"

#include <QString>
#include <QtGlobal>

class Animal : public Creature
{
	friend class AnimalWidget;

public:
	Animal( QString species, Position& pos, Gender gender, bool adult, Game* game );
	Animal( QVariantMap in, Game* game );
	Animal();
	~Animal();

	virtual void init();

	virtual void updateSprite();

	virtual void serialize( QVariantMap& out );

	void setState( int state );

	CreatureTickResult onTick( quint64 tick, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged );

	void setTame( bool tame );
	bool isTame();

	void setPregnant( bool pregnant );
	bool isPregnant();

	bool isEgg();
	bool isYoung();
	bool isAdult();

	bool isAggro();

	bool toButcher();
	void setToButcher( bool value );

	bool attack( DamageType dt, AnatomyHeight da, int skill, int strength, Position sourcePos, unsigned int attackerID );

	BT_RESULT conditionIsEgg( bool halt = false );
	BT_RESULT conditionIsYoung( bool halt = false );
	BT_RESULT conditionIsAdult( bool halt = false );

	BT_RESULT conditionIsHungry( bool halt = false );
	BT_RESULT conditionIsCarnivore( bool halt = false );
	BT_RESULT conditionIsHerbivore( bool halt = false );

	BT_RESULT conditionIsEggLayer( bool halt = false );
	BT_RESULT conditionIsProducer( bool halt = false );

	BT_RESULT conditionIsPregnant( bool halt = false );

	BT_RESULT conditionIsInShed( bool halt = false );
	BT_RESULT conditionIsOnPasture( bool halt = false );

	BT_RESULT conditionIsReadyToGiveBirth( bool halt = false );
	BT_RESULT conditionIsWoodVermin( bool halt = false );
	BT_RESULT actionLayEgg( bool halt );
	BT_RESULT actionProduce( bool halt );

	BT_RESULT actionTryHaveSex( bool halt );
	BT_RESULT actionGiveBirth( bool halt );

	BT_RESULT actionFindPrey( bool halt );
	BT_RESULT actionKillPrey( bool halt );
	BT_RESULT actionEatPrey( bool halt );
	BT_RESULT actionMove( bool halt );

	BT_RESULT actionFindRetreat( bool halt );
	BT_RESULT actionSleep( bool halt );

	BT_RESULT actionFindShed( bool halt );
	BT_RESULT actionFindRandomPastureField( bool halt );
	BT_RESULT actionEnterShed( bool halt );
	BT_RESULT actionLeaveShed( bool halt );

	BT_RESULT actionGetTarget( bool halt );
	BT_RESULT actionGuardDogGetTarget( bool halt );
	BT_RESULT actionAttackTarget( bool halt );

	BT_RESULT actionGraze( bool halt );

	BT_RESULT actionRandomMoveBig( bool halt );

	void setPastureID( unsigned int id );
	unsigned int pastureID();

	void setInJob( unsigned int id );
	unsigned int inJob();

	int numProduce();
	QString producedItem();
	void harvest();

	float hunger();

	virtual void updateMoveSpeed();

	void setDye( QString dye );
	QString dye();

	bool isMulti()
	{
		return m_isMulti;
	}
	QList<QPair<Position, unsigned int>> multiSprites()
	{
		return m_multiSprites;
	};

private:
	void initTaskMap();

	void checkInJob();


	bool m_isMulti = false;
	bool m_tame    = false;

	unsigned int m_pastureID = 0;

	unsigned int m_inJob = 0;
	float m_hunger       = 50;
	int m_foodValue      = 1;

	QVariantMap m_stateMap;

	bool m_isProducer = false;
	bool m_isEggLayer = false;
	bool m_isGrazer   = false;

	bool m_isEgg   = false;
	bool m_isYoung = false;
	bool m_isAdult = false;

	bool m_toButcher = false;

	//also used for egg laying
	quint64 m_birthTick = 0;

	quint64 m_produceTick = 0;

	quint64 m_lastSex = 0;

	QStringList m_preyList;

	unsigned int m_currentPrey = 0;

	bool m_inShed = false;

	quint8 m_producedAmount = 0;
	QString m_produce;

	unsigned int m_corpseToEat = 0;

	QString m_dye;

	void move( Position oldPos );

	// behaviors
	bool morph( QVariantMap def );

	QList<QPair<Position, unsigned int>> m_multiSprites;
};
