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
#include "job.h"

#include "../base/config.h"
#include "../base/gamestate.h"
#include "../base/position.h"
#include "../base/util.h"

#include <QDebug>

Job::Job()
{
	m_id = GameState::createID();
}

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
}

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

Job::~Job()
{
}

unsigned int Job::id() const
{
	return m_id;
}

QString Job::type() const
{
	return m_type;
}
void Job::setType( QString type )
{
	m_type = type;
}

QString Job::requiredSkill() const
{
	return m_requiredSkill;
}
void Job::setRequiredSkill( QString skill )
{
	m_requiredSkill = skill;
}

void Job::setDescription( QString desc )
{
	m_description = desc;
}
QString Job::description() const
{
	return m_description;
}

Position Job::pos() const
{
	return m_position;
}
void Job::setPos( const Position& pos )
{
	m_position = pos;
}

Position Job::posItemInput() const
{
	return m_posItemInput;
}
void Job::setPosItemInput( const Position& pos )
{
	m_posItemInput = pos;
}

Position Job::posItemOutput() const
{
	return m_posItemOutput;
}
void Job::setPosItemOutput( const Position& pos )
{
	m_posItemOutput = pos;
}

Position Job::toolPosition() const
{
	return m_toolPosition;
}
void Job::setToolPosition( const Position& pos )
{
	m_toolPosition = pos;
}

Position Job::workPos() const
{
	return m_workPosition;
}
void Job::setWorkPos( const Position& pos )
{
	m_workPosition = pos;
}

QList<Position> Job::possibleWorkPositions()
{
	return m_possibleWorkPositions;
}

void Job::addPossibleWorkPosition( const Position& wp )
{
	m_possibleWorkPositions.append( wp );
}
void Job::clearPossibleWorkPositions()
{
	m_possibleWorkPositions.clear();
}

QList<Position> Job::origWorkPosOffsets()
{
	return m_origWorkPosOffsets;
}

void Job::setOrigWorkPosOffsets( QString offsets )
{
	auto wps = offsets.split( "|" );
	for ( auto wp : wps )
	{
		m_origWorkPosOffsets.append( Position( wp ) );
	}
}

void Job::setOrigWorkPosOffsets( QList<Position> pl )
{
	m_origWorkPosOffsets = pl;
}


bool Job::isWorked() const
{
	return m_jobIsWorked;
}
void Job::setIsWorked( bool v )
{
	m_jobIsWorked = v;
}

unsigned char Job::rotation() const
{
	return m_rotation;
}
void Job::setRotation( unsigned char rot )
{
	m_rotation = rot;
}

QList<unsigned int> Job::itemsToHaul() const
{
	return m_itemsToHaul;
}

void Job::addItemToHaul( unsigned int item )
{
	m_itemsToHaul.append( item );
}

QString Job::item() const
{
	return m_item;
}
void Job::setItem( QString item )
{
	m_item = item;
}

unsigned int Job::stockpile() const
{
	return m_stockpile;
}
void Job::setStockpile( unsigned int sp )
{
	m_stockpile = sp;
}

int Job::distanceSquare( Position& pos, int zWeight ) const
{
	return pos.distSquare( pos, zWeight );
}

QString Job::material() const
{
	return m_material;
}
void Job::setMaterial( QString material )
{
	m_material = material;
}

bool Job::noJobSprite() const
{
	return m_noJobSprite;
}
void Job::setNoJobSprite( bool v )
{
	m_noJobSprite = v;
}

void Job::setComponentMissing( bool v )
{
	m_componentMissing = v;
}
bool Job::componentMissing() const
{
	return m_componentMissing;
}

QList<RequiredItem> Job::requiredItems() const
{
	return m_requiredItems;
}

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

QVariantMap Job::craft() const
{
	return m_craft;
}

void Job::setCraft( QVariantMap craft )
{
	m_craft = craft;
}

void Job::setCanceled()
{
	m_canceled = true;
}

bool Job::isCanceled() const
{
	return m_canceled;
}

void Job::setAborted( bool v )
{
	m_aborted = v;
}

bool Job::isAborted() const
{
	return m_aborted;
}

void Job::setWorkedBy( unsigned int gnomeID )
{
	m_workedBy = gnomeID;
}

unsigned int Job::workedBy() const
{
	return m_workedBy;
}

RequiredTool Job::requiredTool() const
{
	return m_requiredTool;
}

void Job::setRequiredTool( QString toolID, quint8 level )
{
	m_requiredTool.type  = toolID;
	m_requiredTool.level = level;

	if ( toolID.isEmpty() )
	{
		m_requiredTool.available = true;
	}
}

void Job::setRequiredToolAvailable( bool avail )
{
	m_requiredTool.available = avail;
}

void Job::setConversionMaterial( QString material )
{
	m_conversionMaterial = material;
}

QString Job::conversionMaterial() const
{
	return m_conversionMaterial;
}

void Job::setAmount( int amount )
{
	m_amount = amount;
}

int Job::amount() const
{
	return m_amount;
}

unsigned int Job::animal() const
{
	return m_animal;
}

void Job::setAnimal( unsigned int animal )
{
	m_animal = animal;
}

unsigned int Job::automaton() const
{
	return m_automaton;
}

void Job::setAutomaton( unsigned int automaton )
{
	m_automaton = automaton;
}

unsigned int Job::mechanism() const
{
	return m_mechanism;
}

void Job::setMechanism( unsigned int mechanism )
{
	m_mechanism = mechanism;
}

void Job::setSpell( QString spell )
{
	m_spell = spell;
}

QString Job::spell() const
{
	return m_spell;
}

void Job::setMayTrap( bool value )
{
	m_mayTrap = value;
}

bool Job::mayTrap() const
{
	return m_mayTrap;
}

void Job::raisePrio()
{
	m_priority = qMin( 9, m_priority + 1 );
}

void Job::lowerPrio()
{
	m_priority = qMax( 0, m_priority - 1 );
}

int Job::priority() const
{
	return m_priority;
}

void Job::setCraftID( QString craftID )
{
	m_craftID = craftID;
}

QString Job::craftID() const
{
	return m_craftID;
}

void Job::setCraftJobID( unsigned int craftJobID )
{
	m_craftJobID = craftJobID;
}
	
unsigned int Job::craftJobID() const
{
	return m_craftJobID;
}

void Job::setDestroyOnAbort( bool value )
{
	m_destroyOnAbort = value;
}

bool Job::destroyOnAbort() const
{
	return m_destroyOnAbort;
}

void Job::setPrio( int prio )
{
	m_priority = qMin( 9, qMax( 0, prio ) );
}