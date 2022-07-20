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

#include <QPointer>
#include <QObject>
#include <QList>
#include <absl/container/btree_map.h>
#include <absl/container/btree_set.h>
#include <absl/container/flat_hash_map.h>

#include <sigslot/signal.hpp>

class Game;

struct GuiWatchedItem
{
    QString category = "";
    QString group = "";
    QString item = "";
    QString material = "";
    QString guiString = "";
    int count = 0;
};
Q_DECLARE_METATYPE( GuiWatchedItem )

struct GuiInventoryMaterial
{
	QString id;
    QString name;
    QString cat;
    QString group;
    QString item;
	unsigned int countTotal = 0;
	unsigned int countInJob = 0;
	unsigned int countInStockpiles = 0;
	unsigned int countEquipped = 0;
	unsigned int countConstructed = 0;
	unsigned int countLoose = 0;
	unsigned int totalValue = 0;
    bool watched = false;
};
Q_DECLARE_METATYPE( GuiInventoryMaterial )

struct GuiInventoryItem
{
	QString id;
    QString name;
    QString cat;
    QString group;
	unsigned int countTotal = 0;
	unsigned int countInJob = 0;
	unsigned int countInStockpiles = 0;
	unsigned int countEquipped = 0;
	unsigned int countConstructed = 0;
	unsigned int countLoose = 0;
	unsigned int totalValue = 0;
    bool watched = false;
    QList<GuiInventoryMaterial> materials;
};
Q_DECLARE_METATYPE( GuiInventoryItem )

struct GuiInventoryGroup
{
    QString id;
	QString name;
    QString cat;
    unsigned int countTotal = 0;
	unsigned int countInJob = 0;
	unsigned int countInStockpiles = 0;
	unsigned int countEquipped = 0;
	unsigned int countConstructed = 0;
	unsigned int countLoose = 0;
	unsigned int totalValue = 0;
    bool watched = false;
    QList<GuiInventoryItem> items;
};
Q_DECLARE_METATYPE( GuiInventoryGroup )

struct GuiInventoryCategory
{
    QString id;
	QString name;
    unsigned int countTotal = 0;
	unsigned int countInJob = 0;
	unsigned int countInStockpiles = 0;
	unsigned int countEquipped = 0;
	unsigned int countConstructed = 0;
	unsigned int countLoose = 0;
	unsigned int totalValue = 0;
    bool watched = false;
    QList<GuiInventoryGroup> groups;
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
    void update();

private:
    QPointer<Game> g;

    QList<GuiInventoryCategory> m_categories;
    
    QList<GuiBuildItem> m_buildItems;

    absl::btree_set<std::string> m_watchedItems;

    absl::btree_map<BuildSelection, QString> m_buildSelection2String;
    absl::btree_map<BuildSelection, BuildItemType> m_buildSelection2buildItem;

    void setBuildItemValues( GuiBuildItem& gbi, BuildSelection selection );
    void setAvailableMats( GuiBuildRequiredItem& gbri );
    
    absl::flat_hash_map<QString, QString> m_itemToGroupCache;
    absl::flat_hash_map<QString, QString> m_itemToCategoryCache;

    void updateWatchedItem( const std::string& cat );
    void updateWatchedItem( const std::string& cat, const std::string& group );
    void updateWatchedItem( const std::string& cat, const std::string& group, const std::string& item );
    void updateWatchedItem( const std::string& cat, const std::string& group, const std::string& item, const std::string& mat );

public slots:
	void onRequestCategories();
   
    void onRequestBuildItems( BuildSelection buildSelection, QString category );
	
    void onSetActive( bool active, const GuiWatchedItem& gwi );

    void onAddItem( const std::string& itemSID, const std::string& materialSID );
    void onRemoveItem( const std::string& itemSID, const std::string& materialSID );

public: // signals:
	sigslot::signal<const QList<GuiInventoryCategory>& /*categories*/> signalInventoryCategories;
    
    sigslot::signal<const QList<GuiBuildItem>& /*items*/> signalBuildItems;

    sigslot::signal<const QList<GuiWatchedItem>& /*watchedItemList*/> signalWatchList;
};
