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

#include <QMap>

template <class T>
class Counter
{
public:
	void add( T id )
	{
		unsigned int val = m_counts.value( id );
		++val;
		m_counts.insert( id, val );
	}

	QList<T> keys()
	{
		return m_counts.keys();
	}

	unsigned int count( T key )
	{
		return m_counts.value( key );
	}

	void reset()
	{
		m_counts.clear();
	}

private:
	QMap<T, unsigned int> m_counts;
};