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
#include "../base/pathfinder.h"
#include "../base/priorityqueue.h"
#include "../game/anatomy.h"
#include "object.h"

#include <QPixmap>

class QPainter;
class Game;

struct AggroEntry
{
	int aggro       = 0;
	unsigned int id = 0;

	bool operator==( const AggroEntry& other )
	{
		return this->aggro == other.aggro;
	}
	bool operator!=( const AggroEntry& other )
	{
		return this->aggro != other.aggro;
	}
	bool operator<( const AggroEntry& other )
	{
		return this->aggro < other.aggro;
	}
	friend bool operator<( const AggroEntry& lhs, const AggroEntry& rhs )
	{
		return lhs.aggro < rhs.aggro;
	}
};

enum class Gender : unsigned char
{
	UNDEFINED = 0,
	MALE      = 1,
	FEMALE    = 2
};

enum class CreatureType : unsigned char
{
	UNDEFINED = 0,
	GNOME     = 1,
	GNOME_TRADER,
	ANIMAL,
	MONSTER,
	AUTOMATON
};

enum class CreatureTickResult : unsigned char
{
	DEAD,
	OK,
	JOBCHANGED,
	TODESTROY,
	NOFLOOR,
	LEFTMAP,
	NOFUEL,
	NOCORE
};

struct EquipmentItem
{
	QString item            = "";
	QString material        = "";
	unsigned int itemID     = 0;
	unsigned int materialID = 0;
	QStringList allMats;

	QVariantMap serialize();
	EquipmentItem( const QVariantMap& in );

	EquipmentItem() {};
};

struct Equipment
{
	QString hair       = "GnomeHair1";
	QString facialHair = "";
	int hairColor      = 0;
	QString shirt      = "GnomeShirt1";
	int shirtColor     = 0;

	unsigned int uniformID = 0;
	unsigned int roomID    = 0;

	EquipmentItem head;
	EquipmentItem chest;
	EquipmentItem arm;
	EquipmentItem hand;
	EquipmentItem leg;
	EquipmentItem foot;
	EquipmentItem leftHandHeld;
	EquipmentItem rightHandHeld;
	EquipmentItem back;

	QList<unsigned int> wornItems() const;
	QVariantMap serialize();
	void clearAllItems();
	Equipment( const QVariantMap& in );
	float getDamageReduction( CreaturePart part );
	EquipmentItem& getSlot( CreaturePart part );

	Equipment() {};
};

class Creature : public Object
{
public:
	Creature( const Position& pos, QString name, Gender gender, QString species, Game* game );
	Creature( QVariantMap in, Game* game );
	~Creature();

	bool operator<( const Creature& other ) const
	{
		return this->m_name < other.m_name;
	}

	virtual void serialize( QVariantMap& out ) const;

	virtual void init() = 0;

	virtual void updateSprite() = 0;

	void forceMove( const Position& to );

	virtual CreatureTickResult onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged ) = 0;

	quint8 facing() const
	{
		return m_facing;
	}

	void setGender( Gender gender );
	Gender gender() const
	{
		return m_gender;
	}

	QString species() const
	{
		return m_species;
	}

	void setSpriteUID( QString name, unsigned int spriteUID );
	unsigned int spriteUID( QString name ) const;
	unsigned int spriteUID() const;

	bool renderParamsChanged();

	bool isDead() const
	{
		return m_isDead;
	};
	bool toDestroy() const
	{
		return m_toDestroy;
	}
	void destroy()
	{
		m_toDestroy = true;
	}

	void setImmobile( bool immobile );

	void addAttribute( QString id, int level );
	int attribute( QString id ) const;
	void addSkill( QString id, int level );
	int getSkillLevel( QString id ) const;
	void setSkillLevel( QString id, int level );
	int getSkillXP( QString id ) const;

	virtual void updateMoveSpeed() = 0;
	int moveSpeed() const;

	QString name() const
	{
		return m_name;
	}
	void setName( QString name )
	{
		m_name = name;
	}

	void setIgnoreNoPass( bool state );
	bool ignoreNoPass() const;

	void setType( CreatureType type );
	CreatureType type() const;

	unsigned int followID() const;
	void setFollowID( unsigned int id );
	void setFollowPosition( Position pos );

	void setCurrentTarget( Position pos );
	Position currentTarget() const;

	bool kill( bool nocorpse );
	virtual bool attack( DamageType dt, AnatomyHeight da, int skill, int strength, Position sourcePos, unsigned int attackerID ) = 0;

	QString thoughtBubble() const;
	void setThoughtBubble( QString thought );

	quint64 expires() const;

	QList<AggroEntry>& aggroList();

	void addAggro( unsigned int target, int value );

	virtual void updateAttackValues();

	QStringList needIDs() const;
	QStringList availableSkillIDs() const;

	bool isOnMission() const
	{
		return m_isOnMission;
	}
	void setOnMission( bool v )
	{
		m_isOnMission = v;
	}
	void setMission( unsigned int mission )
	{
		m_mission = mission;
	}

	bool goneOffMap() const
	{
		return m_goneOffMap;
	}

	unsigned int roleID()
	{
		return m_roleID;
	}

	void setRole( unsigned int roleID );

	QList<unsigned int> inventoryItems() const
	{
		return m_inventoryItems;
	}

	QList<unsigned int> carriedItems() const
	{
		return m_carriedItems;
	}

	QList<unsigned int> claimedItems()
	{
		return m_claimedItems;
	}

	bool isAnimal() { return m_type == CreatureType::ANIMAL; }
	bool isMonster() { return m_type == CreatureType::MONSTER; }

	bool hasTransparency() { return m_hasTransparency; }

	bool equipmentChanged();

protected:
	QPointer<Game> g;

	virtual void loadBehaviorTree( QString id ) final;
	virtual void initTaskMap() = 0;

	void processCooldowns( quint64 tickNumber );

	virtual void move( Position oldPos );

	void randomMove();

	bool moveOnPath();

	int getFacing( Position posFrom, Position posTo );

	QStringList& getLog()
	{
		return m_log;
	}

	QStringList m_log;
	int m_sameMsgCounter = 0;
	void log( QString txt );

	CreatureType m_type = CreatureType::UNDEFINED;
	QString m_species   = "not_initialized";
	QString m_name      = "not_initialized";
	Gender m_gender     = Gender::UNDEFINED;

	bool m_immobile     = false;
	bool m_aquatic      = false;
	bool m_ignoreNoPass = true;

	Anatomy m_anatomy;

	bool m_isDead     = false;
	bool m_toDestroy  = false;
	quint64 m_expires = 0;

	//variables formerlin in QVariantMap Creature::m_variables
	QVariantList m_spriteDef;
	QVariantList m_spriteDefBack;

	QVariantMap m_spriteUIDs;
	QString m_thoughtBubble    = "";
	bool m_renderParamsChanged = true;

	Position m_currentTargetPosition;
	std::vector<Position> m_currentPath;
	qint8 m_facingAfterMove = -1;

	unsigned int m_currentAttackTarget = 0;
	bool m_goneOffMap = false;

	QVariantMap m_btBlackBoard;

	QVariantMap m_attributes;
	QVariantMap m_skills;
	QVariantMap m_skillActive;
	QStringList m_skillPriorities;
	QVariantMap m_needs;

	Equipment m_equipment;
	unsigned int m_roleID = 0;

	bool m_equipmentChanged = true;

	//move cooldown cache, set to m_moveCooldown after succesful move
	float m_moveDelay = 1000;
	// current cooldown, prohibits move if >0
	float m_moveCooldown = 1000;
	// subtracted from moveCooldown every tick
	float m_moveSpeed       = 50;
	quint8 m_facing         = 0;
	unsigned int m_followID = 0;
	Position m_followPosition;

	//unsigned char m_currentPriority = 0;
	QString m_currentAction = "idle";

	int m_state               = 0;
	quint64 m_stateChangeTick = 0;

	unsigned char m_lightIntensity = 0;

	quint64 m_lastOnTick        = 0;
	int m_globalCooldown        = 0;
	int m_kickCooldown          = 0;
	int m_biteCooldown          = 0;
	int m_leftHandCooldown      = 0;
	int m_rightHandCooldown     = 0;
	int m_specialAttackCoolDown = 0;
	int m_jobCooldown           = 0;

	int m_leftHandAttackValue  = 0;
	int m_leftHandAttackSkill  = 0;
	int m_rightHandAttackValue = 0;
	int m_rightHandAttackSkill = 0;

	bool m_leftHandArmed     = false;
	bool m_rightHandArmed    = false;
	bool m_leftHandHasWeapon = false;

	bool m_isOnMission      = false;
	unsigned int m_mission  = 0;
	quint64 m_nextCheckTick = 0;

	bool m_hasTransparency = false;

	QHash<QString, std::function<BT_RESULT( bool )>> m_behaviors;
	QHash<QString, std::function<bool( void )>> m_taskFunctions;

	QList<AggroEntry> m_aggroList;

	QString m_btName        = "";
	QScopedPointer<BT_Node> m_behaviorTree;

	BT_RESULT conditionIsMale( bool halt );
	BT_RESULT conditionIsFemale( bool halt );
	BT_RESULT conditionIsDay( bool halt );
	BT_RESULT conditionIsNight( bool halt );

	BT_RESULT conditionAlarm( bool halt );
	BT_RESULT conditionIsInSafeRoom( bool halt );

	BT_RESULT conditionIsInCombat( bool halt );
	BT_RESULT conditionIsOnMission( bool halt );
	BT_RESULT conditionTargetAdjacent( bool halt );
	BT_RESULT conditionTargetPositionValid( bool halt );

	BT_RESULT actionRandomMove( bool halt );
	BT_RESULT actionGetExitPosition( bool halt );
	BT_RESULT actionGetSafeRoomPosition( bool halt );
	BT_RESULT actionLeaveMap( bool halt );
	BT_RESULT actionEnterMap( bool halt );

	QList<unsigned int> m_claimedItems;
	QList<unsigned int> m_carriedItems;
	QList<unsigned int> m_inventoryItems;

	virtual void die();

	void dropInventory();
	void dropEquipment();

	void addClaimedItem( unsigned int item, unsigned int job );
	void unclaimAll();
	void destroyClaimedItems();

	Creature* resolveTarget( unsigned int creatureId );
};

struct CreatureCompare
{
	bool operator()( const Creature* a, const Creature* b )
	{
		return ( *a < *b );
	}
};