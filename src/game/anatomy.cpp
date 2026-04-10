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
/** @file anatomy.cpp
 *  @brief Body part system for creatures: anatomy initialization, wound tracking,
 *         damage application, healing, and armor coverage via Equipment.
 */
#include "anatomy.h"

#include "../base/db.h"
#include "../base/global.h"
#include "../game/creature.h"

#include <QDebug>

/** @brief Default constructor. Creates an uninitialized Anatomy. Call init() to set up body parts. */
Anatomy::Anatomy()
{
}

/** @brief Destructor. */
Anatomy::~Anatomy()
{
}

/**
 * @brief Initializes the anatomy from the database for a given creature type.
 *
 * Loads all body parts from the "Anatomy_Parts" DB table, setting their height,
 * side, vitality, and HP. Also establishes parent-child relationships between parts.
 * Falls back to "Dummy" anatomy if the given type has no root part defined.
 *
 * @param type The anatomy type identifier (e.g., "Humanoid", "Animal", "Fish").
 * @param isAquatic Whether the creature is aquatic (cannot drown).
 */
void Anatomy::init( QString type, bool isAquatic )
{
	m_type = type;
	m_isAquatic = isAquatic || ( type == "Fish" );

	auto def = DB::selectRow( "Anatomy", type );

	QString root = def.value( "Root" ).toString();

	if ( root.isEmpty() )
	{
		type = "Dummy";
		def  = DB::selectRow( "Anatomy", type );
		root = def.value( "Root" ).toString();
	}

	auto rows = DB::selectRows( "Anatomy_Parts", type );

	for ( auto row : rows )
	{
		AnatomyPart ap;
		ap.id     = Global::creaturePartLookUp.value( row.value( "ID2" ).toString() );
		ap.parent = Global::creaturePartLookUp.value( row.value( "Parent" ).toString() );

		QString sh = row.value( "Height" ).toString();
		if ( sh == "High" )
			ap.height = AH_HIGH;
		else if ( sh == "Middle" )
			ap.height = AH_MIDDLE;
		else
			ap.height = AH_LOW;

		QString ss = row.value( "Side" ).toString();
		if ( ss == "Left" )
			ap.side = AS_LEFT;
		else if ( ss == "Right" )
			ap.side = AS_RIGHT;
		else
			ap.side = AS_CENTER;

		ap.isInside = row.value( "IsInside" ).toBool();
		ap.isVital  = row.value( "IsVital" ).toBool();
		ap.hp       = row.value( "HP" ).toInt();
		ap.maxHP    = row.value( "HP" ).toInt();

		m_parts.insert( ap.id, ap );
	}

	for ( auto row : rows )
	{
		auto id     = Global::creaturePartLookUp.value( row.value( "ID2" ).toString() );
		auto parent = Global::creaturePartLookUp.value( row.value( "Parent" ).toString() );

		m_parts[parent].children.append( id );
	}
}

/**
 * @brief Serializes the anatomy state to a QVariantMap for save games.
 *
 * Stores the anatomy type, blood level, bleeding rate, status flags,
 * aquatic flag, and the HP of each body part.
 *
 * @return A QVariantMap containing all anatomy state data.
 */
QVariantMap Anatomy::serialize() const
{
	QVariantMap out;

	out.insert( "Type", m_type );
	out.insert( "Blood", m_blood );
	out.insert( "Bleeding", m_bleeding );
	out.insert( "Status", m_status );
	out.insert( "Aquatic", m_isAquatic );

	QVariantList vlParts;

	for ( auto part : m_parts )
	{
		QVariantMap vm;
		vm.insert( "ID", part.id );
		vm.insert( "HP", part.hp );

		vlParts.append( vm );
	}
	out.insert( "Parts", vlParts );

	return out;
}

/**
 * @brief Restores anatomy state from a previously serialized QVariantMap.
 *
 * Re-initializes the anatomy from the database, then overwrites HP values
 * and status/blood/bleeding from the saved data.
 *
 * @param in The QVariantMap produced by serialize().
 */
void Anatomy::deserialize( QVariantMap in )
{
	m_type = in.value( "Type" ).toString();
	init( m_type, in.value( "Aquatic" ).toBool() );

	m_statusChanged = true;
	m_status        = (AnatomyStatus)in.value( "Status" ).toUInt();
	m_blood         = in.value( "Blood" ).toFloat();
	m_bleeding      = in.value( "Bleeding" ).toFloat();

	auto vl = in.value( "Parts" ).toList();
	for ( auto vpart : vl )
	{
		auto vm         = vpart.toMap();
		CreaturePart id = (CreaturePart)vm.value( "ID" ).toUInt();
		if ( m_parts.contains( id ) )
		{
			m_parts[id].hp = vm.value( "HP" ).toFloat();
		}
	}
}

/**
 * @brief Applies damage to the anatomy from an incoming attack.
 *
 * Determines which body part is hit based on attack height and side, then
 * applies damage reduced by the creature's equipment armor. If the hit part
 * is already destroyed, traverses up to parent parts. Slash/piercing attacks
 * that deal significant damage cause bleeding. Destroying a vital part kills
 * the creature.
 *
 * @param eq Pointer to the creature's Equipment for damage reduction lookup.
 * @param dt The type of damage (slash, piercing, blunt).
 * @param da The height of the attack (low, middle, high).
 * @param ds The side of the attack (left, right, center/front/back).
 * @param strength The raw damage value of the attack.
 */
void Anatomy::damage( Equipment* eq, DamageType dt, AnatomyHeight da, AnatomySide ds, int strength )
{
	// get part that is hit
	CreaturePart hitPart = KCP_NONE;

	bool left   = (bool)( ds & AS_LEFT );
	bool right  = (bool)( ds & AS_RIGHT );
	bool center = (bool)( ds & AS_FRONT ) || (bool)( ds & AS_BACK );

	int ra;
	switch ( da )
	{
		case AH_LOW:
		{ // hit foot or leg
			if ( !left && !right )
			{
				ra    = rand() % 100;
				left  = ( ra > 50 );
				right = !left;
			}
			ra = rand() % 100;
			if ( left )
			{
				if ( ra > 75 )
				{
					hitPart = CP_LEFT_FOOT;
				}
				else
				{
					hitPart = CP_LEFT_LEG;
				}
			}
			else
			{
				ra = rand() % 100;
				if ( ra > 75 )
				{
					hitPart = CP_RIGHT_FOOT;
				}
				else
				{
					hitPart = CP_RIGHT_LEG;
				}
			}
		}
		break;
		case AH_MIDDLE:
		{
			if ( !left && !right )
			{
				hitPart = CP_TORSO;
			}
			else
			{
				ra = rand() % 100;
				if ( left )
				{
					if ( ra > 50 )
					{
						hitPart = CP_TORSO;
					}
					else
					{
						ra = rand() % 100;
						if ( ra > 75 )
						{
							hitPart = CP_LEFT_HAND;
						}
						else
						{
							hitPart = CP_LEFT_ARM;
						}
					}
				}
				else
				{
					{
						if ( ra > 50 )
						{
							hitPart = CP_TORSO;
						}
						else
						{
							ra = rand() % 100;
							if ( ra > 75 )
							{
								hitPart = CP_RIGHT_HAND;
							}
							else
							{
								hitPart = CP_RIGHT_ARM;
							}
						}
					}
				}
			}

			// hit torso, arm or hand
		}
		break;
		case AH_HIGH:
		{
			// hit head
			hitPart = CP_HEAD;
		}
		break;
	}

	if ( m_parts.contains( hitPart ) )
	{
		AnatomyPart& part = m_parts[hitPart];

		if ( part.hp <= 0 )
		{
			if ( m_parts.contains( part.parent ) )
			{
				hitPart = part.parent;
				part    = m_parts[hitPart];
				if ( part.hp <= 0 )
				{
					if ( m_parts.contains( part.parent ) )
					{
						hitPart = part.parent;
						part    = m_parts[hitPart];
						if ( part.hp <= 0 )
						{
							if ( m_parts.contains( part.parent ) )
							{
								hitPart = part.parent;
								part    = m_parts[hitPart];
							}
							else
							{
								m_status        = AnatomyStatus( m_status | AS_DEAD );
								m_statusChanged = true;
								return;
							}
						}
					}
					else
					{
						m_status        = AnatomyStatus( m_status | AS_DEAD );
						m_statusChanged = true;
						return;
					}
				}
			}
			else
			{
				m_status        = AnatomyStatus( m_status | AS_DEAD );
				m_statusChanged = true;
				return;
			}
		}

		float dr          = eq->getDamageReduction( part.id );
		float finalDamage = strength - dr;
		part.hp           = part.hp - finalDamage;
		if ( finalDamage > 0 )
		{
			m_status = AnatomyStatus( m_status | AS_WOUNDED );
		}
		if ( dt == DT_SLASH || dt == DT_PIERCING )
		{
			if ( finalDamage > ( ( part.maxHP / 100 ) * 10 ) )
			{
				m_bleeding += 0.5;
			}
		}

		if ( part.hp <= 0 )
		{
			if ( part.isVital )
			{
				m_status        = AnatomyStatus( m_status | AS_DEAD );
				m_statusChanged = true;
			}
		}
	}
}

/**
 * @brief Processes per-tick bleeding and blood regeneration, and checks for status changes.
 *
 * If bleeding, reduces blood level; death occurs below 1000, unconsciousness below 3000.
 * If not bleeding, slowly regenerates blood and recovers from unconsciousness above 4000.
 * Returns true once when any status flag has changed since the last call.
 *
 * @return True if the anatomy status changed since the previous call.
 */
bool Anatomy::statusChanged()
{
	//m_hp = qMin( 100., m_hp + 0.025 );

	if ( m_bleeding > 0 )
	{
		m_blood -= m_bleeding;

		if ( m_blood < 1000 )
		{
			m_status        = AnatomyStatus( m_status | AS_DEAD );
			m_statusChanged = true;
		}
		else if ( m_blood < 3000 && !(bool)( m_status & AS_UNCONSCIOUS ) )
		{
			m_status        = AnatomyStatus( m_status | AS_UNCONSCIOUS );
			m_statusChanged = true;
		}
	}
	else
	{
		if ( m_blood < 5000 )
		{
			m_blood += 0.25;
			if ( m_blood > 4000 && (bool)( m_status & AS_UNCONSCIOUS ) )
			{
				unsigned char mask = 0xFF ^ AS_UNCONSCIOUS;
				m_status           = ( AnatomyStatus )( m_status & mask );
				m_statusChanged    = true;
			}
		}
	}

	bool out        = m_statusChanged;
	m_statusChanged = false;
	return out;
}

/**
 * @brief Returns the current anatomy status flags (alive, wounded, unconscious, dead).
 * @return The combined AnatomyStatus bitmask.
 */
AnatomyStatus Anatomy::status()
{
	return m_status;
}

/**
 * @brief Checks if the creature drowns based on the fluid level on its tile.
 *
 * Non-aquatic creatures die when the fluid level exceeds 5.
 *
 * @param fluidLevel The fluid level (0-8) on the creature's current tile.
 */
void Anatomy::setFluidLevelonTile( unsigned char fluidLevel )
{
	if ( fluidLevel > 5 && !m_isAquatic )
	{
		m_status        = AnatomyStatus( m_status | AS_DEAD );
		m_statusChanged = true;
		qDebug() << "Anatomy: Dead! Drowned";
	}
}

/**
 * @brief Returns a random attack height for use in combat.
 *
 * 50% chance of middle, 25% low, 25% high.
 *
 * @return A randomly chosen AnatomyHeight value.
 */
AnatomyHeight Anatomy::randomAttackHeight() const
{
	srand( std::chrono::system_clock::now().time_since_epoch().count() );
	auto ra = rand() % 100;

	if ( ra < 50 )
		return AH_MIDDLE;
	else if ( ra < 75 )
		return AH_LOW;
	return AH_HIGH;
}

/**
 * @brief Slowly heals all wounded body parts that still have positive HP.
 *
 * Each damaged part regenerates 0.025 HP per call. Once all parts are fully
 * healed, clears the AS_WOUNDED status flag.
 */
void Anatomy::heal()
{
	if ( m_status | AS_WOUNDED )
	{
		bool stillWounded = false;

		for ( auto& part : m_parts )
		{
			if ( part.hp > 0 && part.hp < part.maxHP )
			{
				part.hp += 0.025;
				if ( part.hp < part.maxHP )
				{
					stillWounded = true;
				}
			}
		}
		if ( !stillWounded )
		{
			unsigned char mask = 0xFF ^ AS_WOUNDED;
			m_status           = ( AnatomyStatus )( m_status & mask );
			m_statusChanged    = true;
		}
	}
}