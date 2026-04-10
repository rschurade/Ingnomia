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
/** @file strings.h
 *  @brief Localisation singleton: loads the current-language string table and provides
 *         lookups (Strings::s) used across the GUI and game code. Also owns a number-to-word
 *         table and a random kingdom-name generator.
 */
#pragma once

#include <QMap>

/// @brief Localisation singleton. Use via `S::s("$ItemName_IronSword")` or
///        `S::gi().numberWord(3)`. The table is populated from the language .xaml files
///        in content/xaml/localization/.
class Strings
{
private:
	// Private Constructor
	Strings();
	// Stop the compiler generating methods of copy the object
	Strings( Strings const& copy );            // Not Implemented
	Strings& operator=( Strings const& copy ); // Not Implemented

	static QMap<QString, QString> m_table;  ///< Localised key → string table (shared across all callers).
	static QString m_language;              ///< Currently loaded language ID.

	QMap<int, QString> m_numberWords;       ///< Cardinal number → word mapping (e.g. 3 → "three").

	static QString replaceNamePart( QString part );
	static QString replaceNamePart2( QString part );
	static QString replaceNamePart2( QString part, QString part2 );

public:
	~Strings();

	/// @brief Returns the single Strings instance, constructed on first access.
	static Strings& getInstance()
	{
		// The only instance
		// Guaranteed to be lazy initialized
		// Guaranteed that it will be destroyed correctly
		static Strings instance;
		return instance;
	}

	/// @brief Short alias for getInstance().
	static Strings& gi()
	{
		// The only instance
		// Guaranteed to be lazy initialized
		// Guaranteed that it will be destroyed correctly
		static Strings instance;
		return instance;
	}

	bool init();

	static QString s( QString key );

	static void insertString( QString key, QString string );

	QString numberWord( int number );
	QString randomKingdomName();
};

typedef Strings S;  ///< Short alias for use in most call sites.