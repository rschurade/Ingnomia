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
#include "strings.h"

#include "../base/config.h"
#include "../base/db.h"

#include <QApplication>
#include <QDebug>
#include <QJsonDocument>
#include <QVariantMap>

QMap<QString, QString> Strings::m_table = QMap<QString, QString>();
QString Strings::m_language             = "";

Strings::Strings()
{
}

Strings::~Strings()
{
}

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

void Strings::insertString( QString key, QString string )
{
	m_table.insert( key, string );
}

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

QString Strings::replaceNamePart2( QString part )
{
	QString tableName = "Words_" + part;
	int ra            = rand() % DB::numRows( tableName );
	return DB::select( "Word", tableName, ra ).toString();
}

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