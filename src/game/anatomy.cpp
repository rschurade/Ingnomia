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
#include "anatomy.h"

#include "../base/db.h"
#include "../base/global.h"
#include "../game/creature.h"

#include <QDebug>

Anatomy::Anatomy()
{
}

Anatomy::~Anatomy()
{
}

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

AnatomyStatus Anatomy::status()
{
	return m_status;
}

void Anatomy::setFluidLevelonTile( unsigned char fluidLevel )
{
	if ( fluidLevel > 5 && !m_isAquatic )
	{
		m_status        = AnatomyStatus( m_status | AS_DEAD );
		m_statusChanged = true;
		qDebug() << "Anatomy: Dead! Drowned";
	}
}

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