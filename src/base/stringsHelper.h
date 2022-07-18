//
// Created by Arcnor on 18/07/2022.
// Copyright (c) 2022 Reversed Games. All rights reserved.
//

#ifndef INGNOMIA_STRINGSHELPER_H
#define INGNOMIA_STRINGSHELPER_H

#include <string>
#include <algorithm>

namespace str
{
	std::string toUpper(const std::string& str) {
		auto result = str;
		// FIXME: This is not I18N safe, replace with ICU
		std::transform(result.begin(), result.end(), result.begin(), ::toupper);
		return result;
	}

	char toUpper(const char c) {
		// FIXME: This is not I18N safe, replace with ICU
		return std::toupper(c);
	}
}

#endif // INGNOMIA_STRINGSHELPER_H
