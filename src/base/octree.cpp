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
#include "octree.h"

Octree::Octree( int x, int y, int z, int dx, int dy, int dz ) :
	m_x( x ),
	m_y( y ),
	m_z( z ),
	m_dx( dx ),
	m_dy( dy ),
	m_dz( dz ),
	m_isLeaf( dx <= 4 || dy <= 4 || dz <= 4 )
{
}

Octree::~Octree()
{
	for ( auto ot : m_children )
	{
		delete ot;
	}
	m_items.clear();
}

void Octree::insertItem( int x, int y, int z, unsigned int item )
{
	if ( m_isLeaf )
	{
		m_items.insert( item );
	}
	else
	{
		int id = 0;
		if ( x >= m_x )
			id += 2;
		if ( y >= m_y )
			id += 1;
		if ( z >= m_z )
			id += 4;
		if ( !m_children[id] )
		{
			int x2         = m_dx / 2;
			int y2         = m_dy / 2;
			int z2         = m_dz / 2;
			m_children[id] = new Octree(
				( x >= m_x ) ? m_x + x2 : m_x - x2,
				( y >= m_y ) ? m_y + y2 : m_y - y2,
				( z >= m_z ) ? m_z + z2 : m_z - z2,
				x2, y2, z2 );
		}
		m_children[id]->insertItem( x, y, z, item );
	}
}

bool Octree::removeItem( int x, int y, int z, unsigned int item )
{
	if ( m_isLeaf )
	{
		m_items.remove( item );
		return m_items.isEmpty();
	}
	else
	{
		int id = 0;
		if ( x >= m_x )
			id += 2;
		if ( y >= m_y )
			id += 1;
		if ( z >= m_z )
			id += 4;
		if ( m_children[id] )
		{
			bool empty = m_children[id]->removeItem( x, y, z, item );
			if ( empty )
			{
				delete m_children[id];
				m_children[id] = nullptr;
			}
		}
		for ( int i = 0; i < 8; ++i )
		{
			if ( m_children[i] )
				return false;
		}
		return true;
	}
}

QList<unsigned int> Octree::query( int x, int y, int z, int limit ) const
{
	if ( m_isLeaf )
	{
		return m_items.values();
	}
	else
	{
		int id = 0;
		if ( x >= m_x )
			id += 2;
		if ( y >= m_y )
			id += 1;
		if ( z >= m_z )
			id += 4;

		QList<unsigned int> out;

		for ( int i = 0; i < 8; ++i )
		{
			int j = ( i + id ) % 8;
			if ( m_children[j] )
			{
				out.append( m_children[j]->query( x, y, z, limit ) );
				if ( out.size() > limit )
				{
					return out;
				}
			}
		}
		return out;
	}
}

bool Octree::visit( int x, int y, int z, const std::function<bool( unsigned int )>& visitor ) const
{
	if ( m_isLeaf )
	{
		for ( const auto& item : m_items )
		{
			if ( !visitor( item ) )
			{
				return false;
			}
		}
		return true;
	}
	else
	{
		int id = 0;
		if ( x >= m_x )
			id += 2;
		if ( y >= m_y )
			id += 1;
		if ( z >= m_z )
			id += 4;

		for ( int i = 0; i < 8; ++i )
		{
			int j = ( i + id ) % 8;
			if ( m_children[j] && !m_children[j]->visit( x, y, z, visitor ) )
			{
				return false;
			}
		}
		return true;
	}
}
