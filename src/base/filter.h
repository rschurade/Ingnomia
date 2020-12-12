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
#include <QMap>
#include <QString>

class FilterItem
{
public:
	void addItem( QString material );

	QStringList materials();

	void setCheckState( bool state );
	void setCheckState( QString material, bool state );

	bool getCheckState( QString material );

private:
	QMap<QString, bool> m_materials;
};

class FilterGroup
{
public:
	void addItem( QString item, QString material );

	QStringList items();
	QStringList materials( QString item );

	void setCheckState( bool state );
	void setCheckState( QString item, bool state );
	void setCheckState( QString item, QString material, bool state );

	bool getCheckState( QString item, QString material );

private:
	QMap<QString, FilterItem> m_items;
};

class FilterCategory
{
public:
	void addItem( QString group, QString item, QString material );
	void addGroup( QString group );

	QStringList groups();
	QStringList items( QString group );
	QStringList materials( QString group, QString item );

	void setCheckState( bool state );
	void setCheckState( QString group, bool state );
	void setCheckState( QString group, QString item, bool state );
	void setCheckState( QString group, QString item, QString material, bool state );

	bool getCheckState( QString group, QString item, QString material );

private:
	QMap<QString, FilterGroup> m_groups;
};

class Filter
{
public:
	Filter();
	Filter( QVariantMap in );

	QVariantMap serialize();

	void clear();

	void addItem( QString category, QString group, QString item, QString material );

	QStringList categories();
	QStringList groups( QString catgory );
	QStringList items( QString category, QString group );
	QStringList materials( QString category, QString group, QString item );

	void setCheckState( QString category, bool state );
	void setCheckState( QString category, QString group, bool state );
	void setCheckState( QString category, QString group, QString item, bool state );
	void setCheckState( QString category, QString group, QString item, QString material, bool state );

	bool getCheckState( QString category, QString group, QString item, QString material );

	const QSet<QPair<QString, QString>>& getActive();
	QSet<QString> getActiveSimple();
	void setActiveSimple( QString val );

	void update();

private:
	QMap<QString, FilterCategory> m_categories;

	bool m_activeDirty       = true;
	bool m_activeSimpleDirty = true;
	QSet<QPair<QString, QString>> m_active;
	QSet<QString> m_activeSimple;
};
