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
#include "region.h"

#include "../base/global.h"
#include "../game/world.h"

#include <QDebug>

Region::Region( unsigned int id ) :
	m_id( id )
{
}

Region::~Region()
{
}

void Region::addConnectionFrom( unsigned int toRegion, const Position& pos )
{
	m_connectionsFrom[toRegion].insert( pos.toString() );
}
void Region::addConnectionTo( unsigned int toRegion, const Position& pos )
{
	m_connectionsTo[toRegion].insert( pos.toString() );
}
void Region::removeConnectionFrom( unsigned int toRegion, const Position& pos )
{
	m_connectionsFrom[toRegion].remove( pos.toString() );
	if ( m_connectionsFrom[toRegion].empty() )
	{
		m_connectionsFrom.remove( toRegion );
	}
}
void Region::removeConnectionTo( unsigned int toRegion, const Position& pos )
{
	m_connectionsTo[toRegion].remove( pos.toString() );
	if ( m_connectionsTo[toRegion].empty() )
	{
		m_connectionsTo.remove( toRegion );
	}
}

void Region::addConnectionFrom( unsigned int toRegion, QString pos )
{
	m_connectionsFrom[toRegion].insert( pos );
}
void Region::addConnectionTo( unsigned int toRegion, QString pos )
{
	m_connectionsTo[toRegion].insert( pos );
}
void Region::removeConnectionFrom( unsigned int toRegion, QString pos )
{
	m_connectionsFrom[toRegion].remove( pos );
	if ( m_connectionsFrom[toRegion].empty() )
	{
		m_connectionsFrom.remove( toRegion );
	}
}
void Region::removeConnectionTo( unsigned int toRegion, QString pos )
{
	m_connectionsTo[toRegion].remove( pos );
	if ( m_connectionsTo[toRegion].empty() )
	{
		m_connectionsTo.remove( toRegion );
	}
}

void Region::removeAllConnectionsFrom( unsigned int id )
{
	m_connectionsFrom.remove( id );
}

void Region::removeAllConnectionsTo( unsigned int id )
{
	m_connectionsTo.remove( id );
}
