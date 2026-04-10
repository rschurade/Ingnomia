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
/** @file bt_factory.cpp
 *  @brief Implementation of BT_Factory -- loads XML behavior trees and builds node graphs.
 */
#include "bt_factory.h"

#include "../config.h"
#include "../global.h"

#include <QDebug>

namespace
{
/** @brief Temporary node used during tree construction to hold the root child.
 *
 *  The factory creates a BT_NodeDummy as a throwaway parent, adds the real
 *  root node as its child via the normal addXxx() helpers, then extracts it
 *  with takeChild() so the dummy can be destroyed on the stack.
 */
class BT_NodeDummy final : public BT_Node
{
public:
	/** @brief Construct the dummy with a name and blackboard reference.
	 *  @param name       Ignored debug name.
	 *  @param blackboard Shared blackboard (forwarded to BT_Node).
	 */
	BT_NodeDummy( QString name, QVariantMap& blackboard ) :
		BT_Node( name, blackboard )
	{
	}
	~BT_NodeDummy()
	{
	}

	/** @brief Remove and return the last child, transferring ownership to the caller.
	 *  @return The last child node, or nullptr if no children exist.
	 */
	BT_Node* takeChild()
	{
		if ( m_children.size() )
		{
			auto child = m_children.back();
			m_children.pop_back();
			return child;
		}
		else
		{
			return nullptr;
		}
	}
};
}

/** @brief Load and construct a complete behavior tree from XML.
 *  @param id         Identifier used to look up the XML document via Global::behaviorTree().
 *  @param actions    Map of action/condition ID strings to their callback functions.
 *  @param blackboard Shared blackboard that all nodes in the tree will reference.
 *  @return Root node of the constructed tree, or nullptr on failure.
 */
BT_Node* BT_Factory::load( const QString id, QHash<QString, std::function<BT_RESULT( bool )>>& actions, QVariantMap& blackboard )
{
	QDomElement root = Global::behaviorTree( id );
	QString mainTree = root.attribute( "main_tree_to_execute" );

	BT_Node* behaviorTree = getTree( mainTree, root, actions, blackboard );
	if ( !behaviorTree )
	{
		qDebug() << "Fatal error. Failed to load behavior tree";
		return nullptr;
	}

	return behaviorTree;
}

/** @brief Find a \<BehaviorTree\> element by ID and build its node subtree.
 *  @param treeID       The ID attribute to search for among \<BehaviorTree\> elements.
 *  @param documentRoot The root DOM element containing all \<BehaviorTree\> definitions.
 *  @param actions      Map of action/condition ID strings to callbacks.
 *  @param blackboard   Shared blackboard reference.
 *  @return Root node of the matching subtree, or nullptr if treeID was not found.
 */
BT_Node* BT_Factory::getTree( QString treeID, QDomElement documentRoot, QHash<QString, std::function<BT_RESULT( bool )>>& actions, QVariantMap& blackboard )
{
	QDomElement treeElement = documentRoot.firstChildElement();
	while ( !treeElement.isNull() )
	{
		QString nodeName = treeElement.nodeName();

		if ( nodeName == "BehaviorTree" )
		{
			QString thisTreeID = treeElement.attribute( "ID" );
			if ( thisTreeID == treeID )
			{
				QDomElement treeRoot = treeElement.firstChildElement();

				BT_NodeDummy dummy( "dummy", blackboard );

				createBTNode( treeRoot, &dummy, documentRoot, actions, blackboard );

				BT_Node* rootNode = dummy.takeChild();

				assert( rootNode );
				if ( rootNode )
				{
					getNodes( rootNode, treeRoot, documentRoot, actions, blackboard );
				}
				else
				{
					qCritical() << "failed to create root node for behavior tree " << treeID;
				}

				return rootNode;
			}
		}

		treeElement = treeElement.nextSiblingElement();
	}
	return nullptr;
}

/** @brief Recursively walk DOM children and create corresponding BT_Node children.
 *  @param parent       Parent node to attach new children to.
 *  @param root         Current DOM element whose child elements will be processed.
 *  @param documentRoot Top-level DOM element (needed for SubTree lookups).
 *  @param actions      Map of action/condition ID strings to callbacks.
 *  @param blackboard   Shared blackboard reference.
 */
void BT_Factory::getNodes( BT_Node* parent, QDomElement root, QDomElement& documentRoot, QHash<QString, std::function<BT_RESULT( bool )>>& actions, QVariantMap& blackboard )
{
	QDomElement treeElement = root.firstChildElement();
	while ( !treeElement.isNull() )
	{
		BT_Node* node = createBTNode( treeElement, parent, documentRoot, actions, blackboard );

		if ( treeElement.hasChildNodes() )
		{
			getNodes( node, treeElement, documentRoot, actions, blackboard );
		}

		treeElement = treeElement.nextSiblingElement();
	}
}

/** @brief Instantiate a single BT_Node from a DOM element and add it to a parent.
 *
 *  Maps XML tag names (Action, Condition, Sequence, Fallback, decorators,
 *  SubTree, etc.) to the corresponding BT_Node subclass via parent->addXxx().
 *  For SubTree elements, recursively calls getTree() to inline the referenced tree.
 *
 *  @param domElement   The DOM element describing the node to create.
 *  @param parent       Parent node that will own the new child.
 *  @param documentRoot Top-level DOM element (needed for SubTree lookups).
 *  @param actions      Map of action/condition ID strings to callbacks.
 *  @param blackboard   Shared blackboard reference.
 *  @return Pointer to the newly created node, or nullptr if creation failed.
 */
BT_Node* BT_Factory::createBTNode( QDomElement domElement, BT_Node* parent, QDomElement& documentRoot, QHash<QString, std::function<BT_RESULT( bool )>>& actions, QVariantMap& blackboard )
{
	QString nodeName = domElement.nodeName();
	BT_Node* bn      = nullptr;
	if ( nodeName == "Action" )
	{
		if ( !actions.contains( domElement.attribute( "ID" ) ) )
		{
			qCritical() << "Action " << domElement.attribute( "ID" ) << " doesn't exist in behaviorMap";
			abort();
		}
		bn = parent->addAction( domElement.attribute( "ID" ), actions[domElement.attribute( "ID" )] );
	}
	else if ( nodeName == "Condition" )
	{
		if ( !actions.contains( domElement.attribute( "ID" ) ) )
		{
			qCritical() << "Condition doesn't exist in behaviorMap:" << domElement.attribute( "ID" );
			abort();
		}
		bn = parent->addConditional( domElement.attribute( "ID" ), actions[domElement.attribute( "ID" )] );
	}
	else if ( nodeName == "Fallback" )
	{
		bn = parent->addFallback( domElement.attribute( "name" ) );
	}
	else if ( nodeName == "FallbackStar" )
	{
		bn = parent->addFallbackStar( domElement.attribute( "name" ) );
	}
	else if ( nodeName == "ForceSuccess" )
	{
		bn = parent->addForceSuccess();
	}
	else if ( nodeName == "ForceFailure" )
	{
		bn = parent->addForceFailure();
	}
	else if ( nodeName == "Sequence" )
	{
		bn = parent->addSequence( domElement.attribute( "name" ) );
	}
	else if ( nodeName == "SequenceStar" )
	{
		bn = parent->addSequenceStar( domElement.attribute( "name" ) );
	}
	else if ( nodeName == "Repeat" )
	{
		bn = parent->addRepeat( domElement.attribute( "name" ), domElement.attribute( "num_cycles" ).toInt() );
	}
	else if ( nodeName == "RetryUntilSuccesful" )
	{
		bn = parent->addRepeatUntilSuccess( domElement.attribute( "name" ), domElement.attribute( "num_attempts" ).toInt() );
	}
	else if ( nodeName == "Inverter" )
	{
		bn = parent->addInverter( domElement.attribute( "name" ) );
	}
	else if ( nodeName == "BB_Precondition" )
	{
		bn = parent->addBBPrecondition( domElement.attribute( "name" ), domElement.attribute( "key" ), domElement.attribute( "expected" ) );
	}
	else if ( nodeName == "SubTree" )
	{
		QString subtreeID = domElement.attribute( "ID" );
		//qDebug() << "request subtree: " << subtreeID;
		bn = getTree( subtreeID, documentRoot, actions, blackboard );
		if ( bn )
		{
			//qDebug() << "found subtree";
			parent->addTree( bn );
		}
	}
	return bn;
}
