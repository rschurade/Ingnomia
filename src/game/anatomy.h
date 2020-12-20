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

#include "../base/enums.h"

#include <QSet>
#include <QString>
#include <QVariantMap>

struct Equipment;

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
