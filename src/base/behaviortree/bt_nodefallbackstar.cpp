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
#include "bt_nodefallbackstar.h"

BT_NodeFallbackStar::BT_NodeFallbackStar( QString name, QVariantMap& blackboard ) :
	BT_Node( name, blackboard )
{
}

BT_NodeFallbackStar::~BT_NodeFallbackStar()
{
}

BT_RESULT BT_NodeFallbackStar::tick()
{
	m_status = BT_RESULT::RUNNING;

	while ( m_index < m_children.size() )
	{
		BT_RESULT child_status = m_children[m_index]->tick();

		if ( child_status == BT_RESULT::RUNNING )
		{
			// Suspend execution and return RUNNING.
			// At the next tick, index will be the same.
			return BT_RESULT::RUNNING;
		}
		else if ( child_status == BT_RESULT::FAILURE )
		{
			// continue the while loop
			++m_index;
		}
		else if ( child_status == BT_RESULT::SUCCESS )
		{
			// Suspend execution and return SUCCESS.
			// At the next tick, index will be the same.
			haltAllChildren();
			m_index = 0;
			return BT_RESULT::SUCCESS;
		}
	}
	// all the children returned FAILURE. Return FAILURE too.
	haltAllChildren();
	m_index = 0;
	return BT_RESULT::FAILURE;
}