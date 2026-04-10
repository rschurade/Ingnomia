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
/** @file job.cpp
 *  @brief Job instance implementation: serialization, accessors, phase/progress management,
 *         multi-worker support, and hauling sub-job tracking.
 */
#include "job.h"

#include "../base/config.h"
#include "../base/gamestate.h"
#include "../base/position.h"
#include "../base/util.h"

#include <QDebug>

/** @brief Default constructor. Assigns a unique ID from GameState. */
Job::Job()
{
	m_id = GameState::createID();
}

/** @brief Constructs a job from serialized save data.
 *  @param in Variant map containing all saved job fields. */
Job::Job( QVariantMap in )
{
	m_id            = in.value( "ID" ).toUInt();
	m_type          = in.value( "Type" ).toString();
	m_requiredSkill = in.value( "RequiredSkill" ).toString();
	m_description   = in.value( "Description" ).toString();
	m_rotation      = in.value( "Rotation" ).value<quint8>();
	m_noJobSprite   = in.value( "NoJobSprite" ).toBool();
	m_priority      = in.value( "Priority" ).value<quint8>();

	m_canceled         = in.value( "Canceled" ).toBool();
	m_aborted          = in.value( "Aborted" ).toBool();
	m_componentMissing = in.value( "ComponentMissing" ).toBool();
	m_mayTrap          = in.value( "MayTrap" ).toBool();
	m_destroyOnAbort   = in.value( "DestroyOnAbort" ).toBool();

	m_jobIsWorked = in.value( "JobIsWorked" ).toBool();
	m_workedBy    = in.value( "JobWorkedBy" ).toUInt();

	m_requiredTool.type  = in.value( "RequiredTool" ).toString();
	m_requiredTool.level = in.value( "RequiredToolLevel" ).toInt();

	auto ril = in.value( "RequiredItems" ).toList();
	for ( auto vri : ril )
	{
		auto rim = vri.toMap();
		RequiredItem ri;
		ri.count               = rim.value( "Count" ).toInt();
		ri.itemSID             = rim.value( "ItemSID" ).toString();
		ri.materialSID         = rim.value( "MaterialSID" ).toString();
		ri.materialRestriction = rim.value( "MaterialRestriction" ).toStringList();
		ri.requireSame         = rim.value( "RequireSame" ).toBool();
		m_requiredItems.append( ri );
	}

	m_position              = in.value( "Position" );
	m_posItemInput          = in.value( "PositionItemInput" );
	m_posItemOutput         = in.value( "PositionItemOutput" );
	m_toolPosition          = in.value( "ToolPosition" );
	m_workPosition          = in.value( "WorkPosition" );
	m_possibleWorkPositions = Global::util->variantList2Position( in.value( "PossibleWorkPositions" ).toList() );
	m_origWorkPosOffsets    = Global::util->variantList2Position( in.value( "OriginalWorkPositions" ).toList() );

	m_amount             = in.value( "Amount" ).toInt();
	m_item               = in.value( "Item" ).toString();
	m_material           = in.value( "Material" ).toString();
	m_craftID            = in.value( "CraftID" ).toString();
	m_craftJobID            = in.value( "CraftJobID" ).toUInt();
	m_craft              = in.value( "CraftMap" ).toMap();
	m_conversionMaterial = in.value( "ConversionMaterial" ).toString();

	m_stockpile = in.value( "Stockpile" ).toUInt();
	m_animal    = in.value( "Animal" ).toUInt();
	m_automaton = in.value( "Automaton" ).toUInt();
	m_mechanism = in.value( "Mechanism" ).toUInt();

	m_itemsToHaul = Global::util->variantList2UInt( in.value( "ItemsToHaul" ).toList() );
	m_spell       = in.value( "Spell" ).toString();

	// Phase and progress
	m_phase            = static_cast<JobPhase>( in.value( "Phase" ).toUInt() );
	m_totalWorkTicks   = in.value( "TotalWorkTicks" ).toFloat();
	m_accumulatedTicks = in.value( "AccumulatedTicks" ).toFloat();
	m_currentTaskIndex = in.value( "CurrentTaskIndex" ).toInt();
	m_maxWorkers       = in.value( "MaxWorkers", 1 ).toUInt();
	m_parentJobID      = in.value( "ParentJobID" ).toUInt();
	m_itemsDelivered   = in.value( "ItemsDelivered" ).toInt();
	m_itemsRequired    = in.value( "ItemsRequired" ).toInt();
	m_haulSubJobs      = Global::util->variantList2UInt( in.value( "HaulSubJobs" ).toList() );
}

/** @brief Serializes all job state into a QVariant (wrapping a QVariantMap).
 *  @return QVariant containing the serialized job data. */
QVariant Job::serialize() const
{
	QVariantMap out;
	out.insert( "ID", m_id );
	out.insert( "Type", m_type );
	out.insert( "RequiredSkill", m_requiredSkill );
	out.insert( "Description", m_description );
	out.insert( "Rotation", m_rotation );
	out.insert( "NoJobSprite", m_noJobSprite );
	out.insert( "Priority", m_priority );

	out.insert( "Canceled", m_canceled );
	out.insert( "Aborted", m_aborted );
	out.insert( "ComponentMissing", m_componentMissing );
	out.insert( "MayTrap", m_mayTrap );
	out.insert( "DestroyOnAbort", m_destroyOnAbort );

	out.insert( "JobIsWorked", m_jobIsWorked );
	out.insert( "JobWorkedBy", m_workedBy );

	out.insert( "RequiredTool", m_requiredTool.type );       // QString tool
	out.insert( "RequiredToolLevel", m_requiredTool.level ); // int level

	QVariantList ril;
	for ( const auto& ri : m_requiredItems )
	{
		QVariantMap rim;
		rim.insert( "Count", ri.count );
		rim.insert( "ItemSID", ri.itemSID );
		rim.insert( "MaterialSID", ri.materialSID );
		rim.insert( "MaterialRestriction", ri.materialRestriction );
		rim.insert( "RequireSame", ri.requireSame );
		ril.append( rim );
	}
	out.insert( "RequiredItems", ril );

	out.insert( "Position", m_position.toString() );
	out.insert( "PositionItemInput", m_posItemInput.toString() );
	out.insert( "PositionItemOutput", m_posItemOutput.toString() );
	out.insert( "ToolPosition", m_toolPosition.toString() );
	out.insert( "WorkPosition", m_workPosition.toString() );
	out.insert( "PossibleWorkPositions", Global::util->positionList2Variant( m_possibleWorkPositions ) );
	out.insert( "OriginalWorkPositions", Global::util->positionList2Variant( m_origWorkPosOffsets ) );

	out.insert( "Amount", m_amount );
	out.insert( "Item", m_item );
	out.insert( "Material", m_material );
	out.insert( "CraftID", m_craftID );
	out.insert( "CraftJobID", m_craftJobID );
	out.insert( "CraftMap", m_craft );
	out.insert( "ConversionMaterial", m_conversionMaterial );

	out.insert( "Stockpile", m_stockpile );
	out.insert( "Automaton", m_automaton );
	out.insert( "Mechanism", m_mechanism );
	out.insert( "Animal", m_animal );

	out.insert( "ItemsToHaul", Global::util->uintList2Variant( m_itemsToHaul ) );
	out.insert( "Spell", m_spell );

	// Phase and progress
	out.insert( "Phase", static_cast<unsigned int>( m_phase ) );
	out.insert( "TotalWorkTicks", m_totalWorkTicks );
	out.insert( "AccumulatedTicks", m_accumulatedTicks );
	out.insert( "CurrentTaskIndex", m_currentTaskIndex );
	out.insert( "MaxWorkers", m_maxWorkers );
	out.insert( "ParentJobID", m_parentJobID );
	out.insert( "ItemsDelivered", m_itemsDelivered );
	out.insert( "ItemsRequired", m_itemsRequired );
	out.insert( "HaulSubJobs", Global::util->uintList2Variant( m_haulSubJobs ) );

	return out;
}

/*
Job::Job( const Job& other )
{
	m_id            = other.m_id;
	m_type          = other.m_type;
	m_requiredSkill = other.m_requiredSkill;
	m_description   = other.m_description;
	m_rotation      = other.m_rotation;
	m_noJobSprite   = other.m_noJobSprite;
	m_priority      = other.m_priority;

	m_canceled         = other.m_canceled;
	m_aborted          = other.m_aborted;
	m_componentMissing = other.m_componentMissing;
	m_mayTrap          = other.m_mayTrap;
	m_destroyOnAbort   = other.m_destroyOnAbort;

	m_jobIsWorked = other.m_jobIsWorked;
	m_workedBy    = other.m_workedBy;

	m_requiredTool  = other.m_requiredTool;
	m_requiredItems = other.m_requiredItems;

	m_position              = other.m_position;
	m_posItemInput          = other.m_posItemInput;
	m_posItemOutput         = other.m_posItemOutput;
	m_toolPosition          = other.m_toolPosition;
	m_workPosition          = other.m_workPosition;
	m_possibleWorkPositions = other.m_possibleWorkPositions;
	m_origWorkPosOffsets    = other.m_origWorkPosOffsets;

	m_amount             = other.m_amount;
	m_item               = other.m_item;
	m_material           = other.m_material;
	m_craftID            = other.m_craftID;
	m_craft              = other.m_craft;
	m_conversionMaterial = other.m_conversionMaterial;

	m_stockpile = other.m_stockpile;
	m_animal    = other.m_animal;
	m_automaton = other.m_automaton;

	m_itemsToHaul = other.m_itemsToHaul;
	m_spell       = other.m_spell;
}
*/

/** @brief Destructor. */
Job::~Job()
{
}

/** @brief Returns the unique job ID.
 *  @return Job ID. */
unsigned int Job::id() const
{
	return m_id;
}

/** @brief Returns the job type string (e.g., "Mine", "BuildWall").
 *  @return Job type identifier. */
QString Job::type() const
{
	return m_type;
}

/** @brief Sets the job type.
 *  @param type Job type identifier string. */
void Job::setType( QString type )
{
	m_type = type;
}

/** @brief Returns the skill required to perform this job.
 *  @return Skill identifier string. */
QString Job::requiredSkill() const
{
	return m_requiredSkill;
}

/** @brief Sets the required skill for this job.
 *  @param skill Skill identifier string. */
void Job::setRequiredSkill( QString skill )
{
	m_requiredSkill = skill;
}

/** @brief Sets the human-readable description of this job.
 *  @param desc Description text. */
void Job::setDescription( QString desc )
{
	m_description = desc;
}

/** @brief Returns the job description text.
 *  @return Description string. */
QString Job::description() const
{
	return m_description;
}

/** @brief Returns the position of the tile being worked.
 *  @return Job position. */
Position Job::pos() const
{
	return m_position;
}

/** @brief Sets the position of the tile being worked.
 *  @param pos Target tile position. */
void Job::setPos( const Position& pos )
{
	m_position = pos;
}

/** @brief Returns the item input position (where materials are delivered).
 *  @return Item input position. */
Position Job::posItemInput() const
{
	return m_posItemInput;
}

/** @brief Sets the item input position.
 *  @param pos Item input position. */
void Job::setPosItemInput( const Position& pos )
{
	m_posItemInput = pos;
}

/** @brief Returns the item output position (where products are placed).
 *  @return Item output position. */
Position Job::posItemOutput() const
{
	return m_posItemOutput;
}

/** @brief Sets the item output position.
 *  @param pos Item output position. */
void Job::setPosItemOutput( const Position& pos )
{
	m_posItemOutput = pos;
}

/** @brief Returns the position where the required tool is located.
 *  @return Tool position. */
Position Job::toolPosition() const
{
	return m_toolPosition;
}

/** @brief Sets the tool position.
 *  @param pos Tool position. */
void Job::setToolPosition( const Position& pos )
{
	m_toolPosition = pos;
}

/** @brief Returns the work position (where the worker stands).
 *  @return Work position. */
Position Job::workPos() const
{
	return m_workPosition;
}

/** @brief Sets the work position.
 *  @param pos Work position. */
void Job::setWorkPos( const Position& pos )
{
	m_workPosition = pos;
}

/** @brief Returns the list of walkable positions from which this job can be worked.
 *  @return List of possible work positions. */
QList<Position> Job::possibleWorkPositions()
{
	return m_possibleWorkPositions;
}

/** @brief Adds a position to the list of possible work positions.
 *  @param wp Position to add. */
void Job::addPossibleWorkPosition( const Position& wp )
{
	m_possibleWorkPositions.append( wp );
}

/** @brief Clears all possible work positions. */
void Job::clearPossibleWorkPositions()
{
	m_possibleWorkPositions.clear();
}

/** @brief Returns the original work position offsets (relative to the job position).
 *  @return List of offset positions. */
QList<Position> Job::origWorkPosOffsets()
{
	return m_origWorkPosOffsets;
}

/** @brief Parses and sets work position offsets from a pipe-delimited string.
 *  @param offsets String of positions separated by '|'. */
void Job::setOrigWorkPosOffsets( QString offsets )
{
	auto wps = offsets.split( "|" );
	for ( auto wp : wps )
	{
		m_origWorkPosOffsets.append( Position( wp ) );
	}
}

/** @brief Sets work position offsets from a list of positions.
 *  @param pl List of offset positions. */
void Job::setOrigWorkPosOffsets( QList<Position> pl )
{
	m_origWorkPosOffsets = pl;
}


/** @brief Returns whether this job is currently being worked.
 *  @return True if a worker is actively working this job. */
bool Job::isWorked() const
{
	return m_jobIsWorked;
}

/** @brief Sets the worked status of this job.
 *  @param v True if being worked. */
void Job::setIsWorked( bool v )
{
	m_jobIsWorked = v;
}

/** @brief Returns the rotation value for the job's construction/placement.
 *  @return Rotation (0-3, representing 90-degree increments). */
unsigned char Job::rotation() const
{
	return m_rotation;
}

/** @brief Sets the rotation value.
 *  @param rot Rotation (0-3). */
void Job::setRotation( unsigned char rot )
{
	m_rotation = rot;
}

/** @brief Returns the list of item IDs to be hauled for this job.
 *  @return List of item IDs. */
QList<unsigned int> Job::itemsToHaul() const
{
	return m_itemsToHaul;
}

/** @brief Adds an item ID to the haul list.
 *  @param item ID of the item to haul. */
void Job::addItemToHaul( unsigned int item )
{
	m_itemsToHaul.append( item );
}

/** @brief Returns the item type identifier for this job.
 *  @return Item SID string. */
QString Job::item() const
{
	return m_item;
}

/** @brief Sets the item type identifier.
 *  @param item Item SID string. */
void Job::setItem( QString item )
{
	m_item = item;
}

/** @brief Returns the associated stockpile ID.
 *  @return Stockpile ID, or 0 if none. */
unsigned int Job::stockpile() const
{
	return m_stockpile;
}

/** @brief Sets the associated stockpile ID.
 *  @param sp Stockpile ID. */
void Job::setStockpile( unsigned int sp )
{
	m_stockpile = sp;
}

/** @brief Computes the squared distance from a position to this job's position.
 *  @param pos Reference position to measure from.
 *  @param zWeight Multiplier for vertical distance.
 *  @return Squared distance value. */
int Job::distanceSquare( Position& pos, int zWeight ) const
{
	return pos.distSquare( pos, zWeight );
}

/** @brief Returns the material identifier for this job.
 *  @return Material SID string. */
QString Job::material() const
{
	return m_material;
}

/** @brief Sets the material identifier.
 *  @param material Material SID string. */
void Job::setMaterial( QString material )
{
	m_material = material;
}

/** @brief Returns whether the job sprite is suppressed.
 *  @return True if no job sprite should be displayed. */
bool Job::noJobSprite() const
{
	return m_noJobSprite;
}

/** @brief Sets whether to suppress the job sprite.
 *  @param v True to suppress the sprite. */
void Job::setNoJobSprite( bool v )
{
	m_noJobSprite = v;
}

/** @brief Sets whether a required component is missing.
 *  @param v True if a component is missing. */
void Job::setComponentMissing( bool v )
{
	m_componentMissing = v;
}

/** @brief Returns whether a required component is missing.
 *  @return True if missing. */
bool Job::componentMissing() const
{
	return m_componentMissing;
}

/** @brief Returns the list of required items for this job.
 *  @return List of RequiredItem structs. */
QList<RequiredItem> Job::requiredItems() const
{
	return m_requiredItems;
}

/** @brief Adds a required item specification to this job.
 *  @param count Number of items needed.
 *  @param item Item type SID.
 *  @param material Material SID (or empty for any).
 *  @param materialRestriction List of allowed material types.
 *  @param requireSame Whether all items must share the same material. */
void Job::addRequiredItem( int count, QString item, QString material, QStringList materialRestriction, bool requireSame )
{
	RequiredItem ri;

	ri.count               = count;
	ri.itemSID             = item;
	ri.materialSID         = material;
	ri.materialRestriction = materialRestriction;
	ri.requireSame         = requireSame;

	m_requiredItems.append( ri );
}

/** @brief Returns the craft recipe data for this job.
 *  @return Craft recipe as a variant map. */
QVariantMap Job::craft() const
{
	return m_craft;
}

/** @brief Sets the craft recipe data.
 *  @param craft Craft recipe variant map. */
void Job::setCraft( QVariantMap craft )
{
	m_craft = craft;
}

/** @brief Marks this job as canceled. A working gnome will stop on next check. */
void Job::setCanceled()
{
	m_canceled = true;
}

/** @brief Returns whether this job has been canceled.
 *  @return True if canceled. */
bool Job::isCanceled() const
{
	return m_canceled;
}

/** @brief Sets the aborted flag.
 *  @param v True to mark as aborted. */
void Job::setAborted( bool v )
{
	m_aborted = v;
}

/** @brief Returns whether this job has been aborted.
 *  @return True if aborted. */
bool Job::isAborted() const
{
	return m_aborted;
}

/** @brief Sets the gnome ID that is working this job.
 *  @param gnomeID ID of the worker gnome. */
void Job::setWorkedBy( unsigned int gnomeID )
{
	m_workedBy = gnomeID;
}

/** @brief Returns the ID of the gnome working this job.
 *  @return Worker gnome ID, or 0 if unassigned. */
unsigned int Job::workedBy() const
{
	return m_workedBy;
}

/** @brief Returns the required tool specification.
 *  @return RequiredTool struct. */
RequiredTool Job::requiredTool() const
{
	return m_requiredTool;
}

/** @brief Sets the required tool type and minimum level. Marks available if no tool is needed.
 *  @param toolID Tool type SID (empty means no tool required).
 *  @param level Minimum tool level. */
void Job::setRequiredTool( QString toolID, quint8 level )
{
	m_requiredTool.type  = toolID;
	m_requiredTool.level = level;

	if ( toolID.isEmpty() )
	{
		m_requiredTool.available = true;
	}
}

/** @brief Sets whether the required tool is available in the world.
 *  @param avail True if available. */
void Job::setRequiredToolAvailable( bool avail )
{
	m_requiredTool.available = avail;
}

/** @brief Sets the conversion material (output material after crafting/processing).
 *  @param material Material SID. */
void Job::setConversionMaterial( QString material )
{
	m_conversionMaterial = material;
}

/** @brief Returns the conversion material.
 *  @return Material SID string. */
QString Job::conversionMaterial() const
{
	return m_conversionMaterial;
}

/** @brief Sets the number of items to produce or process.
 *  @param amount Item count. */
void Job::setAmount( int amount )
{
	m_amount = amount;
}

/** @brief Returns the number of items to produce or process.
 *  @return Amount. */
int Job::amount() const
{
	return m_amount;
}

/** @brief Returns the associated animal ID (for animal-related jobs).
 *  @return Animal ID, or 0 if none. */
unsigned int Job::animal() const
{
	return m_animal;
}

/** @brief Sets the associated animal ID.
 *  @param animal Animal ID. */
void Job::setAnimal( unsigned int animal )
{
	m_animal = animal;
}

/** @brief Returns the associated automaton ID.
 *  @return Automaton ID, or 0 if none. */
unsigned int Job::automaton() const
{
	return m_automaton;
}

/** @brief Sets the associated automaton ID.
 *  @param automaton Automaton ID. */
void Job::setAutomaton( unsigned int automaton )
{
	m_automaton = automaton;
}

/** @brief Returns the associated mechanism ID.
 *  @return Mechanism ID, or 0 if none. */
unsigned int Job::mechanism() const
{
	return m_mechanism;
}

/** @brief Sets the associated mechanism ID.
 *  @param mechanism Mechanism ID. */
void Job::setMechanism( unsigned int mechanism )
{
	m_mechanism = mechanism;
}

/** @brief Sets the spell identifier for magic-related jobs.
 *  @param spell Spell SID string. */
void Job::setSpell( QString spell )
{
	m_spell = spell;
}

/** @brief Returns the spell identifier.
 *  @return Spell SID string. */
QString Job::spell() const
{
	return m_spell;
}

/** @brief Sets whether this job may trap the gnome (e.g., digging under self).
 *  @param value True if the job may trap. */
void Job::setMayTrap( bool value )
{
	m_mayTrap = value;
}

/** @brief Returns whether this job may trap the gnome.
 *  @return True if may trap. */
bool Job::mayTrap() const
{
	return m_mayTrap;
}

/** @brief Increases the job priority by 1 (capped at 9). */
void Job::raisePrio()
{
	m_priority = qMin( 9, m_priority + 1 );
}

/** @brief Decreases the job priority by 1 (capped at 0). */
void Job::lowerPrio()
{
	m_priority = qMax( 0, m_priority - 1 );
}

/** @brief Returns the current job priority (0-9, higher = more urgent).
 *  @return Priority value. */
int Job::priority() const
{
	return m_priority;
}

/** @brief Sets the craft recipe ID.
 *  @param craftID Craft recipe identifier. */
void Job::setCraftID( QString craftID )
{
	m_craftID = craftID;
}

/** @brief Returns the craft recipe ID.
 *  @return Craft recipe identifier. */
QString Job::craftID() const
{
	return m_craftID;
}

/** @brief Sets the craft job ID (links to a workshop craft order).
 *  @param craftJobID Craft job ID. */
void Job::setCraftJobID( unsigned int craftJobID )
{
	m_craftJobID = craftJobID;
}

/** @brief Returns the craft job ID.
 *  @return Craft job ID. */
unsigned int Job::craftJobID() const
{
	return m_craftJobID;
}

/** @brief Sets whether the job's target should be destroyed if the job is aborted.
 *  @param value True to destroy on abort. */
void Job::setDestroyOnAbort( bool value )
{
	m_destroyOnAbort = value;
}

/** @brief Returns whether the job destroys its target on abort.
 *  @return True if destroy on abort. */
bool Job::destroyOnAbort() const
{
	return m_destroyOnAbort;
}

/** @brief Sets the priority to a specific value, clamped to [0, 9].
 *  @param prio Priority value. */
void Job::setPrio( int prio )
{
	m_priority = qMin( 9, qMax( 0, prio ) );
}

/** @brief Adds a worker to this job (multi-worker support). Fails if at max capacity.
 *  @param gnomeID ID of the gnome to add.
 *  @param workPos Position where the gnome will work from.
 *  @return True if the worker was added, false if at capacity. */
bool Job::addWorker( unsigned int gnomeID, Position workPos )
{
	if ( m_activeWorkers.size() >= m_maxWorkers )
		return false;
	m_activeWorkers.insert( gnomeID );
	m_workerPositions.insert( gnomeID, workPos );
	m_jobIsWorked = true;
	m_workedBy = gnomeID; // legacy compat — last worker
	return true;
}

/** @brief Removes a worker from this job. Clears worked status if no workers remain.
 *  @param gnomeID ID of the gnome to remove. */
void Job::removeWorker( unsigned int gnomeID )
{
	m_activeWorkers.remove( gnomeID );
	m_workerPositions.remove( gnomeID );
	if ( m_activeWorkers.empty() )
	{
		m_jobIsWorked = false;
		m_workedBy = 0;
	}
	else
	{
		m_workedBy = *m_activeWorkers.begin();
	}
}