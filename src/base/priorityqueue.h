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

#include <queue>

#include <functional>
#include <utility>

template <typename T, typename priority_t, typename value_t = std::pair<priority_t, T>>
class PriorityQueue : public std::priority_queue<value_t, std::vector<value_t>, std::greater<value_t>>
{
public:
	using base_type = std::priority_queue<value_t, std::vector<value_t>, std::greater<value_t>>;
	inline void put( const T &item, const priority_t &priority )
	{
		base_type::emplace( priority, item );
	}

	inline T get()
	{
		T best_item = base_type::top().second;
		base_type::pop();
		return best_item;
	}

	inline const std::vector<value_t>& raw() const
	{
		return base_type::c;
	}
};
