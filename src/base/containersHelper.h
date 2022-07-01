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

namespace maps
{

template <template <class, class, class...> class C, typename K, typename V, typename... Args>
const V& at_or_default( const C<K, V, Args...>& map, K const& key, const V& defval )
{
	typename C<K, V, Args...>::const_iterator it = map.find( key );
	return it == map.end() ? defval : it->second;
}

template <template <class, class, class...> class C, typename K, typename V, typename... Args>
bool try_at( const C<K, V, Args...>& map, K const& key, V& outVal )
{
	typename C<K, V, Args...>::const_iterator it = map.find( key );
	if ( it == map.end() )
	{
		return false;
	}
	else
	{
		outVal = it->second;
		return true;
	}
}

template <class T, typename... Args>
const T& get_or_default( const std::variant<Args...>& variant, const T& defval )
{
	const auto* result = std::get_if<T>( &variant );
	if ( result != nullptr )
	{
		return *result;
	}
	else
	{
		return defval;
	}
}

}
