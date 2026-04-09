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
/** @file priorityqueue.h
 * @brief Min-priority queue wrapper used by pathfinding and job scheduling.
 */

#pragma once

#include <queue>

#include <functional>
#include <utility>

/**
 * @brief Min-priority queue — lowest priority value is dequeued first.
 *
 * Wraps std::priority_queue with std::greater for min-heap behavior.
 * Used by A* pathfinding and job priority sorting.
 * @tparam T The element type.
 * @tparam priority_t The priority type (e.g. int, float).
 * @tparam value_t Internal pair type, defaults to std::pair<priority_t, T>.
 */
template <typename T, typename priority_t, typename value_t = std::pair<priority_t, T>>
class PriorityQueue : public std::priority_queue<value_t, std::vector<value_t>, std::greater<value_t>>
{
public:
	using base_type = std::priority_queue<value_t, std::vector<value_t>, std::greater<value_t>>;

	/**
	 * @brief Inserts an item with the given priority.
	 * @param item The element to insert.
	 * @param priority The priority value (lower = dequeued sooner).
	 */
	inline void put( const T &item, const priority_t &priority )
	{
		base_type::emplace( priority, item );
	}

	/**
	 * @brief Removes and returns the item with the lowest priority value.
	 * @return The highest-priority (lowest value) element.
	 */
	inline T get()
	{
		T best_item = base_type::top().second;
		base_type::pop();
		return best_item;
	}

	/**
	 * @brief Returns a const reference to the underlying storage vector.
	 * @return Reference to the internal vector of (priority, item) pairs.
	 */
	inline const std::vector<value_t>& raw() const
	{
		return base_type::c;
	}
};
