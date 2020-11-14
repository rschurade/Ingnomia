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

#include "../base/behaviortree/bt_node.h"
#include "canwork.h"

#include <QHash>

#include <functional>
#include <string>
#include <unordered_map>

struct Uniform;

class Gnome : public CanWork
{
	friend class GnomeWidget;

public:
	Gnome( Position& pos, QString name, Gender gender );
	Gnome( QVariantMap& in );
	~Gnome();

	virtual void init();

	virtual void serialize( QVariantMap& out );

	virtual void updateSprite();
	QVariantList createSpriteDef( QString type, bool isBack );

	virtual CreatureTickResult onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged );

	// return true if no floor present
	bool checkFloor();

	//return true if need needs action
	bool evalNeeds( bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged );

	void addNeed( QString id, int level );
	int need( QString id );

	void selectProfession( QString profession );

	QString profession()
	{
		return m_profession;
	}
	void setProfession( QString profession )
	{
		selectProfession( profession );
	}

	void updateLight( Position oldPos, Position newPos );

	void setJobAborted( QString caller );

	QStringList skillPrios()
	{
		return m_skillPriorities;
	}
	void setSkillPrios( QStringList prioList )
	{
		m_skillPriorities = prioList;
	}

	QString getActivity();

	void setOwnedRoom( unsigned int id );
	unsigned int ownedRoom();

	ScheduleActivity schedule( unsigned char hour );
	void setSchedule( unsigned char hour, ScheduleActivity activity );
	QList<ScheduleActivity> schedule() { return m_schedule; }

	virtual void updateMoveSpeed();

	void assignWorkshop( unsigned int id );

	Equipment equipment();

	QString rightHandItem();
	QString rightHandAttackSkill();
	QString rightHandAttackValue();

	void updateAttackValues();
	bool attack( DamageType dt, AnatomyHeight da, int skill, int strength, Position sourcePos, unsigned int attackerID );

	void setAllowedCarryItems( bool bandages, bool food, bool drinks );

	bool equipItem();

protected:
	float m_gainFromSleep = 0.035f;

	QString m_profession = "Gnomad";

	bool m_hungryLog     = false;
	bool m_veryHungryLog = false;
	bool m_thirstyLog    = false;
	bool m_veryThirstLog = false;
	bool m_sleepyLog     = false;

	bool m_carryBandages            = false;
	bool m_carryFood                = false;
	bool m_carryDrinks              = false;
	unsigned char m_carriedBandages = 0;
	unsigned char m_carriedFood     = 0;
	unsigned char m_carriedDrinks   = 0;

	QList<ScheduleActivity> m_schedule;

	void initTaskMap();

	void setConcealed( QString part, bool concealed );
	bool checkUniformItem( QString slot, Uniform* uniform, bool& dropped );
	bool m_uniformWorn             = false;
	quint64 m_nextUniformCheckTick = 0;

	BT_RESULT conditionIsHungry( bool halt = false );
	BT_RESULT conditionIsVeryHungry( bool halt = false );
	BT_RESULT conditionIsThirsty( bool halt = false );
	BT_RESULT conditionIsVeryThirsty( bool halt = false );
	BT_RESULT conditionIsSleepy( bool halt = false );
	BT_RESULT conditionAllItemsInPlaceForJob( bool halt = false );
	BT_RESULT conditionIsButcherJob( bool halt = false );
	BT_RESULT conditionAllPickedUp( bool halt = false );
	BT_RESULT conditionIsFull( bool halt = false );
	BT_RESULT conditionIsDrinkFull( bool halt = false );
	BT_RESULT conditionIsTrainingTime( bool halt = false );
	BT_RESULT conditionIsTrainer( bool halt = false );
	BT_RESULT conditionIsCivilian( bool halt = false );
	BT_RESULT conditionHasHuntTarget( bool halt = false );

	BT_RESULT actionSleep( bool halt );
	BT_RESULT actionFindBed( bool halt );

	BT_RESULT actionFindFood( bool halt );
	BT_RESULT actionFindDrink( bool halt );

	BT_RESULT actionFindDining( bool halt );
	BT_RESULT actionPickUpItem( bool halt );

	BT_RESULT actionEat( bool halt );
	BT_RESULT actionDrink( bool halt );

	BT_RESULT actionMove( bool halt );

	BT_RESULT actionClaimItems( bool halt );
	bool claimFromLinkedStockpile( QString itemSID, QString materialSID, int count, bool requireSame, QStringList restriction );

	BT_RESULT actionDropItem( bool halt );
	BT_RESULT actionDropAllItems( bool halt );
	BT_RESULT actionEquipTool( bool halt );
	BT_RESULT actionCheckUniform( bool halt );
	BT_RESULT actionCheckBandages( bool halt );
	BT_RESULT actionUniformCleanUp( bool halt );
	BT_RESULT actionFindTool( bool halt );
	BT_RESULT actionGetItemDropPosition( bool halt );
	BT_RESULT actionGetWorkPosition( bool halt );

	BT_RESULT actionGetJob( bool halt );
	BT_RESULT actionInitJob( bool halt );
	BT_RESULT actionAbortJob( bool halt );
	BT_RESULT actionFinishJob( bool halt );
	BT_RESULT actionWork( bool halt );

	BT_RESULT actionInitAnimalJob( bool halt );
	BT_RESULT actionGrabAnimal( bool halt );
	BT_RESULT actionReleaseAnimal( bool halt );
	BT_RESULT actionButcherAnimal( bool halt );
	BT_RESULT actionDyeAnimal( bool halt );
	BT_RESULT actionHarvestAnimal( bool halt );
	BT_RESULT actionTameAnimal( bool halt );
	BT_RESULT actionFinalMoveAnimal( bool halt );

	BT_RESULT actionAlwaysRunning( bool halt );

	BT_RESULT actionAttackTarget( bool halt );
	BT_RESULT actionFindTrainingGround( bool halt );
	BT_RESULT actionTrain( bool halt );
	BT_RESULT actionFindTrainerPosition( bool halt );
	BT_RESULT actionSuperviseTraining( bool halt );

	BT_RESULT actionGetTarget( bool halt );

	BT_RESULT actionDoMission( bool halt );
	BT_RESULT actionLeaveForMission( bool halt );
	BT_RESULT actionReturnFromMission( bool halt );
};
