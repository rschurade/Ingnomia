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
#include "bt_nodesequencestar.h"

#include <QDebug>

BT_NodeSequenceStar::BT_NodeSequenceStar( QString name, QVariantMap& blackboard, bool resetOnFailure ) :
	BT_Node( name, blackboard ),
	m_resetOnFailure( resetOnFailure )
{
}

BT_NodeSequenceStar::~BT_NodeSequenceStar()
{
}

QVariantMap BT_NodeSequenceStar::serialize()
{
	QVariantMap out;
	out.insert( "Name", m_name );
	out.insert( "ID", m_index );
	out.insert( "Status", (unsigned char)m_status );
	out.insert( "RoF", m_resetOnFailure );

	QVariantList childs;
	for ( auto child : m_children )
	{
		childs.append( child->serialize() );
	}
	out.insert( "Childs", childs );

	return out;
}

void BT_NodeSequenceStar::deserialize( QVariantMap in )
{
	if ( m_name != in.value( "Name" ).toString() )
	{
		qDebug() << "error loading behavior tree state - nodes don't match";
	}
	m_index          = in.value( "ID" ).toInt();
	m_status         = (BT_RESULT)in.value( "Status" ).toInt();
	m_resetOnFailure = in.value( "RoF" ).toBool();

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

BT_RESULT BT_NodeSequenceStar::tick()
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
		else if ( child_status == BT_RESULT::SUCCESS )
		{
			// continue the while loop
			++m_index;
		}
		else if ( child_status == BT_RESULT::FAILURE )
		{
			// Suspend execution and return FAILURE.
			// At the next tick, index will be the same.
			//if( m_resetOnFailure )
			{
				haltAllChildren();
				m_index = 0;
			}
			return BT_RESULT::FAILURE;
		}
	}
	// all the children returned success. Return SUCCESS too.
	m_index = 0;
	haltAllChildren();
	return BT_RESULT::SUCCESS;
}