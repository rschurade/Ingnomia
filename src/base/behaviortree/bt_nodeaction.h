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

#include "bt_node.h"

class BT_NodeAction final : public BT_Node
{
public:
	BT_NodeAction( QString name, QVariantMap& blackboard, std::function<BT_RESULT( bool )> callback );
	~BT_NodeAction();

	BT_RESULT tick();

	void halt();

	BT_Node* addFallback( QString name )
	{
		return nullptr;
	};
	BT_Node* addFallbackStar( QString name )
	{
		return nullptr;
	};
	BT_Node* addSequence( QString name )
	{
		return nullptr;
	};
	BT_Node* addSequenceStar( QString name )
	{
		return nullptr;
	};
	BT_Node* addInverter( QString name )
	{
		return nullptr;
	};
	BT_Node* addRepeat( QString name, int num )
	{
		return nullptr;
	};
	BT_Node* addRepeatUntilSuccess( QString name, int num )
	{
		return nullptr;
	};

	void addTree( BT_Node* tree ) {};

	BT_Node* addConditional( QString name )
	{
		return nullptr;
	};
	BT_Node* addAction( QString name )
	{
		return nullptr;
	};

private:
	std::function<BT_RESULT( bool )> m_callback;
};
