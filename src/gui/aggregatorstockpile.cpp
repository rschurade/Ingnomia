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
/** @file aggregatorstockpile.cpp
 *  @brief AggregatorStockpile implementation: fills GuiStockpileInfo (basic settings plus the
 *         per-entry content summary) and routes GUI edits back to the stockpile manager.
 */
#include "aggregatorstockpile.h"

#include "../base/counter.h"
#include "../base/db.h"
#include "../base/dbhelper.h"
#include "../base/global.h"
#include "../game/game.h"
#include "../game/inventory.h"
#include "../game/stockpilemanager.h"
#include "../game/world.h"
#include "../gui/strings.h"

/// @brief Constructs the AggregatorStockpile and registers GuiStockpileInfo as a metatype.
/// @param parent Qt parent object.
AggregatorStockpile::AggregatorStockpile( QObject* parent ) :
	QObject(parent)
{
	qRegisterMetaType<GuiStockpileInfo>();
}

/// @brief Destructor.
AggregatorStockpile::~AggregatorStockpile()
{
}

/// @brief Binds the aggregator to a Game instance.
/// @param game Game to bind to.
void AggregatorStockpile::init( Game* game )
{
	g = game;
}

/// @brief Opens the stockpile window for whichever stockpile owns @p tileID.
/// @param tileID Integer tile key (Position::toInt()).
void AggregatorStockpile::onOpenStockpileInfoOnTile( unsigned int tileID )
{
	if( !g ) return;
	Position pos( tileID );
	auto sp = g->spm()->getStockpileAtPos( pos );
	if ( sp )
	{
		emit signalOpenStockpileWindow( sp->id() );
		onUpdateStockpileInfo( sp->id() );
	}
}

/// @brief Opens the stockpile window for a given stockpile UID and pushes a fresh info payload.
/// @param stockpileID Stockpile UID.
void AggregatorStockpile::onOpenStockpileInfo( unsigned int stockpileID )
{
	if( !g ) return;
	emit signalOpenStockpileWindow( stockpileID );
	onUpdateStockpileInfo( stockpileID );
}

/// @brief Re-aggregates and emits signalUpdateInfo for the given stockpile, if it exists.
/// @param stockpileID Stockpile UID.
void AggregatorStockpile::onUpdateStockpileInfo( unsigned int stockpileID )
{
	if( !g ) return;
	if ( aggregate( stockpileID ) )
	{
		emit signalUpdateInfo( m_info );
	}
}

/// @brief Fills m_info with the current stockpile state (basic fields + per-entry summary).
/// @param stockpileID Stockpile UID.
/// @return true if the stockpile exists, false otherwise.
bool AggregatorStockpile::aggregate( unsigned int stockpileID )
{
	if( !g ) return false;
	auto sp = g->spm()->getStockpile( stockpileID );
	if ( sp )
	{
		m_info.stockpileID       = stockpileID;
		m_info.name              = sp->name();
		m_info.priority          = sp->priority();
		m_info.maxPriority       = g->spm()->maxPriority();
		m_info.suspended         = !sp->active();
		m_info.allowPullFromHere = sp->allowsPull();
		m_info.pullFromOthers    = sp->pullsOthers();

		m_info.filter = sp->filter();

		m_info.summary.clear();

		auto active = m_info.filter.getActive();
		for ( auto entry : active )
		{
			int count = sp->count( entry.first, entry.second );
			//if( count > 0 )
			{
				//QIcon icon( Global::util->smallPixmap( Global::sf().createSprite( entry.first, { entry.second } ), season, 0 ) );
				ItemsSummary is;
				is.itemName     = S::s( "$ItemName_" + entry.first );
				is.materialName = S::s( "$MaterialName_" + entry.second );
				is.count        = count;

				m_info.summary.append( is );
			}
		}

		return true;
	}
	return false;
}

/// @brief Marks the content summary dirty if the open stockpile matches; the next tick hook
///        will actually re-emit it.
/// @param stockpileID Stockpile UID whose content changed.
void AggregatorStockpile::onUpdateStockpileContent( unsigned int stockpileID )
{
	if( !g ) return;
	if ( m_info.stockpileID == stockpileID )
	{
		m_contentDirty = true;
	}
}

/// @brief Post-tick hook: if the open stockpile's content is dirty, rebuilds the summary
///        rows and emits signalUpdateContent. Throttled to once per tick to avoid spam.
void AggregatorStockpile::onUpdateAfterTick()
{
	if( !g ) return;
	if ( m_info.stockpileID && m_contentDirty )
	{
		auto sp       = g->spm()->getStockpile( m_info.stockpileID );
		m_info.filter = sp->filter();
		m_info.summary.clear();
		auto active = m_info.filter.getActive();
		for ( auto entry : active )
		{
			int count = sp->count( entry.first, entry.second );
			//if( count > 0 )
			{
				//QIcon icon( Global::util->smallPixmap( Global::sf().createSprite( entry.first, { entry.second } ), season, 0 ) );
				ItemsSummary is;
				is.itemName     = S::s( "$ItemName_" + entry.first );
				is.materialName = S::s( "$MaterialName_" + entry.second );
				is.count        = count;

				m_info.summary.append( is );
			}
		}
		emit signalUpdateContent( m_info );
		m_contentDirty = false;
	}
}

/// @brief Applies basic stockpile edits (name, priority, suspended, pull, allow-pull) from the GUI.
/// @param stockpileID Stockpile UID.
/// @param name        New display name.
/// @param priority    New priority index.
/// @param suspended   New suspended flag.
/// @param pull        Pull-from-others flag.
/// @param allowPull   Allow-others-to-pull flag.
void AggregatorStockpile::onSetBasicOptions( unsigned int stockpileID, QString name, int priority, bool suspended, bool pull, bool allowPull )
{
	if( !g ) return;
	auto sp = g->spm()->getStockpile( stockpileID );
	if ( sp )
	{
		//qDebug() << stockpileID << name << priority << suspended << pull << allowPull;
		sp->setName( name );
		g->spm()->setPriority( stockpileID, priority );
		sp->setActive( !suspended );
		sp->setAllowPull( allowPull );
		sp->setPullOthers( pull );
	}
}

/// @brief Toggles a filter entry in the stockpile's item filter tree at the specified level
///        (category / group / item / material) and pushes a fresh info payload.
/// @param stockpileID Stockpile UID.
/// @param active      True to enable, false to disable.
/// @param category    Category key.
/// @param group       Group key (empty to apply to whole category).
/// @param item        Item key (empty to apply to whole group).
/// @param material    Material key (empty to apply to whole item).
void AggregatorStockpile::onSetActive( unsigned int stockpileID, bool active, QString category, QString group, QString item, QString material )
{
	if( !g ) return;
	qDebug() << "set active:" << stockpileID << active << category << group << item << material;
	auto sp = g->spm()->getStockpile( stockpileID );
	if ( sp )
	{
		auto filter = sp->pFilter();
		if ( filter )
		{
			if ( group.isEmpty() )
			{
				filter->setCheckState( category, active );
			}
			else if ( item.isEmpty() )
			{
				filter->setCheckState( category, group, active );
			}
			else if ( material.isEmpty() )
			{
				filter->setCheckState( category, group, item, active );
			}
			else
			{
				filter->setCheckState( category, group, item, material, active );
			}
		}
		onUpdateStockpileInfo( stockpileID );
	}
}

/// @brief Clears the currently open stockpile when the GUI closes the window.
void AggregatorStockpile::onCloseWindow()
{
	m_info.stockpileID = 0;
}
