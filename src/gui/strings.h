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

#include <QString>
#include <absl/container/btree_map.h>

class Strings
{
private:
	// Private Constructor
	Strings();
	// Stop the compiler generating methods of copy the object
	Strings( Strings const& copy );            // Not Implemented
	Strings& operator=( Strings const& copy ); // Not Implemented

	static absl::btree_map<QString, QString> m_table;
	static std::string m_language;

	absl::btree_map<int, QString> m_numberWords;

	static QString replaceNamePart( QString part );
	static QString replaceNamePart2( QString part );
	static QString replaceNamePart2( QString part, QString part2 );

public:
	~Strings();

	static Strings& getInstance()
	{
		// The only instance
		// Guaranteed to be lazy initialized
		// Guaranteed that it will be destroyed correctly
		static Strings instance;
		return instance;
	}

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
	std::string randomKingdomName();
};

typedef Strings S;