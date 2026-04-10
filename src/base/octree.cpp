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
/** @file octree.cpp
 *  @brief Octree spatial index for 3D item lookups.
 *
 *  Recursively subdivides 3D space into octants until leaf nodes (with any
 *  dimension <= 4) are reached. Supports insert, remove, query by proximity,
 *  and visitor-pattern traversal.
 */
#include "octree.h"

/** @brief Constructs an Octree node centered at (x, y, z) with half-extents (dx, dy, dz).
 *
 *  Becomes a leaf node if any half-extent is 4 or less.
 *
 *  @param x  Center X coordinate.
 *  @param y  Center Y coordinate.
 *  @param z  Center Z coordinate.
 *  @param dx Half-extent along X.
 *  @param dy Half-extent along Y.
 *  @param dz Half-extent along Z.
 */
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

/** @brief Destructor. Recursively deletes all child nodes and clears items. */
Octree::~Octree()
{
	for ( auto ot : m_children )
	{
		delete ot;
	}
	m_items.clear();
}

/** @brief Inserts an item at the given 3D position.
 *
 *  If this node is a leaf, the item is stored directly. Otherwise, determines
 *  the appropriate octant child based on the position relative to the center,
 *  creating the child node if it does not yet exist, and recurses.
 *
 *  @param x    X coordinate of the item.
 *  @param y    Y coordinate of the item.
 *  @param z    Z coordinate of the item.
 *  @param item The item identifier to insert.
 */
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

/** @brief Removes an item from the given 3D position.
 *
 *  Navigates to the appropriate leaf and removes the item. On the way back up,
 *  deletes child nodes that have become empty, pruning the tree.
 *
 *  @param x    X coordinate of the item.
 *  @param y    Y coordinate of the item.
 *  @param z    Z coordinate of the item.
 *  @param item The item identifier to remove.
 *  @return True if this node is now empty and can be deleted by the parent.
 */
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

/** @brief Queries items near the given 3D position.
 *
 *  At leaf nodes, returns all stored items. At internal nodes, visits children
 *  starting from the octant closest to the query position and spiraling outward.
 *  Stops collecting once the result count exceeds @p limit.
 *
 *  @param x     X coordinate of the query point.
 *  @param y     Y coordinate of the query point.
 *  @param z     Z coordinate of the query point.
 *  @param limit Maximum number of items to collect before stopping.
 *  @return List of item identifiers found near the query position.
 */
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

/** @brief Visits items near the given position using a visitor callback.
 *
 *  Similar to query(), but invokes @p visitor for each item. The visitor returns
 *  true to continue, false to stop early. Children are visited starting from
 *  the octant closest to the query position.
 *
 *  @param x       X coordinate of the query point.
 *  @param y       Y coordinate of the query point.
 *  @param z       Z coordinate of the query point.
 *  @param visitor Callback invoked for each item; return false to stop traversal.
 *  @return True if all items were visited, false if the visitor stopped early.
 */
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
