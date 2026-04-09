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
/** @file bt_nodebbprecondition.cpp
 *  @brief Implementation of BT_NodeBBPrecondition -- blackboard-gated decorator.
 */
#include "bt_nodebbprecondition.h"

/** @brief Construct a blackboard precondition decorator.
 *  @param name       Debug name for the node.
 *  @param key        Blackboard key whose value will be tested.
 *  @param expected   Required string value, or "*" to accept any value.
 *  @param blackboard Shared blackboard reference.
 */
BT_NodeBBPrecondition::BT_NodeBBPrecondition( QString name, QString key, QString expected, QVariantMap& blackboard ) :
	BT_Node( name, blackboard ),
	m_key( key ),
	m_expected( expected )
{
}

/** @brief Destructor. */
BT_NodeBBPrecondition::~BT_NodeBBPrecondition()
{
}

/** @brief Check the blackboard value; tick the child if it matches, else return FAILURE.
 *  @return The child's tick result if the precondition holds, otherwise FAILURE.
 */
BT_RESULT BT_NodeBBPrecondition::tick()
{
	if ( m_blackboard.value( m_key ).toString() == m_expected || m_expected == "*" )
	{
		if ( m_children.size() > 0 )
		{
			return m_children[0]->tick();
		}
	}
	return BT_RESULT::FAILURE;
}
