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

#include <QDebug>
#include <QString>
#include <absl/container/btree_set.h>
#include <absl/container/btree_map.h>

#include <ranges>
#include "containersHelper.h"

class FilterItem
{
public:
	void addItem( QString material );

	[[nodiscard]] auto materials() const {
		return m_materials | std::views::keys;
	}

	void setCheckState( bool state );
	void setCheckState( QString material, bool state );

	bool getCheckState( QString material );

private:
	absl::btree_map<QString, bool> m_materials;
};

class FilterGroup
{
public:
	void addItem( QString item, QString material );

	auto items() {
		return m_items | std::views::keys;
	}


	void setCheckState( bool state );
	void setCheckState( QString item, bool state );
	void setCheckState( QString item, QString material, bool state );

	bool getCheckState( QString item, QString material );

private:
	absl::btree_map<QString, FilterItem> m_items;
	static inline FilterItem EmptyFilter;

public:
	auto materials( QString item ) {
		return maps::at_or_default(m_items, item, EmptyFilter).materials();
	}
};

class FilterCategory
{
public:
	void addItem( QString group, QString item, QString material );
	void addGroup( QString group );

	auto groups() {
		return m_groups | std::views::keys;
	}

	auto items( QString group ) {
		return m_groups.at(group).items();
	}

	auto materials( QString group, QString item )
	{
		return m_groups.at(group).materials( item );
	}

	void setCheckState( bool state );
	void setCheckState( QString group, bool state );
	void setCheckState( QString group, QString item, bool state );
	void setCheckState( QString group, QString item, QString material, bool state );

	bool getCheckState( QString group, QString item, QString material );

private:
	absl::btree_map<QString, FilterGroup> m_groups;
};

class Filter
{
public:
	Filter();
	Filter( QVariantMap in );

	QVariantMap serialize();

	void clear();

	void addItem( QString category, QString group, QString item, QString material );

	auto categories() {
		return m_categories | std::views::keys;
	}

	auto groups( QString category ) {
		return m_categories.at(category).groups();
	}

	auto items( QString category, QString group ) {
		return m_categories.at(category).items( group );
	}

	auto materials( QString category, QString group, QString item )
	{
		return m_categories.at(category).materials( group, item );
	}

	void setCheckState( QString category, bool state );
	void setCheckState( QString category, QString group, bool state );
	void setCheckState( QString category, QString group, QString item, bool state );
	void setCheckState( QString category, QString group, QString item, QString material, bool state );

	bool getCheckState( QString category, QString group, QString item, QString material );

	const absl::btree_set<QPair<QString, QString>>& getActive();
	absl::btree_set<QString> getActiveSimple();
	void setActiveSimple( QString val );

	void update();

private:
	absl::btree_map<QString, FilterCategory> m_categories;

	bool m_activeDirty       = true;
	bool m_activeSimpleDirty = true;
	absl::btree_set<QPair<QString, QString>> m_active;
	absl::btree_set<QString> m_activeSimple;
};
