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
/** @file bt_nodefallback.h
 *  @brief Composite node implementing selector / fallback (OR) logic.
 */
#pragma once

#include "bt_node.h"

/** @brief Fallback (selector) composite -- ticks children until one succeeds.
 *
 *  Each tick iterates children from index 0.  Returns SUCCESS as soon as any
 *  child succeeds, RUNNING if a child is still running, or FAILURE only when
 *  all children have failed.  Does not remember which child was running between
 *  ticks (stateless); see BT_NodeFallbackStar for the variant with memory.
 */
class BT_NodeFallback final : public BT_Node
{
public:
	BT_NodeFallback( QString name, QVariantMap& blackboard );
	~BT_NodeFallback();

	BT_RESULT tick();
};
