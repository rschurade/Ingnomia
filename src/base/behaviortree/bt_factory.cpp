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

#include "spdlog/spdlog.h"

namespace
{
class BT_NodeDummy final : public BT_Node
{
public:
	BT_NodeDummy( const std::string& name, QVariantMap& blackboard ) :
		BT_Node( name, blackboard )
	{
	}
	~BT_NodeDummy() = default;

	BT_Node* takeChild()
	{
		if ( !m_children.empty() )
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

BT_Node* BT_Factory::load( const pugi::xml_document& doc, BT_ActionMap& actions, QVariantMap& blackboard )
{
	const auto &root = doc.document_element();
	const std::string mainTree = root.attribute( "main_tree_to_execute" ).value();

	BT_Node* behaviorTree = getTree( mainTree, root, actions, blackboard );
	if ( !behaviorTree )
	{
		spdlog::debug("Fatal error. Failed to load behavior tree");
		return nullptr;
	}

	return behaviorTree;
}

BT_Node* BT_Factory::getTree( const std::string& treeID, const pugi::xml_node& documentRoot, BT_ActionMap& actions, QVariantMap& blackboard )
{
	pugi::xml_node treeElement = documentRoot.first_child();
	while ( treeElement )
	{
		std::string nodeName = treeElement.name();

		if ( nodeName == "BehaviorTree" )
		{
			std::string thisTreeID = treeElement.attribute( "ID" ).value();
			if ( thisTreeID == treeID )
			{
				pugi::xml_node treeRoot = treeElement.first_child();

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
					spdlog::critical("failed to create root node for behavior tree {}", treeID);
				}

				return rootNode;
			}
		}

		treeElement = treeElement.next_sibling();
	}
	return nullptr;
}

void BT_Factory::getNodes( BT_Node* parent, const pugi::xml_node& root, const pugi::xml_node& documentRoot, BT_ActionMap& actions, QVariantMap& blackboard )
{
	pugi::xml_node treeElement = root.first_child();
	while ( treeElement )
	{
		BT_Node* node = createBTNode( treeElement, parent, documentRoot, actions, blackboard );

		if ( treeElement.first_child() )
		{
			getNodes( node, treeElement, documentRoot, actions, blackboard );
		}

		treeElement = treeElement.next_sibling();
	}
}

BT_Node* BT_Factory::createBTNode( const pugi::xml_node& domElement, BT_Node* parent, const pugi::xml_node& documentRoot, BT_ActionMap& actions, QVariantMap& blackboard )
{
	std::string nodeName = domElement.name();
	BT_Node* bn      = nullptr;
	if ( nodeName == "Action" )
	{
		const std::string idKey = domElement.attribute( "ID" ).value();
		if ( idKey.empty() ) {
			spdlog::critical("Action missing ID");
			abort();
		}
		if ( !actions.contains( idKey ) )
		{
			spdlog::critical("Action '{}' doesn't exist in behaviorMap", idKey);
			abort();
		}
		bn = parent->addAction( idKey, actions[idKey] );
	}
	else if ( nodeName == "Condition" )
	{
		const std::string idKey = domElement.attribute( "ID" ).value();
		if ( idKey.empty() ) {
			spdlog::critical("Condition missing ID");
			abort();
		}
		if ( !actions.contains( idKey ) )
		{
			spdlog::critical("Condition doesn't exist in behaviorMap: '{}'", idKey);
			abort();
		}
		bn = parent->addConditional( idKey, actions[idKey] );
	}
	else if ( nodeName == "Fallback" )
	{
		bn = parent->addFallback( domElement.attribute( "name" ).value() );
	}
	else if ( nodeName == "FallbackStar" )
	{
		bn = parent->addFallbackStar( domElement.attribute( "name" ).value() );
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
		bn = parent->addSequence( domElement.attribute( "name" ).value() );
	}
	else if ( nodeName == "SequenceStar" )
	{
		bn = parent->addSequenceStar( domElement.attribute( "name" ).value() );
	}
	else if ( nodeName == "Repeat" )
	{
		bn = parent->addRepeat( domElement.attribute( "name" ).value(), domElement.attribute( "num_cycles" ).as_int() );
	}
	else if ( nodeName == "RetryUntilSuccesful" )
	{
		bn = parent->addRepeatUntilSuccess( domElement.attribute( "name" ).value(), domElement.attribute( "num_attempts" ).as_int() );
	}
	else if ( nodeName == "Inverter" )
	{
		bn = parent->addInverter( domElement.attribute( "name" ).value() );
	}
	else if ( nodeName == "BB_Precondition" )
	{
		bn = parent->addBBPrecondition( domElement.attribute( "name" ).value(), domElement.attribute( "key" ).value(), domElement.attribute( "expected" ).value() );
	}
	else if ( nodeName == "SubTree" )
	{
		std::string subtreeID = domElement.attribute( "ID" ).value();
		if ( subtreeID.empty() ) {
			spdlog::critical("SubTree missing ID");
			abort();
		}
		//spdlog::debug("request subtree: {}", subtreeID);
		bn = getTree( subtreeID, documentRoot, actions, blackboard );
		if ( bn )
		{
			//spdlog::debug("found subtree");
			parent->addTree( bn );
		}
	}
	return bn;
}
