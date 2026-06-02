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
/** @file bt_nodeforcefailure.h
 *  @brief Decorator that forces its child's result to FAILURE.
 */
#pragma once

#include "bt_node.h"

/** @brief Decorator that always returns FAILURE regardless of the child's result.
 *
 *  Ticks its single child.  If the child returns RUNNING, RUNNING is
 *  propagated; any other result (SUCCESS or FAILURE) is mapped to FAILURE.
 *  If there is no child, returns FAILURE immediately.
 */
class BT_NodeForceFailure final : public BT_Node
{
public:
	BT_NodeForceFailure( QVariantMap& blackboard );
	~BT_NodeForceFailure();

	BT_RESULT tick();
};
