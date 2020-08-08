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
#include "bt_nodesequence.h"

BT_NodeSequence::BT_NodeSequence( QString name, QVariantMap& blackboard ) :
	BT_Node( name, blackboard )
{
}

BT_NodeSequence::~BT_NodeSequence()
{
}

BT_RESULT BT_NodeSequence::tick()
{
	m_status = BT_RESULT::RUNNING;

	for ( int index = 0; index < m_children.size(); ++index )
	{
		BT_RESULT child_status = m_children[index]->tick();

		if ( child_status == BT_RESULT::RUNNING )
		{
			// Suspend execution and return RUNNING.
			// At the next tick, index will be the same.
			return BT_RESULT::RUNNING;
		}
		else if ( child_status == BT_RESULT::FAILURE )
		{
			// Suspend execution and return FAILURE.
			// index is reset and children are halted.
			haltAllChildren();
			return BT_RESULT::FAILURE;
		}
	}
	// all the children returned success. Return SUCCESS too.
	haltAllChildren();
	return BT_RESULT::SUCCESS;
}