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

#include <QObject>
#include <QList>

struct GuiInventoryItem
{
	QString item;
    QString material;
    int countInStockpiles = 0;
    int countTotal = 0;
};
Q_DECLARE_METATYPE( GuiInventoryItem )

struct GuiInventoryGroup
{
    QString id;
	QString name;
};
Q_DECLARE_METATYPE( GuiInventoryGroup )

struct GuiInventoryCategory
{
    QString id;
	QString name;
};
Q_DECLARE_METATYPE( GuiInventoryCategory )



class AggregatorInventory : public QObject
{
	Q_OBJECT

public:
	AggregatorInventory( QObject* parent = nullptr );
	~AggregatorInventory();

private:
    QList<GuiInventoryCategory> m_categories;
    QList<GuiInventoryGroup> m_groups;
    QList<GuiInventoryItem> m_items;

public slots:
	void onRequestCategories();
    void onRequestGroups( QString category );
    void onRequestItems( QString category, QString group );
	
signals:
	void signalInventoryCategories( const QList<GuiInventoryCategory>& categories );
    void signalInventoryGroups( const QList<GuiInventoryGroup>& groups );
    void signalInventoryItems( const QList<GuiInventoryItem>& items );
};
