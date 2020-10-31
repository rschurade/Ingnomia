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
#include "aggregatorinventory.h"

#include "../base/global.h"

#include "../game/inventory.h"

#include "../gui/strings.h"


AggregatorInventory::AggregatorInventory( QObject* parent )
{
}

AggregatorInventory::~AggregatorInventory()
{
}

void AggregatorInventory::onRequestCategories()
{
    m_categories.clear();
    for( const auto& cat : Global::inv().categories() )
	{
        int catTotal = 0;
        for( const auto& group : Global::inv().groups( cat ) )
		{
            int groupTotal = 0;
            for( const auto& item : Global::inv().items( cat, group ) )
			{
                for( const auto& mat : Global::inv().materials( cat, group, item ) )
				{
                    QString iName = S::s( "$ItemName_" + item );
                    QString mName = S::s( "$MaterialName_" + mat );
                    int total = Global::inv().itemCount( item, mat );
                    int inSP = Global::inv().itemCountInStockpile( item, mat );
                    groupTotal += total;
				}
			}
            catTotal += groupTotal;
		}
        //if( catTotal > 0 )
		{
            m_categories.append( { cat, S::s( "$CategoryName_" + cat ) } );
        }
	}

    emit signalInventoryCategories( m_categories );
}

void AggregatorInventory::onRequestGroups( QString category )
{
    m_groups.clear();

    for( const auto& group : Global::inv().groups( category ) )
	{
        int total = 0;
        for( const auto& item : Global::inv().items( category, group ) )
		{
            for( const auto& mat : Global::inv().materials( category, group, item ) )
			{
                total += Global::inv().itemCount( item, mat );
			}
		}
        //if( total )
		{
            m_groups.append( { group, S::s( "$GroupName_" + group ) } );
		}
	}

    emit signalInventoryGroups( m_groups );
}
    
void AggregatorInventory::onRequestItems( QString category, QString group )
{
    m_items.clear();

    for( const auto& item : Global::inv().items( category, group ) )
	{
        for( const auto& mat : Global::inv().materials( category, group, item ) )
		{
            QString iName = S::s( "$ItemName_" + item );
            QString mName = S::s( "$MaterialName_" + mat );
            int total = Global::inv().itemCount( item, mat );
            int inSP = Global::inv().itemCountInStockpile( item, mat );

            if( total > 0 )
			{
			    m_items.append( { iName, mName, inSP, total } );
            }
		}
	}

    emit signalInventoryItems( m_items );
}

