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

#include "../base/position.h"

#include <absl/container/btree_map.h>
#include <absl/container/btree_set.h>

#include <ranges>

class Region
{
public:
	Region( unsigned int id = 0 );
	~Region();

	void addConnectionFrom( unsigned int toRegion, const Position& pos );
	void addConnectionTo( unsigned int toRegion, const Position& pos );
	void addConnectionFrom( unsigned int toRegion, QString pos );
	void addConnectionTo( unsigned int toRegion, QString pos );

	void removeConnectionFrom( unsigned int toRegion, const Position& pos );
	void removeConnectionTo( unsigned int toRegion, const Position& pos );
	void removeConnectionFrom( unsigned int toRegion, QString pos );
	void removeConnectionTo( unsigned int toRegion, QString pos );

	void clearConnectionsFrom()
	{
		m_connectionsFrom.clear();
	}
	void clearConnectionsTo()
	{
		m_connectionsTo.clear();
	}

	void removeAllConnectionsFrom( unsigned int id );
	void removeAllConnectionsTo( unsigned int id );

	absl::btree_map<unsigned int, absl::btree_set<QString>> connectionSetFrom()
	{
		return m_connectionsFrom;
	};
	absl::btree_map<unsigned int, absl::btree_set<QString>> connectionSetTo()
	{
		return m_connectionsTo;
	}

	absl::btree_set<QString> connectionsToRegion( unsigned int region )
	{
		return m_connectionsTo.at( region );
	}

	unsigned id()
	{
		return m_id;
	}
	auto connectionsFrom()
	{
		return std::views::keys(m_connectionsFrom);
	}
	auto connectionsTo()
	{
		return std::views::keys(m_connectionsTo);
	}

private:
	unsigned int m_id = 0;
	absl::btree_map<unsigned int, absl::btree_set<QString>> m_connectionsFrom;
	absl::btree_map<unsigned int, absl::btree_set<QString>> m_connectionsTo;
};
