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
/** @file bt_noderepeat.h
 *  @brief Decorator that repeats its child a fixed number of times.
 */
#pragma once

#include "bt_node.h"

/** @brief Decorator that ticks its child up to N times.
 *
 *  Repeats the child node @c m_num times.  If the child returns RUNNING,
 *  execution is suspended and RUNNING is returned.  If the child fails on
 *  any iteration the loop aborts with FAILURE.  Returns SUCCESS after all
 *  iterations complete successfully.
 */
class BT_NodeRepeat final : public BT_Node
{
public:
	BT_NodeRepeat( QString name, int num, QVariantMap& blackboard );
	~BT_NodeRepeat();

	QVariantMap serialize();
	void deserialize( QVariantMap in );

	BT_RESULT tick();

private:
	int m_num = 0; ///< Total number of repetitions requested.
};
