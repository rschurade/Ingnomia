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
/** @file techtree.h
 *  @brief Technology research tree that computes item levels from crafting dependencies.
 */
#pragma once

#include <QMap>
#include <QObject>
#include <QSet>

/** @brief A node in the tech tree representing an item, its crafting recipe, and parent/child relationships. */
struct TTNode
{
	char level = 0;
	QString itemSID;
	QString craftID;

	QSet<QString> parents;
	QSet<QString> childs;
};

/** @brief Builds a technology tree by computing item levels from crafting recipe dependencies.
 *
 *  Iterates over all crafting recipes, determines the tech level of each item based
 *  on the levels of its components, and writes the computed values back to the Items DB table.
 */
class TechTree : public QObject
{
	Q_OBJECT

public:
	TechTree( QObject* parent = 0 );
	~TechTree();

	void create();

private:
	bool compsLowerLevel( QString craftID, QHash<QString, int>& itemLevels );
	int levelPlusOne( QString craftID, QHash<QString, int>& itemLevels );

	QMap<QString, TTNode> m_items;
};
