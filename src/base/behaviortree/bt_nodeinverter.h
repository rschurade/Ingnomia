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
/** @file bt_nodeinverter.h
 *  @brief Decorator that inverts SUCCESS and FAILURE of its child.
 */
#pragma once

#include "bt_node.h"

/** @brief Decorator that inverts the child's result.
 *
 *  Ticks its single child.  SUCCESS becomes FAILURE, FAILURE becomes SUCCESS,
 *  and RUNNING is passed through unchanged.  If there is no child, returns
 *  FAILURE.
 */
class BT_NodeInverter final : public BT_Node
{
public:
	BT_NodeInverter( QString name, QVariantMap& blackboard );
	~BT_NodeInverter();

	BT_RESULT tick();
};
