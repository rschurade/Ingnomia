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
/** @file vptr.h
 * @brief Type-safe void pointer wrapper for storing C++ pointers in QVariant.
 */

#pragma once

#include <QVariant>

/**
 * @brief Template helper for round-tripping typed pointers through QVariant.
 * @tparam T The pointed-to type.
 */
template <class T>
class VPtr
{
public:
	/**
	 * @brief Extracts a typed pointer from a QVariant containing a void*.
	 * @param v The QVariant holding the pointer.
	 * @return The typed pointer.
	 */
	static T* asPtr( QVariant v )
	{
		return (T*)v.value<void*>();
	}

	/**
	 * @brief Wraps a typed pointer into a QVariant as void*.
	 * @param ptr The pointer to wrap.
	 * @return A QVariant containing the pointer.
	 */
	static QVariant asQVariant( T* ptr )
	{
		return qVariantFromValue( (void*)ptr );
	}
};
