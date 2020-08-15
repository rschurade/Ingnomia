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
#include "canwork.h"

#include "../base/config.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/position.h"
#include "../base/util.h"
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

#include "../../3rdparty/exprtk/exprtk.hpp"
#include "../gfx/sprite.h"
#include "../gfx/spritefactory.h"

#include <QDebug>

typedef exprtk::symbol_table<double> symbol_table_t;
typedef exprtk::expression<double> expression_t;
typedef exprtk::parser<double> parser_t;
typedef exprtk::parser_error::type _error_t;

CanWork::CanWork( Position& pos, QString name, Gender gender, QString species ) :
	Creature( pos, name, gender, species )
{
}

CanWork::CanWork( QVariantMap in ) :
	Creature( in )
{
	m_jobID = in.value( "JobID" ).toUInt();

	if ( m_jobID )
	{
		m_job = Global::jm().getJob( m_jobID );

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
			m_effectedTiles = Util::variantList2Position( in.value( "EffectedTiles" ).toList() );
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

void CanWork::serialize( QVariantMap& out )
{
	Creature::serialize( out );

	out.insert( "JobID", m_jobID );
	if ( m_jobID )
	{
		out.insert( "WorkPosition", m_workPosition.toString() );
		out.insert( "CurrentTask", m_currentTask );

		auto vp = m_workPositionQueue.getElements();
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

bool CanWork::getSkillActive( QString id )
{
	if ( m_skillActive.contains( id ) )
	{
		return m_skillActive.value( id ).toBool();
	}
	return false;
}

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

void CanWork::clearAllSkills()
{
	setAllSkillsActive( false );
}

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
	for ( auto&& p : m_skillActive )
	{
		p = active;
	}
}

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

void CanWork::cleanUpJob( bool finished )
{
	if ( Global::debugMode )
		log( "cleanUpJob" );
	bool spriteNeedsUpdate = false;
	if ( m_jobID && m_job )
	{
		// do stuff we need variables from the job object first
		// pointer may become invalid after giveBackJob or finishJob

		Animal* a = Global::cm().animal( m_job->animal() );
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

			Global::jm().finishJob( m_jobID );
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
			Global::jm().giveBackJob( m_jobID );
		}
	}

	Inventory& inv = Global::inv();
	if ( !claimedItems().empty() )
	{
		for ( auto itemID : claimedItems() )
		{
			inv.setInJob( itemID, 0 );
			if ( inv.isPickedUp( itemID ) && !inv.isConstructedOrEquipped( itemID ) )
			{
				inv.putDownItem( itemID, m_position );
			}
		}
	}
	if( m_btBlackBoard.contains( "ClaimedUniformItem" ) )
	{
		auto itemID = m_btBlackBoard.value( "ClaimedUniformItem" ).toUInt();
		inv.setInJob( itemID, 0 );
		m_btBlackBoard.remove( "ClaimedUniformItem" );
		m_btBlackBoard.remove( "ClaimedUniformItemSlot" );
	}

	for ( auto itemID : m_carriedItems )
	{
		Inventory& inv = Global::inv();
		if ( inv.isPickedUp( itemID ) && !inv.isConstructedOrEquipped( itemID ) )
		{
			inv.putDownItem( itemID, m_position );
		}
		inv.setInJob( itemID, 0 );
	}
	m_carriedItems.clear();

	spriteNeedsUpdate = dropEquippedItem();

	// clean up magic
	World& world = Global::w();

	for ( auto pos : m_effectedTiles )
	{
		Global::w().clearJobSprite( pos, false );
	}

	m_currentPath.clear();
	resetJobVars();
	m_currentAction = "idle";
	m_jobID         = 0;
	m_job           = nullptr;
	m_jobChanged    = true;

	if ( spriteNeedsUpdate )
	{
		updateSprite();
	}
}

int CanWork::getDurationTicks( QVariant value, Job* job )
{
	if ( value.toString() == "$Craft" )
	{
		value = DB::select( "ProductionTime", "Crafts", job->craftID() );
	}

	int ticks = value.toInt() * Util::ticksPerMinute;
	//if( Global::debugMode ) qDebug() << m_name << "getDurationTicks: " << job->type() << ticks;
	return ticks;
}

void CanWork::gainSkill( QVariant skillGain, Job* job )
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

void CanWork::gainSkill( QString skillID, int gain )
{
	int current = m_skills.value( skillID ).toInt();
	m_skills.insert( skillID, current + gain );
	//if( Global::debugMode )	qDebug() << name() << " gain skill: " << skillID << gain;
}

void CanWork::gainTech( QVariant techGain, Job* job )
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

bool CanWork::dropEquippedItem()
{
	Inventory& inv = Global::inv();
	// release a claimed tool if this is run before the gnome picked it up
	unsigned int claimedItem = m_btBlackBoard.value( "ClaimedTool" ).toUInt();
	if ( claimedItem )
	{
		inv.setInJob( claimedItem, 0 );
	}
	m_btBlackBoard.remove( "ClaimedTool" );

	unsigned int equippedItem = m_equipment.rightHandHeld.itemID;
	if ( equippedItem )
	{
		// check if item belongs to uniform
		if ( m_roleID )
		{
			auto uniform = Global::mil().uniform( m_roleID );
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

		inv.putDownItem( equippedItem, m_position );
		inv.gravity( m_position );
		inv.setInJob( equippedItem, 0 );
		inv.setConstructedOrEquipped( equippedItem, false );
		m_equipment.rightHandHeld.itemID = 0;
		m_equipment.rightHandHeld.item.clear();
		m_equipment.rightHandHeld.materialID = 0;
		m_equipment.rightHandHeld.material.clear();

		equipHand( 0, "Right" );

		return true;
	}

	return false;
}

void CanWork::equipHand( unsigned int item, QString side )
{
	if ( item )
	{
		if ( side == "Left" )
		{
			m_leftHandAttackSkill = getSkillLevel( "Melee" );
			m_leftHandAttackValue = Global::inv().attackValue( item );
			m_leftHandArmed       = true;
			if ( Global::inv().isWeapon( item ) )
			{
				m_leftHandHasWeapon = true;
			}
			else
			{
				m_leftHandHasWeapon = false;
			}

			m_lightIntensity = DB::select( "LightIntensity", "Items", Global::inv().itemSID( item ) ).toInt();
			if ( m_lightIntensity )
			{
				Global::w().addLight( m_id, m_position, m_lightIntensity );
			}
		}
		else
		{
			m_rightHandAttackSkill = getSkillLevel( "Melee" );
			m_rightHandAttackValue = Global::inv().attackValue( item );
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
			Global::w().removeLight( m_id );
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

bool CanWork::mineWall()
{
	World& world = Global::w();
	// remove the wall
	Position offset;
	if ( m_currentTask.contains( "Offset" ) )
	{
		offset = Position( m_currentTask.value( "Offset" ).toString() );
	}
	Position pos( m_job->pos() + offset );

	auto mats = world.mineWall( pos, m_workPosition );

	Util::createRawMaterialItem( pos, mats.first );
	Util::createRawMaterialItem( pos, mats.second );

	return true;
}

bool CanWork::mineFloor()
{
	Position offset;
	if ( m_currentTask.contains( "Offset" ) )
	{
		offset = Position( m_currentTask.value( "Offset" ).toString() );
	}
	Position pos( m_job->pos() + offset );
	auto mat = Global::w().removeFloor( pos, m_position );

	QString materialSID = "None";
	QString type        = "None";
	materialSID         = DBH::materialSID( mat );
	type                = DB::select( "Type", "Materials", materialSID ).toString();

	if ( type == "Soil" )
	{
		Global::inv().createItem( m_job->workPos(), "RawSoil", materialSID );
	}
	else if ( type == "Stone" )
	{
		Global::inv().createItem( m_job->workPos(), "RawStone", materialSID );
	}
	return true;
}

bool CanWork::digHole()
{
	World& world = Global::w();
	// remove the wall
	Position offset( 0, 0, -1 );
	Position pos( m_job->pos() );

	/*
	if( Global::w().checkTrapGnomeFloor( pos, m_position ) )
	{
		return false;
	}
	*/

	world.removeFloor( pos, m_position );
	auto mats = world.mineWall( pos + offset, m_position );

	Util::createRawMaterialItem( m_position, mats.first );
	Util::createRawMaterialItem( m_position, mats.second );

	Position rp( m_job->pos() + offset );
	Global::w().createRamp( rp.x, rp.y, rp.z );
	Global::w().createRamp( rp.x + 1, rp.y, rp.z );
	Global::w().createRamp( rp.x - 1, rp.y, rp.z );
	Global::w().createRamp( rp.x, rp.y + 1, rp.z );
	Global::w().createRamp( rp.x, rp.y - 1, rp.z );

	Global::w().updateRampAtPos( rp.northOf() );
	Global::w().updateRampAtPos( rp.southOf() );
	Global::w().updateRampAtPos( rp.eastOf() );
	Global::w().updateRampAtPos( rp.westOf() );

	Global::w().addToUpdateList( pos );
	Global::w().addToUpdateList( rp );

	Global::w().addToUpdateList( pos.northOf() );
	Global::w().addToUpdateList( pos.southOf() );
	Global::w().addToUpdateList( pos.eastOf() );
	Global::w().addToUpdateList( pos.westOf() );

	Global::w().addToUpdateList( rp.northOf() );
	Global::w().addToUpdateList( rp.southOf() );
	Global::w().addToUpdateList( rp.eastOf() );
	Global::w().addToUpdateList( rp.westOf() );

	return true;
}

bool CanWork::explorativeMineWall()
{
	World& world = Global::w();
	// remove the wall
	Position offset;
	if ( m_currentTask.contains( "Offset" ) )
	{
		offset = Position( m_currentTask.value( "Offset" ).toString() );
	}
	Position pos( m_job->pos() + offset );

	auto mats = world.mineWall( pos, m_workPosition );

	Util::createRawMaterialItem( pos, mats.first );
	Util::createRawMaterialItem( pos, mats.second );

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
			if ( world.getTile(candidate).embeddedMaterial == mats.second )
			{
				Global::jm().addJob( "ExplorativeMine", candidate, 0 );
			}
		}
	}
	return true;
}

bool CanWork::removeRamp()
{
	// remove the wall
	Position offset;
	if ( m_currentTask.contains( "Offset" ) )
	{
		offset = Position( m_currentTask.value( "Offset" ).toString() );
	}
	Position pos( m_job->pos() + offset );

	unsigned short materialInt = Global::w().removeRamp( pos, m_workPosition );

	Util::createRawMaterialItem( pos, materialInt );

	return true;
}

bool CanWork::removeWall()
{
	Position offset;

	if ( m_currentTask.contains( "Offset" ) )
	{
		offset = Position( m_currentTask.value( "Offset" ).toString() );
	}
	Position pos( m_job->pos() + offset );

	Global::w().removeWall( pos, m_workPosition );

	return true;
}

bool CanWork::removeFloor()
{
	Position offset;
	if ( m_currentTask.contains( "Offset" ) )
	{
		offset = Position( m_currentTask.value( "Offset" ).toString() );
	}
	Position pos( m_job->pos() + offset );
	Global::w().removeFloor( pos, m_position );

	return true;
}

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
		World& world = Global::w();

		if ( !claimedItems().empty() )
		{
			for ( auto item : claimedItems() )
			{
				materials.push_back( Global::inv().materialSID( item ) );
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
			Sprite* s = Global::sf().createSprite( spriteID, { materials.first() } );
			world.setJobSprite( pos, s->uID, m_job->rotation(), isFloor, m_jobID, false );
			return true;
		}
	}
	return true;
}

bool CanWork::constructDugStairs()
{
	Position offset( m_currentTask.value( "Offset" ) );

	m_currentTask.insert( "Offset", ( offset + Position( 0, 0, 1 ) ).toString() );
	removeWall();
	removeFloor();
	m_currentTask.insert( "Offset", offset.toString() );

	Position pos( m_job->pos() + offset );
	auto wallMat = Global::w().wallMaterial( pos );
	removeWall();

	auto item = Util::createRawMaterialItem( pos, wallMat );
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
	QString materialSID = Global::inv().materialSID( itemID );
	QString type        = DB::select( "Type", "Materials", materialSID ).toString();

	bool result = false;
	if ( type == "Soil" || type == "Sand" || type == "Clay" )
	{
		m_job->setItem( "SoilStairs" );
		//result = Global::w().construct( "SoilStairs", m_job->pos(), m_job->rotation(), claimedItems(), m_position );
	}
	else if ( type == "Stone" )
	{
		m_job->setItem( "StoneStairs" );
		//result = Global::w().construct( "StoneStairs", m_job->pos(), m_job->rotation(), claimedItems(), m_position );
	}

	return construct();
}

bool CanWork::constructDugRamp()
{
	m_currentTask.insert( "Offset", "0 0 0" );
	removeWall();  // offset 0 0 0
	removeFloor(); // offset 0 0 0
	m_currentTask.insert( "Offset", "0 0 -1" );

	Position pos( m_job->pos() + Position( 0, 0, -1 ) );
	auto wallMat = Global::w().wallMaterial( pos );
	removeWall(); // offset 0 0 -1

	auto item = Util::createRawMaterialItem( pos, wallMat );
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

	QString materialSID = Global::inv().materialSID( itemID );
	QString type        = DB::select( "Type", "Materials", materialSID ).toString();

	if ( type == "Soil" || type == "Sand" || type == "Clay" )
	{
		m_job->setItem( "SoilRamp" );
		//result = Global::w().construct( "SoilStairs", m_job->pos(), m_job->rotation(), claimedItems(), m_position );
	}
	else if ( type == "Stone" )
	{
		m_job->setItem( "StoneRamp" );
		//result = Global::w().construct( "StoneStairs", m_job->pos(), m_job->rotation(), claimedItems(), m_position );
	}
	return construct();
}

bool CanWork::construct()
{
	if ( claimedItems().empty() )
	{
		return false;
	}

	//qDebug() << "#" << m_job->type() << m_job->item();

	if ( m_job->item() == "" )
	{
		m_job->setItem( m_currentTask.value( "ConstructionID" ).toString() );
	}

	Position offset( m_currentTask.value( "Offset" ).toString() );
	Position pos( m_job->pos() + offset );

	Inventory& inv = Global::inv();

	bool result = false;

	if ( m_job->type() == "BuildWorkshop" )
	{
		result = Global::w().constructWorkshop( m_job->item(), pos, m_job->rotation(), claimedItems(), m_position );
	}
	else if ( m_job->type() == "BuildItem" )
	{
		result = Global::w().constructItem( m_job->item(), pos, m_job->rotation(), claimedItems(), m_position );
	}
	else
	{
		result = Global::w().construct( m_job->item(), pos, m_job->rotation(), claimedItems(), m_position );
	}

	if ( result )
	{
		for ( auto itemUID : claimedItems() )
		{
			inv.setInJob( itemUID, 0 );

			if ( inv.isConstructedOrEquipped( itemUID ) )
			{
				/*
				if( Global::inv().isContainer( itemUID ) )
				{
					for( auto iic : Global::inv().itemsInContainer( itemUID ) )
					{
						Global::inv().setInContainer( iic, 0 );
						Global::inv().moveItemToPos( iic, m_position );
					}
				}
				*/
			}
			else
			{
				Global::inv().pickUpItem( itemUID );
				Global::inv().destroyObject( itemUID );
			}
		}
		clearClaimedItems();
		return true;
	}
	return false;
}

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
					Global::inv().createItem( pos, im.value( "ItemID" ).toString(), materialSID );
					return true;
				}
			}
		}
	}
	qDebug() << "create item failed";
	return false;
}

bool CanWork::harvest()
{
	// get Tree
	Position offset;
	if ( m_currentTask.contains( "Offset" ) )
	{
		offset = Position( m_currentTask.value( "Offset" ).toString() );
	}
	Position pos( m_job->pos() + offset );
	QMap<unsigned int, Plant>& plants = Global::w().plants();
	if ( plants.contains( pos.toInt() ) )
	{
		Plant& plant = plants[pos.toInt()];
		if ( plant.harvest( m_position ) ) // plant.harvest() returns true if the plant is to be destroyed after harvest
		{
			// remove plant;
			if ( plant.isPlant() )
			{
				Global::w().clearTileFlag( pos, TileFlag::TF_TILLED );
				Sprite* s = Global::sf().createSprite( "RoughFloor", { DBH::materialSID( Global::w().floorMaterial( pos ) ) } );
				Global::w().setFloorSprite( pos, s->uID );
				Global::w().setWallSprite( pos, 0 );
				Global::w().removeGrass( pos );
			}
			plants.remove( pos.toInt() );

			return true;
		}
		if ( plant.harvestable() )
		{
			m_repeatJob      = true;
			m_taskFinishTick = GameState::tick + m_currentTask.value( "Duration" ).toInt() * Util::ticksPerMinute;
			return true;
		}
	}
	m_repeatJob = false;

	if ( Global::fm().isBeehive( pos ) )
	{

		if ( Global::fm().harvestBeehive( pos ) )
		{
			Global::inv().createItem( pos, "Honey", "Bee" );
		}
	}

	return true;
}

bool CanWork::harvestHay()
{
	// get Tree
	Position offset;
	if ( m_currentTask.contains( "Offset" ) )
	{
		offset = Position( m_currentTask.value( "Offset" ).toString() );
	}
	Position pos( m_job->pos() + offset );

	if ( Global::w().hasMaxGrass( pos ) )
	{
		Global::inv().createItem( pos, "Hay", "Grass" );
		Global::w().setVegetationLevel( pos, 30 );

		return true;
	}

	return false;
}

bool CanWork::plantTree()
{
	if ( !claimedItems().empty() )
	{
		Global::w().plantTree( m_job->pos(), m_job->item() );
		Global::inv().pickUpItem( claimedItems().first() );
		for ( auto ci : claimedItems() )
		{
			Global::inv().destroyObject( ci );
		}
		clearClaimedItems();
		return true;
	}
	return false;
}

bool CanWork::plant()
{
	if ( !claimedItems().empty() )
	{
		Global::w().plant( m_job->pos(), claimedItems().first() );
		Global::inv().pickUpItem( claimedItems().first() );
		for ( auto ci : claimedItems() )
		{
			Global::inv().destroyObject( ci );
		}
		clearClaimedItems();
		return true;
	}
	return false;
}

bool CanWork::removePlant()
{
	Global::w().removePlant( m_job->pos() );
	return true;
}

bool CanWork::till()
{
	Position offset;
	if ( m_currentTask.contains( "Offset" ) )
	{
		offset = Position( m_currentTask.value( "Offset" ).toString() );
	}
	Position pos( m_job->pos() + offset );
	Global::w().setTileFlag( pos, TileFlag::TF_TILLED );
	Global::w().removeGrass( pos );
	Sprite* s = Global::sf().createSprite( "TilledSoil", { DBH::materialSID( Global::w().floorMaterial( pos ) ) } );
	Global::w().setFloorSprite( pos, s->uID );
	return true;
}

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

						auto sourceMaterial = Global::inv().materialSID( item );
						auto material       = Util::randomMetalSliver( sourceMaterial );
						unsigned int itemID = Global::inv().createItem( m_job->posItemOutput(), m_job->item(), material );
						Global::inv().setMadeBy( itemID, id() );
					}
				}
			}
			else
			{
				unsigned int itemID = Global::inv().createItem( m_job->posItemOutput(), m_job->item(), resultMaterial );
				Global::inv().setMadeBy( itemID, id() );
				Global::inv().setQuality( itemID, qIndex );
			}
		}
		else if ( m_job->conversionMaterial().isEmpty() )
		{
			if ( claimedItems().isEmpty() )
			{
				return false;
			}

			unsigned int itemID = Global::inv().createItem( m_job->posItemOutput(), m_job->item(), claimedItems() );
			Global::inv().setMadeBy( itemID, id() );
			Global::inv().setQuality( itemID, qIndex );
			if ( m_job->item() == "Automaton" )
			{
				Automaton* a = new Automaton( m_job->posItemOutput(), itemID );
				//a->setSpriteID( Global::sf().setAutomatonSprite( a->id(), Global::inv().spriteID( itemID ) ) );
				//a->updateSprite();

				Global::gm().addAutomaton( a );
				Global::inv().pickUpItem( itemID );
				Global::inv().setInJob( itemID, a->id() );
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
					if ( Global::inv().itemSID( item ) == "Dye" )
					{
						dyeItem = item;
					}
					else
					{
						sourceItem = item;
					}
				}
				QString sourceMaterial = Global::inv().materialSID( sourceItem );
				QString dyeMaterial    = Global::inv().materialSID( dyeItem );

				QString targetMaterial = Util::addDyeMaterial( sourceMaterial, dyeMaterial );

				unsigned int itemID = Global::inv().createItem( m_job->posItemOutput(), m_job->item(), targetMaterial );
				Global::inv().setMadeBy( itemID, id() );
				Global::inv().setQuality( itemID, Global::inv().quality( sourceItem ) );
			}
			else if ( m_job->conversionMaterial() == "$GnomeHair" )
			{
				if ( claimedItems().isEmpty() )
				{
					//abortJob( "error claimed items empty" );
					return false;
				}
				unsigned int sourceItem = claimedItems().first();
				QString sourceMaterial  = Global::inv().materialSID( sourceItem );
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
				QString sourceMaterial  = Global::inv().materialSID( sourceItem );

				unsigned int itemID = Global::inv().createItem( m_job->posItemOutput(), m_job->item(), sourceMaterial + "Leather" );
				Global::inv().setMadeBy( itemID, id() );
				Global::inv().setQuality( itemID, qIndex );
			}
			else
			{
				unsigned int itemID = Global::inv().createItem( m_job->posItemOutput(), m_job->item(), m_job->conversionMaterial() );
				Global::inv().setMadeBy( itemID, id() );
				Global::inv().setQuality( itemID, qIndex );
			}
		}
	}

	for ( auto item : claimedItems() )
	{
		Global::inv().pickUpItem( item );
		Global::inv().destroyObject( item );
	}
	clearClaimedItems();
	return true;
}

bool CanWork::fellTree()
{
	// get Tree
	Position offset;
	if ( m_currentTask.contains( "Offset" ) )
	{
		offset = Position( m_currentTask.value( "Offset" ).toString() );
	}
	Position pos( m_job->pos() + offset );
	QMap<unsigned int, Plant>& plants = Global::w().plants();
	if ( plants.contains( pos.toInt() ) )
	{
		Plant& plant = plants[pos.toInt()];
		if ( plant.fell() )
		{
			// remove plant;
			Global::w().removePlant( pos );
			GameState::addChange2( NetworkCommand::PLANTREMOVE, pos.toString() );
			return true;
		}
	}
	return false;
}

bool CanWork::deconstruct()
{
	Position offset;
	if ( m_currentTask.contains( "Offset" ) )
	{
		offset = Position( m_currentTask.value( "Offset" ).toString() );
	}
	Position pos( m_job->pos() + offset );

	Global::w().deconstruct( pos, m_position, false );
	return true;
}

bool CanWork::butcherFish()
{
	for ( auto itemUID : claimedItems() )
	{
		QString materialSID = Global::inv().materialSID( itemUID );

		Global::inv().createItem( m_job->posItemOutput(), "Meat", materialSID );
		Global::inv().createItem( m_job->posItemOutput(), "FishBone", materialSID );

		Global::inv().pickUpItem( itemUID );
		Global::inv().destroyObject( itemUID );
	}
	clearClaimedItems();
	return true;
}

bool CanWork::butcherCorpse()
{
	for ( auto itemUID : claimedItems() )
	{
		QString materialSID = Global::inv().materialSID( itemUID );

		Global::inv().createItem( m_job->posItemOutput(), "Meat", materialSID );
		Global::inv().createItem( m_job->posItemOutput(), "Bone", materialSID );

		Global::inv().pickUpItem( itemUID );
		Global::inv().destroyObject( itemUID );
	}
	clearClaimedItems();
	return true;
}

bool CanWork::fish()
{
	unsigned int itemID = Global::inv().createItem( m_job->posItemOutput(), "Fish", "GreenFish" );
	Global::inv().setMadeBy( itemID, id() );
	return true;
}

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
	unsigned int spriteUID = Global::sf().createSprite( "Sparkles", { "None" } )->uID + 2048;

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
				Global::w().setJobSprite( Position( x, y, center.z ), spriteUID, 0, false, m_job->id(), true );
			}
		}
	}
	return true;
}

bool CanWork::castSpell()
{
	return true;
}

bool CanWork::castSpellAnimate()
{
	//
	return true;
}

bool CanWork::finishSpell()
{
	QString jobID        = m_job->type();
	QStringList jobParts = jobID.split( "_" );
	QString spellID      = jobParts.last();

	auto spellMap        = DB::selectRow( "Spells", spellID );
	QVariantList reqs    = spellMap.value( "EffectRequirements" ).toList();
	QVariantList effects = spellMap.value( "Effects" ).toList();

	World& world = Global::w();

	for ( auto pos : m_effectedTiles )
	{
		bool effected = true;
		for ( auto vReq : reqs )
		{
			QString sReq = vReq.toString();
			if ( sReq == "Plant" )
			{
				effected &= world.plants().contains( pos.toInt() );
			}
			else if ( sReq == "RoughWall" )
			{
				effected &= ( world.wallType( pos ) == WT_ROUGH );
			}
			else if ( sReq == "EmbeddedMaterial" )
			{
				effected &= ( world.embeddedMaterial( pos ) != 0 );
			}
		}

		if ( effected )
		{
			for ( auto vEffect : effects )
			{
				QString sEffect = vEffect.toString();
				if ( sEffect == "Reveal" )
				{
					world.clearTileFlag( pos, TileFlag::TF_UNDISCOVERED );
					world.addToUpdateList( pos );
				}
				else if ( sEffect == "PlantGrowth" )
				{
					world.plants()[pos.toInt()].speedUpGrowth( Util::ticksPerDay * getSkillLevel( m_job->requiredSkill() ) );
				}
			}
		}

		Global::w().clearJobSprite( pos, false );
	}
	return true;
}

bool CanWork::switchMechanism()
{
	if ( m_job )
	{
		Global::mcm().toggleActive( m_job->mechanism() );
		return true;
	}

	return false;
}

bool CanWork::invertMechanism()
{
	if ( m_job )
	{
		Global::mcm().toggleInvert( m_job->mechanism() );
		return true;
	}

	return false;
}

bool CanWork::refuel()
{
	if ( m_job )
	{
		int burnValue = DB::select( "BurnValue", "Items", m_job->requiredItems().first().itemSID ).toInt();

		if ( m_job->automaton() )
		{
			auto a = Global::gm().automaton( m_job->automaton() );
			if ( a )
			{
				a->fillUp( burnValue );
			}
		}
		else if ( m_job->mechanism() )
		{
			Global::mcm().refuel( m_job->mechanism(), burnValue );
		}

		for ( auto item : claimedItems() )
		{
			Global::inv().pickUpItem( item );
			Global::inv().destroyObject( item );
		}
		clearClaimedItems();

		return true;
	}

	return false;
}

bool CanWork::install()
{
	if ( m_job )
	{

		if ( m_job->automaton() )
		{
			auto a = Global::gm().automaton( m_job->automaton() );
			if ( a )
			{
				for ( auto item : claimedItems() )
				{
					a->installCore( item );
				}
				clearClaimedItems();
			}
		}
		return true;
	}
	return false;
}

bool CanWork::uninstall()
{
	if ( m_job )
	{
		if ( m_job->automaton() )
		{
			auto a = Global::gm().automaton( m_job->automaton() );
			if ( a )
			{
				a->installCore( 0 );
			}
		}
		return true;
	}
	return false;
}

bool CanWork::fillTrough()
{
	if ( m_job )
	{
		auto pasture = Global::fm().getPastureAtPos( m_job->pos() );
		if ( pasture && m_claimedItems.size() == 1 )
		{
			pasture->addFood( m_claimedItems.first() );

			for ( auto item : claimedItems() )
			{
				Global::inv().pickUpItem( item );
				Global::inv().destroyObject( item );
			}
			clearClaimedItems();
			return true;
		}
		else
		{

			for ( auto item : claimedItems() )
			{
				Global::inv().setInJob( item, 0 );
			}
			clearClaimedItems();
			return false;
		}
	}

	return false;
}

bool CanWork::soundAlarm()
{
	if ( m_job )
	{
		if ( GameState::alarm == 1 )
		{
			auto room = Global::rm().getRoomAtPos( m_job->pos() );
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
