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

#include <map>

//template<typename Key, typename Value>
//using Map = absl::btree_map<Key, Value>;
//
//template<typename Key, typename Value>
//using MapIterator = typename Map<Key, Value>::iterator;
//
//template<typename Key, typename Value>
//class MapKeyIterator : public MapIterator<Key, Value> {
//public:
//	MapKeyIterator() : MapIterator<Key, Value>() {};
//	MapKeyIterator(MapIterator<Key, Value> it_) : MapIterator<Key, Value>( it_ ) {};
//
//	Key* operator->() { return (Key* const)&( MapIterator<Key, Value>::operator->()->first ); }
//	Key operator*() { return MapIterator<Key, Value>::operator*().first; }
//};
//
//template<typename Key, typename Value>
//class MapValueIterator : public MapIterator<Key, Value> {
//public:
//	MapValueIterator() : MapIterator<Key, Value>() {};
//	MapValueIterator(MapIterator<Key, Value> it_) : MapIterator<Key, Value>( it_ ) {};
//
//	Value* operator->() { return (Value* const)&( MapIterator<Key, Value>::operator->()->second ); }
//	Value operator*() { return MapIterator<Key, Value>::operator*().second; }
//};

template <typename T>
class MapKeyIterator : public T
{
public:
	MapKeyIterator() :
		T()
	{
	}
	MapKeyIterator( T iter ) :
		T( iter )
	{
	}
	auto* operator->()
	{
		return &( T::operator->()->first );
	}
	auto& operator*()
	{
		return T::operator*().first;
	}
};