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
/** @file strings.cpp
 *  @brief Strings implementation: loads the Translation DB table into the in-memory lookup
 *         table, provides lookup with an obvious "Error: <key>" fallback, and generates
 *         random kingdom names by composing word-table entries per the Namerules_Rule schema.
 */
#include "strings.h"

#include "../base/config.h"
#include "../base/db.h"

#include <QApplication>
#include <QDebug>
#include <QJsonDocument>
#include <QVariantMap>

QMap<QString, QString> Strings::m_table = QMap<QString, QString>();
QString Strings::m_language             = "";

/// @brief Private constructor (singleton pattern).
Strings::Strings()
{
}

/// @brief Destructor.
Strings::~Strings()
{
}

/// @brief Loads the Translation DB table into the in-memory key → string map. The current
///        language is read from Global::cfg["language"].
/// @return Always true (kept as bool for future error reporting).
bool Strings::init()
{
	m_language = Global::cfg->get( "language" ).toString();
	m_table.clear();
	//for( auto row : DB::selectRows( "Translation_" + m_language ) )
	for ( auto row : DB::selectRows( "Translation" ) )
	{
		m_table.insert( row.value( "ID" ).toString(), row.value( "Text" ).toString() );
	}

	return true;
}

/// @brief Looks up a localised string by key. Returns "Error: <key>" when the key is
///        missing so untranslated strings are visible in the GUI during development.
/// @param key Translation key (e.g. "$ItemName_IronSword").
/// @return Localised string, or an error placeholder.
QString Strings::s( QString key )
{
	if ( m_table.contains( key ) )
	{
		return m_table[key];
	}
	else
	{
		return "Error: " + key;
		//return m_table["$ERROR_TranslationKeyUnknown"].toString();
	}
}

/// @brief Inserts or overwrites a single translation at runtime (used for dynamic strings
///        like gnome names that aren't present in the DB).
/// @param key    Translation key.
/// @param string Localised value.
void Strings::insertString( QString key, QString string )
{
	m_table.insert( key, string );
}

/// @brief Generates a random kingdom name by picking one of the Namerules_Rule "Faction"
///        entries and composing its parts via replaceNamePart().
/// @return Freshly generated kingdom name.
QString Strings::randomKingdomName()
{
	auto ruleList = DB::selectRows( "Namerules_Rule", "Faction" );

	srand( std::chrono::system_clock::now().time_since_epoch().count() );
	auto rule = ruleList.at( rand() % ruleList.size() ).value( "Part" ).toString().split( "|" );
	QString name;
	for ( auto part : rule )
	{
		name += replaceNamePart( part );
		name += " ";
	}
	name.remove( name.size() - 1, 1 );
	return name;
}

/// @brief Resolves one name-rule part into an actual word. Reserved words ("The", "of",
///        "Land", "Kingdom") are returned verbatim; otherwise the part is split on '_' and
///        dispatched to replaceNamePart2() based on arity.
/// @param part Name-rule token.
/// @return Resolved word with the first letter upper-cased.
QString Strings::replaceNamePart( QString part )
{
	QSet<QString> noReplaceables = { "The", "of", "Land", "Kingdom" };
	if ( noReplaceables.contains( part ) )
	{
		return part;
	}
	QStringList pl = part.split( "_" );
	QString out;

	switch ( pl.size() )
	{
		case 1: // only "Adjective" so far
			out = replaceNamePart2( pl.at( 0 ) );
			break;
		case 2:
			out = replaceNamePart2( pl.at( 0 ), pl.at( 1 ) );
			break;
		case 3: // comppounds
		{
			if ( pl.at( 0 ) == "Compound" )
			{
				out = replaceNamePart2( pl.at( 1 ), pl.at( 2 ) );
				out += replaceNamePart2( pl.at( 1 ), pl.at( 2 ) );
			}
		}
		break;
	}
	out.replace( 0, 1, out[0].toUpper() );
	return out;
}

/// @brief Picks a random row from the Words_<part> table and returns its "Word" column.
/// @param part Word category (e.g. "Adjective").
/// @return Randomly chosen word.
QString Strings::replaceNamePart2( QString part )
{
	QString tableName = "Words_" + part;
	int ra            = rand() % DB::numRows( tableName );
	return DB::select( "Word", tableName, ra ).toString();
}

/// @brief Picks a random row from the Words_<part> table and returns a specified column.
///        "Singular" is normalised to "Word" to match the column naming in most tables.
/// @param part  Word category.
/// @param part2 Column name to read (or "Singular").
/// @return Randomly chosen word in the requested form.
QString Strings::replaceNamePart2( QString part, QString part2 )
{
	if ( part2 == "Singular" )
	{
		part2 = "Word";
	}
	QString tableName = "Words_" + part;
	int ra            = rand() % DB::numRows( tableName );
	return DB::select( part2, tableName, ra ).toString();
}

/// @brief Returns the English word for a non-negative integer up to 19 (from the
///        Words_Numbers DB table). For values outside that range, returns the numeric
///        string directly. Negative values return "no number".
/// @param number Integer to spell out.
/// @return Word form, numeric string, or "no number".
QString Strings::numberWord( int number )
{
	if ( number >= 0 )
	{
		if ( number < 20 )
		{
			auto result = DB::select2( "Word", "Words_Numbers", "Number", number );
			if ( result.size() > 0 )
			{
				return result.first().toString();
			}
		}
		return QString::number( number );
	}
	return "no number";
}