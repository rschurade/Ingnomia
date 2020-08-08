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

#include <QList>
#include <QSet>

#include <functional>

class Octree
{
public:
	Octree() = delete;
	Octree( int x, int y, int z, int dx, int dy, int dz );
	~Octree();

	void insertItem( int x, int y, int z, unsigned int item );
	bool removeItem( int x, int y, int z, unsigned int item );

	QList<unsigned int> query( int x, int y, int z, int limit = 999999999 ) const;
	bool visit( int x, int y, int z, const std::function<bool( unsigned int )>& visitor ) const;

private:
	const int m_x;
	const int m_y;
	const int m_z;

	const int m_dx;
	const int m_dy;
	const int m_dz;

	const bool m_isLeaf;

	Octree* m_children[8] = { 0 };
	QSet<unsigned int> m_items;
};
