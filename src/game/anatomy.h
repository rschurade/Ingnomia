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
/** @file anatomy.h
 *  Body part system for creatures. Tracks individual body parts with HP,
 *  applies damage with armor reduction, handles bleeding and healing,
 *  and determines death/unconsciousness from blood loss or vital part destruction.
 */
#pragma once

#include "../base/enums.h"

#include <QSet>
#include <QString>
#include <QVariantMap>

struct Equipment;

/** @brief A single body part with HP, parent/child hierarchy, and location metadata. */
struct AnatomyPart
{
	CreaturePart id;

	CreaturePart parent = (CreaturePart)KCP_NONE;
	QList<CreaturePart> children;

	bool isInside = false;
	bool isVital  = false;

	AnatomyHeight height;
	AnatomySide side;

	int hp;
	int maxHP;
};

/** @brief Manages a creature's body parts, damage, bleeding, healing, and death state.
 *
 *  Initialized from a database anatomy type (e.g. "Humanoid", "Animal").
 *  Tracks per-part HP, applies incoming damage with armor reduction,
 *  manages blood level and bleeding rate, and reports status changes
 *  (wounded, unconscious, dead).
 */
class Anatomy
{
	friend class GnomeWidget;
	friend class MonsterWidget;
	friend class AnimalWidget;

public:
	Anatomy();
	~Anatomy();

	void init( QString type, bool isAquatic );
	QVariantMap serialize() const;
	void deserialize( QVariantMap state );

	void damage( Equipment* eq, DamageType dt, AnatomyHeight da, AnatomySide ds, int strength );

	void heal();

	bool statusChanged();
	AnatomyStatus status();

	void setFluidLevelonTile( unsigned char fluidLevel );

	AnatomyHeight randomAttackHeight() const;

private:
	QString m_type;

	bool m_isAquatic = false;

	float m_blood    = 5000;
	float m_bleeding = 0.0;

	bool m_statusChanged   = false;
	AnatomyStatus m_status = AS_HEALTHY;

	QHash<CreaturePart, AnatomyPart> m_parts;
};
