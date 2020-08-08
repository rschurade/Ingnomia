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
#include "object.h"

#include "../base/gamestate.h"

Object::Object()
{
}

Object::Object( const Position& position ) :
	m_position( position ),
	m_spriteID( 0 )
{
	m_id = GameState::createID();
}

Object::Object( QVariantMap in ) :
	m_id( in.value( "ID" ).toUInt() ),
	m_position( in.value( "Position" ).toString() ),
	m_spriteID( in.value( "SpriteID" ).toUInt() )
{
}

void Object::serialize( QVariantMap& out ) const
{
	out.insert( "ID", m_id );
	out.insert( "Position", m_position.toString() );
	out.insert( "SpriteID", m_spriteID );
}

Object::~Object()
{
}

unsigned int Object::id() const
{
	return m_id;
}

const Position& Object::getPos() const
{
	return m_position;
}

void Object::setPos( Position position )
{
	m_position = position;
}

unsigned int Object::spriteID() const
{
	return m_spriteID;
}

void Object::setSpriteID( unsigned int id )
{
	m_spriteID = id;
}
