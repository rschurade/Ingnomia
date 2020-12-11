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
#include "aggregatorneighbors.h"

#include "../game/game.h"
#include "../game/neighbormanager.h"
#include "../game/gnomemanager.h"

#include "../gui/strings.h"

AggregatorNeighbors::AggregatorNeighbors( QObject* parent ) :
	QObject(parent)
{
	
}

AggregatorNeighbors::~AggregatorNeighbors()
{
}

void AggregatorNeighbors::init( Game* game )
{
	g = game;
}

void AggregatorNeighbors::onRequestNeighborsUpdate()
{
	if( !g ) return;
	m_neighborsInfo.clear();

	for( const auto& kingdom : g->nm()->kingdoms() )
	{
		GuiNeighborInfo gni;

		gni.id = kingdom.id;

		//if( kingdom.discovered || Global::debugMode )
		{
			gni.discovered = true;

			QString name = kingdom.name + ", ";
			switch( kingdom.type )
			{
				case KingdomType::GNOME:
				name += " a gnome kingdom";
				gni.diploMission = true;
				break;
			case KingdomType::GOBLIN:
				name += " a goblin kingdom";
				
				gni.spyMission = true;
				gni.sabotageMission = true;
				gni.raidMission = true;
				gni.diploMission = true;

				break;
			default:
				name += " we don't know what they are";
				break;
			}

			gni.name = name;

			QString distance = "It takes about ";
			distance += S::gi().numberWord( kingdom.distance / 24 );
			distance += " days to travel there.";
			gni.distance = distance;
			
			QString wealth = "They are ";
			switch ( kingdom.wealth )	
			{
				case KingdomWealth::VERYPOOR:
				wealth += "very poor.";
				break;
			case KingdomWealth::POOR:
				wealth += "poor.";
				break;
			case KingdomWealth::AVERAGE:
				wealth += "doing ok.";
				break;
			case KingdomWealth::RICH:
				wealth += "rich.";
				break;
			case KingdomWealth::VERYRICH:
				wealth += "very rich.";
				break;
			}
			gni.wealth = wealth;

			QString economy = "They specialize in ";
			switch (kingdom.economy )
			{
				case KingdomEconomy::FARMING:
				economy += "farming.";
				break;
			case KingdomEconomy::LOGGING:
				economy += "logging.";
				break;
			case KingdomEconomy::MINING:
				economy += "mining.";
				break;
			case KingdomEconomy::ANIMALBREEDING:
				economy += "breeding animals.";
				break;
			case KingdomEconomy::TRADING:
				economy += "trading.";
				break;
			}
			gni.economy = economy;

			QString military = "Their military is ";
			switch ( kingdom.military )	
			{
				case KingdomMilitary::VERYWEAK:
				military += "very weak.";
				break;
			case KingdomMilitary::WEAK:
				military += "weak.";
				break;
			case KingdomMilitary::AVERAGE:
				military += "about average.";
				break;
			case KingdomMilitary::STRONG:
				military += "strong.";
				break;
			case KingdomMilitary::VERYSTRONG:
				military += "very strong.";
				break;
			}
			gni.military = military;
	
			QString attitude = "They are neutral towards us.";
			if( kingdom.attitude < -75 )
			{
				attitude = "They consider us their enemies.";
			}
			else if( kingdom.attitude < -50  )
			{
				attitude = "They are very hostile.";
			}
			else if( kingdom.attitude < -25 )
			{
				attitude = "They are hostile.";
			}
			else if( kingdom.attitude > 25 )
			{
				attitude = "They are friendly.";
			}
			else if( kingdom.attitude > 50 )
			{
				attitude = "They are very friendly";
			}
			else if( kingdom.attitude > 75 )
			{
				attitude = "They consider us their friends.";
			}
			gni.attitude = attitude;
		}

		m_neighborsInfo.append( gni );
	}

	emit signalNeighborsUpdate( m_neighborsInfo );
}

void AggregatorNeighbors::onRequestMissions()
{
	if( !g ) return;
	emit signalMissions( g->em()->missions() );
}

void AggregatorNeighbors::onRequestAvailableGnomes()
{
	if( !g ) return;
	m_availableGnomes.clear();

	for( auto gnome : g->gm()->gnomes() )
	{
		if( !gnome->isOnMission() )
		{
			m_availableGnomes.append( { gnome->id(), gnome->name() } );
		}
	}

	emit signalAvailableGnomes( m_availableGnomes );
}

void AggregatorNeighbors::onStartMission( MissionType type, MissionAction action, unsigned int targetKingdom, unsigned int gnomeID )
{
	if( !g ) return;
	g->em()->startMission( type, action, targetKingdom, gnomeID );

	emit signalMissions( g->em()->missions() );
}

void AggregatorNeighbors::onUpdateMission( const Mission& mission )
{
	if( !g ) return;
	emit signalUpdateMission( mission );
}