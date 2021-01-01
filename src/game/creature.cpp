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
#include "creature.h"
#include "../game/game.h"

#include "../base/behaviortree/bt_factory.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/logger.h"
#include "../base/util.h"
#include "../game/creaturemanager.h"
#include "../game/eventmanager.h"
#include "../game/gnomemanager.h"
#include "../game/inventory.h"
#include "../game/room.h"
#include "../game/roommanager.h"
#include "../game/world.h"

#include <QDebug>

Creature::Creature( const Position& pos, QString name, Gender gender, QString species, Game* game ) :
	g( game ),
	Object( pos ),
	m_name( name ),
	m_gender( gender ),
	m_species( species ),
	m_moveDelay( 250 ),
	m_moveCooldown( m_moveDelay ),
	m_moveSpeed( 30 ),
	m_facing( 0 )
{
}

Creature::Creature( QVariantMap in, Game* game ) :
	g( game ),
	Object( in ),
	m_attributes( in.value( "Attributes" ).toMap() ),
	m_skills( in.value( "Skills" ).toMap() ),
	//QMap<QString, unsigned int> m_spriteUIDs;
	m_spriteUIDs( in.value( "spritUIDs" ).toMap() ),
	m_spriteDef( in.value( "SpriteDef" ).toList() ),
	m_spriteDefBack( in.value( "SpriteDefBack" ).toList() ),
	//float m_moveDelay;
	//m_moveDelay( in.value( "MoveDelay" ).toFloat() ),
	m_moveDelay( 250 ),
	m_moveCooldown( in.value( "MoveCooldown" ).toFloat() ),
	//float m_moveSpeed;
	m_moveSpeed( in.value( "Speed" ).toFloat() ),
	//quint8 m_facing;
	m_facing( in.value( "Orientation" ).value<quint8>() ),
	//unsigned int m_followID = 0;
	m_followID( in.value( "FollowID" ).toUInt() ),
	//Position m_followPosition;
	m_followPosition( in.value( "FollowPosition" ) ),
	//Gender m_gender = UNDEFINED;
	m_gender( (Gender)in.value( "Gender" ).value<quint8>() ),
	m_species( in.value( "Species" ).toString() ),
	//QString m_name = "not_initialized";
	m_name( in.value( "Name" ).toString() ),
	//bool m_immobile = false;
	m_immobile( in.value( "Immobile" ).toBool() ),
	//bool m_renderParamsChanged = true;
	m_renderParamsChanged( in.value( "rpc" ).toBool() ),
	//QString m_currentAction = "idle";
	m_currentAction( in.value( "CurrentAction" ).toString() ),
	//int m_state = 0;
	m_state( in.value( "State" ).toInt() ),
	//quint64 m_stateChangeTick = 0;
	m_stateChangeTick( in.value( "sct" ).value<quint64>() ),
	//bool m_isDead = false;
	m_isDead( in.value( "IsDead" ).toBool() ),
	//bool m_toDestroy = false;
	m_toDestroy( in.value( "ToDestroy" ).toBool() ),
	//unsigned char m_type = CreatureType::UNDEFINED
	m_type( (CreatureType)in.value( "Type" ).value<quint8>() ),
	m_ignoreNoPass( in.value( "IgnoreNoPass" ).toBool() ),
	m_lightIntensity( in.value( "LightIntensity" ).toInt() ),
	m_roleID( in.value( "Role" ).toUInt() ),

	m_currentTargetPosition( Position( in.value( "CurrentTargetPos" ) ) ),

	m_facingAfterMove( in.value( "FacingAfterMove" ).toUInt() ),

	m_expires( in.value( "Expires" ).value<quint64>() ),

	m_currentAttackTarget( in.value( "CurrentAttackTarget" ).toUInt() ),

	m_goneOffMap( in.value( "GoneOffMap" ).toBool() ),

	//combat variables
	m_lastOnTick( in.value( "LastonTick" ).value<quint64>() ),
	m_globalCooldown( in.value( "globalCooldown" ).toInt() ),
	m_kickCooldown( in.value( "kickCooldown" ).toInt() ),
	m_leftHandCooldown( in.value( "leftHandCooldown" ).toInt() ),
	m_rightHandCooldown( in.value( "rightHandCooldown" ).toInt() ),
	m_specialAttackCoolDown( in.value( "specialAttackCoolDown" ).toInt() ),
	m_jobCooldown( in.value( "jobCoolDown" ).toInt() ),

	m_leftHandAttackValue( in.value( "leftHandAttackValue" ).toInt() ),
	m_leftHandAttackSkill( in.value( "leftHandAttackSkill" ).toInt() ),
	m_rightHandAttackValue( in.value( "rightHandAttackValue" ).toInt() ),
	m_rightHandAttackSkill( in.value( "rightHandAttackSkill" ).toInt() ),

	m_leftHandArmed( in.value( "leftHandArmed" ).toBool() ),
	m_rightHandArmed( in.value( "rightHandArmed" ).toBool() ),
	m_leftHandHasWeapon( in.value( "leftHandHasWeapon" ).toBool() ),

	m_isOnMission( in.value( "IsOnMission" ).toBool() ),
	m_mission( in.value( "Mission" ).toUInt() ),
	m_nextCheckTick( in.value( "NextCheckTick" ).value<quint64>() ),

	m_btName( in.value( "BTName" ).toString() ),

	m_btBlackBoard( in.value( "BTBlackBoard" ).toMap() )
{
	m_currentPath.clear();

	for ( auto cpp : in.value( "CurrentPath" ).toList() )
	{
		m_currentPath.push_back( Position( cpp ) );
	}

	if ( in.contains( "BehaviorTreeState" ) )
	{
		m_btBlackBoard.insert( "State", in.value( "BehaviorTreeState" ) );
	}
	if ( in.contains( "Anatomy" ) )
	{
		m_anatomy.deserialize( in.value( "Anatomy" ).toMap() );
	}
	if ( in.contains( "ClaimedItems" ) )
	{
		m_claimedItems = Global::util->variantList2UInt( in.value( "ClaimedItems" ).toList() );
	}
	if ( in.contains( "CarriedItems" ) )
	{
		m_carriedItems = Global::util->variantList2UInt( in.value( "CarriedItems" ).toList() );
	}
	if ( in.contains( "InventoryItems" ) )
	{
		m_inventoryItems = Global::util->variantList2UInt( in.value( "InventoryItems" ).toList() );
	}
	if( m_lightIntensity )
	{
		g->w()->addLight( m_id, m_position, m_lightIntensity );
	}
}

void Creature::serialize( QVariantMap& out ) const
{
	out.insert( "Species", m_species );
	//std::vector<Position>m_currentPath;
	if ( m_currentPath.size() )
	{
		QVariantList curPa;
		for ( auto cpp : m_currentPath )
		{
			curPa.append( cpp.toString() );
		}
		out.insert( "CurrentPath", curPa );
	}
	out.insert( "Attributes", m_attributes );
	out.insert( "Skills", m_skills );

	out.insert( "BTBlackBoard", m_btBlackBoard );

	//QMap<QString, unsigned int> m_spriteUIDs;
	out.insert( "spritUIDs", m_spriteUIDs );
	out.insert( "SpriteDef", m_spriteDef );
	out.insert( "SpriteDefBack", m_spriteDefBack );
	//float m_moveDelay;
	out.insert( "MoveDelay", m_moveDelay );
	out.insert( "MoveCooldown", m_moveCooldown );
	//float m_moveSpeed;
	out.insert( "Speed", m_moveSpeed );
	//quint8 m_facing;
	out.insert( "Orientation", m_facing );
	//unsigned int m_followID = 0;
	out.insert( "FollowID", m_followID ),
	//Position m_followPosition;
	out.insert( "FollowPosition", m_followPosition.toString() ),
	//Gender m_gender = UNDEFINED;
	out.insert( "Gender", (unsigned char)m_gender );
	//QString m_name = "not_initialized";
	out.insert( "Name", m_name );
	//bool m_immobile = false;
	out.insert( "Immobile", m_immobile );
	//bool m_renderParamsChanged = true;
	out.insert( "rpc", m_renderParamsChanged );
	//QString m_currentAction = "idle";
	out.insert( "CurrentAction", m_currentAction );
	//int m_state = 0;
	out.insert( "State", m_state );
	//quint64 m_stateChangeTick = 0;
	out.insert( "sct", m_stateChangeTick );

	//bool m_isDead = false;
	out.insert( "IsDead", m_isDead );
	//bool m_toDestroy = false;
	out.insert( "ToDestroy", m_toDestroy );

	out.insert( "Type", (unsigned char)m_type );

	out.insert( "IgnoreNoPass", m_ignoreNoPass );
	out.insert( "Role", m_roleID );

	if ( m_behaviorTree )
	{
		out.insert( "BehaviorTreeState", m_behaviorTree->serialize() );
	}

	out.insert( "Anatomy", m_anatomy.serialize() );

	out.insert( "LightIntensity", m_lightIntensity );

	out.insert( "CurrentTargetPos", m_currentTargetPosition.toString() );

	out.insert( "CurrentAttackTarget", m_currentAttackTarget );

	out.insert( "FacingAfterMove", m_facingAfterMove );
	out.insert( "Expires", m_expires );

	out.insert( "GoneOffMap", m_goneOffMap );

	if ( m_claimedItems.size() )
	{
		out.insert( "ClaimedItems", Global::util->uintList2Variant( m_claimedItems ) );
	}
	if ( m_carriedItems.size() )
	{
		out.insert( "CarriedItems", Global::util->uintList2Variant( m_carriedItems ) );
	}
	if ( m_inventoryItems.size() )
	{
		out.insert( "InventoryItems", Global::util->uintList2Variant( m_inventoryItems ) );
	}

	//combat variables
	out.insert( "LastOnTick", m_lastOnTick );
	out.insert( "globalCooldown", m_globalCooldown );
	out.insert( "kickCooldown", m_kickCooldown );
	out.insert( "leftHandCooldown", m_leftHandCooldown );
	out.insert( "rightHandCooldown", m_rightHandCooldown );
	out.insert( "specialAttackCoolDown", m_specialAttackCoolDown );
	out.insert( "jobCoolDown", m_jobCooldown );

	out.insert( "leftHandAttackValue", m_leftHandAttackValue );
	out.insert( "leftHandAttackSkill", m_leftHandAttackSkill );
	out.insert( "rightHandAttackValue", m_rightHandAttackValue );
	out.insert( "rightHandAttackSkill", m_rightHandAttackSkill );

	out.insert( "leftHandArmed", m_leftHandArmed );
	out.insert( "rightHandArmed", m_rightHandArmed );
	out.insert( "leftHandHasWeapon", m_leftHandHasWeapon );

	out.insert( "IsOnMission", m_isOnMission );
	out.insert( "Mission", m_mission );
	out.insert( "NextCheckTick", m_nextCheckTick );

	out.insert( "BTName", m_btName );

	Object::serialize( out );
}

Creature::~Creature()
{
}

void Creature::setRole( unsigned int roleID )
{
	m_roleID = roleID;
}

void Creature::processCooldowns( quint64 tickNumber )
{
	int diff = tickNumber - m_lastOnTick;

	m_moveCooldown -= diff * m_moveSpeed;

	m_globalCooldown -= diff;
	m_kickCooldown -= diff;
	m_biteCooldown -= diff;
	m_leftHandCooldown -= diff;
	m_rightHandCooldown -= diff;
	m_specialAttackCoolDown -= diff;
	m_jobCooldown -= diff;
}

void Creature::loadBehaviorTree( QString id )
{
	m_behaviorTree.reset( BT_Factory::load( id, m_behaviors, m_btBlackBoard ) );

	if ( !m_behaviorTree )
	{
		qCritical() << "failed to load behavior tree!" << id;
	}
}

void Creature::addAttribute( QString id, int level )
{
	m_attributes.insert( id, level );
}

int Creature::attribute( QString id ) const
{
	if ( m_attributes.contains( id ) )
	{
		return m_attributes[id].toInt();
	}
	return 0;
}

void Creature::addSkill( QString id, int level )
{
	m_skills.insert( id, level );
	m_skillActive.insert( id, false );
}

int Creature::getSkillLevel( QString id ) const
{
	if ( m_skills.contains( id ) )
	{
		return Global::util->reverseFib( m_skills.value( id ).toInt() );
	}
	return -1;
}

int Creature::getSkillXP( QString id ) const
{
	if ( m_skills.contains( id ) )
	{
		return m_skills.value( id ).toInt();
	}
	return -1;
}

void Creature::setSkillLevel( QString id, int level )
{
	m_skills[id] = level;
}

void Creature::forceMove( const Position& to )
{
	//qDebug() << name() << " force move from " << m_position.toString() << " to " << to.toString();
	g->w()->removeCreatureFromPosition( m_position, m_id );
	m_position = to;
	g->w()->insertCreatureAtPosition( m_position, m_id );
}

void Creature::randomMove()
{
	if ( m_immobile )
		return;
	m_moveCooldown -= m_moveSpeed;

	if ( m_moveCooldown <= 0 )
	{
		bool move = !( rand() % 25 );
		Position newPos;
		Position testPos;
		qint8 newFacing = -1;
		if ( move )
		{
			if ( m_aquatic )
			{
				int dir = rand() % 6;

				switch ( dir )
				{
					case 0:
					{
						testPos = m_position.westOf();
						if ( ( g->w()->getTileFlag( testPos ) & TileFlag::TF_WATER ) || g->w()->fluidLevel( testPos ) > 6 )
						{
							newPos    = testPos;
							newFacing = 2;
						}
					}
					break;
					case 1:
					{
						testPos = m_position.eastOf();
						if ( ( g->w()->getTileFlag( testPos ) & TileFlag::TF_WATER ) || g->w()->fluidLevel( testPos ) > 6 )
						{
							newPos    = testPos;
							newFacing = 0;
						}
					}
					break;
					case 2:
					{
						testPos = m_position.northOf();
						if ( ( g->w()->getTileFlag( testPos ) & TileFlag::TF_WATER ) || g->w()->fluidLevel( testPos ) > 6 )
						{
							newPos    = testPos;
							newFacing = 3;
						}
					}
					break;
					case 3:
					{
						testPos = m_position.southOf();
						if ( ( g->w()->getTileFlag( testPos ) & TileFlag::TF_WATER ) || g->w()->fluidLevel( testPos ) > 6 )
						{
							newPos    = testPos;
							newFacing = 1;
						}
					}
					break;
					case 4:
					{
						testPos = m_position.aboveOf();
						if ( ( g->w()->getTileFlag( testPos ) & TileFlag::TF_WATER ) || g->w()->fluidLevel( testPos ) > 6 )
						{
							newPos    = testPos;
							newFacing = m_facing;
						}
					}
					break;
					case 5:
					{
						testPos = m_position.belowOf();
						if ( ( g->w()->getTileFlag( testPos ) & TileFlag::TF_WATER ) || g->w()->fluidLevel( testPos ) > 6 )
						{
							newPos    = testPos;
							newFacing = m_facing;
						}
					}
					break;
				}

				if ( newFacing != -1 )
				{
					m_position = newPos;
					m_facing   = newFacing;
				}
			}
			else
			{
				auto neighbors = g->w()->connectedNeighbors( m_position );
				if ( neighbors.size() > 0 )
				{
					int dir   = rand() % neighbors.size();
					newPos    = neighbors[dir];
					if ( !g->w()->isWalkableGnome( newPos ) )
					{
						return;
					}
					newFacing = m_facing;
					if ( m_position.x != newPos.x )
					{
						if ( m_position.x - newPos.x > 0 )
						{
							newFacing = 2;
						}
						else
						{
							newFacing = 0;
						}
					}
					if ( m_position.y != newPos.y )
					{
						if ( m_position.y - newPos.y > 0 )
						{
							newFacing = 3;
						}
						else
						{
							newFacing = 1;
						}
					}
				}

				switch ( m_type )
				{
					case CreatureType::GNOME:
					case CreatureType::GNOME_TRADER:
						if ( g->cm()->monstersAtPosition( newPos ).size() )
						{
							return;
						}
						if ( g->w()->getTileFlag( newPos ) & TileFlag::TF_NOPASS )
						{
							return;
						}
						break;
					case CreatureType::ANIMAL:
					{
						if ( g->rm()->isDoor( newPos.toInt() ) )
						{
							if ( g->rm()->blockAnimals( newPos.toInt() ) )
							{
								return;
							}
						}
					}
					break;
					case CreatureType::MONSTER:
						if ( g->gm()->gnomesAtPosition( newPos ).size() )
						{
							return;
						}
						break;
				}
				if ( newFacing != -1 )
				{
					m_position = newPos;
					m_facing   = newFacing;
				}
			}
		}
		m_moveCooldown = m_moveDelay;
	}
}

int Creature::moveSpeed() const
{
	return m_moveSpeed;
}

bool Creature::moveOnPath()
{
	if ( m_moveCooldown <= 0 )
	{
		//get position from path
		if ( !m_currentPath.empty() )
		{
			Position p = m_currentPath.back();
			m_currentPath.pop_back();

			// check if we can move onto the next position
			if ( !g->w()->isWalkableGnome( p ) )
			{
				m_currentPath.clear();
				if ( Global::debugMode )
					qDebug() << "moveOnPath tile blocked " << p.toString();
				return false;
			}

			Door* door = g->rm()->getDoor( p.toInt() );
			if ( door )
			{
				switch ( m_type )
				{
					case CreatureType::GNOME:
					case CreatureType::GNOME_TRADER:
						if ( door->blockGnomes )
						{
							m_currentPath.clear();
							if ( Global::debugMode )
								qDebug() << "moveOnPath door closed for gnomes " << p.toString();
							return false;
						}
						break;
					case CreatureType::ANIMAL:
						if ( door->blockAnimals )
						{
							m_currentPath.clear();
							if ( Global::debugMode )
								qDebug() << "moveOnPath door closed for animals " << p.toString();
							return false;
						}
						break;
					case CreatureType::MONSTER:
						if ( door->blockMonsters )
						{
							m_currentPath.clear();
							if ( Global::debugMode )
								qDebug() << "moveOnPath door closed for monsters " << p.toString();
							return false;
						}
						break;
				}
			}

			switch ( m_type )
			{
				case CreatureType::GNOME:
				case CreatureType::GNOME_TRADER:
					if ( g->cm()->monstersAtPosition( p ).size() )
					{
						m_currentPath.clear();
						if ( Global::debugMode )
							qDebug() << "moveOnPath gnome can't move onto a tile occupied by monsters " << p.toString();
						return false;
					}
					break;
				case CreatureType::ANIMAL:
					break;
				case CreatureType::MONSTER:
					if ( g->gm()->gnomesAtPosition( p ).size() )
					{
						m_currentPath.clear();
						if ( Global::debugMode )
							qDebug() << "moveOnPath monster can't move onto a tile occupied by gnomes " << p.toString();
						return false;
					}
					break;
			}
			// move on path
			m_facing   = getFacing( m_position, p );
			m_position = p;

			//m_sprite.dimX = 32;
			//m_sprite.dimY = 32;
			if ( m_currentPath.empty() )
			{
				if ( m_facingAfterMove != -1 )
				{
					m_facing = m_facingAfterMove;
				}
			}
		}

		m_renderParamsChanged = true;
		m_moveCooldown        = m_moveDelay; // TODO movedelay =
	}
	return true;
}

int Creature::getFacing( Position posFrom, Position posTo )
{
	int dx = posTo.x - posFrom.x;
	int dy = posTo.y - posFrom.y;

	if ( ( dx == 1 && dy == 0 ) || ( dx == 1 && dy == 1 ) )
	{
		return 0;
	}
	if ( ( dx == 0 && dy == 1 ) || ( dx == -1 && dy == 1 ) )
	{
		return 1;
	}
	if ( ( dx == -1 && dy == 0 ) || ( dx == -1 && dy == -1 ) )
	{
		return 2;
	}
	if ( ( dx == 0 && dy == -1 ) || ( dx == 1 && dy == -1 ) )
	{
		return 3;
	}
	return 0;
}

void Creature::setFollowID( unsigned int id )
{
	m_followID = id;
}

unsigned int Creature::followID() const
{
	return m_followID;
}

void Creature::setFollowPosition( Position pos )
{
	m_followPosition = pos;
	int dx           = pos.x - m_position.x;
	int dy           = pos.y - m_position.y;

	if ( ( dx == 1 && dy == 0 ) || ( dx == 1 && dy == 1 ) )
	{
		m_facing = 0;
	}
	if ( ( dx == 0 && dy == 1 ) || ( dx == -1 && dy == 1 ) )
	{
		m_facing = 1;
	}
	if ( ( dx == -1 && dy == 0 ) || ( dx == -1 && dy == -1 ) )
	{
		m_facing = 2;
	}
	if ( ( dx == 0 && dy == -1 ) || ( dx == 1 && dy == -1 ) )
	{
		m_facing = 3;
	}
}

void Creature::setSpriteUID( QString name, unsigned int spriteUID )
{
	m_spriteUIDs.insert( name, spriteUID );
}

unsigned int Creature::spriteUID( QString name ) const
{
	if ( !m_spriteUIDs.empty() )
	{
		return m_spriteUIDs.value( name ).toUInt();
	}
	else
	{
		return m_spriteID;
	}
}

unsigned int Creature::spriteUID() const
{
	return m_spriteID;
}

void Creature::setGender( Gender gender )
{
	m_gender = gender;
}

void Creature::move( Position oldPos )
{
	if ( m_position != oldPos )
	{
		g->w()->removeCreatureFromPosition( oldPos, m_id );
		g->w()->insertCreatureAtPosition( m_position, m_id );

		if( m_hasTransparency )
		{
			g->w()->setTileFlag( m_position, TileFlag::TF_TRANSPARENT );
			// check if no other creatures with transparency on tile
			bool transp = false;
			for( const auto& c : g->cm()->creaturesAtPosition( oldPos ) )
			{
				if( c->hasTransparency() )
				{
					transp = true;
					break;
				}
			}
			if( !transp )
			{
				g->w()->clearTileFlag( oldPos, TileFlag::TF_TRANSPARENT );
			}
		}

		m_renderParamsChanged = true;
	}
}

bool Creature::renderParamsChanged()
{
	bool out              = m_renderParamsChanged;
	m_renderParamsChanged = false;
	return out;
}

void Creature::setImmobile( bool immobile )
{
	m_immobile = immobile;
}

void Creature::setIgnoreNoPass( bool state )
{
	m_ignoreNoPass = state;
}

bool Creature::ignoreNoPass() const
{
	return m_ignoreNoPass;
}

void Creature::setType( CreatureType type )
{
	m_type = type;
}

CreatureType Creature::type() const
{
	return m_type;
}

BT_RESULT Creature::actionRandomMove( bool halt )
{
	Q_UNUSED( halt ); // action takes only one tick, halt has no effect
	Position oldPos = m_position;

	randomMove();

	if ( m_lightIntensity && m_position != oldPos )
	{
		g->w()->moveLight( m_id, m_position, m_lightIntensity );
	}

	m_currentAction = "random move";
	return BT_RESULT::SUCCESS;
}

BT_RESULT Creature::conditionIsMale( bool halt )
{
	if ( m_gender == Gender::MALE )
	{
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Creature::conditionIsFemale( bool halt )
{
	if ( m_gender == Gender::FEMALE )
	{
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Creature::conditionIsDay( bool halt )
{
	if ( GameState::daylight )
	{
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Creature::conditionIsNight( bool halt )
{
	if ( !GameState::daylight )
	{
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

void Creature::setCurrentTarget( Position pos )
{
	m_currentTargetPosition = pos;
	//m_variables.insert( "CurrentTargetPos", pos.toString() );
}

Position Creature::currentTarget() const
{
	return m_currentTargetPosition;
	//return Position( m_variables.value( "CurrentTargetPos" ) );
}

bool Creature::kill( bool nocorpse )
{
	if ( !toDestroy() && !isDead() )
	{
		//kill succeeds
		if( nocorpse )
		{
			destroy();
		}
		die();
		return true;
	}
	// already dead or something else destroyed it
	return false;
}

void Creature::setThoughtBubble( QString thought )
{
	m_thoughtBubble = thought;
}

QString Creature::thoughtBubble() const
{
	return m_thoughtBubble;
}

quint64 Creature::expires() const
{
	return m_expires;
}

BT_RESULT Creature::conditionTargetAdjacent( bool halt )
{
	Creature* creature = nullptr;
	if ( m_currentAttackTarget )
	{
		creature = resolveTarget( m_currentAttackTarget );
	}

	if ( creature )
	{
		Position cPos = creature->getPos();
		if ( m_position.z == cPos.z )
		{
			if ( abs( m_position.x - cPos.x ) < 2 && abs( m_position.y - cPos.y ) < 2 )
			{
				if ( Global::debugMode )
					qDebug() << m_name << "Target is adjacent";
				return BT_RESULT::SUCCESS;
			}
		}
	}
	if ( Global::debugMode )
		qDebug() << m_name << "Target is not adjacent";
	return BT_RESULT::FAILURE;
}

BT_RESULT Creature::conditionTargetPositionValid( bool halt )
{
	Creature* creature = nullptr;
	if ( m_currentAttackTarget )
	{
		creature = resolveTarget( m_currentAttackTarget );
	}
	if (creature)
	{
		if ( creature->getPos() == m_currentTargetPosition )
		{
			return BT_RESULT::SUCCESS;
		}
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Creature::conditionIsInCombat( bool halt )
{
	if ( m_currentAttackTarget )
	{
		const Creature* creature = g->cm()->creature( m_currentAttackTarget );
		if ( !creature || creature->isDead() || !g->cm()->hasPathTo( m_position, m_currentAttackTarget ) )
		{
			m_currentAttackTarget = 0;
		}
	}
	if ( m_aggroList.size() || m_currentAttackTarget )
	{
		setThoughtBubble( "Combat" );
		return BT_RESULT::SUCCESS;
	}
	if ( m_thoughtBubble == "Combat" )
	{
		setThoughtBubble( "" );
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Creature::conditionIsOnMission( bool halt )
{
	if ( m_isOnMission )
	{
		return BT_RESULT::SUCCESS;
	}

	return BT_RESULT::FAILURE;
}

BT_RESULT Creature::actionGetExitPosition( bool halt )
{
	if ( halt )
	{
		return BT_RESULT::IDLE;
	}
	if ( m_mission )
	{
		auto mission = g->em()->getMission( m_mission );
		if ( mission )
		{
			if ( mission->leavePos != Position( 0, 0, 0 ) )
			{
				setCurrentTarget( mission->leavePos );
				return BT_RESULT::SUCCESS;
			}
			else
			{
				bool found        = false;
				Position leavePos = Global::util->reachableBorderPos( m_position, found );
				if ( found )
				{
					mission->leavePos = leavePos;
					setCurrentTarget( leavePos );
					return BT_RESULT::SUCCESS;
				}
			}
		}
		else
		{
			m_mission = 0;
		}
	}
	else
	{
		bool found        = false;
		Position leavePos = Global::util->reachableBorderPos( m_position, found );
		if ( found )
		{
			setCurrentTarget( leavePos );
			return BT_RESULT::SUCCESS;
		}
	}
	return BT_RESULT::RUNNING;
}

BT_RESULT Creature::actionLeaveMap( bool halt )
{
	Q_UNUSED( halt ); // action takes only one tick, halt has no effect

	g->w()->removeCreatureFromPosition( m_position, m_id );
	m_goneOffMap = true;
	return BT_RESULT::SUCCESS;
}

BT_RESULT Creature::actionEnterMap( bool halt )
{
	Q_UNUSED( halt ); // action takes only one tick, halt has no effect

	g->w()->insertCreatureAtPosition( m_position, m_id );
	m_goneOffMap = false;

	m_mission     = 0;
	m_isOnMission = false;

	return BT_RESULT::SUCCESS;
}

QList<AggroEntry>& Creature::aggroList()
{
	return m_aggroList;
}

void Creature::addAggro( unsigned int target, int value )
{
	bool exists = false;
	for ( auto& ae : m_aggroList )
	{
		if ( ae.id == target )
		{
			ae.aggro += value;
			exists = true;
			break;
		}
	}
	if ( !exists )
	{
		AggroEntry ae { value, target };
		m_aggroList.append( ae );
	}
	std::sort( m_aggroList.begin(), m_aggroList.end() );
}

void Creature::updateAttackValues()
{
	m_leftHandAttackSkill  = getSkillLevel( "Unarmed" );
	m_leftHandAttackValue  = m_leftHandAttackSkill;
	m_rightHandAttackSkill = getSkillLevel( "Unarmed" );
	m_rightHandAttackValue = m_rightHandAttackSkill;
}

void Creature::die()
{
	m_isDead = true;
	dropInventory();
	dropEquipment();
	updateSprite();
}

void Creature::dropInventory()
{
	for ( const unsigned int it : inventoryItems() )
	{
		g->inv()->putDownItem( it, m_position );
	}
	m_inventoryItems.clear();
}

void Creature::dropEquipment()
{
	for ( const unsigned int it : m_equipment.wornItems() )
	{
		g->inv()->putDownItem( it, m_position );
	}
	m_equipment.clearAllItems();
}

void Creature::addClaimedItem( unsigned int item, unsigned int job )
{
	if( job == m_id )
	{
		g->inv()->setIsUsedBy( item, job );
	}
	else
	{
		g->inv()->setInJob( item, job );
	}
	m_claimedItems.append( item );
}

void Creature::unclaimAll()
{
	if ( !m_claimedItems.empty() )
	{
		for ( auto item : m_claimedItems )
		{
			g->inv()->setInJob( item, 0 );
			g->inv()->setIsUsedBy( item, 0 );
			if ( g->inv()->isHeldBy( item ) == m_id )
			{
				g->inv()->putDownItem( item, m_position );
			}
		}
	}
	m_claimedItems.clear();
}

void Creature::destroyClaimedItems()
{
	for ( auto item : m_claimedItems )
	{
		g->inv()->pickUpItem( item, m_id );
		g->inv()->destroyObject( item );
	}
	m_claimedItems.clear();
}

Creature* Creature::resolveTarget( unsigned int creatureId )
{
	Creature* creature = g->cm()->creature( m_currentAttackTarget );
	if ( !creature )
	{
		creature = g->gm()->gnome( m_currentAttackTarget );
	}
	return creature;
}

void Creature::log( QString txt )
{
	if ( m_log.size() > 0 )
	{
		if ( m_log.last().endsWith( txt ) )
		{
			++m_sameMsgCounter;
		}
		else
		{
			if ( m_sameMsgCounter > 0 )
			{
				m_log.last() += " x" + QString::number( m_sameMsgCounter );
				m_sameMsgCounter = 0;
			}
			m_log.append( GameState::currentDayTime + ": " + txt );
		}
	}
	else
	{
		m_log.append( GameState::currentDayTime + ": " + txt );
	}
	if ( m_log.size() > 50 )
	{
		for ( int i = 0; i < 25; ++i )
		{
			m_log.takeFirst();
		}
	}
}

QStringList Creature::needIDs() const
{
	return m_needs.keys();
}

QStringList Creature::availableSkillIDs() const
{
	return m_skills.keys();
}

BT_RESULT Creature::conditionAlarm( bool halt )
{
	if ( GameState::alarm == 2 )
	{
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Creature::conditionIsInSafeRoom( bool halt )
{
	auto room = g->rm()->getRoomAtPos( m_position );
	if ( room )
	{
		if ( room->id() == GameState::alarmRoomID )
		{
			return BT_RESULT::SUCCESS;
		}
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Creature::actionGetSafeRoomPosition( bool halt )
{
	auto room = g->rm()->getRoom( GameState::alarmRoomID );
	if ( room )
	{
		auto pos = room->randomTilePos();
		if ( pos.isZero() )
		{
			return BT_RESULT::FAILURE;
		}
		else
		{
			m_currentTargetPosition = pos;
			m_thoughtBubble         = "";
			return BT_RESULT::SUCCESS;
		}
	}
	return BT_RESULT::FAILURE;
}

bool Creature::equipmentChanged()
{
	bool tmp = m_equipmentChanged;
	m_equipmentChanged = false;
	return tmp;
}