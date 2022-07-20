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

#include <absl/container/btree_map.h>
#include <QObject>
#include <absl/container/btree_set.h>

struct IH_values
{
	int total = 0;
	int plus  = 0;
	int minus = 0;
};

struct IH_dayData
{
	absl::btree_map<std::string, absl::btree_map<std::string, IH_values>> data;
};

struct IH_day
{
	int day    = 1;
	int season = 1;
	int year   = 1;

	IH_dayData dayData;
};

class ItemHistory : public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY_MOVE( ItemHistory )
public:
	ItemHistory( QObject* parent = nullptr );
	~ItemHistory();

	void serialize( QVariantMap& out );
	void deserialize( const QVariantMap& in );

	void reset();
	void init();
	void onTick( bool dayChanged );
	void plusItem( const std::string& itemSID, const std::string& materialSID );
	void minusItem( const std::string& itemSID, const std::string& materialSID );

	void finishStart()
	{
		m_startUp = false;
	}

	absl::btree_map<std::string, absl::btree_set<std::string>> allItems()
	{
		return m_itemsPresent;
	}

	absl::btree_map<std::string, std::vector<IH_values>> getHistory( const std::string& itemSID );

	absl::btree_map<std::string, std::vector<IH_values>> getRandomHistory( const std::string& itemSID );

private:
	void newDay();

	std::vector<IH_day> m_data;
	IH_day m_currentDay;

	absl::btree_map<std::string, absl::btree_set<std::string>> m_itemsPresent;

	bool m_startUp = true;
};
