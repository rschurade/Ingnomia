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
#include "bt_noderepeatuntilsuccess.h"

#include <QDebug>

BT_NodeRepeatUntilSuccess::BT_NodeRepeatUntilSuccess( QString name, int num, QVariantMap& blackboard ) :
	BT_Node( name, blackboard ),
	m_num( num )
{
}

BT_NodeRepeatUntilSuccess::~BT_NodeRepeatUntilSuccess()
{
}

QVariantMap BT_NodeRepeatUntilSuccess::serialize()
{
	QVariantMap out;
	out.insert( "Name", m_name );
	out.insert( "ID", m_index );
	out.insert( "Status", (unsigned char)m_status );
	out.insert( "Num", m_num );

	QVariantList childs;
	for ( auto child : m_children )
	{
		childs.append( child->serialize() );
	}
	out.insert( "Childs", childs );

	return out;
}

void BT_NodeRepeatUntilSuccess::deserialize( QVariantMap in )
{
	if ( m_name != in.value( "Name" ).toString() )
	{
		qDebug() << "error loading behavior tree state - nodes don't match";
	}
	m_index  = in.value( "ID" ).toInt();
	m_status = (BT_RESULT)in.value( "Status" ).toInt();
	m_num    = in.value( "Num" ).toInt();

	auto vcl  = in.value( "Childs" ).toList();
	int index = 0;
	if ( vcl.size() == m_children.size() )
	{

		for ( auto child : m_children )
		{
			child->deserialize( vcl[index++].toMap() );
		}
	}
	else
	{
		//tree changed between saving and loading, this will have undetermined results
		// TODO throw exception or make config option to allow or deny loading this
		if ( vcl.size() < m_children.size() )
		{
			for ( auto vcm : vcl )
			{
				m_children[index++]->deserialize( vcm.toMap() );
			}
		}
		else
		{
			for ( auto child : m_children )
			{
				child->deserialize( vcl[index++].toMap() );
			}
		}
	}
}

BT_RESULT BT_NodeRepeatUntilSuccess::tick()
{
	while ( m_index < m_num )
	{
		if ( m_children.size() > 0 )
		{
			BT_RESULT result = m_children[0]->tick();

			if ( result == BT_RESULT::RUNNING )
			{
				return BT_RESULT::RUNNING;
			}
			else if ( result == BT_RESULT::SUCCESS )
			{
				m_index = 0;
				return BT_RESULT::SUCCESS;
			}
		}
		++m_index;
	}
	m_index = 0;
	return BT_RESULT::FAILURE;
}