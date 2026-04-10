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
/** @file bt_nodeaction.cpp
 *  @brief Implementation of BT_NodeAction -- leaf node that invokes an action callback.
 */
#include "bt_nodeaction.h"

/** @brief Construct an action leaf node.
 *  @param name       Debug name (typically the action ID from XML).
 *  @param blackboard Shared blackboard reference.
 *  @param callback   Function to call on each tick; param is true when halting.
 */
BT_NodeAction::BT_NodeAction( QString name, QVariantMap& blackboard, std::function<BT_RESULT( bool )> callback ) :
	BT_Node( name, blackboard ),
	m_callback( callback )
{
}

/** @brief Destructor. */
BT_NodeAction::~BT_NodeAction()
{
}

/** @brief Execute the action callback with halt=false and return its result.
 *  @return The BT_RESULT produced by the callback.
 */
BT_RESULT BT_NodeAction::tick()
{
	m_status = m_callback( false );
	return m_status;
}

/** @brief If RUNNING, call the callback with halt=true so it can clean up, then go IDLE. */
void BT_NodeAction::halt()
{
	if ( m_status == BT_RESULT::RUNNING )
	{
		m_callback( true );
		m_status = BT_RESULT::IDLE;
	}
}