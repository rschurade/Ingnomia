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

/** @file worldobject.cpp
 *  @brief Implementation of WorldObject: construction, deserialization, and serialization.
 */

#include "worldobject.h"

#include "../base/gamestate.h"
#include "../game/game.h"

/**
 * @brief Constructs a new WorldObject with a freshly generated unique ID.
 * @param game Pointer to the owning Game instance.
 */
WorldObject::WorldObject( Game* game ) :
	g( game )
{
	m_id = GameState::createID();
}

/**
 * @brief Virtual destructor.
 */
WorldObject::~WorldObject()
{
}

/**
 * @brief Deserializes a WorldObject from a saved QVariantMap.
 * @param vals Map containing keys "ID", "LastUpdate", "Name", "Active", and optionally "Suspended".
 * @param game Pointer to the owning Game instance.
 */
WorldObject::WorldObject( QVariantMap vals, Game* game ) :
	g( game )
{
	m_id             = vals.value( "ID" ).toUInt();
	m_lastUpdateTick = vals.value( "LastUpdate" ).value<quint64>();
	m_name           = vals.value( "Name" ).toString();
	m_active         = vals.value( "Active" ).toBool();

	if ( vals.contains( "Suspended" ) )
	{
		m_active = !vals.value( "Suspended" ).toBool();
	}
}

/**
 * @brief Serializes this object's base fields into the given map for saving.
 * @param out Output map to insert "ID", "Name", "LastUpdate", and "Active" into.
 */
void WorldObject::serialize( QVariantMap& out ) const
{
	out.insert( "ID", m_id );
	out.insert( "Name", m_name );
	out.insert( "LastUpdate", m_lastUpdateTick );
	out.insert( "Active", m_active );
}
