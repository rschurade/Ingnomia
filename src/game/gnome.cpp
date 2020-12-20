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
#include "gnome.h"
#include "../game/gnomemanager.h"
#include "../game/game.h"

#include "../base/behaviortree/bt_tree.h"
#include "../base/config.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/util.h"
#include "../game/inventory.h"
#include "../game/militarymanager.h"
#include "../game/plant.h"
#include "../game/stockpilemanager.h"
#include "../game/world.h"
#include "../game/workshop.h"
#include "../game/workshopmanager.h"
#include "../gfx/spritefactory.h"
#include "../gui/strings.h"

#include <QDebug>
#include <QDomDocument>
#include <QElapsedTimer>
#include <QFile>
#include <QPainter>
#include <QThreadPool>

Gnome::Gnome( Position& pos, QString name, Gender gender, Game* game ) :
	CanWork( pos, name, gender, "Gnome", game )
{
	m_ignoreNoPass = false;

	int shirt         = rand() % 14 + 1;
	m_equipment.shirt = "GnomeShirt" + QString::number( shirt );
	int hair          = rand() % 14 + 1;
	m_equipment.hair  = "GnomeHair" + QString::number( hair );

	auto numHairColors = DB::ids( "HairColors" ).size();

	m_equipment.hairColor  = rand() % numHairColors;
	m_equipment.shirtColor = rand() % 6;

	int fhair = rand() % 15;
	if ( m_gender == Gender::MALE )
	{
		m_equipment.facialHair = "GnomeFacialHair" + QString::number( fhair );
	}

	m_type = CreatureType::GNOME;

	m_anatomy.init( "Humanoid", false );

	for ( int i = 0; i < 24; ++i )
	{
		m_schedule.append( ScheduleActivity::None );
	}

	log( GameState::currentYearAndSeason );
}

Gnome::Gnome( QVariantMap& in, Game* game ) :
	CanWork( in, game )
{
	m_skillActive     = in.value( "SkillActive" ).toMap();
	m_skillPriorities = in.value( "SkillPriorities" ).toStringList();

	m_needs = in.value( "Needs" ).toMap();

	if ( in.contains( "Profession" ) )
	{
		m_profession = in.value( "Profession" ).toString();
	}
	else
	{
		m_profession = "Gnomad";
	}

	m_equipment = in.value( "Equipment" ).toMap();

	if ( in.contains( "Schedule" ) )
	{
		auto list = in.value( "Schedule" ).toList();
		m_schedule.clear();
		for( auto vs : list )
		{
			auto ss = vs.toString();
			ScheduleActivity sa = ScheduleActivity::None;
			if ( ss == "eat" ) sa = ScheduleActivity::Eat;
			else if ( ss == "sleep" ) sa = ScheduleActivity::Sleep;
			else if ( ss == "train" ) sa = ScheduleActivity::Training;

			m_schedule.append( sa );
		}
	}
	else
	{
		for ( int i = 0; i < 24; ++i )
		{
			m_schedule.append( ScheduleActivity::None );
		}
	}

	m_carryBandages = in.value( "CarryBandages" ).toBool();
	m_carryFood     = in.value( "CarryFood" ).toBool();
	m_carryDrinks   = in.value( "CarryDrinks" ).toBool();

	// TODO update carried counts from inventory
	for ( auto item : m_inventoryItems )
	{
		if ( g->inv()->itemSID( item ) == "Bandage" )
		{
			++m_carriedBandages;
		}
		else if ( g->inv()->nutritionalValue( item ) > 0 )
		{
			++m_carriedFood;
		}
		else if ( g->inv()->drinkValue( item ) > 0 )
		{
			++m_carriedDrinks;
		}
	}

	m_leftHandAttackSkill  = getSkillLevel( "Unarmed" );
	m_leftHandAttackValue  = m_leftHandAttackSkill;
	m_rightHandAttackSkill = getSkillLevel( "Unarmed" );
	m_rightHandAttackValue = m_rightHandAttackSkill;

	log( GameState::currentYearAndSeason );
}

void Gnome::serialize( QVariantMap& out )
{
	CanWork::serialize( out );
	////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////

	out.insert( "SkillActive", m_skillActive );
	out.insert( "SkillPriorities", m_skillPriorities );
	//basic needs
	out.insert( "Needs", m_needs );
	out.insert( "Equipment", m_equipment.serialize() );

	out.insert( "Profession", m_profession );

	

	QVariantList vSchedule;
	for( const auto& sa : m_schedule )
	{
		switch ( sa )
		{
			case ScheduleActivity::Eat:
				vSchedule.append( "eat" );
				break;
			case ScheduleActivity::Sleep:
				vSchedule.append( "sleep" );
				break;
			case ScheduleActivity::Training:
				vSchedule.append( "train" );
				break;
			default:
				vSchedule.append( "none" );
		}
	}
	out.insert( "Schedule", vSchedule );

	out.insert( "CarryBandages", m_carryBandages );
	out.insert( "CarryFood", m_carryFood );
	out.insert( "CarryDrinks", m_carryDrinks );
}

Gnome::~Gnome()
{
}

void Gnome::init()
{
	g->w()->insertCreatureAtPosition( m_position, m_id );

	updateSprite();
	initTaskMap();
	loadBehaviorTree( "Gnome" );

	if ( m_btBlackBoard.contains( "State" ) )
	{
		QVariantMap btm = m_btBlackBoard.value( "State" ).toMap();
		if ( m_behaviorTree )
		{
			m_behaviorTree->deserialize( btm );
			m_btBlackBoard.remove( "State" );
		}
	}
}

void Gnome::setConcealed( QString part, bool concealed )
{
	for ( int k = 0; k < m_spriteDef.size(); ++k )
	{
		auto cm = m_spriteDef[k].toMap();
		if ( cm.value( "Part" ).toString() == part )
		{
			cm.insert( "Hidden", concealed );
			m_spriteDef.replace( k, cm );
			break;
		}
	}

	for ( int k = 0; k < m_spriteDefBack.size(); ++k )
	{
		auto cm = m_spriteDefBack[k].toMap();
		if ( cm.value( "Part" ).toString() == part )
		{
			cm.insert( "Hidden", concealed );
			m_spriteDefBack.replace( k, cm );
			break;
		}
	}
}

void Gnome::updateSprite()
{
	m_spriteDef     = createSpriteDef( "Gnome", false );
	m_spriteDefBack = createSpriteDef( "GnomeBack", true );

	g->sf()->setCreatureSprite( m_id, m_spriteDef, m_spriteDefBack, isDead() );
	m_equipmentChanged = true;
	m_renderParamsChanged = true;
}

QVariantList Gnome::createSpriteDef( QString type, bool isBack )
{
	auto parts = DB::selectRows( "Creature_Parts", type );

	QVariantMap ordered;
	for ( auto pm : parts )
	{
		ordered.insert( pm.value( "Order" ).toString(), pm );
	}

	Uniform* uniform = nullptr;
	if ( m_roleID )
	{
		uniform = g->mil()->uniform( m_roleID );
	}

	QVariantList def;
	for ( auto vpm : ordered )
	{
		auto pm = vpm.toMap();

		CreaturePart part = Global::creaturePartLookUp.value( pm.value( "Part" ).toString() );

		QString tint = pm.value( "Tint" ).toString();

		QString bs = pm.value( "BaseSprite" ).toString();

		bool hairConcealed = false;
		if ( m_equipment.head.itemID )
		{
			hairConcealed = true;
		}

		unsigned int item = 0;

		switch ( part )
		{
			case CP_HEAD:
				break;
			case CP_TORSO:
				break;
			case CP_LEFT_ARM:
				break;
			case CP_RIGHT_ARM:
				break;
			case CP_LEFT_HAND:
				break;
			case CP_RIGHT_HAND:
				break;
			case CP_LEFT_LEG:
				break;
			case CP_RIGHT_LEG:
				break;
			case CP_LEFT_FOOT:
				break;
			case CP_RIGHT_FOOT:
				break;

			case CP_HAIR:
				pm.insert( "Tint", m_equipment.hairColor );
				bs = m_equipment.hair;
				pm.insert( "IsHair", true );
				pm.insert( "Hidden", hairConcealed );
				break;
			case CP_FACIAL_HAIR:
				pm.insert( "Tint", m_equipment.hairColor );
				bs = m_equipment.facialHair;
				break;
			case CP_CLOTHING:
				pm.insert( "Tint", m_equipment.shirtColor );
				bs = m_equipment.shirt;
				break;
			case CP_BOOTS:
				break;
			case CP_HAT:
				break;

			case CP_ARMOR_HEAD:
				if ( m_equipment.head.itemID )
				{
					bs = "Gnome";
					bs += g->inv()->itemGroup( m_equipment.head.itemID );
					bs += "HeadArmor";
					hairConcealed = true;
					pm.insert( "Material", m_equipment.head.material );
				}
				break;
			case CP_ARMOR_TORSO:
				if ( m_equipment.chest.itemID )
				{
					bs = "Gnome";
					bs += g->inv()->itemGroup( m_equipment.chest.itemID );
					bs += "ChestArmor";
					pm.insert( "Material", m_equipment.chest.material );
				}
				break;
			case CP_ARMOR_LEFT_ARM:
				if ( m_equipment.arm.itemID )
				{
					bs = "Gnome";
					bs += g->inv()->itemGroup( m_equipment.arm.itemID );
					bs += "LeftArmArmor";
					pm.insert( "Material", m_equipment.arm.material );
				}
				break;
			case CP_ARMOR_RIGHT_ARM:
				if ( m_equipment.arm.itemID )
				{
					bs = "Gnome";
					bs += g->inv()->itemGroup( m_equipment.arm.itemID );
					bs += "RightArmArmor";
					pm.insert( "Material", m_equipment.arm.material );
				}
				break;
			case CP_ARMOR_LEFT_HAND:
				if ( m_equipment.hand.itemID )
				{
					bs = "Gnome";
					bs += g->inv()->itemGroup( m_equipment.hand.itemID );
					bs += "LeftHandArmor";
					pm.insert( "Material", m_equipment.hand.material );
				}
				break;
			case CP_ARMOR_RIGHT_HAND:
				if ( m_equipment.hand.itemID )
				{
					bs = "Gnome";
					bs += g->inv()->itemGroup( m_equipment.hand.itemID );
					bs += "RightHandArmor";
					pm.insert( "Material", m_equipment.hand.material );
				}
				break;
			case CP_ARMOR_LEFT_FOOT:
				if ( m_equipment.foot.itemID )
				{
					bs = "Gnome";
					bs += g->inv()->itemGroup( m_equipment.foot.itemID );
					bs += "LeftFootArmor";
					pm.insert( "Material", m_equipment.foot.material );
				}
				break;
			case CP_ARMOR_RIGHT_FOOT:
				if ( m_equipment.foot.itemID )
				{
					bs = "Gnome";
					bs += g->inv()->itemGroup( m_equipment.foot.itemID );
					bs += "RightFootArmor";
					pm.insert( "Material", m_equipment.foot.material );
				}
				break;
			case CP_LEFT_HAND_HELD:
				if ( m_equipment.leftHandHeld.itemID )
				{
					bs = "Gnome";
					bs += m_equipment.leftHandHeld.item;
					bs += "Left";
					pm.insert( "Material", m_equipment.leftHandHeld.material );
					pm.insert( "HasBase", true );
				}
				break;
			case CP_RIGHT_HAND_HELD:
				if ( m_equipment.rightHandHeld.itemID )
				{
					bs = "Gnome";
					bs += m_equipment.rightHandHeld.item;
					bs += "Right";
					pm.insert( "Material", m_equipment.leftHandHeld.material );
					pm.insert( "HasBase", true );
				}
				break;
			case CP_BACK:
				if ( isBack && m_equipment.back.itemID )
				{
					bs = "Gnome";
					bs += m_equipment.back.item;
					pm.insert( "Material", m_equipment.leftHandHeld.material );
				}
				break;
		}
		

		if ( isBack && !bs.endsWith( "Back" ) )
		{
			bs += "Back";
		}

		pm.insert( "BaseSprite", bs );

		def.append( pm );
	}
	return def;
}

void Gnome::initTaskMap()
{
	using namespace std::placeholders;

	m_behaviors.insert( "IsSleepy", std::bind( &Gnome::conditionIsSleepy, this, _1 ) );
	m_behaviors.insert( "IsHungry", std::bind( &Gnome::conditionIsHungry, this, _1 ) );
	m_behaviors.insert( "IsVeryHungry", std::bind( &Gnome::conditionIsVeryHungry, this, _1 ) );
	m_behaviors.insert( "IsThirsty", std::bind( &Gnome::conditionIsThirsty, this, _1 ) );
	m_behaviors.insert( "IsVeryThirsty", std::bind( &Gnome::conditionIsVeryThirsty, this, _1 ) );
	m_behaviors.insert( "AllItemsInPlaceForJob", std::bind( &Gnome::conditionAllItemsInPlaceForJob, this, _1 ) );
	m_behaviors.insert( "IsButcherJobJob", std::bind( &Gnome::conditionIsButcherJob, this, _1 ) );
	m_behaviors.insert( "AllPickedUp", std::bind( &Gnome::conditionAllPickedUp, this, _1 ) );
	m_behaviors.insert( "IsFull", std::bind( &Gnome::conditionIsFull, this, _1 ) );
	m_behaviors.insert( "IsDrinkFull", std::bind( &Gnome::conditionIsDrinkFull, this, _1 ) );
	m_behaviors.insert( "IsCivilian", std::bind( &Gnome::conditionIsCivilian, this, _1 ) );
	m_behaviors.insert( "Alarm", std::bind( &Gnome::conditionAlarm, this, _1 ) );
	m_behaviors.insert( "IsInSafeRoom", std::bind( &Gnome::conditionIsInSafeRoom, this, _1 ) );

	// combat conditions
	m_behaviors.insert( "TargetAdjacent", std::bind( &Gnome::conditionTargetAdjacent, this, _1 ) );
	m_behaviors.insert( "IsInCombat", std::bind( &Gnome::conditionIsInCombat, this, _1 ) );
	m_behaviors.insert( "IsTrainingTime", std::bind( &Gnome::conditionIsTrainingTime, this, _1 ) );
	m_behaviors.insert( "IsTrainer", std::bind( &Gnome::conditionIsTrainer, this, _1 ) );

	m_behaviors.insert( "HasHuntTarget", std::bind( &Gnome::conditionHasHuntTarget, this, _1 ) );

	m_behaviors.insert( "IsOnMission", std::bind( &Gnome::conditionIsOnMission, this, _1 ) );

	//m_behaviors.insert( "", std::bind( &Gnome::condition, this, _1 ) );

	m_behaviors.insert( "Sleep", std::bind( &Gnome::actionSleep, this, _1 ) );
	m_behaviors.insert( "FindBed", std::bind( &Gnome::actionFindBed, this, _1 ) );

	m_behaviors.insert( "FindFood", std::bind( &Gnome::actionFindFood, this, _1 ) );
	m_behaviors.insert( "FindDining", std::bind( &Gnome::actionFindDining, this, _1 ) );
	m_behaviors.insert( "Eat", std::bind( &Gnome::actionEat, this, _1 ) );
	m_behaviors.insert( "FindDrink", std::bind( &Gnome::actionFindDrink, this, _1 ) );
	m_behaviors.insert( "Drink", std::bind( &Gnome::actionDrink, this, _1 ) );

	m_behaviors.insert( "Move", std::bind( &Gnome::actionMove, this, _1 ) );
	m_behaviors.insert( "RandomMove", std::bind( &Gnome::actionRandomMove, this, _1 ) );

	m_behaviors.insert( "PickUpItem", std::bind( &Gnome::actionPickUpItem, this, _1 ) );

	m_behaviors.insert( "ClaimItems", std::bind( &Gnome::actionClaimItems, this, _1 ) );
	m_behaviors.insert( "DropItem", std::bind( &Gnome::actionDropItem, this, _1 ) );
	m_behaviors.insert( "EquipTool", std::bind( &Gnome::actionEquipTool, this, _1 ) );
	m_behaviors.insert( "FindTool", std::bind( &Gnome::actionFindTool, this, _1 ) );

	m_behaviors.insert( "CheckUniform", std::bind( &Gnome::actionCheckUniform, this, _1 ) );
	m_behaviors.insert( "CheckBandages", std::bind( &Gnome::actionCheckBandages, this, _1 ) );
	m_behaviors.insert( "UniformCleanUp", std::bind( &Gnome::actionUniformCleanUp, this, _1 ) );

	m_behaviors.insert( "GetItemDropPosition", std::bind( &Gnome::actionGetItemDropPosition, this, _1 ) );
	m_behaviors.insert( "GetWorkPosition", std::bind( &Gnome::actionGetWorkPosition, this, _1 ) );
	m_behaviors.insert( "GetSafeRoomPosition", std::bind( &Gnome::actionGetSafeRoomPosition, this, _1 ) );

	m_behaviors.insert( "GetJob", std::bind( &Gnome::actionGetJob, this, _1 ) );
	m_behaviors.insert( "InitJob", std::bind( &Gnome::actionInitJob, this, _1 ) );
	m_behaviors.insert( "FinishJob", std::bind( &Gnome::actionFinishJob, this, _1 ) );
	m_behaviors.insert( "AbortJob", std::bind( &Gnome::actionAbortJob, this, _1 ) );
	m_behaviors.insert( "Work", std::bind( &Gnome::actionWork, this, _1 ) );

	m_behaviors.insert( "InitAnimalJob", std::bind( &Gnome::actionInitAnimalJob, this, _1 ) );
	m_behaviors.insert( "GrabAnimal", std::bind( &Gnome::actionGrabAnimal, this, _1 ) );
	m_behaviors.insert( "ReleaseAnimal", std::bind( &Gnome::actionReleaseAnimal, this, _1 ) );
	m_behaviors.insert( "ButcherAnimal", std::bind( &Gnome::actionButcherAnimal, this, _1 ) );
	m_behaviors.insert( "DyeAnimal", std::bind( &Gnome::actionDyeAnimal, this, _1 ) );
	m_behaviors.insert( "HarvestAnimal", std::bind( &Gnome::actionHarvestAnimal, this, _1 ) );
	m_behaviors.insert( "TameAnimal", std::bind( &Gnome::actionTameAnimal, this, _1 ) );

	m_behaviors.insert( "FinalMoveAnimal", std::bind( &Gnome::actionFinalMoveAnimal, this, _1 ) );

	m_behaviors.insert( "DropAllItems", std::bind( &Gnome::actionDropAllItems, this, _1 ) );
	m_behaviors.insert( "ReturnAlwaysRunning", std::bind( &Gnome::actionAlwaysRunning, this, _1 ) );

	m_behaviors.insert( "GetTarget", std::bind( &Gnome::actionGetTarget, this, _1 ) );
	m_behaviors.insert( "AttackTarget", std::bind( &Gnome::actionAttackTarget, this, _1 ) );
	m_behaviors.insert( "FindTrainingGround", std::bind( &Gnome::actionFindTrainingGround, this, _1 ) );
	m_behaviors.insert( "Train", std::bind( &Gnome::actionTrain, this, _1 ) );
	m_behaviors.insert( "FindTrainerPosition", std::bind( &Gnome::actionFindTrainerPosition, this, _1 ) );
	m_behaviors.insert( "SuperviseTraining", std::bind( &Gnome::actionSuperviseTraining, this, _1 ) );

	m_behaviors.insert( "GetExitPosition", std::bind( &Gnome::actionGetExitPosition, this, _1 ) );
	m_behaviors.insert( "LeaveMap", std::bind( &Gnome::actionLeaveMap, this, _1 ) );
	m_behaviors.insert( "EnterMap", std::bind( &Gnome::actionEnterMap, this, _1 ) );

	m_behaviors.insert( "DoMission", std::bind( &Gnome::actionDoMission, this, _1 ) );
	m_behaviors.insert( "LeaveForMission", std::bind( &Gnome::actionLeaveForMission, this, _1 ) );
	m_behaviors.insert( "ReturnFromMission", std::bind( &Gnome::actionReturnFromMission, this, _1 ) );

	//m_behaviors.insert( "", std::bind( &Gnome::action, this, _1 ) );

	m_taskFunctions.insert( "Deconstruct", std::bind( &Gnome::deconstruct, this ) );

	m_taskFunctions.insert( "MineWall", std::bind( &Gnome::mineWall, this ) );
	m_taskFunctions.insert( "MineFloor", std::bind( &Gnome::mineFloor, this ) );
	m_taskFunctions.insert( "DigHole", std::bind( &Gnome::digHole, this ) );
	m_taskFunctions.insert( "ExplorativeMineWall", std::bind( &Gnome::explorativeMineWall, this ) );
	m_taskFunctions.insert( "RemoveWall", std::bind( &Gnome::removeWall, this ) );
	m_taskFunctions.insert( "RemoveRamp", std::bind( &Gnome::removeRamp, this ) );
	m_taskFunctions.insert( "RemoveFloor", std::bind( &Gnome::removeFloor, this ) );

	m_taskFunctions.insert( "Construct", std::bind( &Gnome::construct, this ) );
	m_taskFunctions.insert( "ConstructAnimate", std::bind( &Gnome::constructAnimate, this ) );
	m_taskFunctions.insert( "ConstructDugStairs", std::bind( &Gnome::constructDugStairs, this ) );
	m_taskFunctions.insert( "ConstructDugRamp", std::bind( &Gnome::constructDugRamp, this ) );

	m_taskFunctions.insert( "CreateItem", std::bind( &Gnome::createItem, this ) );
	m_taskFunctions.insert( "PlantTree", std::bind( &Gnome::plantTree, this ) );
	m_taskFunctions.insert( "FellTree", std::bind( &Gnome::fellTree, this ) );
	m_taskFunctions.insert( "Harvest", std::bind( &Gnome::harvest, this ) );
	m_taskFunctions.insert( "HarvestHay", std::bind( &Gnome::harvestHay, this ) );
	m_taskFunctions.insert( "Plant", std::bind( &Gnome::plant, this ) );
	m_taskFunctions.insert( "RemovePlant", std::bind( &Gnome::removePlant, this ) );
	m_taskFunctions.insert( "Till", std::bind( &Gnome::till, this ) );
	m_taskFunctions.insert( "Craft", std::bind( &Gnome::craft, this ) );

	m_taskFunctions.insert( "Fish", std::bind( &Gnome::fish, this ) );
	m_taskFunctions.insert( "ButcherFish", std::bind( &Gnome::butcherFish, this ) );
	m_taskFunctions.insert( "ButcherCorpse", std::bind( &Gnome::butcherCorpse, this ) );
	m_taskFunctions.insert( "FillTrough", std::bind( &Gnome::fillTrough, this ) );

	m_taskFunctions.insert( "PrepareSpell", std::bind( &Gnome::prepareSpell, this ) );
	m_taskFunctions.insert( "CastSpell", std::bind( &Gnome::castSpell, this ) );
	m_taskFunctions.insert( "CastSpellAnimate", std::bind( &Gnome::castSpellAnimate, this ) );
	m_taskFunctions.insert( "FinishSpell", std::bind( &Gnome::finishSpell, this ) );

	m_taskFunctions.insert( "SwitchMechanism", std::bind( &Gnome::switchMechanism, this ) );
	m_taskFunctions.insert( "InvertMechanism", std::bind( &Gnome::invertMechanism, this ) );
	m_taskFunctions.insert( "Refuel", std::bind( &Gnome::refuel, this ) );
	m_taskFunctions.insert( "Install", std::bind( &Gnome::install, this ) );
	m_taskFunctions.insert( "Uninstall", std::bind( &Gnome::uninstall, this ) );
	m_taskFunctions.insert( "SoundAlarm", std::bind( &Gnome::soundAlarm, this ) );
	m_taskFunctions.insert( "EquipItem", std::bind( &Gnome::equipItem, this ) );
}

void Gnome::addNeed( QString id, int level )
{
	m_needs.insert( id, level );
}

int Gnome::need( QString id )
{
	if ( m_needs.contains( id ) )
	{
		return m_needs[id].toInt();
	}
	return 0;
}

void Gnome::selectProfession( QString profession )
{
	if ( m_profession != profession )
	{
		m_profession = profession;

		clearAllSkills();

		m_skillPriorities = g->gm()->professionSkills( profession );

		for ( auto skill : m_skillPriorities )
		{
			setSkillActive( skill, true );
		}
	}
}

// returns job changed, used to signal for activiti indicator
CreatureTickResult Gnome::onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	processCooldowns( tickNumber );

	if ( !m_isOnMission )
	{
		evalNeeds( seasonChanged, dayChanged, hourChanged, minuteChanged );
	}

	m_jobChanged    = false;
	Position oldPos = m_position;

	if ( checkFloor() )
	{
		m_lastOnTick = tickNumber;
		return CreatureTickResult::NOFLOOR;
	}
	m_anatomy.setFluidLevelonTile( g->w()->fluidLevel( m_position ) );
	if ( m_anatomy.statusChanged() )
	{
		auto status = m_anatomy.status();
		if ( status & AS_DEAD )
		{
			Global::logger().log( LogType::COMBAT, m_name + "died. Bummer!", m_id );
			die();
			// TODO check for other statuses
		}
	}

	if ( isDead() )
	{
		qDebug() << m_name << " expires " << GameState::tick + Global::util->ticksPerDay;
		m_expires    = GameState::tick + Global::util->ticksPerDay * 2;
		m_lastOnTick = tickNumber;
		return CreatureTickResult::DEAD;
	}

	if ( m_job && ( m_job->isAborted() || m_job->isCanceled() ) )
	{
		qDebug() << m_job->type() << " job is canceled";
		cleanUpJob( false );
		m_behaviorTree->halt();
	}

	if ( !m_carryBandages && m_carriedBandages > 0 )
	{
		while ( m_carriedBandages > 0 )
		{
			if ( m_inventoryItems.size() > 0 )
			{
				auto item = m_inventoryItems.takeFirst();
				if ( g->inv()->itemSID( item ) == "Bandage" )
				{
					g->inv()->setInJob( item, 0 );
					g->inv()->putDownItem( item, m_position );
					--m_carriedBandages;
				}
				else
				{
					m_carriedItems.append( item );
				}
			}
			else
			{
				m_carriedBandages = 0;
				m_carriedFood     = 0;
				m_carriedDrinks   = 0;
			}
		}
	}
	if ( !m_carryFood && m_carriedFood > 0 )
	{
		while ( m_carriedFood > 0 )
		{
			if ( m_inventoryItems.size() > 0 )
			{
				auto item = m_inventoryItems.takeFirst();
				if ( g->inv()->nutritionalValue( item ) > 0 )
				{
					g->inv()->setInJob( item, 0 );
					g->inv()->putDownItem( item, m_position );
					--m_carriedFood;
				}
				else
				{
					m_carriedItems.append( item );
				}
			}
			else
			{
				m_carriedBandages = 0;
				m_carriedFood     = 0;
				m_carriedDrinks   = 0;
			}
		}
	}
	if ( !m_carryDrinks && m_carriedDrinks > 0 )
	{
		while ( m_carriedDrinks > 0 )
		{
			if ( m_inventoryItems.size() > 0 )
			{
				auto item = m_inventoryItems.takeFirst();
				if ( g->inv()->drinkValue( item ) > 0 )
				{
					g->inv()->setInJob( item, 0 );
					g->inv()->putDownItem( item, m_position );
					--m_carriedDrinks;
				}
				else
				{
					m_carriedItems.append( item );
				}
			}
			else
			{
				m_carriedBandages = 0;
				m_carriedFood     = 0;
				m_carriedDrinks   = 0;
			}
		}
	}

#ifdef CHECKTIME
	QElapsedTimer timer;
	timer.start();
	if ( m_behaviorTree )
	{
		m_behaviorTree->tick();
	}
	auto elapsed = timer.elapsed();
	if ( elapsed > 100 )
	{
		qDebug() << m_name << "just needed" << elapsed << "ms for bt tick";
		Global::cfg->set( "Pause", true );
	}
#else
	if ( m_behaviorTree )
	{
		if ( Global::debugMode )
		{
			QElapsedTimer et;
			et.start();

			m_behaviorTree->tick();

			auto ela = et.elapsed();
			if ( ela > 30 )
			{
				if ( m_job )
				{
					qDebug() << "LOOPTIME" << m_name << ela << "ms"
							 << "job:" << m_job->type() << m_job->pos().toString();
				}
			}
		}
		else
		{
			m_behaviorTree->tick();
		}
	}
#endif

	move( oldPos );
	updateLight( oldPos, m_position );

	m_lastOnTick = tickNumber;

	if ( m_jobChanged )
	{
		return CreatureTickResult::JOBCHANGED;
	}
	return CreatureTickResult::OK;
}

void Gnome::die()
{
	Creature::die();
	cleanUpJob( false );
	for ( Workshop* w : g->wsm()->workshops() )
	{
		if ( w->assignedGnome() == id() )
		{
			w->assignGnome( 0 );
		}
	}
}

bool Gnome::checkFloor()
{
	FloorType ft = g->w()->floorType( m_position );
	if ( ft == FloorType::FT_NOFLOOR )
	{
		if ( m_job )
		{
			m_job->setAborted( true );
		}

		Position oneDown( m_position.x, m_position.y, m_position.z - 1 );
		forceMove( oneDown );
		g->w()->discover( oneDown );
		return true;
	}
	return false;
}

void Gnome::setJobAborted( QString caller )
{
	if ( m_job )
	{
		m_job->setAborted( true );
		log( "GnomeManager aborted my job." );
	}
}

bool Gnome::evalNeeds( bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	if ( seasonChanged )
	{
		//log( GameState::CurrentYearAndSeason" ).toString() );
	}

	bool needAction        = false;
	unsigned char priority = 0;
	QString action;
	QString needKey;

	if ( minuteChanged )
	{
		for ( auto need : Global::needIDs )
		{
			//update need values
			float decay  = Global::needDecays.value( need );
			float oldVal = m_needs[need].toFloat();
			float newVal = oldVal + decay;

			m_needs.insert( need, newVal );

			if ( need == "Hunger" || need == "Thirst" )
			{
				if ( newVal < -100 )
				{
					m_thoughtBubble = "";
					die();
					if ( need == "Hunger" )
					{
						log( "Starved to death." );
					}
					else if ( need == "Thirst" )
					{
						log( "Died from thirst." );
					}
				}
			}
		}
	}
	return true;
}

void Gnome::updateLight( Position oldPos, Position newPos )
{
#if 0
	if( oldPos == newPos )
	{
		return;
	}
	World& w = Global::w();
	int intensity = 10;
	w.putLight( oldPos, -intensity );
	w.putLight( oldPos.x - 1, oldPos.y, oldPos.z, -intensity );
	w.putLight( oldPos.x, oldPos.y - 1, oldPos.z, -intensity );
	w.putLight( oldPos.x + 1, oldPos.y, oldPos.z, -intensity );
	w.putLight( oldPos.x, oldPos.y + 1, oldPos.z, -intensity );

	w.putLight( newPos, intensity );
	w.putLight( newPos.x - 1, newPos.y, newPos.z, intensity );
	w.putLight( newPos.x, newPos.y - 1, newPos.z, intensity );
	w.putLight( newPos.x + 1, newPos.y, newPos.z, intensity );
	w.putLight( newPos.x, newPos.y + 1, newPos.z, intensity );
#endif
}

QString Gnome::getActivity()
{
	if ( m_job )
	{
		return m_job->requiredSkill();
	}
	else
	{
		return "idle";
	}
}

void Gnome::setOwnedRoom( unsigned int id )
{
	m_equipment.roomID = id;
	if ( id )
	{
		log( GameState::currentDayTime + ": I got a new room." );
	}
	else
	{
		log( GameState::currentDayTime + ": They took my room away." );
	}
}

unsigned int Gnome::ownedRoom()
{
	return m_equipment.roomID;
}

ScheduleActivity Gnome::schedule( unsigned char hour )
{
	if ( hour < 24 && m_schedule.size() == 24 )
	{
		return m_schedule[hour];
	}
	return ScheduleActivity::None;
}

void Gnome::setSchedule( unsigned char hour, ScheduleActivity activity )
{
	if ( hour < 24 && m_schedule.size() == 24 )
	{
		m_schedule[hour] = activity;
	}
	else
	{
		m_schedule.clear();
		for ( int i = 0; i < 24; ++i )
		{
			m_schedule.append( ScheduleActivity::None );
		}
		m_schedule[hour] = activity;
	}
}

void Gnome::updateMoveSpeed()
{
	int skill   = getSkillLevel( "Hauling" );
	int speed   = DB::execQuery( "SELECT Speed FROM MoveSpeed WHERE CREATURE = \"Gnome\" AND Skill = \"" + QString::number( skill ) + "\"" ).toInt();
	m_moveSpeed = qMax( 30, speed );
}

void Gnome::assignWorkshop( unsigned int id )
{
	m_assignedWorkshop = id;
}

Equipment Gnome::equipment()
{
	return m_equipment;
}

QString Gnome::rightHandItem()
{
	return m_equipment.rightHandHeld.material + " " + m_equipment.rightHandHeld.item;
}

QString Gnome::rightHandAttackSkill()
{
	return QString::number( m_rightHandAttackSkill );
}

QString Gnome::rightHandAttackValue()
{
	return QString::number( m_rightHandAttackValue );
}

void Gnome::updateAttackValues()
{
	m_leftHandAttackSkill  = getSkillLevel( "Unarmed" );
	m_leftHandAttackValue  = attribute( "Str" );
	m_rightHandAttackSkill = getSkillLevel( "Unarmed" );
	m_rightHandAttackValue = attribute( "Str" );
}

bool Gnome::attack( DamageType dt, AnatomyHeight da, int skill, int strength, Position sourcePos, unsigned int attackerID )
{
	srand( std::chrono::system_clock::now().time_since_epoch().count() );

	// from which side is the attack coming
	AnatomySide ds = AS_CENTER;
	switch ( m_facing )
	{
		case 0:
			if ( m_position.x < sourcePos.x )
				ds = AS_FRONT;
			else if ( m_position.x > sourcePos.x )
				ds = AS_BACK;
			if ( m_position.y < sourcePos.y )
				ds = AnatomySide( ds | AS_RIGHT );
			else if ( m_position.y > sourcePos.y )
				ds = AnatomySide( ds | AS_LEFT );
			break;
		case 1:
			if ( m_position.x < sourcePos.x )
				ds = AS_LEFT;
			else if ( m_position.x > sourcePos.x )
				ds = AS_RIGHT;
			if ( m_position.y < sourcePos.y )
				ds = AnatomySide( ds | AS_FRONT );
			else if ( m_position.y > sourcePos.y )
				ds = AnatomySide( ds | AS_BACK );
			break;
		case 2:
			if ( m_position.x > sourcePos.x )
				ds = AS_FRONT;
			else if ( m_position.x < sourcePos.x )
				ds = AS_BACK;
			if ( m_position.y > sourcePos.y )
				ds = AnatomySide( ds | AS_RIGHT );
			else if ( m_position.y < sourcePos.y )
				ds = AnatomySide( ds | AS_LEFT );
			break;
		case 3:
			if ( m_position.x > sourcePos.x )
				ds = AS_LEFT;
			else if ( m_position.x < sourcePos.x )
				ds = AS_RIGHT;
			if ( m_position.y > sourcePos.y )
				ds = AnatomySide( ds | AS_FRONT );
			else if ( m_position.y < sourcePos.y )
				ds = AnatomySide( ds | AS_BACK );
			break;
	}
	// attacker skill vs our dodge or block chance

	int dodge = getSkillLevel( "Dodge" );
	if ( ds & AS_BACK )
	{
		skill *= 1.5;
	}

	bool hit = skill >= dodge;
	if ( dodge > skill )
	{
		int diff = dodge - skill;
		diff     = qMax( 5, 20 - diff );
		hit |= rand() % 100 > diff;
	}

	if ( hit ) // check block
	{
	}

	if ( hit )
	{
		Global::logger().log( LogType::COMBAT, m_name + " took " + QString::number( strength ) + " damage.", m_id );
		m_anatomy.damage( &m_equipment, dt, da, ds, strength );
	}
	else
	{
		Global::logger().log( LogType::COMBAT, m_name + " dogded the attack. Skill:" + QString::number( skill ) + " Dodge: " + QString::number( dodge ), m_id );
	}

	bool aeExists = false;
	for ( auto& ae : m_aggroList )
	{
		if ( ae.id == attackerID )
		{
			ae.aggro += strength;
			aeExists = true;
			break;
		}
	}
	if ( !aeExists )
	{
		AggroEntry newAE { strength, attackerID };
		m_aggroList.append( newAE );
	}

	return true;
}

void Gnome::setAllowedCarryItems( bool bandages, bool food, bool drinks )
{
	m_carryBandages = bandages;
	m_carryFood     = food;
	m_carryDrinks   = drinks;
}