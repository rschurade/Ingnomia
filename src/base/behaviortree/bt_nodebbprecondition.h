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
/** @file bt_nodebbprecondition.h
 *  @brief Decorator that gates its child on a blackboard value check.
 */
#pragma once

#include "bt_node.h"

/** @brief Decorator node that checks a blackboard entry before ticking its child.
 *
 *  On tick, compares @c m_blackboard[m_key] against @c m_expected.  If they
 *  match (or @c m_expected is @c "*", acting as a wildcard), the single child
 *  is ticked and its result returned.  Otherwise returns FAILURE immediately.
 */
class BT_NodeBBPrecondition final : public BT_Node
{
public:
	BT_NodeBBPrecondition( QString name, QString key, QString expected, QVariantMap& blackboard );
	~BT_NodeBBPrecondition();

	BT_RESULT tick();

private:
	QString m_key;      ///< Blackboard key to look up.
	QString m_expected; ///< Expected value, or "*" to match any.
};
