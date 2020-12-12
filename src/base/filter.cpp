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
#include "filter.h"

#include "../base/db.h"
#include "../base/global.h"
#include "../game/game.h"
#include "../game/inventory.h"

#include "../gui/eventconnector.h"

#pragma region Filter
Filter::Filter()
{
	update();
}

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

void Filter::clear()
{
	m_categories.clear();
	update();
}

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

void Filter::addItem( QString category, QString group, QString item, QString material )
{
	m_categories[category].addItem( group, item, material );
	m_activeDirty       = true;
	m_activeSimpleDirty = true;
}

QStringList Filter::categories()
{
	return m_categories.keys();
}

QStringList Filter::groups( QString category )
{
	return m_categories[category].groups();
}

QStringList Filter::items( QString category, QString group )
{
	return m_categories[category].items( group );
}

QStringList Filter::materials( QString category, QString group, QString item )
{
	return m_categories[category].materials( group, item );
}

void Filter::setCheckState( QString category, bool state )
{
	m_categories[category].setCheckState( state );
	m_activeDirty       = true;
	m_activeSimpleDirty = true;
}

void Filter::setCheckState( QString category, QString group, bool state )
{
	m_categories[category].setCheckState( group, state );
	m_activeDirty       = true;
	m_activeSimpleDirty = true;
}

void Filter::setCheckState( QString category, QString group, QString item, bool state )
{
	m_categories[category].setCheckState( group, item, state );
	m_activeDirty       = true;
	m_activeSimpleDirty = true;
}

void Filter::setCheckState( QString category, QString group, QString item, QString material, bool state )
{
	//qDebug() << category << group << item << material << state;
	m_categories[category].setCheckState( group, item, material, state );
	m_activeDirty       = true;
	m_activeSimpleDirty = true;
}

bool Filter::getCheckState( QString category, QString group, QString item, QString material )
{
	return m_categories[category].getCheckState( group, item, material );
}

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
void FilterCategory::addItem( QString group, QString item, QString material )
{
	m_groups[group].addItem( item, material );
}

void FilterCategory::addGroup( QString group )
{
	if ( !m_groups.contains( group ) )
	{
		m_groups.insert( group, FilterGroup() );
	}
}

QStringList FilterCategory::groups()
{
	return m_groups.keys();
}

QStringList FilterCategory::items( QString group )
{
	return m_groups[group].items();
}

QStringList FilterCategory::materials( QString group, QString item )
{
	return m_groups[group].materials( item );
}

void FilterCategory::setCheckState( bool state )
{
	for ( const auto& key : m_groups.keys() )
	{
		m_groups[key].setCheckState( state );
	}
}

void FilterCategory::setCheckState( QString group, bool state )
{
	m_groups[group].setCheckState( state );
}

void FilterCategory::setCheckState( QString group, QString item, bool state )
{
	m_groups[group].setCheckState( item, state );
}

void FilterCategory::setCheckState( QString group, QString item, QString material, bool state )
{
	m_groups[group].setCheckState( item, material, state );
}

bool FilterCategory::getCheckState( QString group, QString item, QString material )
{
	return m_groups[group].getCheckState( item, material );
}
#pragma endregion FilterCategory

#pragma region FilterGroup
void FilterGroup::addItem( QString item, QString material )
{
	m_items[item].addItem( material );
}

QStringList FilterGroup::items()
{
	return m_items.keys();
}

QStringList FilterGroup::materials( QString item )
{
	if ( m_items.contains( item ) )
	{
		return m_items[item].materials();
	}
	return QStringList();
}

void FilterGroup::setCheckState( bool state )
{
	for ( const auto& key : m_items.keys() )
	{
		m_items[key].setCheckState( state );
	}
}

void FilterGroup::setCheckState( QString item, bool state )
{
	m_items[item].setCheckState( state );
}

void FilterGroup::setCheckState( QString item, QString material, bool state )
{
	if ( m_items.contains( item ) )
	{
		m_items[item].setCheckState( material, state );
	}
}

bool FilterGroup::getCheckState( QString item, QString material )
{
	return m_items[item].getCheckState( material );
}

#pragma endregion FilterGroup

#pragma region FilterItem

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

QStringList FilterItem::materials()
{
	return m_materials.keys();
}

void FilterItem::setCheckState( bool state )
{
	for ( const auto& key : m_materials.keys() )
	{
		m_materials[key] = state;
	}
}

void FilterItem::setCheckState( QString material, bool state )
{
	if ( m_materials.contains( material ) )
	{
		m_materials[material] = state;
	}
}

bool FilterItem::getCheckState( QString material )
{
	return m_materials[material];
}
#pragma endregion FilterItem