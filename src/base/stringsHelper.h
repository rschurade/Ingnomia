//
// Created by Arcnor on 18/07/2022.
// Copyright (c) 2022 Reversed Games. All rights reserved.
//

#ifndef INGNOMIA_STRINGSHELPER_H
#define INGNOMIA_STRINGSHELPER_H

#include <string>
#include <algorithm>
#include <vector>
#include <QStringList>

namespace str
{
	std::string toUpper(const std::string& str);

	char toUpper(const char c);

	QStringList toStringList(const std::vector<std::string>& vector);

	std::vector<std::string> fromStringList(const QStringList& list);
}

#endif // INGNOMIA_STRINGSHELPER_H
