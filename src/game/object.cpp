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
/** @file object.cpp
 *  @brief Implementation of the Object base class for positioned world entities.
 */
#include "object.h"

#include "../base/gamestate.h"

/** @brief Default constructor. Members are left uninitialized. */
Object::Object()
{
}

/** @brief Construct a new Object at the given position with a freshly generated unique ID.
 *  @param position World position for this object.
 */
Object::Object( const Position& position ) :
	m_position( position ),
	m_spriteID( 0 )
{
	m_id = GameState::createID();
}

/** @brief Deserialize an Object from a saved QVariantMap.
 *  @param in Map containing "ID", "Position", and "SpriteID" keys.
 */
Object::Object( QVariantMap in ) :
	m_id( in.value( "ID" ).toUInt() ),
	m_position( in.value( "Position" ).toString() ),
	m_spriteID( in.value( "SpriteID" ).toUInt() )
{
}

/** @brief Serialize object state into a QVariantMap for save games.
 *  @param out Map to insert "ID", "Position", and "SpriteID" into.
 */
void Object::serialize( QVariantMap& out ) const
{
	out.insert( "ID", m_id );
	out.insert( "Position", m_position.toString() );
	out.insert( "SpriteID", m_spriteID );
}

/** @brief Destructor. */
Object::~Object()
{
}

/** @brief Return the unique ID of this object.
 *  @return Unique object ID assigned at creation.
 */
unsigned int Object::id() const
{
	return m_id;
}

/** @brief Return the world position of this object.
 *  @return Const reference to the object's Position.
 */
const Position& Object::getPos() const
{
	return m_position;
}

/** @brief Set the world position of this object.
 *  @param position New position.
 */
void Object::setPos( Position position )
{
	m_position = position;
}

/** @brief Return the sprite ID used to render this object.
 *  @return Sprite unique ID.
 */
unsigned int Object::spriteID() const
{
	return m_spriteID;
}

/** @brief Set the sprite ID used to render this object.
 *  @param id New sprite unique ID.
 */
void Object::setSpriteID( unsigned int id )
{
	m_spriteID = id;
}
