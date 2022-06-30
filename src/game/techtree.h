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

#include <absl/container/btree_map.h>
#include <QObject>
#include <absl/container/btree_set.h>
#include <absl/container/flat_hash_map.h>

struct TTNode
{
	char level = 0;
	QString itemSID;
	QString craftID;

	absl::btree_set<QString> parents;
	absl::btree_set<QString> childs;
};

class TechTree : public QObject
{
	Q_OBJECT

public:
	TechTree( QObject* parent = 0 );
	~TechTree();

	void create();

private:
	bool compsLowerLevel( QString craftID, absl::flat_hash_map<QString, int>& itemLevels );
	int levelPlusOne( QString craftID, absl::flat_hash_map<QString, int>& itemLevels );

	absl::btree_map<QString, TTNode> m_items;
};
