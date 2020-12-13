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
#include "bt_node.h"

#include "bt_tree.h"

#include <QDebug>

BT_Node::BT_Node( QString name, QVariantMap& blackboard ) :
	m_name( name ),
	m_blackboard( blackboard )
{
}

BT_Node::~BT_Node()
{
	for ( const auto& child : m_children )
	{
		delete child;
	}
}

QVariantMap BT_Node::serialize() const
{
	QVariantMap out;
	out.insert( "Name", m_name );
	out.insert( "ID", m_index );
	out.insert( "Status", (unsigned char)m_status );

	QVariantList childs;
	for ( const auto& child : m_children )
	{
		childs.append( child->serialize() );
	}
	out.insert( "Childs", childs );

	return out;
}

void BT_Node::deserialize( QVariantMap in )
{
	if ( m_name != in.value( "Name" ).toString() )
	{
		qDebug() << "error loading behavior tree state - nodes don't match";
	}
	m_index  = in.value( "ID" ).toInt();
	m_status = (BT_RESULT)in.value( "Status" ).toInt();

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

BT_Node* BT_Node::addFallback( QString name )
{
	BT_NodeFallback* bn = new BT_NodeFallback( name, m_blackboard );
	m_children.push_back( bn );

	return bn;
}

BT_Node* BT_Node::addFallbackStar( QString name )
{
	BT_NodeFallbackStar* bn = new BT_NodeFallbackStar( name, m_blackboard );
	m_children.push_back( bn );

	return bn;
}

BT_Node* BT_Node::addSequence( QString name )
{
	BT_NodeSequence* bn = new BT_NodeSequence( name, m_blackboard );
	m_children.push_back( bn );

	return bn;
}

BT_Node* BT_Node::addSequenceStar( QString name )
{
	BT_NodeSequenceStar* bn = new BT_NodeSequenceStar( name, m_blackboard, true );
	m_children.push_back( bn );

	return bn;
}

BT_Node* BT_Node::addForceSuccess()
{
	BT_NodeForceSuccess* bn = new BT_NodeForceSuccess( m_blackboard );
	m_children.push_back( bn );

	return bn;
}

BT_Node* BT_Node::addForceFailure()
{
	BT_NodeForceFailure* bn = new BT_NodeForceFailure( m_blackboard );
	m_children.push_back( bn );

	return bn;
}

BT_Node* BT_Node::addInverter( QString name )
{
	BT_NodeInverter* bn = new BT_NodeInverter( name, m_blackboard );
	m_children.push_back( bn );

	return bn;
}

BT_Node* BT_Node::addRepeat( QString name, int num )
{
	BT_NodeRepeat* bn = new BT_NodeRepeat( name, num, m_blackboard );
	m_children.push_back( bn );

	return bn;
}

BT_Node* BT_Node::addRepeatUntilSuccess( QString name, int num )
{
	BT_NodeRepeatUntilSuccess* bn = new BT_NodeRepeatUntilSuccess( name, num, m_blackboard );
	m_children.push_back( bn );

	return bn;
}

void BT_Node::addTree( BT_Node* tree )
{
	m_children.push_back( tree );
}

BT_Node* BT_Node::addConditional( QString name, std::function<BT_RESULT( bool )> callback )
{
	BT_NodeConditional* bn = new BT_NodeConditional( name, m_blackboard, callback );
	m_children.push_back( bn );

	return bn;
}

BT_Node* BT_Node::addAction( QString name, std::function<BT_RESULT( bool )> callback )
{
	BT_NodeAction* bn = new BT_NodeAction( name, m_blackboard, callback );
	m_children.push_back( bn );

	return bn;
}

BT_Node* BT_Node::addBBPrecondition( QString name, QString key, QString expected )
{
	BT_NodeBBPrecondition* bn = new BT_NodeBBPrecondition( name, key, expected, m_blackboard );
	m_children.push_back( bn );

	return bn;
}

void BT_Node::haltAllChildren()
{
	for ( auto child : m_children )
	{
		child->halt();
	}
}

void BT_Node::halt()
{
	m_index = 0;
	haltAllChildren();
}