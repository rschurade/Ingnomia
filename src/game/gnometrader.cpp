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
#include "gnometrader.h"
#include "gnomemanager.h"
#include "game.h"

#include "../base/behaviortree/bt_tree.h"
#include "../base/config.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/util.h"
#include "../game/workshop.h"
#include "../game/workshopmanager.h"
#include "../game/world.h"

#include <QDebug>

TraderDefinition::TraderDefinition( QVariantMap& in )
{
	id = in.value( "ID" ).toString();

	auto vItems = in.value( "Items" ).toList();
	for( const auto& vItem : vItems )
	{
		auto vin = vItem.toMap();
		int amount = 1;
		if( vin.contains( "Amount" ) )
		{
			amount = vin.value( "Amount" ).toInt();
		}

		TraderItem ti{	vin.value( "Type" ).toString(), 
						vin.value( "Item" ).toString(), 
						vin.value( "Material" ).toString(), 
						vin.value( "Gender" ).toString(), 
						vin.value( "Quality" ).value<unsigned char>(), 
						vin.value( "Value_").toInt(), 
						amount,
						vin.value( "Reserved" ).toInt() };

		items.append( ti );
	}
}
	
void TraderDefinition::serialize( QVariantMap& out )
{
	QVariantMap tdOut;

	tdOut.insert( "ID", id );
	QVariantList outItems;
	for( const auto& item : items )
	{
		QVariantMap vItem;
		vItem.insert( "Type", item.type );
		vItem.insert( "Item", item.itemSID );
		vItem.insert( "Material", item.materialSID );
		vItem.insert( "Gender", item.gender );
		vItem.insert( "Quality", item.quality );
		vItem.insert( "Value_", item.value );
		vItem.insert( "Amount", item.amount );
		vItem.insert( "Reserved", item.reserved );

		outItems.append( vItem );
	}
	tdOut.insert( "Items", outItems );

	out.insert( "TraderDefinition", tdOut );
}




GnomeTrader::GnomeTrader( Position& pos, QString name, Gender gender, Game* game ) :
	Gnome( pos, name, gender, game )
{
	m_type = CreatureType::GNOME_TRADER;

	m_leavesOnTick = GameState::tick + Global::util->ticksPerDayRandomized( 5 );
}

GnomeTrader::GnomeTrader( QVariantMap& in, Game* game ) :
	Gnome( in, game ),
	m_leavesOnTick( in.value( "LeavesOnTick" ).value<quint64>() ),
	m_marketStall( in.value( "MarketStall" ).toUInt() )
{
	auto vtd = in.value( "TraderDefinition" ).toMap();
	m_traderDefinition = TraderDefinition( vtd );

	m_type = CreatureType::GNOME_TRADER;
}

void GnomeTrader::serialize( QVariantMap& out )
{
	Gnome::serialize( out );

	out.insert( "LeavesOnTick", m_leavesOnTick );
	out.insert( "MarketStall", m_marketStall );
	m_traderDefinition.serialize( out );
}

void GnomeTrader::init()
{
	g->w()->insertCreatureAtPosition( m_position, m_id );

	updateSprite();

	initTaskMap();
	initTaskMapTrader();

	loadBehaviorTree( "GnomeTrader" );

	if ( m_btBlackBoard.contains( "BehaviorTreeState" ) )
	{
		QVariantMap btm = m_btBlackBoard.value( "State" ).toMap();
		if ( m_behaviorTree )
		{
			m_behaviorTree->deserialize( btm );
			m_btBlackBoard.remove( "State" );
		}
	}
}

GnomeTrader::~GnomeTrader()
{
}

CreatureTickResult GnomeTrader::onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	processCooldowns( tickNumber );

	m_jobChanged    = false;
	Position oldPos = m_position;

	if ( checkFloor() )
	{
		m_lastOnTick = tickNumber;
		return CreatureTickResult::NOFLOOR;
	}

	if ( isDead() )
	{
		m_lastOnTick = tickNumber;
		return CreatureTickResult::DEAD;
	}

	if ( m_behaviorTree )
	{
		m_behaviorTree->tick();
	}

	move( oldPos );
	updateLight( oldPos, m_position );

	m_lastOnTick = tickNumber;

	if ( m_goneOffMap )
	{
		return CreatureTickResult::LEFTMAP;
	}

	return CreatureTickResult::OK;
}

void GnomeTrader::initTaskMapTrader()
{
	using namespace std::placeholders;

	m_behaviors.insert( "IsTimeToLeave", std::bind( &GnomeTrader::conditionIsTimeToLeave, this, _1 ) );

	m_behaviors.insert( "GetExitPosition", std::bind( &GnomeTrader::actionGetExitPosition, this, _1 ) );
	m_behaviors.insert( "LeaveMap", std::bind( &GnomeTrader::actionLeaveMap, this, _1 ) );
	m_behaviors.insert( "GetMarketStallPosition", std::bind( &GnomeTrader::actionGetMarketStallPosition, this, _1 ) );
	m_behaviors.insert( "Trade", std::bind( &GnomeTrader::actionTrade, this, _1 ) );
}

BT_RESULT GnomeTrader::conditionIsTimeToLeave( bool halt )
{
	Q_UNUSED( halt ); // action takes only one tick, halt has no effect

	if ( GameState::tick > m_leavesOnTick )
	{
		log( "It's time to leave" );

		auto ws = g->wsm()->workshop( m_marketStall );
		ws->assignGnome( 0 );

		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT GnomeTrader::actionGetMarketStallPosition( bool halt )
{
	Q_UNUSED( halt ); // action takes only one tick, halt has no effect
	auto ws = g->wsm()->workshop( m_marketStall );
	setCurrentTarget( ws->inputPos() );
	m_facingAfterMove = ws->rotation();
	return BT_RESULT::SUCCESS;
}

BT_RESULT GnomeTrader::actionTrade( bool halt )
{
	if ( halt )
	{
		return BT_RESULT::IDLE;
	}
	return BT_RESULT::SUCCESS;
}




void GnomeTrader::setInventory( QVariantList items )
{
	srand( std::chrono::system_clock::now().time_since_epoch().count() );

	m_traderDefinition.items.clear();

	for ( auto vEntry : items )
	{
		QVariantMap entry = vEntry.toMap();
		int min           = entry.value( "Min_" ).toInt();
		int max           = entry.value( "Max_" ).toInt();
		int modVal        = ( max - min ) + min;
		int amount        = 0;
		if ( modVal > 0 )
		{
			amount = rand() % modVal;
		}

		if ( amount > 0 )
		{
			TraderItem ti{  entry.value( "Type" ).toString(), 
						    entry.value( "Item" ).toString(), 
						    entry.value( "Material" ).toString(), 
							entry.value( "Gender" ).toString(), 
							entry.value( "Quality" ).value<unsigned char>(), 
							entry.value( "Value_").toInt(), 
							amount, 0 };
			
			m_traderDefinition.items.append( ti );
		}
	}
}

QList<TraderItem>& GnomeTrader::inventory()
{
	return m_traderDefinition.items;
}

void GnomeTrader::setMarketStall( unsigned int id )
{
	m_marketStall = id;
}

void GnomeTrader::setTraderDefinition( QVariantMap map )
{
	m_traderDefinition.id = map.value( "ID" ).toString();
	setInventory( map.value( "Items" ).toList() );
}
