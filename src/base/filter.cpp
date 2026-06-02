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

/** @file filter.cpp
 *  @brief Implementation of the four-tier filter system (Filter > FilterCategory > FilterGroup > FilterItem).
 *
 *  Filters are used throughout the game to let the player select subsets of items
 *  by category, group, item type, and material. Each tier delegates check-state
 *  changes downward and aggregates results upward. The Filter class also provides
 *  serialization for save/load and cached active-set queries.
 */

#include "filter.h"

#include "../base/db.h"
#include "../base/global.h"
#include "../game/game.h"
#include "../game/inventory.h"

#include "../gui/eventconnector.h"

#pragma region Filter

/** @brief Default constructor. Builds the filter hierarchy from the current inventory state. */
Filter::Filter()
{
	update();
}

/** @brief Deserializing constructor. Rebuilds the filter and restores previously active items.
 *  @param in A QVariantMap previously produced by serialize(), containing an "Items" list
 *            of "itemSID_materialSID" strings.
 */
Filter::Filter( QVariantMap in )
{
	update();

	QVariantList ffl = in.value( "Items" ).toList();
	for ( const auto& vf : ffl )
	{
		QString entry = vf.toString();
		setActiveSimple( entry );
	}
}

/** @brief Serializes the filter state to a QVariantMap for save games.
 *  @return A QVariantMap with an "Items" key containing a list of active "itemSID_materialSID" strings.
 */
QVariantMap Filter::serialize()
{
	QVariantMap out;

	QVariantList filter;
	for ( const auto& entry : getActiveSimple() )
	{
		filter.append( entry );
	}
	out.insert( "Items", filter );

	return out;
}

/** @brief Removes all categories and rebuilds the filter hierarchy from scratch. */
void Filter::clear()
{
	m_categories.clear();
	update();
}

/** @brief Rebuilds the filter hierarchy from the DB item-grouping table and current inventory.
 *
 *  Ensures all categories from the DB exist, then populates groups, items, and materials
 *  from the game inventory. Marks both active-set caches as dirty.
 */
void Filter::update()
{
	if( !Global::eventConnector || !Global::eventConnector->game() ) return;

	auto inv = Global::eventConnector->game()->inv();

	QStringList keys = DB::ids( "ItemGrouping" );

	for ( const auto& category : keys )
	{
		if ( !m_categories.contains( category ) )
		{
			m_categories.insert( category, FilterCategory() );
		}
	}
	for ( const auto& category : inv->categories() )
	{
		for ( const auto& group : inv->groups( category ) )
		{
			m_categories[category].addGroup( group );
		}
	}
	for ( const auto& category : inv->categories() )
	{
		for ( const auto& group : inv->groups( category ) )
		{
			for ( const auto& item : inv->items( category, group ) )
			{
				for ( const auto& material : inv->materials( category, group, item ) )
				{
					addItem( category, group, item, material );
				}
			}
		}
	}
	m_activeDirty       = true;
	m_activeSimpleDirty = true;
}

/** @brief Adds a single item-material pair into the filter hierarchy.
 *  @param category The top-level category SID (e.g. "Food", "Crafted").
 *  @param group    The group SID within the category.
 *  @param item     The item type SID.
 *  @param material The material SID for this item.
 */
void Filter::addItem( QString category, QString group, QString item, QString material )
{
	m_categories[category].addItem( group, item, material );
	m_activeDirty       = true;
	m_activeSimpleDirty = true;
}

/** @brief Returns the list of all category SIDs currently in the filter.
 *  @return A QStringList of category keys.
 */
QStringList Filter::categories()
{
	return m_categories.keys();
}

/** @brief Returns the group SIDs within a given category.
 *  @param category The category SID to query.
 *  @return A QStringList of group keys in that category.
 */
QStringList Filter::groups( QString category )
{
	return m_categories[category].groups();
}

/** @brief Returns the item SIDs within a given category and group.
 *  @param category The category SID.
 *  @param group    The group SID within the category.
 *  @return A QStringList of item keys in that group.
 */
QStringList Filter::items( QString category, QString group )
{
	return m_categories[category].items( group );
}

/** @brief Returns the material SIDs for a specific item within a category and group.
 *  @param category The category SID.
 *  @param group    The group SID.
 *  @param item     The item SID.
 *  @return A QStringList of material keys for that item.
 */
QStringList Filter::materials( QString category, QString group, QString item )
{
	return m_categories[category].materials( group, item );
}

/** @brief Sets the checked state for an entire category, propagating to all groups, items, and materials.
 *  @param category The category SID to check or uncheck.
 *  @param state    True to activate, false to deactivate.
 */
void Filter::setCheckState( QString category, bool state )
{
	m_categories[category].setCheckState( state );
	m_activeDirty       = true;
	m_activeSimpleDirty = true;
}

/** @brief Sets the checked state for a specific group within a category.
 *  @param category The category SID.
 *  @param group    The group SID to check or uncheck.
 *  @param state    True to activate, false to deactivate.
 */
void Filter::setCheckState( QString category, QString group, bool state )
{
	m_categories[category].setCheckState( group, state );
	m_activeDirty       = true;
	m_activeSimpleDirty = true;
}

/** @brief Sets the checked state for a specific item within a category and group.
 *  @param category The category SID.
 *  @param group    The group SID.
 *  @param item     The item SID to check or uncheck.
 *  @param state    True to activate, false to deactivate.
 */
void Filter::setCheckState( QString category, QString group, QString item, bool state )
{
	m_categories[category].setCheckState( group, item, state );
	m_activeDirty       = true;
	m_activeSimpleDirty = true;
}

/** @brief Sets the checked state for a single item-material combination.
 *  @param category The category SID.
 *  @param group    The group SID.
 *  @param item     The item SID.
 *  @param material The material SID to check or uncheck.
 *  @param state    True to activate, false to deactivate.
 */
void Filter::setCheckState( QString category, QString group, QString item, QString material, bool state )
{
	//qDebug() << category << group << item << material << state;
	m_categories[category].setCheckState( group, item, material, state );
	m_activeDirty       = true;
	m_activeSimpleDirty = true;
}

/** @brief Queries the checked state of a specific item-material combination.
 *  @param category The category SID.
 *  @param group    The group SID.
 *  @param item     The item SID.
 *  @param material The material SID.
 *  @return True if the item-material pair is currently checked (active).
 */
bool Filter::getCheckState( QString category, QString group, QString item, QString material )
{
	return m_categories[category].getCheckState( group, item, material );
}

/** @brief Returns the set of all active (checked) item-material pairs.
 *
 *  Uses lazy evaluation: only rebuilds the set when check states have changed
 *  since the last call (tracked by m_activeDirty).
 *
 *  @return A const reference to a QSet of QPair<itemSID, materialSID> for all checked entries.
 */
const QSet<QPair<QString, QString>>& Filter::getActive()
{
	if ( m_activeDirty )
	{
		m_active.clear();

		for ( const auto& category : categories() )
		{
			for ( const auto& group : groups( category ) )
			{
				for ( const auto& item : items( category, group ) )
				{
					for ( const auto& material : materials( category, group, item ) )
					{
						if ( getCheckState( category, group, item, material ) )
						{
							QPair<QString, QString> i( item, material );
							m_active.insert( i );
						}
					}
				}
			}
		}
		m_activeDirty = false;
	}
	return m_active;
}

/** @brief Returns a simplified set of active entries as "itemSID_materialSID" strings.
 *
 *  Uses lazy evaluation like getActive(). The returned strings are the format
 *  used for serialization and for setActiveSimple() lookups.
 *
 *  @return A QSet of "itemSID_materialSID" strings for all checked entries.
 */
QSet<QString> Filter::getActiveSimple()
{
	if ( m_activeSimpleDirty )
	{
		m_activeSimple.clear();
		for ( const auto& category : categories() )
		{
			for ( const auto& group : groups( category ) )
			{
				for ( const auto& item : items( category, group ) )
				{
					for ( const auto& material : materials( category, group, item ) )
					{
						if ( getCheckState( category, group, item, material ) )
						{
							m_activeSimple.insert( item + "_" + material );
						}
					}
				}
			}
		}
		m_activeSimpleDirty = false;
	}
	return m_activeSimple;
}

/** @brief Activates a single item-material pair from a combined "itemSID_materialSID" string.
 *
 *  Splits the string on "_", then searches all categories and groups for a matching
 *  item and material. Sets the check state to true on the first match found.
 *
 *  @param val A string in "itemSID_materialSID" format (as produced by getActiveSimple()).
 */
void Filter::setActiveSimple( QString val )
{
	QStringList lv = val.split( "_" );
	if ( lv.size() == 2 )
	{
		for ( const auto& category : categories() )
		{
			for ( const auto& group : groups( category ) )
			{
				if ( materials( category, group, lv[0] ).contains( lv[1] ) )
				{
					//if( Global::debugMode ) qDebug() << "set " << category << group << lv[0] << lv[1] << true;
					setCheckState( category, group, lv[0], lv[1], true );
					return;
				}
			}
		}
	}
}

#pragma endregion Filter

#pragma region FilterCategory

/** @brief Adds an item-material pair to a group within this category.
 *  @param group    The group SID to add the item to.
 *  @param item     The item SID.
 *  @param material The material SID.
 */
void FilterCategory::addItem( QString group, QString item, QString material )
{
	m_groups[group].addItem( item, material );
}

/** @brief Ensures a group entry exists in this category, creating it if absent.
 *  @param group The group SID to add.
 */
void FilterCategory::addGroup( QString group )
{
	if ( !m_groups.contains( group ) )
	{
		m_groups.insert( group, FilterGroup() );
	}
}

/** @brief Returns all group SIDs in this category.
 *  @return A QStringList of group keys.
 */
QStringList FilterCategory::groups()
{
	return m_groups.keys();
}

/** @brief Returns all item SIDs within a specific group.
 *  @param group The group SID to query.
 *  @return A QStringList of item keys in that group.
 */
QStringList FilterCategory::items( QString group )
{
	return m_groups[group].items();
}

/** @brief Returns all material SIDs for a specific item in a group.
 *  @param group The group SID.
 *  @param item  The item SID.
 *  @return A QStringList of material keys for that item.
 */
QStringList FilterCategory::materials( QString group, QString item )
{
	return m_groups[group].materials( item );
}

/** @brief Sets the checked state for the entire category, propagating to all groups.
 *  @param state True to activate, false to deactivate.
 */
void FilterCategory::setCheckState( bool state )
{
	for ( const auto& key : m_groups.keys() )
	{
		m_groups[key].setCheckState( state );
	}
}

/** @brief Sets the checked state for a specific group within this category.
 *  @param group The group SID.
 *  @param state True to activate, false to deactivate.
 */
void FilterCategory::setCheckState( QString group, bool state )
{
	m_groups[group].setCheckState( state );
}

/** @brief Sets the checked state for a specific item within a group.
 *  @param group The group SID.
 *  @param item  The item SID.
 *  @param state True to activate, false to deactivate.
 */
void FilterCategory::setCheckState( QString group, QString item, bool state )
{
	m_groups[group].setCheckState( item, state );
}

/** @brief Sets the checked state for a specific item-material pair within a group.
 *  @param group    The group SID.
 *  @param item     The item SID.
 *  @param material The material SID.
 *  @param state    True to activate, false to deactivate.
 */
void FilterCategory::setCheckState( QString group, QString item, QString material, bool state )
{
	m_groups[group].setCheckState( item, material, state );
}

/** @brief Queries the checked state of a specific item-material pair in a group.
 *  @param group    The group SID.
 *  @param item     The item SID.
 *  @param material The material SID.
 *  @return True if the item-material pair is checked.
 */
bool FilterCategory::getCheckState( QString group, QString item, QString material )
{
	return m_groups[group].getCheckState( item, material );
}
#pragma endregion FilterCategory

#pragma region FilterGroup

/** @brief Adds a material to an item within this group. Creates the item entry if needed.
 *  @param item     The item SID.
 *  @param material The material SID to add.
 */
void FilterGroup::addItem( QString item, QString material )
{
	m_items[item].addItem( material );
}

/** @brief Returns all item SIDs in this group.
 *  @return A QStringList of item keys.
 */
QStringList FilterGroup::items()
{
	return m_items.keys();
}

/** @brief Returns all material SIDs for a given item in this group.
 *  @param item The item SID to query.
 *  @return A QStringList of material keys, or an empty list if the item is not found.
 */
QStringList FilterGroup::materials( QString item )
{
	if ( m_items.contains( item ) )
	{
		return m_items[item].materials();
	}
	return QStringList();
}

/** @brief Sets the checked state for the entire group, propagating to all items and materials.
 *  @param state True to activate, false to deactivate.
 */
void FilterGroup::setCheckState( bool state )
{
	for ( const auto& key : m_items.keys() )
	{
		m_items[key].setCheckState( state );
	}
}

/** @brief Sets the checked state for all materials of a specific item.
 *  @param item  The item SID.
 *  @param state True to activate, false to deactivate.
 */
void FilterGroup::setCheckState( QString item, bool state )
{
	m_items[item].setCheckState( state );
}

/** @brief Sets the checked state for a single item-material pair.
 *  @param item     The item SID.
 *  @param material The material SID.
 *  @param state    True to activate, false to deactivate.
 */
void FilterGroup::setCheckState( QString item, QString material, bool state )
{
	if ( m_items.contains( item ) )
	{
		m_items[item].setCheckState( material, state );
	}
}

/** @brief Queries the checked state of a specific item-material pair.
 *  @param item     The item SID.
 *  @param material The material SID.
 *  @return True if the item-material pair is checked.
 */
bool FilterGroup::getCheckState( QString item, QString material )
{
	return m_items[item].getCheckState( material );
}

#pragma endregion FilterGroup

#pragma region FilterItem

/** @brief Adds a material variant to this item.
 *
 *  If this is the first material, it is added as unchecked (false). If materials
 *  already exist and all of them are currently checked, the new material inherits
 *  the active state; otherwise it is added as unchecked.
 *
 *  @param material The material SID to add.
 */
void FilterItem::addItem( QString material )
{
	if ( m_materials.empty() )
	{
		m_materials.insert( material, false );
	}
	else
	{
		// Inherit active state from silblings, if alle known are already active
		bool allActive = true;
		for ( const auto& material : m_materials )
		{
			allActive = allActive && material;
		}
		m_materials.insert( material, allActive );
	}
}

/** @brief Returns all material SIDs registered for this item.
 *  @return A QStringList of material keys.
 */
QStringList FilterItem::materials()
{
	return m_materials.keys();
}

/** @brief Sets the checked state for all materials of this item.
 *  @param state True to activate, false to deactivate.
 */
void FilterItem::setCheckState( bool state )
{
	for ( const auto& key : m_materials.keys() )
	{
		m_materials[key] = state;
	}
}

/** @brief Sets the checked state for a single material of this item.
 *  @param material The material SID.
 *  @param state    True to activate, false to deactivate.
 */
void FilterItem::setCheckState( QString material, bool state )
{
	if ( m_materials.contains( material ) )
	{
		m_materials[material] = state;
	}
}

/** @brief Queries the checked state of a specific material.
 *  @param material The material SID.
 *  @return True if the material is checked (active).
 */
bool FilterItem::getCheckState( QString material )
{
	return m_materials[material];
}
#pragma endregion FilterItem