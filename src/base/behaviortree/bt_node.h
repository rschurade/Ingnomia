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
/** @file bt_node.h
 *  @brief Base class for all behavior tree nodes and the BT_RESULT status enum.
 */
#pragma once

#include <QString>
#include <QVariantMap>
#include <QVector>

#include <vector>

/** @brief Possible return values for a behavior tree tick. */
enum class BT_RESULT
{
	FAILURE,  ///< The node failed.
	SUCCESS,  ///< The node succeeded.
	RUNNING,  ///< The node is still executing and needs more ticks.
	IDLE      ///< The node has not been ticked yet.
};

/** @brief Abstract base class for every behavior tree node.
 *
 *  Provides the common interface (tick, halt, serialize/deserialize) and
 *  factory helpers for adding child nodes.  Composite and decorator
 *  subclasses override tick() to implement their control-flow logic.
 *
 *  Each node holds a reference to a shared QVariantMap blackboard used for
 *  inter-node communication within one tree instance.
 */
class BT_Node
{
public:
	BT_Node( QString name, QVariantMap& blackboard );
	virtual ~BT_Node();

	virtual QVariantMap serialize() const;
	virtual void deserialize( QVariantMap in );

	virtual BT_RESULT tick()
	{
		return BT_RESULT::FAILURE;
	};

	virtual void haltAllChildren();
	virtual void halt();

	BT_RESULT status() const
	{
		return m_status;
	}

	BT_Node* addFallback( QString name );
	BT_Node* addFallbackStar( QString name );
	BT_Node* addForceSuccess();
	BT_Node* addForceFailure();
	BT_Node* addSequence( QString name );
	BT_Node* addSequenceStar( QString name );
	BT_Node* addInverter( QString name );
	BT_Node* addRepeat( QString name, int num );
	BT_Node* addRepeatUntilSuccess( QString name, int num );
	BT_Node* addBBPrecondition( QString name, QString key, QString expected );

	void addTree( BT_Node* tree );

	BT_Node* addConditional( QString name, std::function<BT_RESULT( bool )> callback );
	BT_Node* addAction( QString name, std::function<BT_RESULT( bool )> callback );

protected:
	const QString m_name;           ///< Display / debug name of this node.
	QVariantMap& m_blackboard;      ///< Shared blackboard for the owning tree.

	std::vector<BT_Node*> m_children; ///< Owned child nodes (deleted in destructor).

	BT_RESULT m_status = BT_RESULT::IDLE; ///< Last tick result.

	int m_index = 0; ///< Current child index used by composites with memory.
};
