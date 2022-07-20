//
// Created by Arcnor on 20/07/2022.
// Copyright (c) 2022 Reversed Games. All rights reserved.
//
#include "stringsHelper.h"

std::string str::toUpper( const std::string& str )
{
	auto result = str;
	// FIXME: This is not I18N safe, replace with ICU
	std::transform(result.begin(), result.end(), result.begin(), ::toupper);
	return result;
}

char str::toUpper( const char c )
{
	// FIXME: This is not I18N safe, replace with ICU
	return std::toupper(c);
}

QStringList str::toStringList( const std::vector<std::string>& vector )
{
	QStringList result;

	for ( const auto& item : vector ) {
		result.push_back( QString::fromStdString( item ) );
	}

	return result;
}

std::vector<std::string> str::fromStringList( const QStringList& list )
{
	std::vector<std::string> result;

	for ( const auto& item : list ) {
		result.push_back( item.toStdString() );
	}

	return result;
}
