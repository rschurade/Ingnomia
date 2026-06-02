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

/** @file gnomeconditions.cpp
 *  @brief Gnome behavior-tree condition callbacks (hunger, thirst, sleep, combat readiness, etc.).
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

/// @brief Returns SUCCESS if the gnome should eat: scheduled Eat hour with Hunger < 90, or Hunger < 30 at any time.
/// @param halt Unused halt signal.
/// @return BT_RESULT::SUCCESS if hungry, BT_RESULT::FAILURE otherwise.
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

/// @brief Returns SUCCESS if the gnome is critically hungry (Hunger need below 0).
/// @param halt Unused halt signal.
/// @return BT_RESULT::SUCCESS if critically hungry, BT_RESULT::FAILURE otherwise.
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

/// @brief Returns SUCCESS if the gnome should drink: scheduled Eat hour with Thirst < 90, or Thirst < 30 at any time.
/// @param halt Unused halt signal.
/// @return BT_RESULT::SUCCESS if thirsty, BT_RESULT::FAILURE otherwise.
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

/// @brief Returns SUCCESS if the gnome is critically thirsty (Thirst need below 0).
/// @param halt Unused halt signal.
/// @return BT_RESULT::SUCCESS if critically thirsty, BT_RESULT::FAILURE otherwise.
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

/// @brief Returns SUCCESS if the gnome should sleep: it is a scheduled Sleep hour, or Sleep need is below 30.
/// @param halt Unused halt signal.
/// @return BT_RESULT::SUCCESS if sleepy, BT_RESULT::FAILURE otherwise.
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

/// @brief Returns SUCCESS if every item claimed for the current job is already at the job's input position.
///        Sets m_itemToPickUp and the current movement target for the first out-of-place item, then returns FAILURE.
/// @param halt Unused halt signal.
/// @return BT_RESULT::SUCCESS if all items are at the input position, BT_RESULT::FAILURE otherwise.
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

/// @brief Returns SUCCESS if the gnome's current job is of type "ButcherAnimal".
/// @param halt Unused halt signal.
/// @return BT_RESULT::SUCCESS if the active job is a butcher job, BT_RESULT::FAILURE otherwise.
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

/// @brief Returns SUCCESS when all items in the haul job have been picked up (carried count equals haul list size).
///        If items remain, selects the nearest unclaimed item as the next pickup target and returns FAILURE.
/// @param halt Unused halt signal.
/// @return BT_RESULT::SUCCESS if every item is now carried, BT_RESULT::FAILURE otherwise.
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

/// @brief Returns RUNNING while the gnome is eating and Hunger has not reached 100.
///        Clears m_startedEating and returns SUCCESS once Hunger is at 100 or eating was not started.
/// @param halt Unused halt signal.
/// @return BT_RESULT::RUNNING while eating, BT_RESULT::SUCCESS when full or not eating.
BT_RESULT Gnome::conditionIsFull( bool halt )
{
	if ( m_needs["Hunger"].toInt() < 100 && m_startedEating )
	{
		return BT_RESULT::RUNNING;
	}
	m_startedEating = false;
	return BT_RESULT::SUCCESS;
}

/// @brief Returns RUNNING while the gnome is drinking and Thirst has not reached 100.
///        Clears m_startedDrinking and returns SUCCESS once Thirst is at 100 or drinking was not started.
/// @param halt Unused halt signal.
/// @return BT_RESULT::RUNNING while drinking, BT_RESULT::SUCCESS when satiated or not drinking.
BT_RESULT Gnome::conditionIsDrinkFull( bool halt )
{
	if ( m_needs["Thirst"].toInt() < 100 && m_startedDrinking )
	{
		return BT_RESULT::RUNNING;
	}
	m_startedDrinking = false;
	return BT_RESULT::SUCCESS;
}

/// @brief Returns SUCCESS if the gnome's schedule slot for the current hour is Training.
/// @param halt Unused halt signal.
/// @return BT_RESULT::SUCCESS during a scheduled training hour, BT_RESULT::FAILURE otherwise.
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

/// @brief Returns SUCCESS if the gnome is assigned to a MeleeTraining workshop (i.e. acts as the trainer).
/// @param halt Unused halt signal.
/// @return BT_RESULT::SUCCESS if the assigned workshop type is "MeleeTraining", BT_RESULT::FAILURE otherwise.
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

/// @brief Returns SUCCESS if the gnome belongs to no military role or to a civilian role.
/// @param halt Unused halt signal.
/// @return BT_RESULT::SUCCESS if m_roleID is 0 or the role is civilian, BT_RESULT::FAILURE otherwise.
BT_RESULT Gnome::conditionIsCivilian( bool halt )
{
	bool roleIsCivilian = g->mil()->roleIsCivilian( m_roleID);
	if( m_roleID == 0 || roleIsCivilian )
	{
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

/// @brief Returns SUCCESS if the gnome's squad has a reachable target matching any non-FLEE priority.
///        Checks HUNT (any reachable animal), DEFEND (within distance 4, line of sight), and
///        ATTACK (within distance 100, line of sight). Also returns SUCCESS if a squad-mate
///        currently has an attack target the gnome can path to.
/// @param halt Unused halt signal.
/// @return BT_RESULT::SUCCESS if a valid hunt/attack target exists, BT_RESULT::FAILURE otherwise.
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