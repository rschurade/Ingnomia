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

#include "spdlog/spdlog.h"

AggregatorDebug::AggregatorDebug( QObject* parent ) :
	QObject(parent)
{
	
}

AggregatorDebug::~AggregatorDebug()
{
}

void AggregatorDebug::onSpawnCreature( QString type )
{
	spdlog::debug( "spawn creature: {}", type.toStdString() );
	if( type == "Gnome" )
	{
		signalTriggerEvent( EventType::MIGRATION, QVariantMap() );
	}
	else if( type == "Trader" )
	{
		signalTriggerEvent( EventType::TRADER, QVariantMap() );
	}
	else if( type == "Goblin" )
	{
		QVariantMap args; 
		args.insert( "Amount", 1 );
		args.insert( "Type", "Goblin" );
		signalTriggerEvent( EventType::INVASION, args );
	}
}


void AggregatorDebug::onSetWindowSize( int width, int height )
{
	signalSetWindowSize( width, height );
}