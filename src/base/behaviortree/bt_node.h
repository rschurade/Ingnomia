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
#pragma once

#include <QString>
#include <QVariantMap>
#include <QVector>

#include <vector>

enum class BT_RESULT
{
	FAILURE,
	SUCCESS,
	RUNNING,
	IDLE
};

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
	const QString m_name;
	QVariantMap& m_blackboard;

	std::vector<BT_Node*> m_children;

	BT_RESULT m_status = BT_RESULT::IDLE;

	int m_index = 0;
};
