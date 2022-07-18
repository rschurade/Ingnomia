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

#include "bt_tree.h"

#include "pugixml.hpp"
#include <absl/container/flat_hash_map.h>
#include <QObject>
#include <QVariantMap>

#include <functional>

using BT_ActionMap = absl::flat_hash_map<std::string, std::function<BT_RESULT( bool )>>;

class BT_Factory
{
public:
	BT_Factory()  = delete;
	~BT_Factory() = delete;

	static BT_Node* load( const pugi::xml_document& doc, BT_ActionMap& actions, QVariantMap& blackboard );

private:
	static BT_Node* createBTNode( const pugi::xml_node& treeElement, BT_Node* parent, const pugi::xml_node& documentRoot, BT_ActionMap& actions, QVariantMap& blackboard );

	static BT_Node* getTree( const std::string& treeID, const pugi::xml_node& documentRoot, BT_ActionMap& actions, QVariantMap& blackboard );

	static void getNodes( BT_Node* parent, const pugi::xml_node& root, const pugi::xml_node& documentRoot, BT_ActionMap& actions, QVariantMap& blackboard );
};
