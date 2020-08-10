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
#include "monster.h"

#include "../base/db.h"
#include "../base/global.h"
#include "../base/logger.h"
#include "../base/priorityqueue.h"
#include "../base/util.h"
#include "../game/creaturemanager.h"
#include "../game/gnomemanager.h"
#include "../game/inventory.h"
#include "../gfx/sprite.h"
#include "../gfx/spritefactory.h"
#include "world.h"

#include <QDebug>

Monster::Monster( QString species, int level, Position& pos, Gender gender ) :
	Creature( pos, species, gender, species )
{
	m_type    = CreatureType::MONSTER;

	QVariantMap mvm = DB::selectRow( "Monsters", species );

	m_btName = mvm.value( "BehaviorTree" ).toString();

	updateSprite();

	m_anatomy.init( "Humanoid" );
}

Monster::Monster( QVariantMap in ) :
	Creature( in ),
	m_level( in.value( "Level" ).toInt() )
{
	m_type = CreatureType::MONSTER;

	updateSprite();
}

void Monster::serialize( QVariantMap& out )
{
	// animal
	out.insert( "Level", m_level );
	Creature::serialize( out );
}

Monster::~Monster()
{
}

void Monster::init()
{
	Global::w().insertCreatureAtPosition( m_position, m_id );

	initTaskMap();
	loadBehaviorTree( m_btName );
	generateAggroList();

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

void Monster::updateSprite()
{
	auto parts = DB::selectRows( "Creature_Parts", m_species );

	QVariantMap ordered;
	for ( auto pm : parts )
	{
		ordered.insert( pm.value( "Order" ).toString(), pm );
	}

	QVariantMap randTemp;

	QVariantList def;
	for ( auto vpm : ordered )
	{
		auto pm = vpm.toMap();
		if ( pm.value( "BaseSprite" ).toString().startsWith( "#" ) )
		{
			QString bsa = pm.value( "BaseSprite" ).toString();
			bsa.remove( 0, 1 );
			auto bsl = bsa.split( "|" );

			srand( std::chrono::system_clock::now().time_since_epoch().count() );
			int rn = rand() % bsl.size();
			randTemp.insert( pm.value( "Part" ).toString() + "Rand", rn );
			pm.insert( "BaseSprite", bsl[rn] );
		}
		def.append( pm );
	}

	auto partsBack = DB::selectRows( "Creature_Parts", m_species + "Back" );

	QVariantMap orderedBack;
	for ( auto pm : partsBack )
	{
		orderedBack.insert( pm.value( "Order" ).toString(), pm );
	}

	QVariantList defBack;
	for ( auto vpm : orderedBack )
	{
		auto pm = vpm.toMap();
		if ( pm.value( "BaseSprite" ).toString().startsWith( "#" ) )
		{
			QString bsa = pm.value( "BaseSprite" ).toString();
			bsa.remove( 0, 1 );
			auto bsl = bsa.split( "|" );

			srand( std::chrono::system_clock::now().time_since_epoch().count() );
			int rn = randTemp.value( pm.value( "Part" ).toString() + "Rand" ).toInt();
			pm.insert( "BaseSprite", bsl[rn] + "Back" );
		}
		defBack.append( pm );
	}

	m_spriteID = Global::sf().setCreatureSprite( m_id, def, defBack, m_isDead )->uID;
}

void Monster::updateMoveSpeed()
{
	//TODO add monster values to movespeed table
	//int skill = getSkillLevel( "Hauling" );
	int speed   = 50; //DB::execQuery( "SELECT Speed FROM MoveSpeed WHERE CREATURE = \"Gnome\" AND SkillLevel = \"" + QString::number( skill ) + "\"" ).toInt();
	m_moveSpeed = qMax( 30, speed );
}

void Monster::initTaskMap()
{
	using namespace std::placeholders;
	//m_behaviors.insert( "", std::bind( &Monster::condition, this, _1 ) );
	//m_behaviors.insert( "", std::bind( &Monster::action, this, _1 ) );
	m_behaviors.insert( "IsMale", std::bind( &Monster::conditionIsMale, this, _1 ) );
	m_behaviors.insert( "IsFemale", std::bind( &Monster::conditionIsFemale, this, _1 ) );
	m_behaviors.insert( "IsDay", std::bind( &Monster::conditionIsDay, this, _1 ) );
	m_behaviors.insert( "IsNight", std::bind( &Monster::conditionIsNight, this, _1 ) );

	m_behaviors.insert( "TargetAdjacent", std::bind( &Monster::conditionTargetAdjacent, this, _1 ) );

	m_behaviors.insert( "RandomMove", std::bind( &Monster::actionRandomMove, this, _1 ) );
	m_behaviors.insert( "Move", std::bind( &Monster::actionMove, this, _1 ) );
	m_behaviors.insert( "GetTarget", std::bind( &Monster::actionGetTarget, this, _1 ) );
	m_behaviors.insert( "AttackTarget", std::bind( &Monster::actionAttackTarget, this, _1 ) );
}

void Monster::generateAggroList()
{
	m_aggroList.clear();
	srand( std::chrono::system_clock::now().time_since_epoch().count() );
	for ( auto& g : Global::gm().gnomes() )
	{
		if ( !g->isOnMission() )
		{
			AggroEntry ae { rand() % 100, g->id() };
			m_aggroList.append( ae );
		}
	}
	std::sort( m_aggroList.begin(), m_aggroList.end() );
}

CreatureTickResult Monster::onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	processCooldowns( tickNumber );

	m_anatomy.setFluidLevelonTile( Global::w().fluidLevel( m_position ) );

	if ( m_anatomy.statusChanged() )
	{
		auto status = m_anatomy.status();
		if ( status & AS_DEAD )
		{
			Global::logger().log( LogType::COMBAT, "The " + m_name + " died. Bummer!", m_id );
			m_isDead = true;
			// TODO check for other statuses
		}
	}

	if ( m_toDestroy )
	{
		m_lastOnTick = tickNumber;
		return CreatureTickResult::TODESTROY;
	}
	if ( m_isDead )
	{
		m_lastOnTick = tickNumber;
		return CreatureTickResult::DEAD;
	}

	Position oldPos = m_position;

	if ( m_followID && !m_followPosition.isZero() && m_position != m_followPosition )
	{
		m_position = m_followPosition;
		move( oldPos );
		m_lastOnTick = tickNumber;
		return CreatureTickResult::OK;
	}

	setThoughtBubble( "" );

	if ( m_aggroList.isEmpty() )
	{
		generateAggroList();
	}

	if ( m_behaviorTree )
	{
		m_behaviorTree->tick();
	}

	move( oldPos );

	m_lastOnTick = tickNumber;

	return CreatureTickResult::OK;
}

BT_RESULT Monster::actionMove( bool halt )
{
	if ( halt )
	{
		m_currentPath.clear();
		return BT_RESULT::IDLE;
	}

	// monster has a path, move on path and return
	if ( !m_currentPath.empty() )
	{
		if ( conditionTargetAdjacent( false ) == BT_RESULT::SUCCESS )
		{
			m_currentPath.clear();
			return BT_RESULT::SUCCESS;
		}

		if ( !moveOnPath() )
		{
			if ( !m_aggroList.isEmpty() )
			{
				auto curTarget = m_aggroList.takeFirst();
				m_aggroList.append( curTarget );
			}

			return BT_RESULT::FAILURE;
		}
		return BT_RESULT::RUNNING;
	}

	Position targetPos = currentTarget();

	// check if we are already on the tile
	if ( m_position == targetPos )
	{
		return BT_RESULT::SUCCESS;
	}

	PathFinderResult pfr = PathFinder::getInstance().getPath( m_id, m_position, targetPos, m_ignoreNoPass, m_currentPath );
	switch ( pfr )
	{
		case PathFinderResult::NoConnection:
			return BT_RESULT::FAILURE;
		case PathFinderResult::Running:
		case PathFinderResult::FoundPath:
			return BT_RESULT::RUNNING;
	}
	return BT_RESULT::RUNNING;
}

BT_RESULT Monster::actionAttackTarget( bool halt )
{
	Creature* creature = Global::gm().gnome( m_currentAttackTarget );
	if ( creature && !creature->isDead() )
	{
		m_facing = getFacing( m_position, creature->getPos() );

		if ( m_rightHandCooldown <= 0 )
		{
			Global::logger().log( LogType::COMBAT, "The goblin attacks " + creature->name(), m_id );
			int skill    = getSkillLevel( "Unarmed" );
			int strength = attribute( "Str" );
			creature->attack( DT_BLUNT, m_anatomy.randomAttackHeight(), skill, qMin( 5, strength ), m_position, m_id );
			m_rightHandCooldown = qMax( 5, 20 - m_rightHandAttackSkill );
		}
		return BT_RESULT::RUNNING;
	}
	m_currentAttackTarget = 0;

	return BT_RESULT::FAILURE;
}

BT_RESULT Monster::actionGetTarget( bool halt )
{
	if ( m_aggroList.size() )
	{
		unsigned int targetID = m_aggroList.first().id;
		Creature* creature    = Global::gm().gnome( targetID );
		if ( creature && !creature->isDead() )
		{
			m_currentAttackTarget = targetID;
			setCurrentTarget( creature->getPos() );
			return BT_RESULT::SUCCESS;
		}
		else
		{
			m_aggroList.removeFirst();
			return BT_RESULT::FAILURE;
		}
	}
	else
	{
		//TODO even though aggro list is empty, check for other threats around
	}
	return BT_RESULT::FAILURE;
}

bool Monster::attack( DamageType dt, AnatomyHeight da, int skill, int strength, Position sourcePos, unsigned int attackerID )
{
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
		srand( std::chrono::system_clock::now().time_since_epoch().count() );
		int diff = dodge - skill;
		diff     = qMax( 5, 20 - diff );
		hit |= rand() % 100 > diff;
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