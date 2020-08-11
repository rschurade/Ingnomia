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
#include "bt_nodebbprecondition.h"

BT_NodeBBPrecondition::BT_NodeBBPrecondition( QString name, QString key, QString expected, QVariantMap& blackboard ) :
	BT_Node( name, blackboard ),
	m_key( key ),
	m_expected( expected )
{
}

BT_NodeBBPrecondition::~BT_NodeBBPrecondition()
{
}

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
