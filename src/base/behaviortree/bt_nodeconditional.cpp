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
#include "bt_nodeconditional.h"

BT_NodeConditional::BT_NodeConditional( QString name, QVariantMap& blackboard, std::function<BT_RESULT( bool )> callback ) :
	BT_Node( name, blackboard ),
	m_callback( callback )
{
}

BT_NodeConditional::~BT_NodeConditional()
{
}

BT_RESULT BT_NodeConditional::tick()
{
	m_status = m_callback( false );
	return m_status;
}

void BT_NodeConditional::halt()
{
	if ( m_status == BT_RESULT::RUNNING )
	{
		m_status = BT_RESULT::IDLE; // m_callback( true );
	}
}