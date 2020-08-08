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
#include "itemhistory.h"

#include "../base/gamestate.h"
#include "../base/global.h"

#include <QDebug>

ItemHistory::ItemHistory( QObject* parent ) :
	QObject( parent )
{
}

ItemHistory::~ItemHistory()
{
}

void ItemHistory::serialize( QVariantMap& out )
{
	QVariantList items;
	for ( auto key : m_itemsPresent.keys() )
	{
		QVariantMap item;
		item.insert( "ID", key );
		auto entry = m_itemsPresent[key];
		QVariantList mats;
		for ( auto mat : entry )
		{
			mats.push_back( mat );
		}
		item.insert( "Mats", mats );
		items.push_back( item );
	}
	out.insert( "Items", items );

	QVariantList days;
	for ( auto day : m_data )
	{
		QVariantMap vmDay;
		vmDay.insert( "Day", day.day );
		vmDay.insert( "Season", day.season );
		vmDay.insert( "Year", day.year );
		QVariantList dayData;
		for ( auto itemKey : day.dayData.data.keys() )
		{
			auto entry = day.dayData.data[itemKey];
			QVariantMap itemData;
			itemData.insert( "ID", itemKey );
			QVariantList matList;
			for ( auto matKey : entry.keys() )
			{
				QVariantMap matData;
				auto matEntry = entry[matKey];
				matData.insert( "ID", matKey );
				matData.insert( "Total", matEntry.total );
				matData.insert( "Plus", matEntry.plus );
				matData.insert( "Minus", matEntry.minus );
				matList.append( matData );
			}
			itemData.insert( "Mats", matList );
			dayData.append( itemData );
		}
		vmDay.insert( "Data", dayData );
		days.append( vmDay );
	}

	{
		auto day = m_currentDay;
		QVariantMap vmDay;
		vmDay.insert( "Day", day.day );
		vmDay.insert( "Season", day.season );
		vmDay.insert( "Year", day.year );
		QVariantList dayData;
		for ( auto itemKey : day.dayData.data.keys() )
		{
			auto entry = day.dayData.data[itemKey];
			QVariantMap itemData;
			itemData.insert( "ID", itemKey );
			QVariantList matList;
			for ( auto matKey : entry.keys() )
			{
				QVariantMap matData;
				auto matEntry = entry[matKey];
				matData.insert( "ID", matKey );
				matData.insert( "Total", matEntry.total );
				matData.insert( "Plus", matEntry.plus );
				matData.insert( "Minus", matEntry.minus );
				matList.append( matData );
			}
			itemData.insert( "Mats", matList );
			dayData.append( itemData );
		}
		vmDay.insert( "Data", dayData );
		days.append( vmDay );
	}
	out.insert( "Days", days );
}

void ItemHistory::deserialize( const QVariantMap& in )
{
	reset();

	auto vlItems = in.value( "Items" ).toList();
	for ( auto entry : vlItems )
	{
		QString itemSID = entry.toMap().value( "ID" ).toString();
		QSet<QString> mats;
		for ( auto vmat : entry.toMap().value( "Mats" ).toList() )
		{
			mats.insert( vmat.toString() );
		}
		m_itemsPresent.insert( itemSID, mats );
	}

	auto vlDays = in.value( "Days" ).toList();

	QVariantMap vCurrentDay;
	if ( vlDays.size() > 0 )
	{
		vCurrentDay = vlDays.last().toMap();
		vlDays.pop_back();
	}

	for ( auto dayEntry : vlDays )
	{
		auto vmDay = dayEntry.toMap();
		IH_day ihd;
		ihd.day    = vmDay.value( "Day" ).toInt();
		ihd.season = vmDay.value( "Season" ).toInt();
		ihd.year   = vmDay.value( "Year" ).toInt();

		auto vlItemData = vmDay.value( "Data" ).toList();

		IH_dayData dd;

		for ( auto itemEntry : vlItemData )
		{
			QString itemSID = itemEntry.toMap().value( "ID" ).toString();
			auto mats       = itemEntry.toMap().value( "Mats" ).toList();
			QMap<QString, IH_values> vMats;
			for ( auto matEntry : mats )
			{
				auto vmMat = matEntry.toMap();
				IH_values iv;
				iv.total = vmMat.value( "Total" ).toInt();
				iv.plus  = vmMat.value( "Plus" ).toInt();
				iv.minus = vmMat.value( "Minus" ).toInt();
				vMats.insert( vmMat.value( "ID" ).toString(), iv );
			}
			dd.data.insert( itemSID, vMats );
		}
		ihd.dayData = dd;
		m_data.push_back( ihd );
	}
	{
		auto vmDay = vCurrentDay;
		IH_day ihd;
		ihd.day    = vmDay.value( "Day" ).toInt();
		ihd.season = vmDay.value( "Season" ).toInt();
		ihd.year   = vmDay.value( "Year" ).toInt();

		auto vlItemData = vmDay.value( "Data" ).toList();

		IH_dayData dd;

		for ( auto itemEntry : vlItemData )
		{
			QString itemSID = itemEntry.toMap().value( "ID" ).toString();
			auto mats       = itemEntry.toMap().value( "Mats" ).toList();
			QMap<QString, IH_values> vMats;
			for ( auto matEntry : mats )
			{
				auto vmMat = matEntry.toMap();
				IH_values iv;
				iv.total = vmMat.value( "Total" ).toInt();
				iv.plus  = vmMat.value( "Plus" ).toInt();
				iv.minus = vmMat.value( "Minus" ).toInt();
				vMats.insert( vmMat.value( "ID" ).toString(), iv );
			}
			dd.data.insert( itemSID, vMats );
		}
		ihd.dayData  = dd;
		m_currentDay = ihd;
	}
}

void ItemHistory::reset()
{
	m_data.clear();
	m_startUp = true;
	init();
}

void ItemHistory::init()
{
	m_currentDay.dayData.data.clear();
}

void ItemHistory::onTick( bool dayChanged )
{
	if ( dayChanged )
	{
		newDay();
	}
}

void ItemHistory::newDay()
{
	m_data.push_back( m_currentDay );
	m_currentDay.day    = GameState::day;
	m_currentDay.season = GameState::season;
	m_currentDay.year   = GameState::year;

	auto lastData = m_currentDay.dayData.data;
	for ( auto key : lastData.keys() )
	{
		auto& entry = lastData[key];
		QMap<QString, IH_values> newDayData;
		for ( auto matKey : entry.keys() )
		{
			auto& matEntry = entry[matKey];
			IH_values newVals;
			newVals.total = matEntry.total;
			newDayData.insert( matKey, newVals );
		}
		m_currentDay.dayData.data.insert( key, newDayData );
	}
}

void ItemHistory::plusItem( QString itemSID, QString materialSID )
{
	if ( m_currentDay.dayData.data.contains( itemSID ) )
	{
		auto& materialEntry = m_currentDay.dayData.data[itemSID];
		if ( materialEntry.contains( materialSID ) )
		{
			auto& counts = materialEntry[materialSID];
			counts.total = counts.total + 1;
			if ( !m_startUp )
			{
				counts.plus = counts.plus + 1;
			}
		}
		else
		{
			if ( m_startUp )
			{
				IH_values mv { 1, 0, 0 };
				materialEntry.insert( materialSID, mv );
			}
			else
			{
				IH_values mv { 1, 1, 0 };
				materialEntry.insert( materialSID, mv );
			}

			m_itemsPresent[itemSID].insert( materialSID );
		}
	}
	else
	{
		QMap<QString, IH_values> matEntry;
		if ( m_startUp )
		{
			matEntry.insert( materialSID, IH_values { 1, 0, 0 } );
			m_currentDay.dayData.data.insert( itemSID, matEntry );
		}
		else
		{
			matEntry.insert( materialSID, IH_values { 1, 1, 0 } );
			m_currentDay.dayData.data.insert( itemSID, matEntry );
		}
		QSet<QString> mats;
		mats.insert( materialSID );
		m_itemsPresent.insert( itemSID, mats );
	}
}

void ItemHistory::minusItem( QString itemSID, QString materialSID )
{
	if ( m_currentDay.dayData.data.contains( itemSID ) )
	{
		auto& entry = m_currentDay.dayData.data[itemSID];
		if ( entry.contains( materialSID ) )
		{
			auto& counts = entry[materialSID];
			counts.total = counts.total - 1;
			if ( !m_startUp )
			{
				counts.minus = counts.minus + 1;
			}
		}
	}
}

QMap<QString, QVector<IH_values>> ItemHistory::getHistory( QString itemSID )
{
	QMap<QString, QVector<IH_values>> out;

	QSet<QString> mats = m_itemsPresent.value( itemSID );
	out.insert( "all", QVector<IH_values>() );
	for ( auto mat : mats )
	{
		out.insert( mat, QVector<IH_values>() );
	}

	for ( int i = 0; i < m_data.size(); ++i )
	{
		auto dayData = m_data[i];
		if ( dayData.dayData.data.contains( itemSID ) )
		{
			auto itemData = dayData.dayData.data[itemSID];
			IH_values allValues;
			for ( auto mat : mats )
			{
				if ( itemData.contains( mat ) )
				{
					auto data = itemData[mat];
					allValues.total += data.total;
					allValues.plus += data.plus;
					allValues.minus += data.minus;
					out[mat].push_back( data );
				}
				else
				{
					// item with that material doesn't exist yet, insert empty
					IH_values matValues;
					out[mat].push_back( matValues );
				}
			}
			out["all"].push_back( allValues );
		}
		else
		{
			// item doesn't exist yet, insert empty for all materials
			IH_values matValues;
			for ( auto key : out.keys() )
			{
				out[key].push_back( matValues );
			}
		}
	}
	{
		auto dayData = m_currentDay;
		if ( dayData.dayData.data.contains( itemSID ) )
		{
			auto itemData = dayData.dayData.data[itemSID];
			IH_values allValues;
			for ( auto mat : mats )
			{
				if ( itemData.contains( mat ) )
				{
					auto data = itemData[mat];
					allValues.total += data.total;
					allValues.plus += data.plus;
					allValues.minus += data.minus;
					out[mat].push_back( data );
				}
				else
				{
					// item with that material doesn't exist yet, insert empty
					IH_values matValues;
					out[mat].push_back( matValues );
				}
			}
			out["all"].push_back( allValues );
		}
		else
		{
			// item doesn't exist yet, insert empty for all materials
			IH_values matValues;
			for ( auto key : out.keys() )
			{
				out[key].push_back( matValues );
			}
		}
	}

	return out;
}

QMap<QString, QVector<IH_values>> ItemHistory::getRandomHistory( QString itemSID )
{
	QMap<QString, QVector<IH_values>> out;

	for ( int k = 1; k < 8; ++k )
	{
		QVector<IH_values> vec;

		int total = 0;
		for ( int i = 0; i < 20; ++i )
		{
			int plus  = rand() % 20;
			int minus = qMin( total, rand() % 20 );
			total += plus - minus;

			IH_values mv { total, plus, minus };
			vec.push_back( mv );
		}

		out.insert( "material_" + QString::number( k ), vec );
	}

	return out;
}