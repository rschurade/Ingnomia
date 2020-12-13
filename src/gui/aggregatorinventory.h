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

#include "../base/enums.h"

#include <QObject>
#include <QList>

class Game;

struct GuiInventoryItem
{
	QString item;
    QString material;
	unsigned int countTotal;
	unsigned int countInJob;
	unsigned int countInStockpiles;
	unsigned int countEquipped;
	unsigned int countConstructed;
	unsigned int countLoose;
	unsigned int totalValue;
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

struct GuiBuildRequiredItem
{
    QString itemID;
    int amount = 0;

    QList< QPair<QString, int> > availableMats;
};
Q_DECLARE_METATYPE( GuiBuildRequiredItem )

struct GuiBuildItem
{
    QString id;
    QString name;
    BuildItemType biType;
    std::vector<unsigned char> buffer;
    int iconWidth = 0;
    int iconHeight = 0;

    QList<GuiBuildRequiredItem> requiredItems;
};
Q_DECLARE_METATYPE( GuiBuildItem )



class AggregatorInventory : public QObject
{
	Q_OBJECT

public:
	AggregatorInventory( QObject* parent = nullptr );
	~AggregatorInventory();

    void init( Game* game );

private:
    QPointer<Game> g;

    QList<GuiInventoryCategory> m_categories;
    QList<GuiInventoryGroup> m_groups;
    QList<GuiInventoryItem> m_items;
    QList<GuiBuildItem> m_buildItems;

    QMap<BuildSelection, QString> m_buildSelection2String;
    QMap<BuildSelection, BuildItemType> m_buildSelection2buildItem;

    void setBuildItemValues( GuiBuildItem& gbi, BuildSelection selection );
    void setAvailableMats( GuiBuildRequiredItem& gbri );

public slots:
	void onRequestCategories();
    void onRequestGroups( QString category );
    void onRequestItems( QString category, QString group );

    void onRequestBuildItems( BuildSelection buildSelection, QString category );
	
signals:
	void signalInventoryCategories( const QList<GuiInventoryCategory>& categories );
    void signalInventoryGroups( const QList<GuiInventoryGroup>& groups );
    void signalInventoryItems( const QList<GuiInventoryItem>& items );

    void signalBuildItems( const QList<GuiBuildItem>& items );
};
