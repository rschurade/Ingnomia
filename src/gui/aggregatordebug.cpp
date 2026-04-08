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
#include "aggregatordebug.h"

#include "../base/db.h"
#include "../base/global.h"
#include "../base/util.h"
#include "../game/game.h"
#include "../game/gnome.h"
#include "../game/gnomemanager.h"
#include "../game/inventory.h"
#include "../game/world.h"

#include <QDebug>

AggregatorDebug::AggregatorDebug( QObject* parent ) :
	QObject( parent )
{
}

AggregatorDebug::~AggregatorDebug()
{
}

void AggregatorDebug::init( Game* game )
{
	g = game;
	// Push initial gnome list
	onRequestGnomeList();
}

void AggregatorDebug::onSpawnCreature( QString type )
{
	qDebug() << "spawn creature:" << type;
	if ( type == "Gnome" )
	{
		emit signalTriggerEvent( EventType::MIGRATION, QVariantMap() );
	}
	else if ( type == "Trader" )
	{
		emit signalTriggerEvent( EventType::TRADER, QVariantMap() );
	}
	else if ( type == "Goblin" )
	{
		QVariantMap args;
		args.insert( "Amount", 1 );
		args.insert( "Type", "Goblin" );
		emit signalTriggerEvent( EventType::INVASION, args );
	}
}

void AggregatorDebug::onSetWindowSize( int width, int height )
{
	emit signalSetWindowSize( width, height );
}

void AggregatorDebug::onSetNeed( unsigned int gnomeID, QString need, float value )
{
	if ( !g ) return;

	if ( gnomeID == 0 )
	{
		// All gnomes
		for ( auto gnome : g->gm()->gnomes() )
		{
			gnome->addNeed( need, value );
		}
		qDebug() << "Set" << need << "=" << value << "for all gnomes";
	}
	else
	{
		for ( auto gnome : g->gm()->gnomes() )
		{
			if ( gnome->id() == gnomeID )
			{
				gnome->addNeed( need, value );
				qDebug() << "Set" << need << "=" << value << "for" << gnome->name();
				break;
			}
		}
	}
}

void AggregatorDebug::onKillGnome( unsigned int gnomeID )
{
	if ( !g ) return;

	for ( auto gnome : g->gm()->gnomes() )
	{
		if ( gnome->id() == gnomeID )
		{
			qDebug() << "Killing gnome:" << gnome->name();
			gnome->kill( false );
			break;
		}
	}
}

void AggregatorDebug::onSpawnItem( QString itemSID, QString materialSID, int count, int x, int y, int z )
{
	if ( !g ) return;

	Position pos( x, y, z );
	for ( int i = 0; i < count; ++i )
	{
		g->inv()->createItem( pos, itemSID, materialSID );
	}
	qDebug() << "Spawned" << count << materialSID << itemSID << "at" << pos.toString();
}

void AggregatorDebug::onSpawnCompositeItem( QString itemSID, QStringList materialSIDs, int count, int x, int y, int z )
{
	if ( !g ) return;

	Position pos( x, y, z );
	auto comps = DB::selectRows( "Items_Components", itemSID );

	for ( int i = 0; i < count; ++i )
	{
		// Create temporary component items, then create the composite
		QList<unsigned int> compItemIDs;
		for ( int c = 0; c < comps.size() && c < materialSIDs.size(); ++c )
		{
			QString compItemSID = comps[c].value( "ItemID" ).toString();
			unsigned int compID = g->inv()->createItem( pos, compItemSID, materialSIDs[c] );
			compItemIDs.append( compID );
		}
		g->inv()->createItem( pos, itemSID, compItemIDs );
		// Destroy the temporary component items
		for ( unsigned int id : compItemIDs )
		{
			g->inv()->destroyObject( id );
		}
	}
	qDebug() << "Spawned" << count << itemSID << "with materials" << materialSIDs << "at" << pos.toString();
}

void AggregatorDebug::onRequestGnomeList()
{
	if ( !g ) return;

	QList<QPair<QString, unsigned int>> gnomes;
	for ( auto gnome : g->gm()->gnomes() )
	{
		gnomes.append( { gnome->name(), gnome->id() } );
	}
	emit signalGnomeList( gnomes );
}

void AggregatorDebug::onRequestItemGroups()
{
	QStringList groups;
	auto result = DB::execQuery2( "SELECT DISTINCT \"ItemGroup\" FROM Items ORDER BY \"ItemGroup\"" );
	for ( const auto& v : result )
	{
		groups.append( v.toString() );
	}
	emit signalItemGroups( groups );
}

void AggregatorDebug::onRequestItems( QString group )
{
	QStringList items;
	auto result = DB::execQuery2( "SELECT ID FROM Items WHERE \"ItemGroup\" = \"" + group + "\" ORDER BY ID" );
	for ( const auto& v : result )
	{
		items.append( v.toString() );
	}
	emit signalItems( items );
}

void AggregatorDebug::onRequestMaterials( QString itemSID )
{
	auto comps = DB::selectRows( "Items_Components", itemSID );
	int componentCount = comps.size();

	QStringList mats1;
	QStringList mats2;

	if ( componentCount >= 2 )
	{
		// Composite item: materials per component
		mats1 = Global::util->possibleMaterialsForItem( comps[0].value( "ItemID" ).toString() );
		mats2 = Global::util->possibleMaterialsForItem( comps[1].value( "ItemID" ).toString() );
	}
	else
	{
		// Simple item
		mats1 = Global::util->possibleMaterialsForItem( itemSID );
		componentCount = 0;
	}

	emit signalMaterials( componentCount, mats1, mats2 );
}

void AggregatorDebug::onSetNeedDecayMultiplier( float value )
{
	Global::debugNeedDecayMultiplier = value;
	qDebug() << "Need decay multiplier set to" << value;
}

void AggregatorDebug::onSetDisableNeedDecay( QString need, bool disable )
{
	if ( disable )
	{
		Global::disabledNeedDecays.insert( need );
	}
	else
	{
		Global::disabledNeedDecays.remove( need );
	}
	qDebug() << "Need decay for" << need << ( disable ? "disabled" : "enabled" );
}
