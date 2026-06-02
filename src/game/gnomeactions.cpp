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
/** @file gnomeactions.cpp
 *  @brief Gnome behavior-tree action and condition callbacks (movement, jobs, eating, combat, missions, etc.).
 */
#include "../base/config.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/logger.h"
#include "../base/pathfinder.h"
#include "../base/position.h"
#include "../base/util.h"
#include "../game/creaturemanager.h"
#include "../game/eventmanager.h"
#include "../game/farmingmanager.h"
#include "../game/gnomemanager.h"
#include "../game/inventory.h"
#include "../game/militarymanager.h"
#include "../game/neighbormanager.h"
#include "../game/roommanager.h"
#include "../game/plant.h"
#include "../game/room.h"
#include "../game/roommanager.h"
#include "../game/stockpile.h"
#include "../game/stockpilemanager.h"
#include "../game/soundmanager.h"
#include "../game/workshop.h"
#include "../game/workshopmanager.h"
#include "../game/world.h"
#include "../gfx/sprite.h"
#include "../gfx/spritefactory.h"
#include "../gui/strings.h"
#include "game.h"
#include "gnome.h"
#include "gnomemanager.h"

#include <QDebug>
#include <QElapsedTimer>

/// @brief BT action: marks the current job as successfully completed.
/// @param halt Unused (single-tick action).
/// @return SUCCESS.
BT_RESULT Gnome::actionFinishJob( bool halt )
{
	if ( Global::debugMode )
		log( "actionFinishJob" );
	Q_UNUSED( halt ); // action takes only one tick, halt has no effect
	//job is finished
	cleanUpJob( true );

	return BT_RESULT::SUCCESS;
}

/// @brief BT action: aborts the current job and returns it to the job manager.
/// @param halt Unused (single-tick action).
/// @return SUCCESS.
BT_RESULT Gnome::actionAbortJob( bool halt )
{
	if ( Global::debugMode )
		log( "actionAbortJob" );
	Q_UNUSED( halt ); // action takes only one tick, halt has no effect
	cleanUpJob( false );
	return BT_RESULT::SUCCESS;
}

/// @brief BT action: gnome sleeps in the claimed bed, recovering the Sleep need each tick.
///        Wakes up when fully rested or when a critical Hunger/Thirst need requires attention.
///        On halt, releases the bed and unclaims items.
/// @param halt True if the BT subtree is being interrupted.
/// @return RUNNING while asleep; SUCCESS on wake; IDLE on halt.
BT_RESULT Gnome::actionSleep( bool halt )
{
	if ( Global::debugMode )
		log( "actionSleep" );
	if ( halt )
	{
		setThoughtBubble( "" );
		m_log.append( "I was rudely awoken." );
		g->rm()->releaseBed( m_id );
		unclaimAll();
		return BT_RESULT::IDLE;
	}

	if ( thoughtBubble() != "Sleeping" )
	{
		m_gainFromSleep = ( DB::select( "GainFromSleep", "Needs", "Sleep" ).toFloat() / Global::util->ticksPerMinute );
		setThoughtBubble( "Sleeping" );
		m_log.append( "Going to sleep." );
	}

	if ( m_job )
	{
		cleanUpJob( false );
	}

	float oldVal = m_needs["Sleep"].toFloat();
	float newVal = oldVal + m_gainFromSleep;
	m_needs.insert( "Sleep", newVal );

	m_anatomy.heal();

	unsigned int hour = qMin( 23, GameState::hour );
	auto activity     = m_schedule[hour];

	bool criticalNeed = m_needs["Hunger"].toFloat() < 20 || m_needs["Thirst"].toFloat() < 20;

	if ( ( newVal >= 100. && activity != ScheduleActivity::Sleep ) || criticalNeed )
	{
		setThoughtBubble( "" );
		if ( criticalNeed )
		{
			m_log.append( "Woke up to eat or drink." );
		}
		else
		{
			m_log.append( "Woke up." );
		}

		g->rm()->releaseBed( m_id );
		unclaimAll();

		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::RUNNING;
}

/// @brief BT action: locates a free bed for the gnome, preferring the personal room over dormitories.
///        Sets the navigation target to the bed position on success.
/// @param halt Unused (single-tick action).
/// @return SUCCESS if a bed was found; FAILURE otherwise.
BT_RESULT Gnome::actionFindBed( bool halt )
{
	if ( Global::debugMode )
		log( "actionFindBed" );
	Q_UNUSED( halt ); // action takes only one tick, halt has no effect

	Position bedPos;

	// gnome has assigned room? look for bed there first
	if ( m_equipment.roomID )
	{
		bedPos = g->rm()->findFreeBed( m_equipment.roomID, m_id );
	}

	// no bed in personal room — try dormitories
	if ( bedPos.isZero() )
	{
		bedPos = g->rm()->findFreeDormBed( m_id );
	}

	if ( !bedPos.isZero() )
	{
		if ( m_job )
		{
			cleanUpJob( false );
		}
		g->rm()->claimBed( bedPos, m_id );
		setCurrentTarget( bedPos.toString() );
		return BT_RESULT::SUCCESS;
	}

	return BT_RESULT::FAILURE;
}

/// @brief BT action: moves the gnome one step along the current path toward its target.
///        Requests a new path if none exists.  Suspends the job on halt.
///        Stops early if a combat target becomes adjacent.
/// @param halt True if the BT subtree is being interrupted.
/// @return RUNNING while moving; SUCCESS on arrival or target-adjacent; FAILURE if no path.
BT_RESULT Gnome::actionMove( bool halt )
{
	if ( Global::debugMode )
		log( "actionMove" );
	//if ( Global::debugMode )
	//qDebug() << "actionMove" << m_currentPath.size();

	if ( halt )
	{
		m_currentPath.clear();
		if ( m_job )
		{
			suspendJob();
		}
		return BT_RESULT::IDLE;
	}
	// gnome has a path, move on path and return
	if ( !m_currentPath.empty() )
	{
		if ( m_animal )
		{
			if ( m_moveCooldown <= m_moveSpeed )
			{
				auto animal = g->cm()->animal( m_animal );
				if ( animal )
				{
					animal->setFollowPosition( m_position );
				}
			}
		}

		if ( m_currentAttackTarget )
		{
			if ( conditionTargetAdjacent( false ) == BT_RESULT::SUCCESS )
			{
				if ( Global::debugMode )
					qDebug() << m_name << "target adjecent, finish movement";
				m_currentPath.clear();
				return BT_RESULT::SUCCESS;
			}

			if ( conditionTargetPositionValid( false ) == BT_RESULT::FAILURE )
			{
				// Repath
				m_currentPath.clear();
				return BT_RESULT::FAILURE;
			}
		}

		Position oldPos = m_position;

		if ( !moveOnPath() )
		{
			//abortJob( "actionMove - 1" );
			m_currentPath.clear();
			return BT_RESULT::FAILURE;
		}

		if ( m_lightIntensity && m_position != oldPos )
		{
			g->w()->moveLight( m_id, m_position, m_lightIntensity );
		}

		return BT_RESULT::RUNNING;
	}

	Position targetPos = currentTarget();

	// check if we are already on the tile
	if ( m_position == targetPos )
	{
		return BT_RESULT::SUCCESS;
	}

	PathFinderResult pfr = g->pf()->getPath( m_id, m_position, targetPos, m_ignoreNoPass, m_currentPath );
	switch ( pfr )
	{
		case PathFinderResult::NoConnection:
			//abortJob( "actionMove - no connection - [" + m_position.toString() + "] [" + targetPos.toString() + "]" );
			return BT_RESULT::FAILURE;
		case PathFinderResult::Running:
		case PathFinderResult::FoundPath:
			return BT_RESULT::RUNNING;
	}

	return BT_RESULT::RUNNING;
}

/// @brief BT action: locates the nearest available food item and claims it.
///        Sets the navigation target to the item's position.
/// @param halt Unused (single-tick action).
/// @return SUCCESS if food was found; FAILURE otherwise.
BT_RESULT Gnome::actionFindFood( bool halt )
{
	if ( Global::debugMode )
		log( "actionFindFood" );
	Q_UNUSED( halt ); // action takes only one tick, halt has no effect
	auto itemID = g->inv()->getFoodItem( m_position );
	//m_log.append( "Looking for food." );

	if ( itemID )
	{
		// we found food, clean up job before we set some job variables again
		if ( m_job )
		{
			cleanUpJob( false );
		}

		addClaimedItem( itemID, m_id );
		m_itemToPickUp = itemID;
		setCurrentTarget( g->inv()->getItemPos( itemID ).toString() );
		//m_log.append( "Found food." );
		return BT_RESULT::SUCCESS;
	}
	//m_log.append( "Didn't find food." );
	return BT_RESULT::FAILURE;
}

/// @brief BT action: locates the nearest available drink item and claims it.
///        Sets the navigation target to the item's position.
/// @param halt Unused (single-tick action).
/// @return SUCCESS if a drink was found; FAILURE otherwise.
BT_RESULT Gnome::actionFindDrink( bool halt )
{
	if ( Global::debugMode )
		log( "actionFindDrink" );
	Q_UNUSED( halt ); // action takes only one tick, halt has no effect
	auto itemID = g->inv()->getDrinkItem( m_position );
	//m_log.append( "Looking for something to drink." );
	if ( itemID )
	{
		// we found food, clean up job before we set some job variables again
		if ( m_job )
		{
			cleanUpJob( false );
		}

		addClaimedItem( itemID, m_id );
		m_itemToPickUp = itemID;
		setCurrentTarget( g->inv()->getItemPos( itemID ).toString() );
		//m_log.append( "Found drinks." );
		return BT_RESULT::SUCCESS;
	}
	//m_log.append( "Didn't find drinks." );
	return BT_RESULT::FAILURE;
}

/// @brief BT action: finds and claims the nearest free chair in any dining room.
///        Sets the navigation target and facing direction.
/// @param halt Unused (single-tick action).
/// @return SUCCESS if a chair was found; FAILURE otherwise.
BT_RESULT Gnome::actionFindDining( bool halt )
{
	if ( Global::debugMode )
		log( "actionFindDining" );
	Q_UNUSED( halt ); // action takes only one tick, halt has no effect
	QList<unsigned int> dhl = g->rm()->getDinings();
	//m_log.append( "Looking for a dining room." );
	m_currentAction             = "find dining";
	Position closestChairPos;
	int dist = 1000000;
	for ( auto dh : dhl )
	{
		Room* room = g->rm()->getRoom( dh );
		if ( room )
		{
			Position chairPos = room->findFreeChair( m_position );
			if ( !chairPos.isZero() )
			{
				int curDist = m_position.distSquare( chairPos, 5 );
				if ( curDist < dist )
				{
					dist = curDist;
					closestChairPos = chairPos;
				}
			}
		}
	}
	if ( !closestChairPos.isZero() )
	{
		g->rm()->claimBed( closestChairPos, m_id ); // reuses claim mechanism for chairs too
		setCurrentTarget( closestChairPos );

		m_facingAfterMove = g->w()->getTile( closestChairPos ).wallRotation;

		return BT_RESULT::SUCCESS;
	}

	//m_log.append( "Didn't find a dining room." );
	return BT_RESULT::FAILURE;
}

/// @brief BT action: gnome consumes the carried food item over 15 in-game minutes.
///        Restores Hunger need by the item's nutritional value.
/// @param halt True if interrupted — returns IDLE.
/// @return RUNNING while eating; SUCCESS when done; FAILURE if no item.
BT_RESULT Gnome::actionEat( bool halt )
{
	if ( Global::debugMode )
		log( "actionEat" );
	if ( halt )
	{
		//abortJob( "actionEat - halt" );
		return BT_RESULT::IDLE;
	}

	// first visit
	if ( m_currentAction != "eating" )
	{
		//m_log.append( "Starting to eat." );
		m_currentAction  = "eating";
		m_taskFinishTick = GameState::tick + Global::util->ticksPerMinute * 15; // TODO set duration depending on food item or other circumstances
		unsigned int carriedItem = m_carriedItems.first();
		//printf("eat %s \n", g->inv()->itemSID( carriedItem ).toStdString().c_str() );
		g->sm()->playEffect("actionEat" , m_position, g->inv()->itemSID( carriedItem ) );
		
	}

	if ( GameState::tick < m_taskFinishTick )
	{
		return BT_RESULT::RUNNING;
	}

	if ( m_carriedItems.empty() )
	{
		//abortJob( "actionEat - no item" );
		unclaimAll();
		return BT_RESULT::FAILURE;
	}
	unsigned int carriedItem = m_carriedItems.first();
	unsigned char nutrition  = g->inv()->nutritionalValue( carriedItem );

	float oldVal = m_needs["Hunger"].toFloat();
	float newVal = qMin( 150.f, oldVal + nutrition );
	if ( newVal > 30 )
	{
		m_hungryLog     = false;
		m_veryHungryLog = false;
	}

	m_needs.insert( "Hunger", newVal );
	m_startedEating = true;

	QString logText( "I just ate a " + S::s( "$ItemName_" + g->inv()->itemSID( carriedItem ) ) + "." );
	//m_log.append( logText );

	g->inv()->destroyObject( carriedItem );
	m_carriedItems.clear();

	setThoughtBubble( "" );

	unclaimAll();
	//m_log.append( "Finished eating." );
	return BT_RESULT::SUCCESS;
}

/// @brief BT action: gnome consumes the carried drink item over 15 in-game minutes.
///        Restores Thirst need by the item's drink value.
/// @param halt True if interrupted — returns IDLE.
/// @return RUNNING while drinking; SUCCESS when done; FAILURE if no item.
BT_RESULT Gnome::actionDrink( bool halt )
{
	if ( Global::debugMode )
		log( "actionDrink" );
	if ( halt )
	{
		//abortJob( "actionDrink - halt" );
		return BT_RESULT::IDLE;
	}

	// first visit
	if ( m_currentAction != "drinking" )
	{
		//m_log.append( "Starting to drink." );
		m_currentAction  = "drinking";
		m_taskFinishTick = GameState::tick + Global::util->ticksPerMinute * 15; // TODO set duration depending on food item or other circumstances
		
		unsigned int carriedItem = m_carriedItems.first();
		//printf("drink %s %d\n", g->inv()->itemSID( carriedItem ).toStdString().c_str(), g->inv()->quality( carriedItem ) );
		g->sm()->playEffect("actionDrink" , m_position, g->inv()->itemSID( carriedItem ) );
	}

	if ( GameState::tick < m_taskFinishTick )
	{
		return BT_RESULT::RUNNING;
	}

	if ( m_carriedItems.empty() )
	{
		//abortJob( "actionDrink - no item" );
		unclaimAll();
		return BT_RESULT::FAILURE;
	}

	unsigned int carriedItem = m_carriedItems.first();
	unsigned char drinkValue = g->inv()->drinkValue( carriedItem );

	float oldVal = m_needs["Thirst"].toFloat();
	float newVal = qMin( 150.f, oldVal + drinkValue );
	if ( newVal > 30 )
	{
		m_thirstyLog    = false;
		m_veryThirstLog = false;
	}
	m_needs.insert( "Thirst", newVal );
	m_startedDrinking = false;

	QString logText( "I just drank a " + S::s( "$ItemName_" + g->inv()->itemSID( carriedItem ) ) + "." );
	//m_log.append( logText );

	g->inv()->destroyObject( carriedItem );
	m_carriedItems.clear();
	setThoughtBubble( "" );

	unclaimAll();
	//m_log.append( "Finished drinking." );
	return BT_RESULT::SUCCESS;
}

/// @brief BT action: picks up the item set in m_itemToPickUp from the gnome's current tile.
///        Adds it to the inventory list if it was a claimed inventory item, otherwise to carried items.
/// @param halt Unused (single-tick action).
/// @return SUCCESS on pickup; FAILURE if no item or wrong position.
BT_RESULT Gnome::actionPickUpItem( bool halt )
{
	if ( Global::debugMode )
		log( "actionPickUpItem" );
	Q_UNUSED( halt ); // action takes only one tick, halt has no effect

	if ( !m_itemToPickUp )
	{
		//abortJob( "actionPickUpItem - no item" );
		m_log.append( "Strange! Nothing to pick up." );
		return BT_RESULT::FAILURE;
	}

	if ( m_position != g->inv()->getItemPos( m_itemToPickUp ) )
	{
		log( "Cannot pick up item that is not here" );
		return BT_RESULT::FAILURE;
	}

	g->inv()->pickUpItem( m_itemToPickUp, m_id );

	if ( m_btBlackBoard.contains( "ClaimedInventoryItem" ) )
	{
		if ( m_itemToPickUp == m_btBlackBoard.value( "ClaimedInventoryItem" ).toUInt() )
		{
			m_inventoryItems.append( m_itemToPickUp );
			if ( g->inv()->itemSID( m_itemToPickUp ) == "Bandage" )
			{
				m_carriedBandages += 1;
			}
			else if ( g->inv()->nutritionalValue( m_itemToPickUp ) > 0 )
			{
				++m_carriedFood;
			}
			else if ( g->inv()->drinkValue( m_itemToPickUp ) > 0 )
			{
				++m_carriedDrinks;
			}
		}
		else
		{
			m_carriedItems.append( m_itemToPickUp );
		}
		m_btBlackBoard.remove( "ClaimedInventoryItem" );
	}
	else
	{
		m_carriedItems.append( m_itemToPickUp );
	}

	if ( g->inv()->isInStockpile( m_itemToPickUp ) )
	{
		g->spm()->removeItem( m_job->stockpile(), g->inv()->getItemPos( m_itemToPickUp ), m_itemToPickUp );
	}
	if ( g->inv()->isInContainer( m_itemToPickUp ) )
	{
		g->inv()->setInContainer( m_itemToPickUp, 0 );
	}

	m_itemToPickUp = 0;
	log( "Picked up an item. " + g->inv()->materialSID( m_itemToPickUp ) + " " + g->inv()->itemSID( m_itemToPickUp ) );
	return BT_RESULT::SUCCESS;
}

/// @brief BT action: requests a job from JobManager matching the gnome's skill priority list.
///        Builds the work-position priority queue and sets the job changed flag.
///        Sets a 100-tick cooldown if no job is available to avoid spamming the manager.
/// @param halt Unused.
/// @return SUCCESS if a job was acquired; FAILURE otherwise.
BT_RESULT Gnome::actionGetJob( bool halt )
{
	if ( Global::debugMode )
		log( "actionGetJob" );

	if ( m_jobCooldown > 0 )
	{
		return BT_RESULT::FAILURE;
	}

#ifdef CHECKTIME
	QElapsedTimer timer;
	timer.start();
	m_jobID      = g->jm()->getJob( m_skillPriorities, m_id, m_position );
	auto elapsed = timer.elapsed();
	if ( elapsed > 100 )
	{
		qDebug() << m_name << "JobManager just needed" << elapsed << "ms for getJob";
		Global::cfg->set( "Pause", true );
	}
#else
	QElapsedTimer et;
	et.start();

	m_jobID = g->jm()->getJob( m_skillPriorities, m_id, m_position );
#endif

	if ( m_jobID != 0 )
	{
		m_jobChanged = true;
		m_job        = g->jm()->getJob( m_jobID );
		if ( !m_job )
		{
			qDebug() << "jm returned null ptr for jobid " << m_jobID;
			return BT_RESULT::FAILURE;
		}

		//no change to jobsprite
		g->jm()->setJobBeingWorked( m_jobID, false );
		m_job->setIsWorked( true );
		m_job->setWorkedBy( m_id );

		log( "JobType " + m_job->type() );
		m_btBlackBoard.insert( "JobType", m_job->type() );
		m_currentAction = "job";

		bool mayTrap = DB::select( "MayTrapGnome", "Jobs", m_job->type() ).toBool();

		m_workPositionQueue = PriorityQueue<Position, int>();

		// determine which position is the work position
		if ( mayTrap )
		{
			for ( auto s : m_job->possibleWorkPositions() )
			{
				Position ss( s );
				int hasJob = g->w()->hasJob( ss );
				m_workPositionQueue.put( ss, ( 5 - g->w()->walkableNeighbors( s ) ) * 100 + m_position.distSquare( ss ) + hasJob * 10 );
			}
		}
		else
		{
			for ( auto s : m_job->possibleWorkPositions() )
			{
				Position ss( s );
				int hasJob = g->w()->hasJob( ss );
				//qDebug() << m_job->id() << "[" << ss.toString() << "]" ;
				m_workPositionQueue.put( ss, m_position.distSquare( ss ) + hasJob * 10 );
			}
		}
		QString logText( "Got a new " + S::s( "$SkillName_" + m_job->requiredSkill() ) + " job" );
		log( logText );

		if ( Global::debugMode )
		{
			auto ela = et.elapsed();
			if ( ela > 20 )
			{
				if ( m_job )
				{
					qDebug() << "GETJOB" << m_name << ela << "ms"
							 << "job:" << m_job->type() << m_job->pos().toString();
				}
			}
		}

		return BT_RESULT::SUCCESS;
	}
	else
	{
		// didn't get a suitable job, so wait some ticks before asking again
		m_jobCooldown = 100;
		return BT_RESULT::FAILURE;
	}

	return BT_RESULT::SUCCESS;
}

/// @brief BT action: sets the navigation target to the job's haul destination position.
/// @param halt Unused.
/// @return SUCCESS if a job exists; FAILURE otherwise.
BT_RESULT Gnome::actionGetHaulTarget( bool halt )
{
	Q_UNUSED( halt );
	if ( !m_job )
		return BT_RESULT::FAILURE;

	// The job position IS the haul target (where items need to go)
	setCurrentTarget( m_job->pos() );
	return BT_RESULT::SUCCESS;
}

/// @brief BT action: notifies the JobManager that a haul sub-job has been completed.
/// @param halt Unused.
/// @return SUCCESS if a job exists; FAILURE otherwise.
BT_RESULT Gnome::actionNotifyParentJob( bool halt )
{
	Q_UNUSED( halt );
	if ( !m_job )
		return BT_RESULT::FAILURE;

	g->jm()->onHaulSubJobComplete( m_job->id() );
	return BT_RESULT::SUCCESS;
}

/// @brief BT action: immobilises the target animal and sets the navigation target to its position.
/// @param halt Unused (single-tick action).
/// @return SUCCESS if the animal exists and is valid; FAILURE otherwise.
BT_RESULT Gnome::actionInitAnimalJob( bool halt )
{
	if ( Global::debugMode )
		log( "actionInitAnimalJob" );
	Q_UNUSED( halt ); // action takes only one tick, halt has no effect

	if ( !m_job )
	{
		return BT_RESULT::FAILURE;
	}

	Animal* a = g->cm()->animal( m_job->animal() );
	if ( !a || a->toDestroy() )
	{
		//abortJob( "grabAnimal()" );
		return BT_RESULT::FAILURE;
	}
	a->setImmobile( true );
	setCurrentTarget( a->getPos() );
	return BT_RESULT::SUCCESS;
}

/// @brief BT action: selects the best reachable work position from the priority queue and
///        stores it on the job.  For HaulToSite jobs, sets the target to the first item's position.
/// @param halt Unused (single-tick action).
/// @return SUCCESS if a reachable work position was found; FAILURE otherwise.
BT_RESULT Gnome::actionInitJob( bool halt )
{
	if ( Global::debugMode )
		log( "actionInitJob" );
	Q_UNUSED( halt ); // action takes only one tick, halt has no effect

	if ( !m_job )
	{
		return BT_RESULT::FAILURE;
	}

	// HaulToSite: set target to item position for pickup
	if ( m_job->type() == "HaulToSite" )
	{
		auto items = m_job->itemsToHaul();
		if ( !items.isEmpty() )
		{
			unsigned int itemID = items.first();
			if ( g->inv()->itemExists( itemID ) )
			{
				m_itemToPickUp = itemID;
				setCurrentTarget( g->inv()->getItemPos( itemID ) );
				return BT_RESULT::SUCCESS;
			}
		}
		return BT_RESULT::FAILURE;
	}

	if ( Global::debugMode )
	{
		log( "job pos:" + m_job->pos().toString() );
		log( "work position queue size:" + QString::number( m_workPositionQueue.size() ) );
	}
	while ( !m_workPositionQueue.empty() )
	{
		Position currentCheckPos = m_workPositionQueue.get();
		if ( Global::debugMode )
			log( "Check connection: " + m_position.toString() + " " + currentCheckPos.toString() );

		if ( g->pf()->checkConnectedRegions( m_position, currentCheckPos ) )
		{
			// found a suitable working position
			if ( Global::debugMode )
				log( "Set workpos: " + currentCheckPos.toString() );
			m_job->setWorkPos( currentCheckPos );
			// if item input was set before ( workshop job) don't overwrite it
			if ( m_job->posItemInput().isZero() )
			{
				m_job->setPosItemInput( currentCheckPos );
			}

			bool hasItems = m_job->requiredItems().size() == claimedItems().size();
			if ( Global::debugMode )
				log( "Has items: " + hasItems ? "true" : "false" );
			g->jm()->setJobBeingWorked( m_jobID, m_job->requiredTool().type.isEmpty() && hasItems );
			//log( "Init " + S::s( "$SkillName_" + m_job->requiredSkill() ) + " job done." );

			return BT_RESULT::SUCCESS;
		}
	}
	//abortJob( "initJob - job not reachable " );
	return BT_RESULT::FAILURE;
}

/// @brief Tries to claim the required items directly from the workshop's linked stockpile.
///        Moves matching items to the job's input position and registers them as claimed.
/// @param itemSID      Item type string ID.
/// @param materialSID  Required material, or "any".
/// @param count        Number of items needed.
/// @param requireSame  If true, all items must share the same material.
/// @param restriction  If non-empty, only these material SIDs are allowed.
/// @return True if all @p count items were successfully claimed; false otherwise.
bool Gnome::claimFromLinkedStockpile( QString itemSID, QString materialSID, int count, bool requireSame, QStringList restriction )
{
	if ( !m_job )
	{
		return false;
	}
	int claimed  = 0;
	Workshop* ws = g->wsm()->workshopAt( m_job->pos() );
	if ( ws && ws->linkedStockpile() && g->spm()->getStockpile( ws->linkedStockpile() ) )
	{
		Stockpile* sp = g->spm()->getStockpile( ws->linkedStockpile() );
		// is the whole needed number in the stockpile?
		if ( materialSID == "any" )
		{
			QList<QString> materials = g->inv()->materialsForItem( itemSID, count );

			if ( requireSame )
			{
				for ( auto mat : materials )
				{
					bool matAllowed = ( restriction.empty() || restriction.contains( mat ) );
					if ( matAllowed )
					{
						if ( sp->count( itemSID, mat ) < count )
						{
							return false;
						}
						for ( int i = 0; i < count; ++i )
						{
							unsigned int item = 0;
							for ( auto spf : sp->getFields() )
							{
								// if exists get item from that position
								item = g->inv()->getItemAtPos( spf->pos, true, itemSID, mat );
								if ( item )
								{
									g->inv()->moveItemToPos( item, m_job->posItemInput() );
									sp->setInfiNotFull( spf->pos );
									addClaimedItem( item, m_job->id() );
									++claimed;
									break;
								}
							}
						}
					}
				}
			}
			else
			{
				if ( restriction.empty() )
				{
					for ( int i = 0; i < count; ++i )
					{
						unsigned int item = 0;
						for ( auto spf : sp->getFields() )
						{
							// if exists get item from that position
							item = g->inv()->getItemAtPos( spf->pos, true, itemSID, "any" );
							if ( item )
							{
								g->inv()->moveItemToPos( item, m_job->posItemInput() );
								sp->setInfiNotFull( spf->pos );
								addClaimedItem( item, m_job->id() );
								++claimed;
								break;
							}
						}
					}
				}
				else
				{
					for ( int i = 0; i < count; ++i )
					{
						unsigned int item = 0;
						for ( auto spf : sp->getFields() )
						{
							// if exists get item from that position
							item = g->inv()->getItemAtPos( spf->pos, true, itemSID, "any" );
							if ( item && restriction.contains( g->inv()->materialSID( item ) ) )
							{
								g->inv()->moveItemToPos( item, m_job->posItemInput() );
								sp->setInfiNotFull( spf->pos );
								addClaimedItem( item, m_job->id() );
								++claimed;
								break;
							}
						}
					}
				}
			}
			if ( claimed == count )
			{
				return true;
			}
		}
		else
		{
			if ( sp->count( itemSID, materialSID ) < count )
			{
				return false;
			}
		}
		//if yes
		if ( materialSID != "any" )
		{
			// get from linked stockpile
			for ( int i = 0; i < count; ++i )
			{
				unsigned int item = 0;
				for ( auto spf : sp->getFields() )
				{
					// if exists get item from that position
					item = g->inv()->getItemAtPos( spf->pos, true, itemSID, materialSID );
					if ( item )
					{
						g->inv()->moveItemToPos( item, m_job->posItemInput() );
						sp->setInfiNotFull( spf->pos );
						addClaimedItem( item, m_job->id() );
						++claimed;
						break;
					}
				}
			}
			if ( claimed == count )
			{
				return true;
			}
		}
	}
	return false;
}

/// @brief BT action: claims all input items required by the current job.
///        Handles workshop crafts (with linked-stockpile preference), haul-item jobs, and generic jobs.
///        Items are reserved in the inventory so other gnomes won't take them.
/// @param halt Unused (single-tick action).
/// @return SUCCESS if all items were claimed; FAILURE if any item was unavailable.
BT_RESULT Gnome::actionClaimItems( bool halt )
{
	QElapsedTimer et;
	et.start();

	if ( Global::debugMode )
		log( "actionClaimItems" );
	Q_UNUSED( halt ); // action takes only one tick, halt has no effect

	if ( !m_job )
	{
		return BT_RESULT::FAILURE;
	}

	// If job went through phased hauling, items are already at the site — skip claiming
	if ( !m_job->haulSubJobs().isEmpty() || m_job->itemsRequired() > 0 )
	{
		return BT_RESULT::SUCCESS;
	}

	if ( !m_job )
	{
		return BT_RESULT::FAILURE;
	}

	if ( claimedItems().size() )
	{
		qDebug() << "Error: no items should be claimed here.";
		unclaimAll();
	}

	if ( m_job->type() == "CraftAtWorkshop" )
	{
		Workshop* ws = g->wsm()->workshopAt( m_job->pos() );
		for ( auto component : m_job->requiredItems() )
		{
			QString itemSID         = component.itemSID;
			QString materialSID     = component.materialSID;
			bool requireSame        = component.requireSame;
			QStringList restriction = component.materialRestriction;
			if ( restriction.size() == 1 && restriction.first().isEmpty() )
			{
				restriction.clear();
			}

			int count        = component.count;
			int countClaimed = 0;
			// check if item exists in linked stockpile
			if ( claimFromLinkedStockpile( itemSID, materialSID, count, requireSame, restriction ) )
			{
				continue;
			}

			else
			{
				if ( requireSame && materialSID == "any" )
				{
					QList<QString> materials = g->inv()->materialsForItem( itemSID, count );
					if ( materials.empty() )
					{
						return BT_RESULT::FAILURE;
					}
					bool found = false;
					for ( auto mat : materials )
					{
						bool matAllowed = ( restriction.empty() || restriction.contains( mat ) );
						if ( matAllowed )
						{
							auto items = g->inv()->getClosestItems( m_job->pos(), true, itemSID, mat, count );
							if ( items.size() < count )
							{
								continue;
							}

							for ( auto item : items )
							{
								addClaimedItem( item, m_job->id() );
							}
							found = true;
							break;
						}
					}
					if ( !found )
					{
						if ( Global::debugMode )
							log( "failed to claim: " + itemSID + " " + materialSID );
						//abortJob( "claimMaterial() not enough" );
						return BT_RESULT::FAILURE;
					}
				}
				else
				{
					for ( int i = 0; i < count; ++i )
					{
						unsigned int item = g->inv()->getClosestItem( m_job->pos(), true, itemSID, materialSID );
						if ( item )
						{
							addClaimedItem( item, m_job->id() );
						}
						else
						{
							if ( Global::debugMode )
								log( "failed to claim: " + itemSID + " " + materialSID );
							//abortJob( "claimMaterial() not enough" );
							return BT_RESULT::FAILURE;
						}
					}
				}
			}
		}
	}
	else if ( m_job->type() == "HauleItem" )
	{
		for ( auto itemID : m_job->itemsToHaul() )
		{
			addClaimedItem( itemID, m_job->id() );
		}
	}
	else
	{
		for ( auto component : m_job->requiredItems() )
		{
			int count          = component.count;
			QString itemID     = component.itemSID;
			QString materialID = component.materialSID;
			if ( Global::debugMode )
				log( "claim " + QString::number( count ) + " " + materialID + " " + itemID );

			QStringList restrictions = component.materialRestriction;
			if ( restrictions.size() == 1 && restrictions.first().isEmpty() )
			{
				restrictions.clear();
			}

			if ( materialID == "any" && !restrictions.empty() )
			{
				QSet<QString> matTypes;
				for ( auto type : restrictions )
				{
					matTypes.insert( type );
					unsigned int item = g->inv()->getClosestItem2( m_job->pos(), true, itemID, matTypes );
					if ( item )
					{
						addClaimedItem( item, m_job->id() );
					}
					else
					{
						if ( Global::debugMode )
							log( "claim failed ##1" );
						//abortJob( "claimMaterial()" );
						return BT_RESULT::FAILURE;
					}
				}
			}
			else
			{
				for ( int i = 0; i < count; ++i )
				{
					unsigned int item = g->inv()->getClosestItem( m_job->workPos(), true, itemID, materialID );
					if ( item )
					{
						addClaimedItem( item, m_job->id() );
					}
					else
					{
						if ( Global::debugMode )
							log( "claim failed ##2" );
						//abortJob( "claimMaterial()" );
						return BT_RESULT::FAILURE;
					}
				}
			}
		}
	}

	g->jm()->setJobBeingWorked( m_jobID, m_job->requiredTool().type.isEmpty() );
	if ( Global::debugMode )
		log( "actionClaimItems success" );

	if ( Global::debugMode )
	{
		auto ela = et.elapsed();
		if ( ela > 30 )
		{
			if ( m_job )
			{
				qDebug() << "CLAIMITEMS" << m_name << ela << "ms"
						 << "job:" << m_job->type() << m_job->pos().toString();
			}
		}
	}

	return BT_RESULT::SUCCESS;
}

/// @brief BT action: locates and claims the required tool for the current job.
///        If the gnome already has the right tool equipped, returns SUCCESS immediately.
///        Otherwise finds the closest tool of the required type and level in the world.
/// @param halt Unused (single-tick action).
/// @return SUCCESS if the tool was found or no tool is needed; FAILURE if unavailable.
BT_RESULT Gnome::actionFindTool( bool halt )
{
	if ( Global::debugMode )
		log( "actionFindTool" );
	Q_UNUSED( halt ); // action takes only one tick, halt has no effect

	if ( !m_job )
	{
		return BT_RESULT::FAILURE;
	}
	auto rt = m_job->requiredTool();
	if ( rt.type.isEmpty() || m_type == CreatureType::AUTOMATON )
	{
		setCurrentTarget( m_position );
		return BT_RESULT::SUCCESS;
	}

	unsigned int equippedItem = m_equipment.rightHandHeld.itemID;
	int equippedToolLevel     = Global::util->toolLevel( equippedItem );

	if ( equippedItem )
	{
		if ( g->inv()->itemSID( equippedItem ) == rt.type && equippedToolLevel >= rt.level )
		{
			// gnome already has the required tool equipped
			setCurrentTarget( m_position );
			return BT_RESULT::SUCCESS;
		}
		else
		{
			// wrong tool equipped,
			// drop tool
			g->inv()->putDownItem( equippedItem, m_position );
			g->inv()->setInJob( equippedItem, 0 );
			m_equipment.rightHandHeld.itemID = 0;
			m_equipment.rightHandHeld.item.clear();
			m_equipment.rightHandHeld.materialID = 0;
			m_equipment.rightHandHeld.material.clear();
			equipHand( 0, "Right" );
			//return BT_RESULT::RUNNING;
		}
	}
	//no item equipped
	QMap<QString, int> mc = g->inv()->materialCountsForItem( rt.type );
	QStringList keys      = mc.keys();

	for ( auto key : keys )
	{
		if ( mc[key] > 0 )
		{
			int tl = Global::util->toolLevel( key );
			if ( tl >= rt.level )
			{
				// there are a number of tools of the required level in the world
				auto tool = g->inv()->getClosestItem( m_position, true, rt.type, key );
				if ( tool )
				{
					m_job->setToolPosition( g->inv()->getItemPos( tool ) );
					g->inv()->setInJob( tool, m_job->id() );
					m_btBlackBoard.insert( "ClaimedTool", tool );

					setCurrentTarget( g->inv()->getItemPos( tool ) );

					return BT_RESULT::SUCCESS;
				}
			}
		}
	}
	//abortJob( "checkTool()" );
	return BT_RESULT::FAILURE;
}

/// @brief BT action: picks up and equips the claimed tool from the gnome's current tile.
///        Marks the job as being worked after successful equip.
/// @param halt Unused (single-tick action).
/// @return SUCCESS if equipped or no tool needed; FAILURE if the tool isn't at the gnome's position.
BT_RESULT Gnome::actionEquipTool( bool halt )
{
	if ( Global::debugMode )
		log( "actionEquipTool" );
	Q_UNUSED( halt ); // action takes only one tick, halt has no effect

	if ( !m_job )
	{
		return BT_RESULT::FAILURE;
	}

	if ( m_job->requiredTool().type.isEmpty() || m_type == CreatureType::AUTOMATON )
	{
		//log( "No tool required" );
		g->jm()->setJobBeingWorked( m_jobID, true );
		return BT_RESULT::SUCCESS;
	}

	if ( m_equipment.rightHandHeld.itemID )
	{
		int equippedToolLevel = Global::util->toolLevel( m_equipment.rightHandHeld.itemID );
		if ( m_equipment.rightHandHeld.item == m_job->requiredTool().type && equippedToolLevel >= m_job->requiredTool().level )
		{
			g->jm()->setJobBeingWorked( m_jobID, true );
			return BT_RESULT::SUCCESS;
		}
	}

	unsigned int claimedTool = m_btBlackBoard.value( "ClaimedTool" ).toUInt();
	if ( claimedTool )
	{
		if ( m_position == g->inv()->getItemPos( claimedTool ) )
		{
			g->inv()->pickUpItem( claimedTool, m_id );
			m_equipment.rightHandHeld.itemID     = claimedTool;
			m_equipment.rightHandHeld.item       = g->inv()->itemSID( claimedTool );
			m_equipment.rightHandHeld.materialID = g->inv()->materialUID( claimedTool );
			m_equipment.rightHandHeld.material   = g->inv()->materialSID( claimedTool );

			m_btBlackBoard.remove( "ClaimedTool" );

			equipHand( claimedTool, "Right" );

			g->jm()->setJobBeingWorked( m_jobID, true );

			return BT_RESULT::SUCCESS;
		}
	}
	//abortJob( "pickUpTool()" );
	return BT_RESULT::FAILURE;
}

/// @brief Checks a single equipment slot against the assigned uniform and fixes any mismatch.
///        If the wrong item is worn it is dropped.  If the correct item isn't yet worn, a claim
///        and EquipItem job are created for the nearest suitable item.
///        Does nothing if the gnome is currently working a job.
/// @param slot    Equipment slot name (e.g. "HeadArmor", "RightHandHeld").
/// @param uniform The target uniform to match.
/// @param dropped Set to true if an existing item was dropped.
/// @return True if a new EquipItem job was started; false if slot is already correct or gnome is busy.
bool Gnome::checkUniformItem( QString slot, Uniform* uniform, bool& dropped )
{
	if ( m_jobID )
	{
		return false;
	}

	auto part = Global::creaturePartLookUp.value( slot );

	QString item     = uniform->parts[slot].item;
	QString material = uniform->parts[slot].material;

	QString wiSID         = "";
	QString wiMat         = "";
	unsigned int wornItem = 0;

	switch ( part )
	{
		case CP_ARMOR_HEAD:
			if ( m_equipment.head.itemID )
			{
				wornItem = m_equipment.head.itemID;
				wiSID    = m_equipment.head.item;
				wiMat    = m_equipment.head.material;
			}
			break;
		case CP_ARMOR_TORSO:
			if ( m_equipment.chest.itemID )
			{
				wornItem = m_equipment.chest.itemID;
				wiSID    = m_equipment.chest.item;
				wiMat    = m_equipment.chest.material;
			}
			break;
		case CP_ARMOR_ARM:
			if ( m_equipment.arm.itemID )
			{
				wornItem = m_equipment.arm.itemID;
				wiSID    = m_equipment.arm.item;
				wiMat    = m_equipment.arm.material;
			}
			break;
		case CP_ARMOR_HAND:
			if ( m_equipment.hand.itemID )
			{
				wornItem = m_equipment.hand.itemID;
				wiSID    = m_equipment.hand.item;
				wiMat    = m_equipment.hand.material;
			}
			break;
		case CP_ARMOR_LEG:
			if ( m_equipment.leg.itemID )
			{
				wornItem = m_equipment.leg.itemID;
				wiSID    = m_equipment.leg.item;
				wiMat    = m_equipment.leg.material;
			}
			break;
		case CP_ARMOR_FOOT:
			if ( m_equipment.foot.itemID )
			{
				wornItem = m_equipment.foot.itemID;
				wiSID    = m_equipment.foot.item;
				wiMat    = m_equipment.foot.material;
			}
			break;
		case CP_LEFT_HAND_HELD:
			if ( m_equipment.leftHandHeld.itemID )
			{
				wornItem = m_equipment.leftHandHeld.itemID;
				wiSID    = m_equipment.leftHandHeld.item;
				wiMat    = m_equipment.leftHandHeld.material;
			}
			break;
		case CP_RIGHT_HAND_HELD:
			if ( m_equipment.rightHandHeld.itemID )
			{
				wornItem = m_equipment.rightHandHeld.itemID;
				wiSID    = m_equipment.rightHandHeld.item;
				wiMat    = m_equipment.rightHandHeld.material;
			}
			break;
		case CP_BACK:
			if ( m_equipment.back.itemID )
			{
				wornItem = m_equipment.back.itemID;
				wiSID    = m_equipment.back.item;
				wiMat    = m_equipment.back.material;
			}
			break;
	}

	if ( wornItem )
	{
		if ( wiSID != item || ( ( wiMat != material ) && ( material != "any" ) ) )
		{
			log( "Drop item:" + wiMat + " " + wiSID + " looking for:" + material + " " + item );
			//drop current item
			dropped = true;
			g->inv()->putDownItem( wornItem, m_position );
			g->inv()->setInJob( wornItem, 0 );

			if ( item == "none" || item.isEmpty() )
			{
				return true;
			}
		}
		else
		{
			return false;
		}
	}

	if ( !item.isEmpty() )
	{
		auto itemToGet = g->inv()->getClosestItem( m_position, true, item, material );

		if ( itemToGet )
		{
			auto pos = g->inv()->getItemPos( itemToGet );

			m_btBlackBoard.insert( "ClaimedUniformItem", itemToGet );
			m_btBlackBoard.insert( "ClaimedUniformItemSlot", slot );

			m_jobID = g->jm()->addJob( "EquipItem", pos, 0, true );

			if ( m_jobID != 0 )
			{
				m_jobChanged = true;
				m_job        = g->jm()->getJob( m_jobID );
				if ( m_job )
				{
					g->inv()->setInJob( itemToGet, m_jobID );
					//no change to jobsprite
					g->jm()->setJobBeingWorked( m_jobID, true );
					m_job->setIsWorked( true );
					m_job->setWorkedBy( m_id );
					m_job->setDestroyOnAbort( true );
					log( "JobType " + m_job->type() );
					m_btBlackBoard.insert( "JobType", m_job->type() );
					m_currentAction = "job";

					m_workPositionQueue = PriorityQueue<Position, int>();
					m_workPositionQueue.put( pos, m_position.distSquare( pos ) );

					return true;
				}
			}
		}
	}

	return false;
}

/// @brief BT action: iterates all uniform equipment slots and fixes any mismatch via checkUniformItem().
///        Throttled to avoid running every tick (300-tick cooldown after a fully-correct check).
///        If the gnome has no role, strips all worn uniform items.
/// @param halt Unused.
/// @return SUCCESS if a correction was initiated; FAILURE if nothing needed changing or gnome has no role.
BT_RESULT Gnome::actionCheckUniform( bool halt = false )
{
	if ( GameState::tick > m_nextUniformCheckTick )
	{
		if ( m_roleID )
		{
			Uniform* uniform = g->mil()->uniform( m_roleID );
			if ( uniform )
			{
				bool dropped = false;
				bool success = false;
				if ( checkUniformItem( "HeadArmor", uniform, dropped ) )
				{
					if ( dropped )
					{
						m_equipment.head.item.clear();
						m_equipment.head.material.clear();
						m_equipment.head.itemID     = 0;
						m_equipment.head.materialID = 0;
					}
					success = true;
				}
				else if ( checkUniformItem( "ChestArmor", uniform, dropped ) )
				{
					if ( dropped )
					{
						m_equipment.chest.item.clear();
						m_equipment.chest.material.clear();
						m_equipment.chest.itemID     = 0;
						m_equipment.chest.materialID = 0;
					}
					success = true;
				}
				else if ( checkUniformItem( "ArmArmor", uniform, dropped ) )
				{
					if ( dropped )
					{
						m_equipment.arm.item.clear();
						m_equipment.arm.material.clear();
						m_equipment.arm.itemID     = 0;
						m_equipment.arm.materialID = 0;
					}
					success = true;
				}
				else if ( checkUniformItem( "HandArmor", uniform, dropped ) )
				{
					if ( dropped )
					{
						m_equipment.hand.item.clear();
						m_equipment.hand.material.clear();
						m_equipment.hand.itemID     = 0;
						m_equipment.hand.materialID = 0;
					}
					success = true;
				}
				else if ( checkUniformItem( "LegArmor", uniform, dropped ) )
				{
					if ( dropped )
					{
						m_equipment.leg.item.clear();
						m_equipment.leg.material.clear();
						m_equipment.leg.itemID     = 0;
						m_equipment.leg.materialID = 0;
					}
					success = true;
				}
				else if ( checkUniformItem( "FootArmor", uniform, dropped ) )
				{
					if ( dropped )
					{
						m_equipment.foot.item.clear();
						m_equipment.foot.material.clear();
						m_equipment.foot.itemID     = 0;
						m_equipment.foot.materialID = 0;
					}
					success = true;
				}
				else if ( checkUniformItem( "Back", uniform, dropped ) )
				{
					if ( dropped )
					{
						m_equipment.back.item.clear();
						m_equipment.back.material.clear();
						m_equipment.back.itemID     = 0;
						m_equipment.back.materialID = 0;

						if ( m_inventoryItems.size() )
						{
							for ( auto item : m_inventoryItems )
							{
								g->inv()->setInJob( item, 0 );
								g->inv()->putDownItem( item, m_position );
							}
							m_inventoryItems.clear();
							m_carriedDrinks   = 0;
							m_carriedFood     = 0;
							m_carriedBandages = 0;
						}
					}
					success = true;
				}
				else if ( checkUniformItem( "LeftHandHeld", uniform, dropped ) )
				{
					if ( dropped )
					{
						m_equipment.leftHandHeld.item.clear();
						m_equipment.leftHandHeld.material.clear();
						m_equipment.leftHandHeld.itemID     = 0;
						m_equipment.leftHandHeld.materialID = 0;

						equipHand( 0, "Left" );
					}
					success = true;
				}
				else if ( checkUniformItem( "RightHandHeld", uniform, dropped ) )
				{
					if ( dropped )
					{
						m_equipment.rightHandHeld.item.clear();
						m_equipment.rightHandHeld.material.clear();
						m_equipment.rightHandHeld.itemID     = 0;
						m_equipment.rightHandHeld.materialID = 0;

						equipHand( 0, "Right" );
					}
					success = true;
				}

				if ( dropped )
				{
					updateSprite();
				}
				if ( success )
				{
					return BT_RESULT::SUCCESS;
				}
				else
				{
					m_nextUniformCheckTick = GameState::tick + 300;
				}
			}
		}
		else
		{
			if ( m_uniformWorn )
			{
				Uniform uniform;
				bool dropped = false;
				checkUniformItem( "HeadArmor", &uniform, dropped );
				checkUniformItem( "ChestArmor", &uniform, dropped );
				checkUniformItem( "ArmArmor", &uniform, dropped );
				checkUniformItem( "HandArmor", &uniform, dropped );
				checkUniformItem( "LegArmor", &uniform, dropped );
				checkUniformItem( "FootArmor", &uniform, dropped );
				checkUniformItem( "LeftHandHeld", &uniform, dropped );
				checkUniformItem( "RightHandHeld", &uniform, dropped );
				checkUniformItem( "Back", &uniform, dropped );
				equipHand( 0, "Left" );
				equipHand( 0, "Right" );
				m_uniformWorn = false;
				if ( dropped )
				{
					m_equipment.clearAllItems();
					updateSprite();
				}
			}
		}
	}

	return BT_RESULT::FAILURE;
}

/// @brief BT action: releases a claimed-but-unequipped uniform item and clears the blackboard entries.
/// @param halt Unused.
/// @return FAILURE (always, so the BT can fall through to cleanup).
BT_RESULT Gnome::actionUniformCleanUp( bool halt )
{
	auto item = m_btBlackBoard.value( "ClaimedUniformItem" ).toUInt();
	g->inv()->setInJob( item, 0 );
	m_btBlackBoard.remove( "ClaimedUniformItem" );
	m_btBlackBoard.remove( "ClaimedUniformItemSlot" );

	return BT_RESULT::FAILURE;
}

/// @brief BT action: stocks the gnome's backpack with bandages, food, or drinks if below threshold (3 each).
///        Only acts when the gnome is wearing a Backpack.  Skips food/drink sources more than 10 tiles away.
/// @param halt Unused.
/// @return SUCCESS if an item was found and claimed for pickup; FAILURE otherwise.
BT_RESULT Gnome::actionCheckBandages( bool halt )
{
	if ( m_equipment.back.item == "Backpack" )
	{
		unsigned int itemToGet = 0;
		if ( m_carryBandages && m_carriedBandages < 3 )
		{
			itemToGet = g->inv()->getClosestItem( m_position, true, "Bandage", "any" );
		}
		else if ( m_carryFood && m_carriedFood < 3 )
		{
			itemToGet = g->inv()->getFoodItem( m_position );
			if ( itemToGet )
			{
				if ( m_position.distSquare( g->inv()->getItemPos( itemToGet ) ) > 10 )
				{
					if ( m_carryDrinks && m_carriedDrinks < 3 )
					{
						itemToGet = g->inv()->getDrinkItem( m_position );
						if ( m_position.distSquare( g->inv()->getItemPos( itemToGet ) ) > 10 )
						{
							itemToGet = 0;
						}
					}
				}
			}
		}
		else if ( m_carryDrinks && m_carriedDrinks < 3 )
		{
			itemToGet = g->inv()->getDrinkItem( m_position );
			if ( m_position.distSquare( g->inv()->getItemPos( itemToGet ) ) > 10 )
			{
				itemToGet = 0;
			}
		}
		if ( itemToGet )
		{
			auto pos = g->inv()->getItemPos( itemToGet );
			g->inv()->setInJob( itemToGet, m_id );
			m_itemToPickUp = itemToGet;
			m_btBlackBoard.insert( "ClaimedInventoryItem", itemToGet );
			setCurrentTarget( pos );
			return BT_RESULT::SUCCESS;
		}
	}
	return BT_RESULT::FAILURE;
}

/// @brief BT action: sets the navigation target to the job's item input position.
/// @param halt Unused (single-tick action).
/// @return SUCCESS if the input position is valid; FAILURE otherwise.
BT_RESULT Gnome::actionGetItemDropPosition( bool halt )
{
	if ( Global::debugMode )
		log( "actionGetItemDropPosition" );
	Q_UNUSED( halt ); // action takes only one tick, halt has no effect

	if ( !m_job )
	{
		return BT_RESULT::FAILURE;
	}

	Position pos = m_job->posItemInput();
	if ( pos.isZero() )
	{
		//abortJob( "actionGetItemDropPosition()" );
		return BT_RESULT::FAILURE;
	}
	setCurrentTarget( pos );
	return BT_RESULT::SUCCESS;
}

/// @brief BT action: drops the first carried item at the gnome's current position.
///        If the job has a stockpile, inserts the item into it.
/// @param halt Unused (single-tick action).
/// @return SUCCESS if an item was dropped; FAILURE if nothing carried.
BT_RESULT Gnome::actionDropItem( bool halt )
{
	if ( Global::debugMode )
		log( "actionDropItem" );
	Q_UNUSED( halt ); // action takes only one tick, halt has no effect

	if ( !m_job )
	{
		return BT_RESULT::FAILURE;
	}
	while ( m_carriedItems.size() )
	{
		unsigned int carriedItem = m_carriedItems.takeFirst();
		if ( carriedItem != 0 )
		{
			g->inv()->putDownItem( carriedItem, m_position );
			if ( m_job->stockpile() != 0 )
			{
				log( "Put " + g->inv()->materialSID( carriedItem ) + " " + g->inv()->itemSID( carriedItem ) + " into stockpile at " + m_position.toString() );
				g->spm()->insertItem( m_job->stockpile(), m_position, carriedItem );
			}
			//m_job->removeClaimedItem2( item );
			return BT_RESULT::SUCCESS;
		}
		/*
		else
		{
			qDebug() << name() << "drop item failed - null item";
			return BT_RESULT::FAILURE;
		}
		*/
	}
	return BT_RESULT::FAILURE;
}

/// @brief BT action: drops all carried items at the gnome's current position.
///        If the job has a stockpile, inserts each item into it.
/// @param halt Unused (single-tick action).
/// @return SUCCESS.
BT_RESULT Gnome::actionDropAllItems( bool halt )
{
	if ( Global::debugMode )
		log( "actionDropItem" );
	Q_UNUSED( halt ); // action takes only one tick, halt has no effect

	if ( !m_job )
	{
		return BT_RESULT::FAILURE;
	}

	for ( auto item : m_carriedItems )
	{
		if ( item )
		{
			g->inv()->putDownItem( item, m_position );
			if ( m_job->stockpile() != 0 )
			{
				log( "Put " + g->inv()->materialSID( item ) + " " + g->inv()->itemSID( item ) + " into stockpile at " + m_position.toString() );
				g->spm()->insertItem( m_job->stockpile(), m_position, item );
			}
		}
	}
	m_carriedItems.clear();

	return BT_RESULT::SUCCESS;
}

/// @brief BT action: sets the navigation target to the job's stored work position.
/// @param halt Unused (single-tick action).
/// @return SUCCESS if the job exists; FAILURE otherwise.
BT_RESULT Gnome::actionGetWorkPosition( bool halt )
{
	if ( Global::debugMode )
		log( "actionGetWorkPosition" );
	Q_UNUSED( halt ); // action takes only one tick, halt has no effect

	if ( !m_job )
	{
		return BT_RESULT::FAILURE;
	}

	setCurrentTarget( m_job->workPos() );
	return BT_RESULT::SUCCESS;
}

/// @brief BT action: executes the current task in the job's task list.
///        Loads task list on first visit, runs the skill-scaled duration countdown, calls task
///        functions (+ optional animate variant each tick), and advances to the next task on completion.
///        Suspends the job on halt.
/// @param halt True if the BT subtree is being interrupted.
/// @return RUNNING while working; SUCCESS when all tasks complete; FAILURE on error.
BT_RESULT Gnome::actionWork( bool halt )
{
	if ( Global::debugMode )
		log( "actionWork" );
	if ( halt )
	{
		if ( m_job )
		{
			suspendJob();
		}
		return BT_RESULT::IDLE;
	}

	if ( !m_job )
	{
		return BT_RESULT::FAILURE;
	}

	if ( m_job->isCanceled() )
	{
		//abortJob( "Canceled" );
		return BT_RESULT::FAILURE;
	}

	//first visit
	if ( m_currentAction != "work job" )
	{
		m_currentAction = "work job";
		m_taskList      = DB::selectRows( "Jobs_Tasks", "ID", m_job->type() );
		if ( m_taskList.size() > 0 )
		{
			m_currentTask = m_taskList.takeFirst();

			QString skillID = m_job->requiredSkill();
			float current   = Global::util->reverseFib( m_skills.value( skillID ).toUInt() );
			float ticks     = getDurationTicks( m_currentTask.value( "Duration" ), m_job );

			ticks                = qMax( 10., qMin( 1000., ticks - ( ( ticks / 20. ) * current ) ) );
			m_taskFinishTick     = GameState::tick + ticks;
			m_totalDurationTicks = ticks;
			g->sm()->playEffect( m_job->type(), m_position, m_job->item() );
		}
		else
		{
			// job has no task, that's for example the case with hauling jobs
			log( "Nothing else to do." );
			m_currentTask.clear();
			return BT_RESULT::SUCCESS;
		}
	}

	QString taskName = m_currentTask.value( "Task" ).toString();

	if ( GameState::tick < m_taskFinishTick )
	{
		if ( m_taskFunctions.contains( taskName + "Animate" ) )
		{
			m_taskFunctions[taskName + "Animate"]();
		}
		if ( (m_taskFinishTick-GameState::tick) % 60 == 0) {
			// Play sound again
			g->sm()->playEffect( m_job->type(), m_position, m_job->item() );
		}
		return BT_RESULT::RUNNING;
	}

	if ( m_taskFunctions.contains( taskName ) )
	{
		if ( !m_taskFunctions[taskName]() )
		{
			return BT_RESULT::FAILURE;
		}

		if ( m_repeatJob )
		{
			return BT_RESULT::RUNNING;
		}

		if ( m_taskList.size() > 0 )
		{
			m_currentTask = m_taskList.takeFirst();

			QString skillID = m_job->requiredSkill();
			float current   = Global::util->reverseFib( m_skills.value( skillID ).toUInt() );
			float ticks     = getDurationTicks( m_currentTask.value( "Duration" ), m_job );
			if ( ticks > 0 )
			{
				ticks = qMax( 10., qMin( 1000., ticks - ( ( ticks / 20. ) * current ) ) );
			}
			m_taskFinishTick     = GameState::tick + ticks;
			m_totalDurationTicks = ticks;

			return BT_RESULT::RUNNING;
		}
		else
		{
			return BT_RESULT::SUCCESS;
		}
	}
	else
	{
		m_job->setCanceled();
		//abortJob( "task name doesn't exist -" + taskName + " - "  );
	}
	return BT_RESULT::FAILURE;
}

/// @brief BT action: makes the target animal follow the gnome (sets follow ID).
/// @param halt Unused (single-tick action).
/// @return SUCCESS if the animal was grabbed; FAILURE if the job or animal is invalid.
BT_RESULT Gnome::actionGrabAnimal( bool halt )
{
	if ( Global::debugMode )
		log( "actionGrabAnimal" );
	Q_UNUSED( halt ); // action takes only one tick, halt has no effect

	if ( !m_job )
	{
		return BT_RESULT::FAILURE;
	}

	Animal* a = g->cm()->animal( m_job->animal() );
	if ( !a || a->toDestroy() )
	{
		//abortJob( "grabAnimal()" );
		return BT_RESULT::FAILURE;
	}
	else
	{
		a->setFollowID( m_id );
		m_animal = m_job->animal();
		return BT_RESULT::SUCCESS;
	}
}

/// @brief BT action: releases the held animal (clears follow ID, restores mobility, clears job).
/// @param halt Unused (single-tick action).
/// @return SUCCESS if the animal was found and released; FAILURE otherwise.
BT_RESULT Gnome::actionReleaseAnimal( bool halt )
{
	if ( Global::debugMode )
		log( "actionReleaseAnimal" );
	Q_UNUSED( halt ); // action takes only one tick, halt has no effect

	if ( !m_job )
	{
		return BT_RESULT::FAILURE;
	}

	m_animal  = 0;
	Animal* a = g->cm()->animal( m_job->animal() );
	if ( a )
	{
		a->setFollowID( 0 );
		a->setImmobile( false );
		a->setInJob( 0 );
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

/// @brief BT action: nudges the followed animal toward the gnome's current position.
/// @param halt Unused (single-tick action).
/// @return SUCCESS if the animal exists; FAILURE otherwise.
BT_RESULT Gnome::actionFinalMoveAnimal( bool halt )
{
	if ( Global::debugMode )
		log( "actionFinalMoveAnimal" );
	Q_UNUSED( halt ); // action takes only one tick, halt has no effect
	auto animal = g->cm()->animal( m_animal );
	if ( animal )
	{
		animal->setFollowPosition( m_position );
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

/// @brief BT action: butchers the target animal after a 50-tick delay, creating species-specific
///        byproducts (meat, bone, hide, etc.) from the Animals_OnButcher DB table.
///        Handles pasture removal and marks the animal for destruction.
/// @param halt True if interrupted — returns IDLE.
/// @return RUNNING while waiting; SUCCESS on completion; FAILURE on missing animal or canceled job.
BT_RESULT Gnome::actionButcherAnimal( bool halt )
{
	if ( Global::debugMode )
		log( "actionButcherAnimal" );
	if ( halt )
	{
		//abortJob( "actionButcherAnimal() - halt" );
		return BT_RESULT::IDLE;
	}
	if ( !m_job )
	{
		return BT_RESULT::FAILURE;
	}

	if ( m_job->isAborted() )
	{
		m_job->setAborted( false );
		//abortJob( "Aborted" );
		return BT_RESULT::FAILURE;
	}
	if ( m_job->isCanceled() )
	{
		//abortJob( "Canceled" );
		return BT_RESULT::FAILURE;
	}

	//first visit?
	if ( m_currentAction != "butcher animal" )
	{
		m_currentAction = "butcher animal";

		QString skillID      = m_job->requiredSkill();
		float current        = Global::util->reverseFib( m_skills.value( skillID ).toUInt() );
		m_totalDurationTicks = 50; //TODO get that number from DB
		m_taskFinishTick     = GameState::tick + 50;
	}

	QString taskName = m_currentTask.value( "Task" ).toString();

	if ( GameState::tick < m_taskFinishTick )
	{
		return BT_RESULT::RUNNING;
	}

	Animal* a = g->cm()->animal( m_job->animal() );
	if ( a )
	{
		a->setFollowID( 0 );
		m_animal = 0;

		if ( a->pastureID() )
		{
			auto pasture = g->fm()->getPasture( a->pastureID() );
			if ( pasture )
			{
				pasture->removeAnimal( a->id() );
				g->fm()->emitUpdateSignalPasture( pasture->id() );
			}
		}

		auto onbutchlist = DB::selectRows( "Animals_OnButcher", "ID", a->species() );
		for ( auto obv : onbutchlist )
		{
			auto obm       = obv;
			QString itemID = obm.value( "ItemID" ).toString();
			int amount     = obm.value( "Amount" ).toInt();
			if ( a->isYoung() )
			{
				amount /= 2;
			}
			for ( int i = 0; i < amount; ++i )
			{
				if ( itemID == "Bone" || itemID == "Skull" )
				{
					g->inv()->createItem( m_job->posItemOutput(), itemID, { a->species() + "Bone" } );
				}
				else
				{
					g->inv()->createItem( m_job->posItemOutput(), itemID, { a->species() } );
				}
			}
		}
		a->destroy();
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

/// @brief BT action: applies a dye to the target animal after a 50-tick delay.
///        Destroys the claimed dye item and sets the animal's dye material.
/// @param halt True if interrupted — returns IDLE.
/// @return RUNNING while waiting; SUCCESS on completion; FAILURE on error.
BT_RESULT Gnome::actionDyeAnimal( bool halt )
{
	if ( Global::debugMode )
		log( "actionDyeAnimal" );
	if ( halt )
	{
		//abortJob( "actionButcherAnimal() - halt" );
		return BT_RESULT::IDLE;
	}
	if ( !m_job )
	{
		return BT_RESULT::FAILURE;
	}

	if ( m_job->isAborted() )
	{
		m_job->setAborted( false );
		//abortJob( "Aborted" );
		return BT_RESULT::FAILURE;
	}
	if ( m_job->isCanceled() )
	{
		//abortJob( "Canceled" );
		return BT_RESULT::FAILURE;
	}

	//first visit?
	if ( m_currentAction != "dye animal" )
	{
		m_currentAction = "dye animal";

		QString skillID      = m_job->requiredSkill();
		float current        = Global::util->reverseFib( m_skills.value( skillID ).toUInt() );
		m_totalDurationTicks = 50; //TODO get that number from DB
		m_taskFinishTick     = GameState::tick + 50;
	}

	QString taskName = m_currentTask.value( "Task" ).toString();

	if ( GameState::tick < m_taskFinishTick )
	{
		return BT_RESULT::RUNNING;
	}

	Animal* a = g->cm()->animal( m_job->animal() );
	if ( a )
	{
		a->setFollowID( 0 );
		m_animal = 0;

		destroyClaimedItems();

		a->setDye( m_job->material() );
		a->setInJob( 0 );

		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

/// @brief BT action: harvests produce from the target animal (wool, milk, eggs, etc.) after 100 ticks.
///        Amount and item type come from the animal's produce definition; dye tint is applied if set.
/// @param halt True if interrupted — returns IDLE.
/// @return RUNNING while waiting; SUCCESS on completion; FAILURE on error.
BT_RESULT Gnome::actionHarvestAnimal( bool halt )
{
	if ( Global::debugMode )
		log( "actionHarvestAnimal" );
	if ( halt )
	{
		//abortJob( "actionButcherAnimal() - halt" );
		return BT_RESULT::IDLE;
	}
	if ( !m_job )
	{
		return BT_RESULT::FAILURE;
	}
	if ( m_job->isAborted() )
	{
		m_job->setAborted( false );
		//abortJob( "Aborted" );
		return BT_RESULT::FAILURE;
	}
	if ( m_job->isCanceled() )
	{
		//abortJob( "Canceled" );
		return BT_RESULT::FAILURE;
	}

	//first visit?
	if ( m_currentAction != "harvest animal" )
	{
		m_currentAction = "harvest animal";

		QString skillID      = m_job->requiredSkill();
		float current        = Global::util->reverseFib( m_skills.value( skillID ).toUInt() );
		m_totalDurationTicks = 100;
		m_taskFinishTick     = GameState::tick + 100;
	}

	QString taskName = m_currentTask.value( "Task" ).toString();

	if ( GameState::tick < m_taskFinishTick )
	{
		return BT_RESULT::RUNNING;
	}

	Animal* a = g->cm()->animal( m_animal );
	if ( a )
	{
		QString type = a->species();
		QString dye  = a->dye();

		int amount     = a->numProduce();
		QString itemID = a->producedItem();

		if ( !dye.isEmpty() )
		{
			type = Global::util->addDyeMaterial( type, dye );
		}

		for ( int i = 0; i < amount; ++i )
		{
			g->inv()->createItem( m_position, itemID, { type } );
		}
		a->harvest();

		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

/// @brief BT action: tames the target animal after a delay, adding it to the colony's roster.
/// @param halt True if interrupted — returns IDLE.
/// @return RUNNING while waiting; SUCCESS on completion; FAILURE on error.
BT_RESULT Gnome::actionTameAnimal( bool halt )
{
	if ( Global::debugMode )
		log( "actionTameAnimal" );
	if ( halt )
	{
		//abortJob( "actionButcherAnimal() - halt" );
		return BT_RESULT::IDLE;
	}
	if ( !m_job )
	{
		return BT_RESULT::FAILURE;
	}
	if ( m_job->isAborted() )
	{
		m_job->setAborted( false );
		//abortJob( "Aborted" );
		return BT_RESULT::FAILURE;
	}
	if ( m_job->isCanceled() )
	{
		//abortJob( "Canceled" );
		return BT_RESULT::FAILURE;
	}

	//first visit?
	if ( m_currentAction != "tame animal" )
	{
		m_currentAction = "tame animal";

		QString skillID      = m_job->requiredSkill();
		float current        = Global::util->reverseFib( m_skills.value( skillID ).toUInt() );
		m_totalDurationTicks = 100;
		m_taskFinishTick     = GameState::tick + 100;
	}

	if ( GameState::tick < m_taskFinishTick )
	{
		return BT_RESULT::RUNNING;
	}

	Animal* a = g->cm()->animal( m_job->animal() );
	if ( a )
	{
		a->setTame( true );

		auto pasture = g->fm()->getPastureAtPos( m_job->posItemInput() );

		pasture->addAnimal( a->id() );
		log( "Tamed a " + a->species() );
		return BT_RESULT::SUCCESS;
	}
	log( "Failed to tame." );
	//abortJob( "tame failed" );
	return BT_RESULT::FAILURE;
}

/// @brief BT action: always returns RUNNING (used as a placeholder/loop node).
/// @param halt Unused.
/// @return RUNNING.
BT_RESULT Gnome::actionAlwaysRunning( bool halt )
{
	return BT_RESULT::RUNNING;
}

/// @brief BT action: attacks the current combat target with the best available hand each tick.
///        Uses per-hand and global cooldowns to regulate attack frequency.
///        Chooses Slash for armed attacks and Blunt for unarmed.
/// @param halt Unused.
/// @return RUNNING while target is alive; FAILURE if target died or doesn't exist.
BT_RESULT Gnome::actionAttackTarget( bool halt )
{
	if ( Global::debugMode )
		log( "actionAttackTarget" );
	Creature* creature = g->cm()->creature( m_currentAttackTarget );

	if ( creature && !creature->isDead() )
	{
		m_facing = getFacing( m_position, creature->getPos() );

		//do we have
		if ( m_globalCooldown <= 0 )
		{
			if ( m_rightHandArmed || m_leftHandArmed )
			{
				if ( m_rightHandCooldown <= 0 )
				{
					Global::logger().log( LogType::COMBAT, m_name + " attacks " + creature->name(), m_id );
					// attack with main hand
					creature->attack( DT_SLASH, m_anatomy.randomAttackHeight(), m_rightHandAttackSkill, m_rightHandAttackValue, m_position, m_id );
					m_rightHandCooldown = qMax( 5, 20 - m_rightHandAttackSkill );
					m_globalCooldown    = 5;
				}
				else if ( m_leftHandCooldown <= 0 )
				{
					// wielding an offhand weapon?
					if ( m_leftHandHasWeapon )
					{
						Global::logger().log( LogType::COMBAT, m_name + " attacks " + creature->name(), m_id );
						creature->attack( DT_SLASH, m_anatomy.randomAttackHeight(), m_leftHandAttackSkill, m_leftHandAttackValue, m_position, m_id );
						m_leftHandCooldown = qMax( 5, 20 - m_leftHandAttackSkill );
						m_globalCooldown   = 5;
					}
				}
			}
			else //unarmed combat
			{
				if ( m_rightHandCooldown <= 0 )
				{
					Global::logger().log( LogType::COMBAT, m_name + " punches " + creature->name(), m_id );
					// attack with main hand
					creature->attack( DT_BLUNT, m_anatomy.randomAttackHeight(), m_rightHandAttackSkill, m_rightHandAttackValue, m_position, m_id );
					m_rightHandCooldown = qMax( 5, 20 - m_rightHandAttackSkill );
					m_globalCooldown    = 5;
				}
				else if ( m_leftHandCooldown <= 0 )
				{
					// wielding an offhand weapon?
					Global::logger().log( LogType::COMBAT, m_name + " punches " + creature->name(), m_id );
					creature->attack( DT_BLUNT, m_anatomy.randomAttackHeight(), m_leftHandAttackSkill, m_leftHandAttackValue, m_position, m_id );
					m_leftHandCooldown = qMax( 5, 20 - m_leftHandAttackSkill );
					m_globalCooldown   = 5;
				}
			}
		}
		return BT_RESULT::RUNNING;
	}
	return BT_RESULT::FAILURE;
}

/// @brief BT action: finds the nearest reachable training ground workshop and sets it as the target.
/// @param halt Unused.
/// @return SUCCESS if a training ground was found; FAILURE otherwise.
BT_RESULT Gnome::actionFindTrainingGround( bool halt )
{
	auto tgs = g->wsm()->getTrainingGrounds();

	PriorityQueue<Workshop*, int> pq;

	for ( auto tg : tgs )
	{
		if ( g->pf()->checkConnectedRegions( m_position, tg->pos() ) )
		{
			pq.put( tg, m_position.distSquare( tg->pos() ) );
		}
	}
	if ( !pq.empty() )
	{
		auto tg = pq.get();
		setCurrentTarget( tg->pos() );
		m_trainCounter   = -1;
		m_trainingGround = tg->id();
		return BT_RESULT::SUCCESS;
	}

	return BT_RESULT::FAILURE;
}

/// @brief BT action: runs a 15-minute training session at the assigned training ground.
///        Awards Melee and Dodge skill points (x2 if a higher-skilled trainer is present).
/// @param halt Unused.
/// @return RUNNING while training; SUCCESS when the session ends.
BT_RESULT Gnome::actionTrain( bool halt )
{
	if ( m_trainCounter == 0 )
	{
		m_log.append( "Finished Training session." );
		m_trainCounter = -1;
		auto ws        = g->wsm()->workshop( m_trainingGround );
		if ( ws )
		{
			QString type = ws->type();
			if ( type == "MeleeTraining" )
			{
				unsigned int trainer = ws->assignedGnome();
				int skillGain        = 3;
				if ( trainer )
				{
					auto tg = g->gm()->gnome( trainer );
					if ( tg )
					{
						if ( tg->getPos() == ws->inputPos() )
						{
							if ( tg->getSkillLevel( "Melee" ) > getSkillLevel( "Melee" ) )
							{
								skillGain = 6;
							}
							if ( tg->getSkillLevel( "Dodge" ) > getSkillLevel( "Dodge" ) )
							{
								skillGain = 6;
							}
						}
					}
				}
				gainSkill( "Melee", skillGain );
				gainSkill( "Dodge", skillGain );
				m_log.append( "Gained " + QString::number( skillGain ) + "xp in " + S::s( "$SkillName_Melee" ) );
				m_log.append( "Gained " + QString::number( skillGain ) + "xp in " + S::s( "$SkillName_Dodge" ) );
			}
		}
		m_thoughtBubble = "";

		return BT_RESULT::SUCCESS;
	}
	else if ( m_trainCounter < 0 )
	{
		// starting training;
		m_trainCounter = Global::util->ticksPerMinute * 15;
	}
	m_trainCounter  = qMax( 0, m_trainCounter - 1 );
	m_thoughtBubble = "Combat";
	return BT_RESULT::RUNNING;
}

/// @brief BT action: moves the trainer gnome to the assigned workshop's trainer input position.
/// @param halt Unused.
/// @return SUCCESS if the assigned workshop exists; FAILURE otherwise.
BT_RESULT Gnome::actionFindTrainerPosition( bool halt )
{
	if ( m_assignedWorkshop )
	{
		auto ws = g->wsm()->workshop( m_assignedWorkshop );
		if ( ws )
		{
			setCurrentTarget( ws->inputPos() );
			return BT_RESULT::SUCCESS;
		}
	}
	m_assignedWorkshop = 0;
	return BT_RESULT::FAILURE;
}

/// @brief BT action: runs a 15-minute trainer session, awarding 1 Melee and 1 Dodge skill point.
/// @param halt Unused.
/// @return RUNNING while supervising; SUCCESS when the session ends.
BT_RESULT Gnome::actionSuperviseTraining( bool halt )
{
	if ( m_trainCounter == 0 )
	{
		m_log.append( "Finished trainer session." );
		m_trainCounter = -1;
		auto ws        = g->wsm()->workshop( m_assignedWorkshop );
		if ( ws )
		{
			QString type = ws->type();
			if ( type == "MeleeTraining" )
			{
				gainSkill( "Melee", 1 );
				gainSkill( "Dodge", 1 );
			}
		}
		m_thoughtBubble = "";

		return BT_RESULT::SUCCESS;
	}
	else if ( m_trainCounter < 0 )
	{
		// starting training;
		m_trainCounter = Global::util->ticksPerMinute * 15;
	}
	m_trainCounter  = qMax( 0, m_trainCounter - 1 );
	m_thoughtBubble = "Combat";
	return BT_RESULT::RUNNING;
}

/// @brief BT action: selects a combat target for this gnome.
///        First re-validates the current target (path reachability, alive status).
///        Then searches squad-mates' targets, then the priority list, then nearby enemies.
///        Falls back to defending civilian gnomes against attackers.
/// @param halt Unused.
/// @return SUCCESS if a target was assigned; FAILURE otherwise.
BT_RESULT Gnome::actionGetTarget( bool halt )
{
	if ( Global::debugMode )
		log( "actionGetTarget" );

	//!TODO Decide when else to invalidate current target, e.g. because there might a target of higher priority closer to us

	// Unset current attack target if invalidated
	if ( m_currentAttackTarget )
	{
		const Creature* creature = g->cm()->creature( m_currentAttackTarget );
		if ( !creature || creature->isDead() || !g->cm()->hasPathTo( m_position, creature->id() ) )
		{
			m_currentAttackTarget = 0;
			m_thoughtBubble       = "";
		}
		else
		{
			// Update target position
			setCurrentTarget( creature->getPos() );
		}
	}

	if ( !m_currentAttackTarget )
	{
		const Creature* bestCandidate = nullptr;
		unsigned int bestDistance     = std::numeric_limits<unsigned int>::max();

		const Squad* squad = g->mil()->getSquadForGnome( m_id );
		if ( squad )
		{
			// Search for targets already attacked by squad mates first
			for ( const auto& gnomeID : squad->gnomes )
			{
				const Gnome* gnome = g->gm()->gnome( gnomeID );
				if ( gnome && gnome->m_currentAttackTarget )
				{
					const Creature* creature = g->cm()->creature( gnome->m_currentAttackTarget );
					if ( creature && g->cm()->hasPathTo( m_position, creature->id() ) )
					{
						const unsigned int dist = m_position.distSquare( creature->getPos() );
						bestDistance            = dist;
						bestCandidate           = creature;
						// Any legal match is a good hit
						break;
					}
				}
			}
			if ( !bestCandidate )
			{
				// Next search for hunting targets, anything we could reach
				for ( const auto& prio : squad->priorities )
				{
					if ( prio.attitude != MilAttitude::FLEE )
					{
						const auto& targetSet = g->cm()->animalsByType( prio.type );
						//!TODO Sort huntTargets into buckets by regionm so hasPathTo will never fail
						for ( const auto& targetID : targetSet )
						{
							const Creature* creature = g->cm()->creature( targetID );
							if ( creature && !creature->isDead() && g->cm()->hasPathTo( m_position, targetID ) )
							{
								const unsigned int dist = m_position.distSquare( creature->getPos() );
								if ( prio.attitude == MilAttitude::ATTACK && ( dist >= 100 || !g->cm()->hasLineOfSightTo( m_position, targetID ) ) )
								{
									// Skip attack targets which we don't see ourselves
									continue;
								}
								if ( prio.attitude == MilAttitude::DEFEND && ( dist >= 4 || !g->cm()->hasLineOfSightTo( m_position, targetID ) ) )
								{
									// Skip targets which are not adjacent
									continue;
								}
								if ( dist < bestDistance )
								{
									bestDistance  = dist;
									bestCandidate = creature;
								}
							}
						}
						// Got the closest target
						if ( bestCandidate )
						{
							break;
						}
					}
				}
			}
		}

		if ( !bestCandidate )
		{
			while ( m_aggroList.size() )
			{
				unsigned int targetID = m_aggroList.first().id;
				Creature* creature    = g->cm()->creature( targetID );

				//!TODO Check if creature isn't in "flee" category
				if ( creature && !creature->isDead() && g->cm()->hasPathTo( m_position, targetID ) )
				{
					const auto dist = m_position.distSquare( creature->getPos() );
					bestDistance    = dist;
					bestCandidate   = creature;
					break;
				}
				else
				{
					m_aggroList.pop_front();
				}
			}
		}

		if ( bestCandidate )
		{
			m_currentAttackTarget = bestCandidate->id();
			setCurrentTarget( bestCandidate->getPos() );
			m_thoughtBubble = "Combat";
			return BT_RESULT::SUCCESS;
		}
	}
	return m_currentAttackTarget ? BT_RESULT::SUCCESS : BT_RESULT::FAILURE;
}

/// @brief BT action: advances the off-map mission simulation each day.
///        Handles three phases — TRAVEL (distance check), ACTION (emissary/raid/spy/sabotage),
///        and RETURN (result processing, kingdom discovery).
/// @param halt Unused.
/// @return RUNNING while the mission is in progress; SUCCESS on return; FAILURE if no mission.
BT_RESULT Gnome::actionDoMission( bool halt )
{
	if ( m_mission )
	{
		if ( GameState::tick >= m_nextCheckTick )
		{
			auto mission = g->em()->getMission( m_mission );

			if ( !mission )
			{
				// how is that possible?
				// need to return the gnome to this world
				g->w()->insertCreatureAtPosition( m_position, m_id );
				m_goneOffMap    = false;
				m_mission       = 0;
				m_isOnMission   = false;
				m_nextCheckTick = 0;
				return BT_RESULT::FAILURE;
			}

			switch ( mission->step )
			{
				case MissionStep::TRAVEL:
				{
					int hours = ( GameState::tick - mission->startTick ) / ( Global::util->ticksPerMinute * Global::util->minutesPerHour );

					switch ( mission->type )
					{
						case MissionType::EXPLORE:
						{
							for ( auto& kingdom : g->nm()->kingdoms() )
							{
								if ( !kingdom.discovered && !kingdom.discoverMission )
								{
									if ( kingdom.distance <= hours )
									{
										kingdom.discoverMission = true;
										mission->result.insert( "Success", true );
										mission->result.insert( "DiscoveredKingdom", kingdom.id );
										//qDebug() << "Found kingdom " << kingdom.name << kingdom.id;
										mission->nextCheckTick = GameState::tick + ( GameState::tick - mission->startTick );
										m_nextCheckTick        = mission->nextCheckTick;
										mission->step          = MissionStep::RETURN;
										return BT_RESULT::RUNNING;
									}
								}
							}
							if ( hours > 300 )
							{
								mission->result.insert( "Success", false );
								//qDebug() << "Found nothing, returning";
								mission->nextCheckTick = GameState::tick + ( GameState::tick - mission->startTick );
								m_nextCheckTick        = mission->nextCheckTick;
								mission->step          = MissionStep::RETURN;
								return BT_RESULT::RUNNING;
							}
						}
						break;
						case MissionType::EMISSARY:
						case MissionType::RAID:
						case MissionType::SPY:
						case MissionType::SABOTAGE:
						{
							if ( mission->distance <= hours )
							{
								mission->step = MissionStep::ACTION;
							}
						}
						break;
					}

					mission->nextCheckTick = GameState::tick + Global::util->ticksPerDay;
					m_nextCheckTick        = mission->nextCheckTick;
					return BT_RESULT::RUNNING;
				}
				break;
				case MissionStep::ACTION:
					switch ( mission->type )
					{
						case MissionType::EXPLORE:
							//not used
							break;
						case MissionType::EMISSARY:
							g->nm()->emissary( mission );
							break;
						case MissionType::RAID:
							g->nm()->raid( mission );
							break;
						case MissionType::SPY:
							g->nm()->spy( mission );
							break;
						case MissionType::SABOTAGE:
							g->nm()->sabotage( mission );
							break;
					}
					mission->nextCheckTick = GameState::tick + mission->distance * Global::util->ticksPerMinute * Global::util->minutesPerHour;
					m_nextCheckTick        = mission->nextCheckTick;
					mission->step          = MissionStep::RETURN;
					return BT_RESULT::RUNNING;
					break;
				case MissionStep::RETURN:
					switch ( mission->type )
					{
						case MissionType::EXPLORE:
							if ( mission->result.contains( "DiscoveredKingdom" ) )
							{
								unsigned int kingdomID = mission->result.value( "DiscoveredKingdom" ).toUInt();
								g->nm()->discoverKingdom( kingdomID );
							}
							break;
						case MissionType::EMISSARY:
							break;
						case MissionType::RAID:
							break;
						case MissionType::SPY:
							break;
						case MissionType::SABOTAGE:
							break;
					}

					return BT_RESULT::SUCCESS;
					break;
			}
		}
		return BT_RESULT::RUNNING;
	}
	//qDebug() << "Mission failure";
	return BT_RESULT::FAILURE;
}

/// @brief BT action: removes the gnome from the world map to begin an off-map mission.
///        Records the departure position and transitions the mission to TRAVEL step.
/// @param halt Unused (single-tick action).
/// @return SUCCESS if the mission was found; FAILURE otherwise.
BT_RESULT Gnome::actionLeaveForMission( bool halt )
{
	Q_UNUSED( halt ); // action takes only one tick, halt has no effect

	g->w()->removeCreatureFromPosition( m_position, m_id );
	m_goneOffMap = true;

	Mission* mission = g->em()->getMission( m_mission );
	if ( mission )
	{
		mission->leavePos = m_position;
		mission->step     = MissionStep::TRAVEL;
		m_nextCheckTick   = mission->nextCheckTick;
		return BT_RESULT::SUCCESS;
	}
	else
	{
		m_mission = 0;
		return BT_RESULT::FAILURE;
	}
}

/// @brief BT action: re-inserts the gnome at its departure position after a completed mission.
///        Records total mission time, clears the mission reference, and marks the gnome as no longer off-map.
/// @param halt Unused (single-tick action).
/// @return SUCCESS.
BT_RESULT Gnome::actionReturnFromMission( bool halt )
{
	Q_UNUSED( halt ); // action takes only one tick, halt has no effect

	Mission* mission = g->em()->getMission( m_mission );
	if ( mission )
	{
		m_position = mission->leavePos;

		mission->step = MissionStep::RETURNED;
		mission->result.insert( "TotalTime", ( GameState::tick - mission->startTick ) / ( Global::util->ticksPerMinute * Global::util->minutesPerHour ) );

		//qDebug() << "Returning from mission at " << m_position.toString();
	}
	else
	{
		// this should never be reached
		qDebug() << "Returning from mission but mission doesn't exist anymore";
	}

	g->w()->insertCreatureAtPosition( m_position, m_id );
	m_goneOffMap = false;

	m_mission       = 0;
	m_isOnMission   = false;
	m_nextCheckTick = 0;

	return BT_RESULT::SUCCESS;
}

/// @brief Task function: equips the claimed uniform item into the appropriate equipment slot.
///        Picks up the item, resolves the slot from the blackboard, updates combat hand stats for held items,
///        and refreshes the sprite.
/// @return True if the item was at the gnome's position and was equipped; false otherwise.
bool Gnome::equipItem()
{
	if ( Global::debugMode )
		log( "actionEquipUniform" );

	auto itemID = m_btBlackBoard.value( "ClaimedUniformItem" ).toUInt();
	m_btBlackBoard.remove( "ClaimedUniformItem" );

	QStringList conc;

	if ( m_position == g->inv()->getItemPos( itemID ) )
	{
		g->inv()->pickUpItem( itemID, m_id );
		g->inv()->setInJob( itemID, 0 );

		QString slot = m_btBlackBoard.value( "ClaimedUniformItemSlot" ).toString();
		m_btBlackBoard.remove( "ClaimedUniformItemSlot" );

		QString itemSID          = g->inv()->itemSID( itemID );
		unsigned int materialUID = g->inv()->materialUID( itemID );
		QString materialSID      = g->inv()->materialSID( itemID );

		auto part = Global::creaturePartLookUp.value( slot );

		auto& itemSlot = m_equipment.getSlot( part );
		if ( itemSlot.itemID )
		{
			qWarning() << "Trying to equip into occupied slot!";
			g->inv()->putDownItem( itemSlot.itemID, m_position );
			g->inv()->setInJob( itemSlot.itemID, 0 );
		}
		itemSlot.itemID     = itemID;
		itemSlot.item       = itemSID;
		itemSlot.materialID = materialUID;
		itemSlot.material   = materialSID;
		if ( part == CP_LEFT_HAND_HELD )
		{
			equipHand( itemID, "Left" );
			itemSlot.allMats = g->inv()->allMats( itemID );
		}
		else if ( part == CP_RIGHT_HAND_HELD )
		{
			equipHand( itemID, "Right" );
			itemSlot.allMats = g->inv()->allMats( itemID );
		}

		updateSprite();

		m_uniformWorn = true;

		m_renderParamsChanged = true;
		return true;
	}

	return false;
}