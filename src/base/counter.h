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
/** @file counter.h
 * @brief Generic counting/histogram template for profiling and diagnostics.
 */

#pragma once

#include <QMap>

/**
 * @brief Simple counting map that tracks how many times each key has been added.
 * @tparam T The key type (e.g. QString for query strings).
 */
template <class T>
class Counter
{
public:
	/**
	 * @brief Increments the count for the given key by one.
	 * @param id The key to count.
	 */
	void add( T id )
	{
		unsigned int val = m_counts.value( id );
		++val;
		m_counts.insert( id, val );
	}

	/**
	 * @brief Returns all tracked keys.
	 * @return List of keys that have been added at least once.
	 */
	QList<T> keys()
	{
		return m_counts.keys();
	}

	/**
	 * @brief Returns the count for a specific key.
	 * @param key The key to query.
	 * @return Number of times add() was called for this key.
	 */
	unsigned int count( T key )
	{
		return m_counts.value( key );
	}

	/**
	 * @brief Resets all counts to zero by clearing the map.
	 */
	void reset()
	{
		m_counts.clear();
	}

private:
	QMap<T, unsigned int> m_counts; ///< Map of key to occurrence count.
};