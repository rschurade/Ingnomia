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
/** @file aggregatorinventory.h
 *  @brief Data types and aggregator for the Inventory / Build GUI windows. Produces the
 *         hierarchical category/group/item/material view of stored items plus the build-menu
 *         item catalogue.
 */
#pragma once

#include "../base/enums.h"

#include <QObject>
#include <QList>
#include <QHash>

class Game;

/// @brief Identifier for a single "watched" item in the top-bar watch widget.
struct GuiWatchedItem
{
    QString category = "";  ///< Category path component (empty for deeper levels).
    QString group = "";     ///< Group path component.
    QString item = "";      ///< Item path component.
    QString material = "";  ///< Material path component.
    QString guiString = ""; ///< Pre-formatted label for the watch widget.
    int count = 0;          ///< Current total count.
};
Q_DECLARE_METATYPE( GuiWatchedItem )

/// @brief Inventory totals for a single (item, material) pair.
struct GuiInventoryMaterial
{
	QString id;                            ///< Material string ID.
    QString name;                          ///< Localised material name.
    QString cat;                           ///< Parent category ID.
    QString group;                         ///< Parent group ID.
    QString item;                          ///< Parent item ID.
	unsigned int countTotal = 0;           ///< Total count across all states.
	unsigned int countInJob = 0;           ///< Items currently reserved by jobs.
	unsigned int countInStockpiles = 0;    ///< Items in stockpiles.
	unsigned int countEquipped = 0;        ///< Items worn/wielded by gnomes.
	unsigned int countConstructed = 0;     ///< Items built into structures/furniture.
	unsigned int countLoose = 0;           ///< Items lying on the ground unclaimed.
	unsigned int totalValue = 0;           ///< Sum of per-item values.
    bool watched = false;                  ///< True if this entry is on the watch list.
};
Q_DECLARE_METATYPE( GuiInventoryMaterial )

/// @brief Inventory totals for a single item, broken down per material.
struct GuiInventoryItem
{
	QString id;                            ///< Item string ID.
    QString name;                          ///< Localised item name.
    QString cat;                           ///< Parent category ID.
    QString group;                         ///< Parent group ID.
	unsigned int countTotal = 0;           ///< Aggregated total across materials.
	unsigned int countInJob = 0;           ///< Aggregated reserved count.
	unsigned int countInStockpiles = 0;    ///< Aggregated stockpile count.
	unsigned int countEquipped = 0;        ///< Aggregated equipped count.
	unsigned int countConstructed = 0;     ///< Aggregated constructed count.
	unsigned int countLoose = 0;           ///< Aggregated loose count.
	unsigned int totalValue = 0;           ///< Aggregated value.
    bool watched = false;                  ///< True if the whole item row is watched.
    QList<GuiInventoryMaterial> materials; ///< Per-material breakdown.
};
Q_DECLARE_METATYPE( GuiInventoryItem )

/// @brief Inventory totals for one group (e.g. "Weapons") within a category.
struct GuiInventoryGroup
{
    QString id;                            ///< Group string ID.
	QString name;                          ///< Localised group name.
    QString cat;                           ///< Parent category ID.
    unsigned int countTotal = 0;           ///< Aggregated total across items.
	unsigned int countInJob = 0;           ///< Aggregated reserved.
	unsigned int countInStockpiles = 0;    ///< Aggregated stockpile count.
	unsigned int countEquipped = 0;        ///< Aggregated equipped count.
	unsigned int countConstructed = 0;     ///< Aggregated constructed count.
	unsigned int countLoose = 0;           ///< Aggregated loose count.
	unsigned int totalValue = 0;           ///< Aggregated value.
    bool watched = false;                  ///< True if the whole group is watched.
    QList<GuiInventoryItem> items;         ///< Items in this group.
};
Q_DECLARE_METATYPE( GuiInventoryGroup )

/// @brief Top-level inventory category (e.g. "Tools", "Furniture").
struct GuiInventoryCategory
{
    QString id;                            ///< Category string ID.
	QString name;                          ///< Localised category name.
    unsigned int countTotal = 0;           ///< Aggregated totals.
	unsigned int countInJob = 0;           ///< Reserved count.
	unsigned int countInStockpiles = 0;    ///< Stockpile count.
	unsigned int countEquipped = 0;        ///< Equipped count.
	unsigned int countConstructed = 0;     ///< Constructed count.
	unsigned int countLoose = 0;           ///< Loose count.
	unsigned int totalValue = 0;           ///< Aggregated value.
    bool watched = false;                  ///< True if the whole category is watched.
    QList<GuiInventoryGroup> groups;       ///< Groups in this category.
};
Q_DECLARE_METATYPE( GuiInventoryCategory )




/// @brief Required component for a buildable entry, with per-material availability counts.
struct GuiBuildRequiredItem
{
    QString itemID;                          ///< Component item string ID.
    int amount = 0;                          ///< Required amount per build.

    QList< QPair<QString, int> > availableMats; ///< (material, count) pairs of currently available stock.
};
Q_DECLARE_METATYPE( GuiBuildRequiredItem )

/// @brief One entry in the build menu: a constructible item with its preview icon and required components.
struct GuiBuildItem
{
    QString id;                              ///< Build entry string ID.
    QString name;                            ///< Localised display name.
    BuildItemType biType;                    ///< Build category (workshop, wall, floor, …).
    std::vector<unsigned char> buffer;       ///< PNG-encoded preview icon for the GUI.
    int iconWidth = 0;                       ///< Preview width in pixels.
    int iconHeight = 0;                      ///< Preview height in pixels.

    QList<GuiBuildRequiredItem> requiredItems; ///< List of required components.
};
Q_DECLARE_METATYPE( GuiBuildItem )



/// @brief Bridges the inventory GUI and build menu with the game inventory. Builds the
///        category/group/item/material tree, the build-menu catalogue, and the top-bar watch list.
class AggregatorInventory : public QObject
{
	Q_OBJECT

public:
	AggregatorInventory( QObject* parent = nullptr );
	~AggregatorInventory();

    void init( Game* game );
    void update();

private:
    QPointer<Game> g;                               ///< Game instance (weak ownership).

    QList<GuiInventoryCategory> m_categories;       ///< Cached inventory category tree.

    QList<GuiBuildItem> m_buildItems;               ///< Cached build menu entries.

    QSet<QString> m_watchedItems;                   ///< Keys of items currently watched (pipe-joined path).

    QMap<BuildSelection, QString> m_buildSelection2String;     ///< BuildSelection enum → DB table prefix.
    QMap<BuildSelection, BuildItemType> m_buildSelection2buildItem; ///< BuildSelection enum → BuildItemType.

    void setBuildItemValues( GuiBuildItem& gbi, BuildSelection selection );
    void setAvailableMats( GuiBuildRequiredItem& gbri );

    QHash<QString, QString> m_itemToGroupCache;     ///< Item ID → group ID lookup cache.
    QHash<QString, QString> m_itemToCategoryCache;  ///< Item ID → category ID lookup cache.

    void updateWatchedItem( QString cat );
    void updateWatchedItem( QString cat, QString group );
    void updateWatchedItem( QString cat, QString group, QString item );
    void updateWatchedItem( QString cat, QString group, QString item, QString mat );

public slots:
	void onRequestCategories();
   
    void onRequestBuildItems( BuildSelection buildSelection, QString category );
	
    void onSetActive( bool active, const GuiWatchedItem& gwi );

    void onAddItem( QString itemSID, QString materialSID );
    void onRemoveItem( QString itemSID, QString materialSID );

signals:
	void signalInventoryCategories( const QList<GuiInventoryCategory>& categories );
    
    void signalBuildItems( const QList<GuiBuildItem>& items );

    void signalWatchList( const QList<GuiWatchedItem>& watchedItemList );
};
