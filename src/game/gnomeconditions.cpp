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

#include "gnome.h"
#include "gnomemanager.h"
#include "game.h"


#include "../base/enums.h"
#include "../base/global.h"

//#include "../base/config.h"
#include "../base/gamestate.h"
//#include "../base/util.h"
//#include "../base/navmesh.h"
//#include "../base/position.h"

#include "../game/creaturemanager.h"
#include "../game/gnomemanager.h"
//#include "../game/farmingmanager.h"
//#include "../game/world.h"
//#include "../game/stockpile.h"
//#include "../game/stockpilemanager.h"
#include "../game/inventory.h"
#include "../game/workshop.h"
#include "../game/workshopmanager.h"
#include "../game/militarymanager.h"
//#include "../game/plant.h"

//#include "../gui/strings.h"

//#include "../gfx/spritefactory.h"
//#include "../gfx/sprite.h"

#include "../base/behaviortree/bt_node.h"

//#include <QDebug>

BT_RESULT Gnome::conditionIsHungry( bool halt )
{
	int hour = qMin( 23, GameState::hour );
	auto activity      = m_schedule[hour];
	if ( activity == ScheduleActivity::Eat && m_needs["Hunger"].toFloat() < 90 )
	{
		setThoughtBubble( "Hungry" );
		return BT_RESULT::SUCCESS;
	}

	if ( m_needs["Hunger"].toFloat() < 30 )
	{
		setThoughtBubble( "Hungry" );
		if ( !m_hungryLog )
		{
			m_log.append( "I'm hungry." );
			m_hungryLog = true;
		}
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Gnome::conditionIsVeryHungry( bool halt )
{
	if ( m_needs["Hunger"].toFloat() < 0 )
	{
		setThoughtBubble( "Hungry" );
		if ( !m_veryHungryLog )
		{
			m_log.append( "I'm very hungry." );
			m_veryHungryLog = true;
		}
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Gnome::conditionIsThirsty( bool halt )
{
	unsigned int hour = qMin( 23, GameState::hour );
	auto activity      = m_schedule[hour];
	if ( activity == ScheduleActivity::Eat && m_needs["Thirst"].toFloat() < 90 )
	{
		setThoughtBubble( "Thirsty" );
		return BT_RESULT::SUCCESS;
	}

	if ( m_needs["Thirst"].toFloat() < 30 )
	{
		setThoughtBubble( "Thirsty" );
		if ( !m_thirstyLog )
		{
			m_log.append( "I'm thirsty." );
			m_veryThirstLog = true;
		}
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Gnome::conditionIsVeryThirsty( bool halt )
{
	if ( m_needs["Thirst"].toFloat() < 0 )
	{
		setThoughtBubble( "Thirsty" );
		if ( !m_veryThirstLog )
		{
			m_log.append( "I'm very thirsty." );
			m_veryThirstLog = true;
		}
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Gnome::conditionIsSleepy( bool halt )
{

	unsigned int hour = qMin( 23, GameState::hour );
	auto activity      = m_schedule[hour];
	if ( activity == ScheduleActivity::Sleep )
	{
		setThoughtBubble( "Tired" );
		return BT_RESULT::SUCCESS;
	}

	if ( m_needs["Sleep"].toFloat() < 30 )
	{

		setThoughtBubble( "Tired" );
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Gnome::conditionAllItemsInPlaceForJob( bool halt )
{
	if ( !m_job || m_itemToPickUp != 0 || !m_carriedItems.isEmpty() )
	{
		return BT_RESULT::FAILURE;
	}

	auto cil = claimedItems();

	Position inputPos = m_job->posItemInput();

	for ( auto ci : cil )
	{
		if ( g->inv()->getItemPos( ci ) != inputPos )
		{
			m_itemToPickUp = ci;
			setCurrentTarget( g->inv()->getItemPos( ci ) );
			log( "Item is at " + g->inv()->getItemPos( ci ).toString() + " and must go to " + inputPos.toString() );
			return BT_RESULT::FAILURE;
		}
	}
	return BT_RESULT::SUCCESS;
}

BT_RESULT Gnome::conditionIsButcherJob( bool halt )
{
	if ( m_job )
	{
		if ( m_job->type() == "ButcherAnimal" )
		{
			return BT_RESULT::SUCCESS;
		}
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Gnome::conditionAllPickedUp( bool halt )
{
	if ( !m_job )
	{
		return BT_RESULT::FAILURE;
	}

	if ( m_job->itemsToHaul().size() == m_carriedItems.size() )
	{
		return BT_RESULT::SUCCESS;
	}

	if ( m_itemToPickUp != 0 )
	{
		return BT_RESULT::FAILURE;
	}

	auto cil = m_job->itemsToHaul();
	PriorityQueue<unsigned int, int> pq;

	for ( auto itemID : cil )
	{
		if ( g->inv()->isHeldBy( itemID ) == 0 )
		{
			pq.put( itemID, g->inv()->distanceSquare( itemID, m_position ) );
		}
	}
	if ( !pq.empty() )
	{
		unsigned int itemID = pq.get();
		setCurrentTarget( g->inv()->getItemPos( itemID ) );
		m_itemToPickUp = itemID;
	}

	return BT_RESULT::FAILURE;
}

BT_RESULT Gnome::conditionIsFull( bool halt )
{
	if ( m_needs["Hunger"].toInt() < 100 && m_startedEating )
	{
		return BT_RESULT::RUNNING;
	}
	m_startedEating = false;
	return BT_RESULT::SUCCESS;
}

BT_RESULT Gnome::conditionIsDrinkFull( bool halt )
{
	if ( m_needs["Thirst"].toInt() < 100 && m_startedDrinking )
	{
		return BT_RESULT::RUNNING;
	}
	m_startedDrinking = false;
	return BT_RESULT::SUCCESS;
}

BT_RESULT Gnome::conditionIsTrainingTime( bool halt )
{
	unsigned int hour = qMin( 23, GameState::hour );
	auto activity      = m_schedule[hour];
	if ( activity == ScheduleActivity::Training )
	{
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Gnome::conditionIsTrainer( bool halt )
{
	if ( m_assignedWorkshop )
	{
		auto ws = g->wsm()->workshop( m_assignedWorkshop );
		if ( ws )
		{
			QString type = ws->type();
			if ( type == "MeleeTraining" )
			{
				return BT_RESULT::SUCCESS;
			}
		}
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Gnome::conditionIsCivilian( bool halt )
{
	bool roleIsCivilian = g->mil()->roleIsCivilian( m_roleID);
	if( m_roleID == 0 || roleIsCivilian )
	{
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Gnome::conditionHasHuntTarget( bool halt )
{
	auto squad = g->mil()->getSquadForGnome( m_id );
	if( squad )
	{
		for( const auto& prio : squad->priorities )
		{
			if ( prio.attitude != MilAttitude::FLEE )
			{
				const auto& targetSet = g->cm()->animalsByType( prio.type );
				for ( const auto& targetID : targetSet )
				{
					//!TODO Bucket targets by region cluster, so this can become amortized constant cost
					if ( g->cm()->hasPathTo( m_position, targetID ) )
					{
						if ( prio.attitude == MilAttitude::HUNT )
						{
							return BT_RESULT::SUCCESS;
						}
						const Creature* creature = g->cm()->creature( targetID );
						const unsigned int dist  = m_position.distSquare( creature->getPos() );

						if ( prio.attitude == MilAttitude::DEFEND && dist < 4 && g->cm()->hasLineOfSightTo( m_position, targetID ) )
						{
							return BT_RESULT::SUCCESS;
						}

						if ( prio.attitude == MilAttitude::ATTACK && dist < 100 && g->cm()->hasLineOfSightTo( m_position, targetID ) )
						{
							return BT_RESULT::SUCCESS;
						}
					}
				}
			}
		}
		for ( const auto& gnomeID : squad->gnomes )
		{
			const Gnome* gnome = g->gm()->gnome( gnomeID );
			if ( gnome && gnome->m_currentAttackTarget )
			{
				const Creature* creature = g->cm()->creature( gnome->m_currentAttackTarget );
				if ( creature && g->cm()->hasPathTo( m_position, creature->id() ) )
				{
					return BT_RESULT::SUCCESS;
				}
			}
		}
	}
	return BT_RESULT::FAILURE;
}