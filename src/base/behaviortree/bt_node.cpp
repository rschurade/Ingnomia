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
/** @file bt_node.cpp
 *  @brief Implementation of BT_Node base class and child-adding factory methods.
 */
#include "bt_node.h"

#include "bt_tree.h"

#include <QDebug>

/** @brief Construct a node with the given debug name and shared blackboard.
 *  @param name       Display name used for serialization and debug output.
 *  @param blackboard Shared key-value store for the owning tree instance.
 */
BT_Node::BT_Node( QString name, QVariantMap& blackboard ) :
	m_name( name ),
	m_blackboard( blackboard )
{
}

/** @brief Destructor -- recursively deletes all owned child nodes. */
BT_Node::~BT_Node()
{
	for ( const auto& child : m_children )
	{
		delete child;
	}
}

/** @brief Serialize this node and all descendants into a QVariantMap.
 *  @return A map containing Name, ID, Status, and a Childs list.
 */
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

/** @brief Restore this node's state (and children) from a previously serialized map.
 *  @param in Map produced by serialize(), containing Name, ID, Status, and Childs.
 */
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

/** @brief Create and append a stateless Fallback child node.
 *  @param name Debug name for the new node.
 *  @return Pointer to the newly created child.
 */
BT_Node* BT_Node::addFallback( QString name )
{
	BT_NodeFallback* bn = new BT_NodeFallback( name, m_blackboard );
	m_children.push_back( bn );

	return bn;
}

/** @brief Create and append a Fallback-with-memory child node.
 *  @param name Debug name for the new node.
 *  @return Pointer to the newly created child.
 */
BT_Node* BT_Node::addFallbackStar( QString name )
{
	BT_NodeFallbackStar* bn = new BT_NodeFallbackStar( name, m_blackboard );
	m_children.push_back( bn );

	return bn;
}

/** @brief Create and append a stateless Sequence child node.
 *  @param name Debug name for the new node.
 *  @return Pointer to the newly created child.
 */
BT_Node* BT_Node::addSequence( QString name )
{
	BT_NodeSequence* bn = new BT_NodeSequence( name, m_blackboard );
	m_children.push_back( bn );

	return bn;
}

/** @brief Create and append a Sequence-with-memory child node.
 *  @param name Debug name for the new node.
 *  @return Pointer to the newly created child.
 */
BT_Node* BT_Node::addSequenceStar( QString name )
{
	BT_NodeSequenceStar* bn = new BT_NodeSequenceStar( name, m_blackboard, true );
	m_children.push_back( bn );

	return bn;
}

/** @brief Create and append a ForceSuccess decorator child node.
 *  @return Pointer to the newly created child.
 */
BT_Node* BT_Node::addForceSuccess()
{
	BT_NodeForceSuccess* bn = new BT_NodeForceSuccess( m_blackboard );
	m_children.push_back( bn );

	return bn;
}

/** @brief Create and append a ForceFailure decorator child node.
 *  @return Pointer to the newly created child.
 */
BT_Node* BT_Node::addForceFailure()
{
	BT_NodeForceFailure* bn = new BT_NodeForceFailure( m_blackboard );
	m_children.push_back( bn );

	return bn;
}

/** @brief Create and append an Inverter decorator child node.
 *  @param name Debug name for the new node.
 *  @return Pointer to the newly created child.
 */
BT_Node* BT_Node::addInverter( QString name )
{
	BT_NodeInverter* bn = new BT_NodeInverter( name, m_blackboard );
	m_children.push_back( bn );

	return bn;
}

/** @brief Create and append a Repeat decorator child node.
 *  @param name Debug name for the new node.
 *  @param num  Number of times to repeat the child.
 *  @return Pointer to the newly created child.
 */
BT_Node* BT_Node::addRepeat( QString name, int num )
{
	BT_NodeRepeat* bn = new BT_NodeRepeat( name, num, m_blackboard );
	m_children.push_back( bn );

	return bn;
}

/** @brief Create and append a RepeatUntilSuccess decorator child node.
 *  @param name Debug name for the new node.
 *  @param num  Maximum number of retry attempts.
 *  @return Pointer to the newly created child.
 */
BT_Node* BT_Node::addRepeatUntilSuccess( QString name, int num )
{
	BT_NodeRepeatUntilSuccess* bn = new BT_NodeRepeatUntilSuccess( name, num, m_blackboard );
	m_children.push_back( bn );

	return bn;
}

/** @brief Attach an externally constructed subtree as a child of this node.
 *  @param tree Root node of the subtree (ownership is transferred).
 */
void BT_Node::addTree( BT_Node* tree )
{
	m_children.push_back( tree );
}

/** @brief Create and append a Conditional leaf child node.
 *  @param name     Debug name for the new node.
 *  @param callback Function to invoke on tick; receives a halt flag.
 *  @return Pointer to the newly created child.
 */
BT_Node* BT_Node::addConditional( QString name, std::function<BT_RESULT( bool )> callback )
{
	BT_NodeConditional* bn = new BT_NodeConditional( name, m_blackboard, callback );
	m_children.push_back( bn );

	return bn;
}

/** @brief Create and append an Action leaf child node.
 *  @param name     Debug name for the new node.
 *  @param callback Function to invoke on tick; receives a halt flag.
 *  @return Pointer to the newly created child.
 */
BT_Node* BT_Node::addAction( QString name, std::function<BT_RESULT( bool )> callback )
{
	BT_NodeAction* bn = new BT_NodeAction( name, m_blackboard, callback );
	m_children.push_back( bn );

	return bn;
}

/** @brief Create and append a BlackboardPrecondition decorator child node.
 *  @param name     Debug name for the new node.
 *  @param key      Blackboard key to check.
 *  @param expected Required value (or "*" for wildcard match).
 *  @return Pointer to the newly created child.
 */
BT_Node* BT_Node::addBBPrecondition( QString name, QString key, QString expected )
{
	BT_NodeBBPrecondition* bn = new BT_NodeBBPrecondition( name, key, expected, m_blackboard );
	m_children.push_back( bn );

	return bn;
}

/** @brief Halt every child node by calling halt() on each one. */
void BT_Node::haltAllChildren()
{
	for ( auto child : m_children )
	{
		child->halt();
	}
}

/** @brief Reset this node's index and halt all children recursively. */
void BT_Node::halt()
{
	m_index = 0;
	haltAllChildren();
}