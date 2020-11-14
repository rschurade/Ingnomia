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

#include "../base/position.h"

#include <QVariant>
#include <QtGlobal>

class Object
{
public:
	Object();
	Object( const Position& position );
	Object( QVariantMap in );
	virtual ~Object();

	bool operator<( const Object& o ) const
	{
		return m_id < o.m_id;
	}

	unsigned int id() const;

	const Position& getPos() const;
	void setPos( Position position );

	virtual unsigned int spriteID() const;
	void setSpriteID( unsigned int id );

	virtual void serialize( QVariantMap& out ) const;

protected:
	unsigned int m_id;

	Position m_position;

	unsigned int m_spriteID;
};
