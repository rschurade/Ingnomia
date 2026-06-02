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
/** @file bt_nodeconditional.cpp
 *  @brief Implementation of BT_NodeConditional -- leaf condition check node.
 */
#include "bt_nodeconditional.h"

/** @brief Construct a conditional leaf node.
 *  @param name       Debug name (typically the condition ID from XML).
 *  @param blackboard Shared blackboard reference.
 *  @param callback   Condition function to evaluate on each tick.
 */
BT_NodeConditional::BT_NodeConditional( QString name, QVariantMap& blackboard, std::function<BT_RESULT( bool )> callback ) :
	BT_Node( name, blackboard ),
	m_callback( callback )
{
}

/** @brief Destructor. */
BT_NodeConditional::~BT_NodeConditional()
{
}

/** @brief Evaluate the condition callback with halt=false and return its result.
 *  @return The BT_RESULT produced by the callback.
 */
BT_RESULT BT_NodeConditional::tick()
{
	m_status = m_callback( false );
	return m_status;
}

/** @brief If RUNNING, reset status to IDLE (conditions do not call back on halt). */
void BT_NodeConditional::halt()
{
	if ( m_status == BT_RESULT::RUNNING )
	{
		m_status = BT_RESULT::IDLE; // m_callback( true );
	}
}