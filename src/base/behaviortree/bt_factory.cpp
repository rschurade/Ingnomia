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
#include "bt_factory.h"

#include "../config.h"
#include "../global.h"

#include <QDebug>

namespace
{
class BT_NodeDummy final : public BT_Node
{
public:
	BT_NodeDummy( QString name, QVariantMap& blackboard ) :
		BT_Node( name, blackboard )
	{
	}
	~BT_NodeDummy()
	{
	}

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
