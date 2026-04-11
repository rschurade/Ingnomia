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
/** @file canwork.cpp
 *  @brief Mixin base class for creatures that can claim and execute jobs.
 */
#include "canwork.h"

#include "../base/config.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/position.h"
#include "../base/util.h"
#include "../game/game.h"
#include "../game/automaton.h"
#include "../game/creaturemanager.h"
#include "../game/farmingmanager.h"
#include "../game/gnomemanager.h"
#include "../game/inventory.h"
#include "../game/jobmanager.h"
#include "../game/mechanismmanager.h"
#include "../game/militarymanager.h"
#include "../game/plant.h"
#include "../game/room.h"
#include "../game/roommanager.h"
#include "../game/stockpile.h"
#include "../game/stockpilemanager.h"
#include "../game/workshop.h"
#include "../game/workshopmanager.h"
#include "../game/world.h"

//#include "../gui/strings.h"

#include <exprtk.hpp>
#include "../gfx/sprite.h"
#include "../gfx/spritefactory.h"

#include <QDebug>

typedef exprtk::symbol_table<double> symbol_table_t;
typedef exprtk::expression<double> expression_t;
typedef exprtk::parser<double> parser_t;
typedef exprtk::parser_error::type _error_t;

/// @brief Constructs a new CanWork creature with no active job.
/// @param pos     Initial world position.
/// @param name    Display name.
/// @param gender  Biological gender.
/// @param species Species string ID.
/// @param game    Owning game instance.
CanWork::CanWork( Position& pos, QString name, Gender gender, QString species, Game* game ) :
	Creature( pos, name, gender, species, game )
{
}

/// @brief Deserialising constructor — restores creature and in-progress job state from a saved map.
/// @param in   Serialised variant map.
/// @param game Owning game instance.
CanWork::CanWork( QVariantMap in, Game* game ) :
	Creature( in, game )
{
	m_jobID = in.value( "JobID" ).toUInt();

	if ( m_jobID )
	{
		m_job = g->jm()->getJob( m_jobID );

		m_currentTask = in.value( "CurrentTask" ).toMap();

		PriorityQueue<Position, int> sq;
		QVariantList sql = in.value( "WorkPositionQueue" ).toList();
		for ( auto sqli : sql )
		{
			sq.put( Position( sqli.toMap().value( "second" ).toString() ), sqli.toMap().value( "first" ).toInt() );
		}
		m_workPositionQueue = sq;

		m_workPosition = Position( in.value( "WorkPosition" ) );

		m_totalDurationTicks = in.value( "TotalDurationTicks" ).toFloat();
		m_repeatJob          = in.value( "RepeatJob" ).toBool();

		m_assignedWorkshop = in.value( "AssignedWorkshop" ).toUInt();

		if ( in.contains( "EffectedTiles" ) )
		{
			m_effectedTiles = Global::util->variantList2Position( in.value( "EffectedTiles" ).toList() );
		}

		m_animal          = in.value( "Animal" ).toUInt();
		m_itemToPickUp    = in.value( "ItemToPickUp" ).toUInt();
		m_startedEating   = in.value( "StartedEating" ).toBool();
		m_startedDrinking = in.value( "StartedDrinking" ).toBool();

		m_taskFinishTick = in.value( "TaskFinishTick" ).value<quint64>();

		QVariantList tl;
		for ( auto task : m_taskList )
		{
			tl.append( task );
		}
		in.value( "TaskList", tl );

		in.value( "TrainCounter", m_trainCounter );
		in.value( "TrainingGround", m_trainingGround );
	}
}

/// @brief Serialises creature and current job state into @p out.
/// @param out Map to receive the serialised data.
void CanWork::serialize( QVariantMap& out )
{
	Creature::serialize( out );

	out.insert( "JobID", m_jobID );
	if ( m_jobID )
	{
		out.insert( "WorkPosition", m_workPosition.toString() );
		out.insert( "CurrentTask", m_currentTask );

		// Copy original queue for iteration
		auto vp = m_workPositionQueue;
		QVariantList sql;
		while ( !vp.empty() )
		{
			const auto& entry = vp.top();
			QVariantMap qEntry;
			qEntry.insert( "first", entry.first );
			qEntry.insert( "second", entry.second.toString() );
			sql.append( qEntry );
			vp.pop();
		}
		out.insert( "WorkPositionQueue", sql );

		out.insert( "AssignedWorkshop", m_assignedWorkshop );

		out.insert( "TotalDurationTicks", m_totalDurationTicks );
		out.insert( "TaskFinishTick", m_taskFinishTick );
		out.insert( "RepeatJob", m_repeatJob );

		if ( m_effectedTiles.size() )
		{
			QVariantList et;
			for ( auto pos : m_effectedTiles )
			{
				et.append( pos.toString() );
			}
			out.insert( "EffectedTiles", et );
		}

		out.insert( "Animal", m_animal );
		out.insert( "ItemToPickUp", m_itemToPickUp );
		out.insert( "StartedEating", m_startedEating );
		out.insert( "StartedDrinking", m_startedDrinking );

		QVariantList tl;
		for ( auto task : m_taskList )
		{
			tl.append( task );
		}
		out.insert( "TaskList", tl );

		out.insert( "TrainCounter", m_trainCounter );
		out.insert( "TrainingGround", m_trainingGround );
	}
}

/// @brief Returns whether the skill with the given ID is enabled for this creature.
/// @param id Skill string ID.
/// @return True if the skill is active; false if disabled or unknown.
bool CanWork::getSkillActive( QString id )
{
	if ( m_skillActive.contains( id ) )
	{
		return m_skillActive.value( id ).toBool();
	}
	return false;
}

/// @brief Enables or disables a skill and updates the skill-priority list accordingly.
///        Disabling the skill required by the current job aborts that job.
///        Combat and Defense skills are toggled but never added to the priority list.
/// @param id     Skill string ID.
/// @param active True to enable, false to disable.
void CanWork::setSkillActive( QString id, bool active )
{
	m_skillActive[id] = active;

	if ( m_job )
	{
		QString skillID = m_job->requiredSkill();

		if ( ( id == skillID ) && !active )
		{
			m_job->setAborted( true );
		}
	}

	QString group = DB::select( "SkillGroup", "Skills", id ).toString();
	if ( group == "Combat" || group == "Defense" )
	{
		return;
	}

	if ( active )
	{
		if ( !m_skillPriorities.contains( id ) )
		{
			m_skillPriorities.push_back( id );
		}
	}
	else
	{
		for ( int i = 0; i < m_skillPriorities.size(); ++i )
		{
			if ( m_skillPriorities[i] == id )
			{
				m_skillPriorities.removeAt( i );
				break;
			}
		}
	}
}

/// @brief Disables all skills (convenience wrapper for setAllSkillsActive(false)).
void CanWork::clearAllSkills()
{
	setAllSkillsActive( false );
}

/// @brief Enables or disables every skill at once.  Disabling aborts the current job.
/// @param active True to activate all skills, false to deactivate.
void CanWork::setAllSkillsActive( bool active )
{
	if ( !active )
	{
		m_skillPriorities.clear();
		if ( m_job )
		{
			m_job->setAborted( true );
		}
	}
	for ( auto& p : m_skillActive )
	{
		p = active;
	}
}

/// @brief Resets all job-related member variables to their default state.
///        Called as part of cleanUpJob() or suspendJob() after a job finishes or is released.
void CanWork::resetJobVars()
{
	m_jobID         = 0;
	m_currentAction = "idle";
	//m_currentPath.clear();

	m_itemToPickUp = 0;
	m_claimedItems.clear();
	m_carriedItems.clear();

	m_totalDurationTicks = 0;
	m_taskFinishTick     = 0;

	m_currentTask.clear();
	m_taskList.clear();

	m_animal = 0;
	m_effectedTiles.clear();

	m_facingAfterMove = -1;

	m_repeatJob = false;

	m_btBlackBoard.remove( "JobType" );
}

/// @brief Releases the current job and cleans up all associated state.
///        If @p finished is true the job is completed (skill/tech gain applied, job removed);
///        otherwise it is returned to the job manager for re-assignment.
///        Carried and claimed items are put down, equipped tools are dropped, and magic
///        effect sprites are cleared.
/// @param finished True if the job was completed successfully, false to give it back.
void CanWork::cleanUpJob( bool finished )
{
	if ( Global::debugMode )
		log( "cleanUpJob" );
	bool spriteNeedsUpdate = false;
	if ( m_jobID && m_job )
	{
		// do stuff we need variables from the job object first
		// pointer may become invalid after giveBackJob or finishJob

		Animal* a = g->cm()->animal( m_job->animal() );
		if ( a )
		{
			a->setFollowID( 0 );
			a->setImmobile( false );
			m_animal = 0;
		}

		if ( finished )
		{
			QVariant sgv = DB::select( "SkillGain", "Jobs", m_job->type() );
			gainSkill( sgv, m_job );
			QVariant tgv = DB::select( "TechGain", "Jobs", m_job->type() );
			gainTech( tgv, m_job );

			g->jm()->finishJob( m_jobID );
		}
		else
		{
			// input pos is different from work pos for workshop jobs
			// this is for other job
			// so input pos gets cleared for reassignment when the gnome is expelled from a tile
			if ( m_job->posItemInput() == m_job->workPos() )
			{
				m_job->setPosItemInput( Position( 0, 0, 0 ) );
			}
			m_job->setAborted( false );
			g->jm()->giveBackJob( m_jobID );
		}
	}

	bool isHaulToSite = m_job && m_job->type() == "HaulToSite";

	if ( !claimedItems().empty() )
	{
		for ( auto itemID : claimedItems() )
		{
			if ( !isHaulToSite )
			{
				g->inv()->setInJob( itemID, 0 );
			}
			if ( g->inv()->isHeldBy( itemID ) == m_id )
			{
				g->inv()->putDownItem( itemID, m_position );
			}
		}
	}
	if( m_btBlackBoard.contains( "ClaimedUniformItem" ) )
	{
		auto itemID = m_btBlackBoard.value( "ClaimedUniformItem" ).toUInt();
		g->inv()->setInJob( itemID, 0 );
		m_btBlackBoard.remove( "ClaimedUniformItem" );
		m_btBlackBoard.remove( "ClaimedUniformItemSlot" );
	}

	for ( auto itemID : m_carriedItems )
	{
		if ( g->inv()->isHeldBy( itemID ) == m_id )
		{
			g->inv()->putDownItem( itemID, m_position );
		}
		if ( !isHaulToSite )
		{
			g->inv()->setInJob( itemID, 0 );
		}
	}
	m_carriedItems.clear();

	spriteNeedsUpdate = dropEquippedItem();

	// clean up magic
	for ( auto pos : m_effectedTiles )
	{
		g->w()->clearJobSprite( pos, false );
	}

	m_currentPath.clear();
	resetJobVars();
	m_currentAction = "idle";
	m_jobID         = 0;
	m_job.reset();
	m_jobChanged    = true;

	if ( spriteNeedsUpdate )
	{
		updateSprite();
	}
}

/// @brief Suspends the current job without aborting or finishing it.
///        The job's progress is preserved and it transitions to SUSPENDED phase
///        so another worker can resume it.  Items at the work site remain in place.
void CanWork::suspendJob()
{
	if ( m_job )
	{
		m_job->removeWorker( m_id );
		if ( m_job->activeWorkers().empty() && m_job->phase() == JobPhase::IN_PROGRESS )
		{
			m_job->setPhase( JobPhase::SUSPENDED );
		}
		// Don't give back or abort the job — it stays in the job list
		// Items at the work site stay there
		// Progress is preserved on the Job
	}
	m_currentPath.clear();
	resetJobVars();
	m_currentAction = "idle";
	m_jobID         = 0;
	m_job.reset();
	m_jobChanged = true;
	updateSprite();
}

/// @brief Converts a task duration value to game ticks.
///        If the value is the special token "$Craft", the production time is looked up
///        from the Crafts table using the job's craft ID.
/// @param value Duration value (integer minutes or "$Craft" token).
/// @param job   The job whose craft ID is used when resolving "$Craft".
/// @return Duration in game ticks.
int CanWork::getDurationTicks( QVariant value, QSharedPointer<Job> job )
{
	if ( value.toString() == "$Craft" )
	{
		value = DB::select( "ProductionTime", "Crafts", job->craftID() );
	}

	int ticks = value.toInt() * Global::util->ticksPerMinute;
	//if( Global::debugMode ) qDebug() << m_name << "getDurationTicks: " << job->type() << ticks;
	return ticks;
}

/// @brief Awards skill experience on job completion.
///        If @p skillGain is empty, increments the job's required skill by 1.
///        If it is "$Craft", the gain definition is loaded from the Crafts_SkillGain table.
///        Otherwise the map is evaluated via parseGain().
/// @param skillGain Gain descriptor (empty, "$Craft", or a QVariantMap).
/// @param job       Completed job providing context (required skill, craft ID).
void CanWork::gainSkill( QVariant skillGain, QSharedPointer<Job> job )
{
	if ( skillGain.toString().isEmpty() )
	{
		QString skillID = job->requiredSkill();
		float current   = m_skills.value( skillID ).toFloat() + 1;
		m_skills.insert( skillID, current );

		if ( skillID == "Hauling" )
		{
			updateMoveSpeed();

			//if( Global::debugMode ) qDebug() << name() << "now has move speed" << m_moveSpeed << "(" << m_moveDelay << ")";
		}

		return;
	}

	if ( skillGain.toString() == "$Craft" )
	{
		skillGain = DB::selectRow( "Crafts_SkillGain", job->craftID() );
	}

	QVariantMap sgm = skillGain.toMap();
	QString skillID = sgm.value( "SkillID" ).toString();
	float gain      = 0;

	if ( skillID.isEmpty() )
	{
		skillID = job->requiredSkill();
	}

	gain = parseGain( sgm );

	if ( gain > 0 )
	{
		float current = m_skills.value( skillID ).toFloat();
		m_skills.insert( skillID, current + gain );
		//if( Global::debugMode )	qDebug() << name() << " gain skill: " << skillID << gain;
	}
}

/// @brief Directly adds @p gain to the named skill value.
/// @param skillID Skill string ID.
/// @param gain    Amount to add.
void CanWork::gainSkill( QString skillID, int gain )
{
	int current = m_skills.value( skillID ).toInt();
	m_skills.insert( skillID, current + gain );
	//if( Global::debugMode )	qDebug() << name() << " gain skill: " << skillID << gain;
}

/// @brief Awards technology research points on job completion.
///        If @p techGain is "$Craft", the gain definition is loaded from Crafts_TechGain.
///        The evaluated gain is added to the global tech level stored in GameState::techs.
/// @param techGain Gain descriptor ("$Craft" or a QVariantMap with TechID and gain formula).
/// @param job      Completed job providing the craft ID when needed.
void CanWork::gainTech( QVariant techGain, QSharedPointer<Job> job )
{
	if ( techGain.toString() == "$Craft" )
	{
		techGain = DB::selectRow( "Crafts_TechGain", job->craftID() );
	}
	QVariantMap tgm = techGain.toMap();
	QString techID  = tgm.value( "TechID" ).toString();
	float gain      = 0;

	if ( techID.isEmpty() )
	{
		return;
	}
	else
	{
		gain = parseGain( tgm );
	}
	if ( gain > 0 )
	{
		//if( Global::debugMode ) qDebug() << "Gain tech: " << techID << gain;
		float current = GameState::techs.value( techID ).toFloat();
		GameState::techs.insert( techID, current + gain );
	}
}

/// @brief Evaluates a gain descriptor map and returns the resulting numeric value.
///        If the map has a "Value" key it is parsed directly; otherwise the "Formula"
///        string is compiled and evaluated via exprtk using the "Args" map as variables.
/// @param gainMap Map containing either a "Value" entry or a "Formula"+"Args" pair.
/// @return Computed gain value.
double CanWork::parseGain( QVariantMap gainMap )
{
	if ( gainMap.contains( "Value" ) )
	{
		return parseValue( gainMap.value( "Value" ) );
	}

	double gain         = 1;
	std::string formula = gainMap.value( "Formula" ).toString().toStdString();
	symbol_table_t symbol_table;
	symbol_table.add_constants();
	QVariantMap args = gainMap.value( "Args" ).toMap();

	QList<std::pair<std::string, double>> values;

	for ( auto arg : args.toStdMap() )
	{
		std::pair<std::string, double> vpair( arg.first.toStdString(), parseValue( arg.second ) );
		values.push_back( vpair );
		symbol_table.add_variable( values.last().first, values.last().second );
	}

	expression_t expression;
	expression.register_symbol_table( symbol_table );

	// Instantiate parser and compile the expression
	parser_t parser;

	if ( !parser.compile( formula, expression ) )
	{
		printf( "Error: %s\tExpression: %s\n", parser.error().c_str(), formula.c_str() );

		for ( std::size_t i = 0; i < parser.error_count(); ++i )
		{
			const _error_t error = parser.get_error( i );
			printf( "Error: %02d Position: %02d Type: [%s] Msg: %s Expr: %s\n", static_cast<int>( i ), static_cast<int>( error.token.position ), exprtk::parser_error::to_str( error.mode ).c_str(), error.diagnostic.c_str(), formula.c_str() );
		}
	}
	else
	{
		// Evaluate and assign result
		gain = expression.value();
	}
	return gain;
}

/// @brief Resolves a single value token to a double.
///        "$Tech*" tokens look up global tech levels; "$Attrib*" look up creature attributes;
///        other "$*" strings look up creature skills; plain values are converted numerically.
/// @param v Value token (QString or numeric).
/// @return Resolved double value.
double CanWork::parseValue( QVariant v )
{
	QString var = v.toString();
	if ( var.startsWith( "$Tech" ) )
	{
		var.remove( 0, 1 );
		return GameState::techs.value( var ).toDouble();
	}
	else if ( var.startsWith( "$Attrib" ) )
	{
		var.remove( 0, 7 );
		return m_attributes.value( var ).toDouble();
	}
	else if ( var.startsWith( "$" ) )
	{
		var.remove( 0, 1 );
		return m_skills.value( var ).toDouble();
	}
	else
	{
		return v.toDouble();
	}
}

/// @brief Drops the item held in the right hand, unless it belongs to the creature's uniform.
///        Also releases any item that was claimed as a tool but not yet picked up.
/// @return True if an item was dropped or a uniform item was retained; false if no item was held.
bool CanWork::dropEquippedItem()
{
	// release a claimed tool if this is run before the gnome picked it up
	unsigned int claimedItem = m_btBlackBoard.value( "ClaimedTool" ).toUInt();
	if ( claimedItem )
	{
		g->inv()->setInJob( claimedItem, 0 );
	}
	m_btBlackBoard.remove( "ClaimedTool" );

	unsigned int equippedItem = m_equipment.rightHandHeld.itemID;
	if ( equippedItem )
	{
		// check if item belongs to uniform
		if ( m_roleID )
		{
			auto uniform = g->mil()->uniform( m_roleID );
			if ( uniform )
			{
				QString rhi = uniform->parts["RightHandHeld"].item;
				QString rhm = uniform->parts["RightHandHeld"].material;
				if ( rhi == m_equipment.rightHandHeld.item && ( rhm == "any" || rhm == m_equipment.rightHandHeld.material ) )
				{
					return true;
				}
			}
		}

		g->inv()->putDownItem( equippedItem, m_position );
		g->inv()->gravity( m_position );
		g->inv()->setInJob( equippedItem, 0 );
		m_equipment.rightHandHeld.itemID = 0;
		m_equipment.rightHandHeld.item.clear();
		m_equipment.rightHandHeld.materialID = 0;
		m_equipment.rightHandHeld.material.clear();

		equipHand( 0, "Right" );

		return true;
	}

	return false;
}

/// @brief Updates combat stats and light for the given hand slot to reflect a newly equipped item.
///        Passing @p item = 0 reverts the hand to unarmed stats and removes any carried light source.
/// @param item UID of the item being equipped, or 0 to unequip.
/// @param side "Left" or "Right" hand slot.
void CanWork::equipHand( unsigned int item, QString side )
{
	if ( item )
	{
		if ( side == "Left" )
		{
			m_leftHandAttackSkill = getSkillLevel( "Melee" );
			m_leftHandAttackValue = g->inv()->attackValue( item );
			m_leftHandArmed       = true;
			if ( g->inv()->isWeapon( item ) )
			{
				m_leftHandHasWeapon = true;
			}
			else
			{
				m_leftHandHasWeapon = false;
			}

			m_lightIntensity = DB::select( "LightIntensity", "Items", g->inv()->itemSID( item ) ).toInt();
			if ( m_lightIntensity )
			{
				g->w()->addLight( m_id, m_position, m_lightIntensity );
			}
		}
		else
		{
			m_rightHandAttackSkill = getSkillLevel( "Melee" );
			m_rightHandAttackValue = g->inv()->attackValue( item );
			m_rightHandArmed       = true;
		}
	}
	else
	{
		if ( side == "Left" )
		{
			m_leftHandAttackSkill = getSkillLevel( "Unarmed" );
			m_leftHandAttackValue = m_leftHandAttackSkill;
			m_leftHandArmed       = false;
			m_leftHandHasWeapon   = false;

			m_lightIntensity = 0;
			g->w()->removeLight( m_id );
		}
		else
		{
			m_rightHandAttackSkill = getSkillLevel( "Unarmed" );
			m_rightHandAttackValue = m_rightHandAttackSkill;
			m_rightHandArmed       = false;
		}
	}
	updateSprite();
}

/// @brief Task: removes a wall tile at the job position (plus optional offset) and spawns raw material items.
/// @return Always true.
bool CanWork::mineWall()
{
	// remove the wall
	Position offset;
	if ( m_currentTask.contains( "Offset" ) )
	{
		offset = Position( m_currentTask.value( "Offset" ).toString() );
	}
	Position pos( m_job->pos() + offset );

	auto mats = g->w()->mineWall( pos, m_workPosition );

	Global::util->createRawMaterialItem( pos, mats.first );
	Global::util->createRawMaterialItem( pos, mats.second );

	return true;
}

/// @brief Task: removes a floor tile and spawns a RawSoil or RawStone item depending on material type.
/// @return Always true.
bool CanWork::mineFloor()
{
	Position offset;
	if ( m_currentTask.contains( "Offset" ) )
	{
		offset = Position( m_currentTask.value( "Offset" ).toString() );
	}
	Position pos( m_job->pos() + offset );
	auto mat = g->w()->removeFloor( pos, m_position );

	QString materialSID = "None";
	QString type        = "None";
	materialSID         = DBH::materialSID( mat );
	type                = DB::select( "Type", "Materials", materialSID ).toString();

	if ( type == "Soil" )
	{
		g->inv()->createItem( m_job->workPos(), "RawSoil", materialSID );
	}
	else if ( type == "Stone" )
	{
		g->inv()->createItem( m_job->workPos(), "RawStone", materialSID );
	}
	return true;
}

/// @brief Task: digs a hole downward — removes the floor above and the wall below,
///        spawns raw material items, and creates ramps around the new opening.
/// @return Always true.
bool CanWork::digHole()
{
	// remove the wall
	Position offset( 0, 0, -1 );
	Position pos( m_job->pos() );

	/*
	if( g->w()->checkTrapGnomeFloor( pos, m_position ) )
	{
		return false;
	}
	*/

	g->w()->removeFloor( pos, m_position );
	auto mats = g->w()->mineWall( pos + offset, m_position );

	Global::util->createRawMaterialItem( m_position, mats.first );
	Global::util->createRawMaterialItem( m_position, mats.second );

	Position rp( m_job->pos() + offset );
	g->w()->createRamp( rp.x, rp.y, rp.z );
	g->w()->createRamp( rp.x + 1, rp.y, rp.z );
	g->w()->createRamp( rp.x - 1, rp.y, rp.z );
	g->w()->createRamp( rp.x, rp.y + 1, rp.z );
	g->w()->createRamp( rp.x, rp.y - 1, rp.z );

	g->w()->updateRampAtPos( rp.northOf() );
	g->w()->updateRampAtPos( rp.southOf() );
	g->w()->updateRampAtPos( rp.eastOf() );
	g->w()->updateRampAtPos( rp.westOf() );

	g->w()->addToUpdateList( pos );
	g->w()->addToUpdateList( rp );

	g->w()->addToUpdateList( pos.northOf() );
	g->w()->addToUpdateList( pos.southOf() );
	g->w()->addToUpdateList( pos.eastOf() );
	g->w()->addToUpdateList( pos.westOf() );

	g->w()->addToUpdateList( rp.northOf() );
	g->w()->addToUpdateList( rp.southOf() );
	g->w()->addToUpdateList( rp.eastOf() );
	g->w()->addToUpdateList( rp.westOf() );

	return true;
}

/// @brief Task: mines a wall and automatically generates ExplorativeMine jobs for neighbouring
///        tiles that contain the same embedded ore material.
/// @return Always true.
bool CanWork::explorativeMineWall()
{
	// remove the wall
	Position offset;
	if ( m_currentTask.contains( "Offset" ) )
	{
		offset = Position( m_currentTask.value( "Offset" ).toString() );
	}
	Position pos( m_job->pos() + offset );

	auto mats = g->w()->mineWall( pos, m_workPosition );

	Global::util->createRawMaterialItem( pos, mats.first );
	Global::util->createRawMaterialItem( pos, mats.second );

	if ( mats.second )
	{
		//check neighbor tiles for ore and create jobs
		const Position candidates[] = {
			pos.northOf(),
			pos.southOf(),
			pos.eastOf(),
			pos.westOf()
		};
		for ( const auto& candidate : candidates )
		{
			if ( g->w()->getTile(candidate).embeddedMaterial == mats.second )
			{
				g->jm()->addJob( "ExplorativeMine", candidate, 0 );
			}
		}
	}
	return true;
}

/// @brief Task: removes a ramp tile and spawns a raw material item from its material.
/// @return Always true.
bool CanWork::removeRamp()
{
	// remove the wall
	Position offset;
	if ( m_currentTask.contains( "Offset" ) )
	{
		offset = Position( m_currentTask.value( "Offset" ).toString() );
	}
	Position pos( m_job->pos() + offset );

	unsigned short materialInt = g->w()->removeRamp( pos, m_workPosition );

	Global::util->createRawMaterialItem( pos, materialInt );

	return true;
}

/// @brief Task: removes a wall tile without spawning any material items.
/// @return Always true.
bool CanWork::removeWall()
{
	Position offset;

	if ( m_currentTask.contains( "Offset" ) )
	{
		offset = Position( m_currentTask.value( "Offset" ).toString() );
	}
	Position pos( m_job->pos() + offset );

	g->w()->removeWall( pos, m_workPosition );

	return true;
}

/// @brief Task: removes a floor tile without spawning any material items.
/// @return Always true.
bool CanWork::removeFloor()
{
	Position offset;
	if ( m_currentTask.contains( "Offset" ) )
	{
		offset = Position( m_currentTask.value( "Offset" ).toString() );
	}
	Position pos( m_job->pos() + offset );
	g->w()->removeFloor( pos, m_position );

	return true;
}

/// @brief Task: paints the intermediate construction sprite for the current progress percentage.
///        Called each tick during a phased build to show the construction animation.
/// @return Always true.
bool CanWork::constructAnimate()
{
	// item to build wasn't set, so this construction job was created otherwise, for instance dig stairs down
	if ( m_job->item() == "" )
	{
		m_job->setItem( m_currentTask.value( "ConstructionID" ).toString() );
	}
	QVariantMap con = DB::selectRow( "Constructions", m_job->item() );

	if ( con.contains( "IntermediateSprites" ) )
	{
		QVariantList isl = con.value( "IntermediateSprites" ).toList();
		QString spriteID;
		QString type;
		QString offset;
		int percent;
		for ( auto isv : isl )
		{
			auto ism = isv.toMap();
			spriteID = ism.value( "SpriteID" ).toString();
			type     = ism.value( "Type" ).toString();
			offset   = ism.value( "Offset" ).toString();
			percent  = ism.value( "Percent" ).toInt();

			if ( ( 100. * ( m_taskFinishTick - GameState::tick ) ) / m_totalDurationTicks > percent )
			{
				break;
			}
		}
		Position pos( m_job->pos() + offset );

		QStringList materials;

		if ( !claimedItems().empty() )
		{
			for ( auto item : claimedItems() )
			{
				materials.push_back( g->inv()->materialSID( item ) );
			}
		}
		// paint the intermediate sprite and return
		if ( !spriteID.isEmpty() )
		{
			bool isFloor = false;
			if ( type == "Floor" )
			{
				isFloor = true;
			}
			Sprite* s = g->sf()->createSprite( spriteID, { materials.first() } );
			g->w()->setJobSprite( pos, s->uID, m_job->rotation(), isFloor, m_jobID, false );
			return true;
		}
	}
	return true;
}

/// @brief Task: removes wall and floor tiles to make room for stairs, then constructs
///        SoilStairs or StoneStairs based on the underlying material.
/// @return False if no material item could be created; true otherwise.
bool CanWork::constructDugStairs()
{
	Position offset( m_currentTask.value( "Offset" ) );

	m_currentTask.insert( "Offset", ( offset + Position( 0, 0, 1 ) ).toString() );
	removeWall();
	removeFloor();
	m_currentTask.insert( "Offset", offset.toString() );

	Position pos( m_job->pos() + offset );
	auto wallMat = g->w()->wallMaterial( pos );
	removeWall();

	auto item = Global::util->createRawMaterialItem( pos, wallMat );
	if ( item )
	{
		addClaimedItem( item, m_job->id() );
	}

	auto cil = claimedItems();
	if ( cil.empty() )
	{
		return false;
	}
	unsigned itemID     = cil.first();
	QString materialSID = g->inv()->materialSID( itemID );
	QString type        = DB::select( "Type", "Materials", materialSID ).toString();

	bool result = false;
	if ( type == "Soil" || type == "Sand" || type == "Clay" )
	{
		m_job->setItem( "SoilStairs" );
		//result = g->w()->construct( "SoilStairs", m_job->pos(), m_job->rotation(), claimedItems(), m_position );
	}
	else if ( type == "Stone" )
	{
		m_job->setItem( "StoneStairs" );
		//result = g->w()->construct( "StoneStairs", m_job->pos(), m_job->rotation(), claimedItems(), m_position );
	}

	return construct();
}

/// @brief Task: clears a wall/floor pair and constructs a SoilRamp or StoneRamp
///        based on the material of the removed wall.
/// @return Result of construct().
bool CanWork::constructDugRamp()
{
	m_currentTask.insert( "Offset", "0 0 0" );
	removeWall();  // offset 0 0 0
	removeFloor(); // offset 0 0 0
	m_currentTask.insert( "Offset", "0 0 -1" );

	Position pos( m_job->pos() + Position( 0, 0, -1 ) );
	auto wallMat = g->w()->wallMaterial( pos );
	removeWall(); // offset 0 0 -1

	auto item = Global::util->createRawMaterialItem( pos, wallMat );
	if ( item )
	{
		addClaimedItem( item, m_job->id() );
	}

	auto cil = claimedItems();
	if ( cil.empty() )
	{
		return false;
	}
	unsigned itemID = cil.first();

	QString materialSID = g->inv()->materialSID( itemID );
	QString type        = DB::select( "Type", "Materials", materialSID ).toString();

	if ( type == "Soil" || type == "Sand" || type == "Clay" )
	{
		m_job->setItem( "SoilRamp" );
		//result = g->w()->construct( "SoilStairs", m_job->pos(), m_job->rotation(), claimedItems(), m_position );
	}
	else if ( type == "Stone" )
	{
		m_job->setItem( "StoneRamp" );
		//result = g->w()->construct( "StoneStairs", m_job->pos(), m_job->rotation(), claimedItems(), m_position );
	}
	return construct();
}

/// @brief Task: finalises a construction by calling the appropriate World build method
///        (constructWorkshop, constructItem, or construct) and destroying all claimed items.
/// @return True if construction succeeded; false if no claimed items are available.
bool CanWork::construct()
{
	// Get items: from job's claimed list (new phased path) or gnome's claimed items (old path).
	// In the new path the haul phase already delivered the items to the build site and the
	// job owns them — the gnome never holds them, so the destroy loop below skips pickUpItem.
	QList<unsigned int> jobItems;
	bool itemsHeldByGnome = false;
	if ( !m_job->claimedItemIDs().isEmpty() )
	{
		jobItems = m_job->claimedItemIDs();
	}
	else
	{
		jobItems = claimedItems();
		itemsHeldByGnome = true;
	}

	if ( jobItems.empty() )
	{
		return false;
	}

	if ( m_job->item() == "" )
	{
		m_job->setItem( m_currentTask.value( "ConstructionID" ).toString() );
	}

	Position offset( m_currentTask.value( "Offset" ).toString() );
	Position pos( m_job->pos() + offset );

	bool result = false;

	if ( m_job->type() == "BuildWorkshop" )
	{
		result = g->w()->constructWorkshop( m_job->item(), pos, m_job->rotation(), jobItems, m_position );
	}
	else if ( m_job->type() == "BuildItem" )
	{
		result = g->w()->constructItem( m_job->item(), pos, m_job->rotation(), jobItems, m_position );
	}
	else
	{
		result = g->w()->construct( m_job->item(), pos, m_job->rotation(), jobItems, m_position );
	}

	if ( result )
	{
		for ( auto itemUID : jobItems )
		{
			if ( itemsHeldByGnome )
			{
				// Legacy path: the gnome was carrying the items as inventory. pickUpItem
				// transfers ownership to the gnome, drops them out of any stockpile, and
				// refreshes the tile sprite — all needed before destruction.
				g->inv()->setInJob( itemUID, 0 );
				g->inv()->pickUpItem( itemUID, m_id );
			}
			g->inv()->destroyObject( itemUID );
		}
		m_claimedItems.clear();
		return true;
	}
	return false;
}

/// @brief Task: spawns item(s) defined in the current task's "Items" list at the job position.
///        Items with a "Condition" of "MaterialType" are only created when the condition matches.
/// @return True if an item was created; false if no matching condition was found.
bool CanWork::createItem()
{
	Position offset;
	if ( m_currentTask.contains( "Offset" ) )
	{
		offset = Position( m_currentTask.value( "Offset" ).toString() );
	}
	Position pos( m_job->pos() + offset );
	if ( m_currentTask.contains( "Items" ) )
	{
		QVariantList items = m_currentTask.value( "Items" ).toList();
		for ( auto item : items )
		{
			QVariantMap im      = item.toMap();
			QString materialSID = "None";
			QString type        = "None";
			if ( im.contains( "Material" ) )
			{
				materialSID = im.value( "Material" ).toString();
			}
			QString condition = im.value( "Condition" ).toString();
			if ( condition == "MaterialType" )
			{
				if ( type == im.value( "ConditionValue" ).toString() )
				{
					g->inv()->createItem( pos, im.value( "ItemID" ).toString(), materialSID );
					return true;
				}
			}
		}
	}
	qDebug() << "create item failed";
	return false;
}

/// @brief Task: harvests a plant or beehive at the job position.
///        If the plant is fully consumed it is removed; if it is still harvestable,
///        the task is repeated.  Beehive honey is created separately.
/// @return Always true.
bool CanWork::harvest()
{
	// get Tree
	Position offset;
	if ( m_currentTask.contains( "Offset" ) )
	{
		offset = Position( m_currentTask.value( "Offset" ).toString() );
	}
	Position pos( m_job->pos() + offset );
	QMap<unsigned int, Plant>& plants = g->w()->plants();
	if ( plants.contains( pos.toInt() ) )
	{
		Plant& plant = plants[pos.toInt()];
		if ( plant.harvest( m_position ) ) // plant.harvest() returns true if the plant is to be destroyed after harvest
		{
			// remove plant;
			if ( plant.isPlant() )
			{
				g->w()->clearTileFlag( pos, TileFlag::TF_TILLED );
				Sprite* s = g->sf()->createSprite( "RoughFloor", { DBH::materialSID( g->w()->floorMaterial( pos ) ) } );
				g->w()->setFloorSprite( pos, s->uID );
				g->w()->setWallSprite( pos, 0 );
				g->w()->removeGrass( pos );
			}
			plants.remove( pos.toInt() );

			return true;
		}
		if ( plant.harvestable() )
		{
			m_repeatJob      = true;
			m_taskFinishTick = GameState::tick + m_currentTask.value( "Duration" ).toInt() * Global::util->ticksPerMinute;
			return true;
		}
	}
	m_repeatJob = false;

	if ( g->fm()->isBeehive( pos ) )
	{

		if ( g->fm()->harvestBeehive( pos ) )
		{
			g->inv()->createItem( pos, "Honey", "Bee" );
		}
	}

	return true;
}

/// @brief Task: cuts grass at max growth into a Hay item and resets vegetation to 30%.
/// @return True if the tile had max grass and hay was created; false otherwise.
bool CanWork::harvestHay()
{
	// get Tree
	Position offset;
	if ( m_currentTask.contains( "Offset" ) )
	{
		offset = Position( m_currentTask.value( "Offset" ).toString() );
	}
	Position pos( m_job->pos() + offset );

	if ( g->w()->hasMaxGrass( pos ) )
	{
		g->inv()->createItem( pos, "Hay", "Grass" );
		g->w()->setVegetationLevel( pos, 30 );

		return true;
	}

	return false;
}

/// @brief Task: plants a tree sapling at the job position using the first claimed item.
/// @return True if a claimed item was available; false otherwise.
bool CanWork::plantTree()
{
	if ( !claimedItems().empty() )
	{
		g->w()->plantTree( m_job->pos(), m_job->item() );
		g->inv()->pickUpItem( claimedItems().first(), m_id );
		destroyClaimedItems();
		return true;
	}
	return false;
}

/// @brief Task: plants a seed/crop item at the job position and consumes the claimed item.
/// @return True if a claimed item was available; false otherwise.
bool CanWork::plant()
{
	if ( !claimedItems().empty() )
	{
		g->w()->plant( m_job->pos(), claimedItems().first() );
		g->inv()->pickUpItem( claimedItems().first(), m_id );
		destroyClaimedItems();
		return true;
	}
	return false;
}

/// @brief Task: removes the plant at the job position from the world.
/// @return Always true.
bool CanWork::removePlant()
{
	g->w()->removePlant( m_job->pos() );
	return true;
}

/// @brief Task: tills the soil at the job position — sets the TF_TILLED flag, removes grass,
///        and replaces the floor sprite with a "TilledSoil" sprite.
/// @return Always true.
bool CanWork::till()
{
	Position offset;
	if ( m_currentTask.contains( "Offset" ) )
	{
		offset = Position( m_currentTask.value( "Offset" ).toString() );
	}
	Position pos( m_job->pos() + offset );
	g->w()->setTileFlag( pos, TileFlag::TF_TILLED );
	g->w()->removeGrass( pos );
	Sprite* s = g->sf()->createSprite( "TilledSoil", { DBH::materialSID( g->w()->floorMaterial( pos ) ) } );
	g->w()->setFloorSprite( pos, s->uID );
	return true;
}

/// @brief Task: produces the crafted output item(s) based on the job's material and conversion rules.
///        Handles direct material crafts, RandomMetal outputs, dye conversion, leather, hair-color
///        application ($GnomeHair), and Automaton assembly.  Quality is skill-based with random variance.
///        Destroys all claimed input items on success.
/// @return False if required claimed items are missing; true otherwise.
bool CanWork::craft()
{
	float skillLevel = getSkillLevel( m_job->requiredSkill() );

	int qSize = DB::numRows( "Quality" );

	int qIndex = skillLevel / 20. * qSize;

	srand( std::chrono::system_clock::now().time_since_epoch().count() );

	int qRand = rand() % 100;
	if ( qRand < 20 )
	{
		qIndex = qMax( 0, qIndex - 1 );
	}
	if ( qRand > 96 )
	{
		qIndex = qMin( qSize - 1, qIndex + 1 );
	}

	QString resultMaterial = m_job->material();

	for ( int i = 0; i < m_job->amount(); ++i )
	{
		if ( !resultMaterial.isEmpty() )
		{
			if ( resultMaterial == "RandomMetal" )
			{
				srand( std::chrono::system_clock::now().time_since_epoch().count() );

				if ( rand() % 100 > 50 )
				{
					if ( claimedItems().size() )
					{
						auto item = claimedItems().first();

						auto sourceMaterial = g->inv()->materialSID( item );
						auto material       = Global::util->randomMetalSliver( sourceMaterial );
						unsigned int itemID = g->inv()->createItem( m_job->posItemOutput(), m_job->item(), material );
						g->inv()->setMadeBy( itemID, id() );
					}
				}
			}
			else
			{
				unsigned int itemID = g->inv()->createItem( m_job->posItemOutput(), m_job->item(), resultMaterial );
				g->inv()->setMadeBy( itemID, id() );
				g->inv()->setQuality( itemID, qIndex );
			}
		}
		else if ( m_job->conversionMaterial().isEmpty() )
		{
			if ( claimedItems().isEmpty() )
			{
				return false;
			}

			unsigned int itemID = g->inv()->createItem( m_job->posItemOutput(), m_job->item(), claimedItems() );
			g->inv()->setMadeBy( itemID, id() );
			g->inv()->setQuality( itemID, qIndex );
			if ( m_job->item() == "Automaton" )
			{
				Automaton* a = new Automaton( m_job->posItemOutput(), itemID, g );
				//a->setSpriteID( g->sf()->setAutomatonSprite( a->id(), g->inv()->spriteID( itemID ) ) );
				//a->updateSprite();

				g->gm()->addAutomaton( a );
				g->inv()->pickUpItem( itemID, a->id() );
			}
		}
		else
		{
			if ( m_job->conversionMaterial() == "$Dye" )
			{
				unsigned int dyeItem    = 0;
				unsigned int sourceItem = 0;
				for ( auto item : claimedItems() )
				{
					if ( g->inv()->itemSID( item ) == "Dye" )
					{
						dyeItem = item;
					}
					else
					{
						sourceItem = item;
					}
				}
				QString sourceMaterial = g->inv()->materialSID( sourceItem );
				QString dyeMaterial    = g->inv()->materialSID( dyeItem );

				QString targetMaterial = Global::util->addDyeMaterial( sourceMaterial, dyeMaterial );

				unsigned int itemID = g->inv()->createItem( m_job->posItemOutput(), m_job->item(), targetMaterial );
				g->inv()->setMadeBy( itemID, id() );
				g->inv()->setQuality( itemID, g->inv()->quality( sourceItem ) );
			}
			else if ( m_job->conversionMaterial() == "$GnomeHair" )
			{
				if ( claimedItems().isEmpty() )
				{
					//abortJob( "error claimed items empty" );
					return false;
				}
				unsigned int sourceItem = claimedItems().first();
				QString sourceMaterial  = g->inv()->materialSID( sourceItem );
				QString dyeColor        = DB::select( "Color", "Materials", sourceMaterial ).toString();
				auto keys               = DB::ids( "Materials", "Type", "Dye" );
				int id                  = 0;
				for ( auto key : keys )
				{
					QString keyColor = DB::select( "Color", "Materials", key ).toString();
					if ( keyColor == dyeColor )
					{
						m_equipment.hairColor = id;
						updateSprite();
						break;
					}
					++id;
				}
			}
			else if ( m_job->conversionMaterial() == "$Leather" )
			{
				unsigned int sourceItem = claimedItems().first();
				QString sourceMaterial  = g->inv()->materialSID( sourceItem );

				unsigned int itemID = g->inv()->createItem( m_job->posItemOutput(), m_job->item(), sourceMaterial + "Leather" );
				g->inv()->setMadeBy( itemID, id() );
				g->inv()->setQuality( itemID, qIndex );
			}
			else
			{
				unsigned int itemID = g->inv()->createItem( m_job->posItemOutput(), m_job->item(), m_job->conversionMaterial() );
				g->inv()->setMadeBy( itemID, id() );
				g->inv()->setQuality( itemID, qIndex );
			}
		}
	}

	destroyClaimedItems();
	return true;
}

/// @brief Task: fells the tree at the job position and removes it from the world.
/// @return True if a tree was found and felled; false otherwise.
bool CanWork::fellTree()
{
	// get Tree
	Position offset;
	if ( m_currentTask.contains( "Offset" ) )
	{
		offset = Position( m_currentTask.value( "Offset" ).toString() );
	}
	Position pos( m_job->pos() + offset );
	QMap<unsigned int, Plant>& plants = g->w()->plants();
	if ( plants.contains( pos.toInt() ) )
	{
		Plant& plant = plants[pos.toInt()];
		if ( plant.fell() )
		{
			// remove plant;
			g->w()->removePlant( pos );
			return true;
		}
	}
	return false;
}

/// @brief Task: deconstructs the tile at the job position, recovering materials.
/// @return Always true.
bool CanWork::deconstruct()
{
	Position offset;
	if ( m_currentTask.contains( "Offset" ) )
	{
		offset = Position( m_currentTask.value( "Offset" ).toString() );
	}
	Position pos( m_job->pos() + offset );

	g->w()->deconstruct( pos, m_position, false );
	return true;
}

/// @brief Task: processes all claimed fish items into Meat and FishBone outputs.
/// @return Always true.
bool CanWork::butcherFish()
{
	for ( auto itemUID : claimedItems() )
	{
		QString materialSID = g->inv()->materialSID( itemUID );

		g->inv()->createItem( m_job->posItemOutput(), "Meat", materialSID );
		g->inv()->createItem( m_job->posItemOutput(), "FishBone", materialSID );
	}
	destroyClaimedItems();
	return true;
}

/// @brief Task: processes all claimed corpse items into Meat and Bone outputs.
/// @return Always true.
bool CanWork::butcherCorpse()
{
	for ( auto itemUID : claimedItems() )
	{
		QString materialSID = g->inv()->materialSID( itemUID );

		g->inv()->createItem( m_job->posItemOutput(), "Meat", materialSID );
		g->inv()->createItem( m_job->posItemOutput(), "Bone", materialSID );
	}
	destroyClaimedItems();
	return true;
}

/// @brief Task: creates one GreenFish item at the job output position.
/// @return Always true.
bool CanWork::fish()
{
	unsigned int itemID = g->inv()->createItem( m_job->posItemOutput(), "Fish", "GreenFish" );
	g->inv()->setMadeBy( itemID, id() );
	return true;
}

/// @brief Task: calculates the spell's area of effect and marks affected tiles with sparkle sprites.
///        Radius is derived from the spell's DB entry, optionally scaled by the caster's skill level.
/// @return Always true.
bool CanWork::prepareSpell()
{
	QString jobID = m_job->type();

	QStringList jobParts = jobID.split( "_" );
	QString spellID      = jobParts.last();

	QString radiusString = DB::select( "Radius", "Spells", spellID ).toString();
	bool ok              = false;
	int radius           = radiusString.toInt( &ok );
	if ( !ok )
	{
		if ( radiusString == "HalfSkill" )
		{
			radius = qMax( 1, getSkillLevel( m_job->requiredSkill() ) / 2 );
		}
		else if ( radiusString == "Skill" )
		{
			radius = qMin( 10, qMax( 1, getSkillLevel( m_job->requiredSkill() ) ) );
		}
	}
	int radius2 = radius * radius;
	m_effectedTiles.clear();

	Position center        = m_job->pos();
	unsigned int spriteUID = g->sf()->createSprite( "Sparkles", { "None" } )->uID + 2048;

	int xMin = qMax( 1, center.x - radius );
	int yMin = qMax( 1, center.y - radius );
	int xMax = qMin( Global::dimX - 2, center.x + radius );
	int yMax = qMin( Global::dimX - 2, center.y + radius );
	for ( int x = xMin; x < xMax; ++x )
	{
		for ( int y = yMin; y < yMax; ++y )
		{
			if ( center.distSquare( x, y, center.z ) <= radius2 )
			{
				m_effectedTiles.append( Position( x, y, center.z ).toString() );
				g->w()->setJobSprite( Position( x, y, center.z ), spriteUID, 0, false, m_job->id(), true );
			}
		}
	}
	return true;
}

/// @brief Task: placeholder for mid-cast spell logic (currently a no-op).
/// @return Always true.
bool CanWork::castSpell()
{
	return true;
}

/// @brief Task: per-tick animation step while a spell is being cast (currently a no-op).
/// @return Always true.
bool CanWork::castSpellAnimate()
{
	//
	return true;
}

/// @brief Task: applies the spell's effects to all previously marked tiles and clears the sparkle sprites.
///        Supported effects: Reveal (uncovers undiscovered tiles), PlantGrowth (speeds up plant growth).
/// @return Always true.
bool CanWork::finishSpell()
{
	QString jobID        = m_job->type();
	QStringList jobParts = jobID.split( "_" );
	QString spellID      = jobParts.last();

	auto spellMap        = DB::selectRow( "Spells", spellID );
	QVariantList reqs    = spellMap.value( "EffectRequirements" ).toList();
	QVariantList effects = spellMap.value( "Effects" ).toList();

	for ( auto pos : m_effectedTiles )
	{
		bool effected = true;
		for ( auto vReq : reqs )
		{
			QString sReq = vReq.toString();
			if ( sReq == "Plant" )
			{
				effected &= g->w()->plants().contains( pos.toInt() );
			}
			else if ( sReq == "RoughWall" )
			{
				effected &= ( g->w()->wallType( pos ) == WT_ROUGH );
			}
			else if ( sReq == "EmbeddedMaterial" )
			{
				effected &= ( g->w()->embeddedMaterial( pos ) != 0 );
			}
		}

		if ( effected )
		{
			for ( auto vEffect : effects )
			{
				QString sEffect = vEffect.toString();
				if ( sEffect == "Reveal" )
				{
					g->w()->clearTileFlag( pos, TileFlag::TF_UNDISCOVERED );
					g->w()->addToUpdateList( pos );
				}
				else if ( sEffect == "PlantGrowth" )
				{
					g->w()->plants()[pos.toInt()].speedUpGrowth( Global::util->ticksPerDay * getSkillLevel( m_job->requiredSkill() ) );
				}
			}
		}

		g->w()->clearJobSprite( pos, false );
	}
	return true;
}

/// @brief Task: toggles the active state of the mechanism referenced by the current job.
/// @return True if the job and mechanism are valid; false otherwise.
bool CanWork::switchMechanism()
{
	if ( m_job )
	{
		g->mcm()->toggleActive( m_job->mechanism() );
		return true;
	}

	return false;
}

/// @brief Task: toggles the invert flag of the mechanism referenced by the current job.
/// @return True if the job and mechanism are valid; false otherwise.
bool CanWork::invertMechanism()
{
	if ( m_job )
	{
		g->mcm()->toggleInvert( m_job->mechanism() );
		return true;
	}

	return false;
}

/// @brief Task: refuels an automaton or mechanism using the burn value of the claimed fuel item.
///        Destroys the consumed fuel item on success.
/// @return True if the job is valid and refueling succeeded; false otherwise.
bool CanWork::refuel()
{
	if ( m_job )
	{
		int burnValue = DB::select( "BurnValue", "Items", m_job->requiredItems().first().itemSID ).toInt();

		if ( m_job->automaton() )
		{
			auto a = g->gm()->automaton( m_job->automaton() );
			if ( a )
			{
				a->fillUp( burnValue );
			}
		}
		else if ( m_job->mechanism() )
		{
			g->mcm()->refuel( m_job->mechanism(), burnValue );
		}

		destroyClaimedItems();

		return true;
	}

	return false;
}

/// @brief Task: installs each claimed item as a core into the target automaton.
/// @return True if the job is valid; false otherwise.
bool CanWork::install()
{
	if ( m_job )
	{

		if ( m_job->automaton() )
		{
			auto a = g->gm()->automaton( m_job->automaton() );
			if ( a )
			{
				for ( auto item : claimedItems() )
				{
					a->installCore( item );
				}
				m_claimedItems.clear();
			}
		}
		return true;
	}
	return false;
}

/// @brief Task: removes the currently installed core from the target automaton (passes itemID 0).
/// @return True if the job is valid; false otherwise.
bool CanWork::uninstall()
{
	if ( m_job )
	{
		if ( m_job->automaton() )
		{
			auto a = g->gm()->automaton( m_job->automaton() );
			if ( a )
			{
				a->installCore( 0 );
			}
		}
		return true;
	}
	return false;
}

/// @brief Task: delivers the single claimed food item to the pasture trough at the job position.
///        If more than one item was claimed, releases them all and returns false.
/// @return True if exactly one item was delivered; false if pasture not found or item count wrong.
bool CanWork::fillTrough()
{
	if ( m_job )
	{
		auto pasture = g->fm()->getPastureAtPos( m_job->pos() );
		if ( pasture && m_claimedItems.size() == 1 )
		{
			pasture->addFood( m_claimedItems.first() );

			destroyClaimedItems();
			return true;
		}
		else
		{

			for ( auto item : claimedItems() )
			{
				g->inv()->setInJob( item, 0 );
			}
			m_claimedItems.clear();
			return false;
		}
	}

	return false;
}

/// @brief Task: activates the alarm for the room at the job position, setting the global alarm room ID.
///        Only succeeds when the alarm is in state 1 (armed but not yet triggered).
/// @return True if the alarm was triggered; false otherwise.
bool CanWork::soundAlarm()
{
	if ( m_job )
	{
		if ( GameState::alarm == 1 )
		{
			auto room = g->rm()->getRoomAtPos( m_job->pos() );
			if ( room )
			{
				GameState::alarmRoomID = room->id();
				GameState::alarm       = 2;
				return true;
			}
		}
	}
	return false;
}
