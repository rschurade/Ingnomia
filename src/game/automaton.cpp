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
/** @file automaton.cpp
 *  @brief Mechanical gnome variant powered by interchangeable fuel cores.
 */
#include "automaton.h"
#include "game.h"

#include "../base/db.h"
#include "../base/global.h"
#include "../game/inventory.h"
#include "../gfx/spritefactory.h"

#include <QDebug>

/// @brief Constructs a new automaton at @p pos from the given item definition.
/// @param pos         World position to place the automaton.
/// @param automatonItem UID of the physical automaton item that defines body materials.
/// @param game        Owning game instance.
Automaton::Automaton( Position pos, unsigned int automatonItem, Game* game ) :
	Gnome( pos, "Automaton", Gender::UNDEFINED, game ),
	m_automatonItem( automatonItem )
{
	m_type = CreatureType::AUTOMATON;

	init();
}

/// @brief Deserialising constructor — restores automaton state from a saved map.
/// @param in   Variant map produced by a previous serialize() call.
/// @param game Owning game instance.
Automaton::Automaton( QVariantMap& in, Game* game ) :
	Gnome( in, game )
{
	m_type = CreatureType::AUTOMATON;

	m_core            = in.value( "Core" ).toUInt();
	m_fuel            = in.value( "Fuel" ).toFloat();
	m_automatonItem   = in.value( "AutoItem" ).toInt();
	m_coreType        = in.value( "CoreType" ).toString();
	m_uninstallCore   = in.value( "UninstallFlag" ).toBool();
	unsigned int maintenaceJobID = in.value( "MaintenanceJob" ).toUInt();
	if( maintenaceJobID )
	{
		m_maintenaceJob = game->jm()->getJob( maintenaceJobID );
	}

	m_maintJobChanged = in.value( "MaintJobChanged" ).toBool();
	m_refuel          = in.value( "Refuel" ).toBool();

	init();
}

/// @brief Serialises automaton-specific state into @p out (delegates base state to Gnome::serialize).
/// @param out Map to receive the serialised key-value pairs.
void Automaton::serialize( QVariantMap& out )
{
	Gnome::serialize( out );

	out.insert( "Core", m_core );
	out.insert( "Fuel", m_fuel );
	out.insert( "AutoItem", m_automatonItem );
	out.insert( "CoreType", m_coreType );
	out.insert( "UninstallFlag", m_uninstallCore );
	out.insert( "MaintenanceJob", maintenanceJobID() );
	out.insert( "MaintJobChanged", m_maintJobChanged );
	out.insert( "Refuel", m_refuel );
}

/// @brief Destructor.
Automaton::~Automaton()
{
}

/// @brief Initialises the automaton: loads the behavior tree from the installed core,
///        removes biological needs, and replaces them with a Fuel need.
void Automaton::init()
{
	initTaskMap();

	if ( m_core )
	{
		QString itemSID = g->inv()->itemSID( m_core );

		auto row = DB::selectRow( "Automaton_Cores", itemSID );
		loadBehaviorTree( row.value( "BehaviorTree" ).toString() );

		if ( m_btBlackBoard.contains( "State" ) )
		{
			QVariantMap btm = m_btBlackBoard.value( "State" ).toMap();
			if ( m_behaviorTree )
			{
				m_behaviorTree->deserialize( btm );
				m_btBlackBoard.remove( "State" );
			}
		}
	}
	m_needs.remove( "Sleep" );
	m_needs.remove( "Hunger" );
	m_needs.remove( "Thirst" );
	m_needs.remove( "Happiness" );
	m_needs.insert( "Fuel", m_fuel );

	/*
	m_skills.clear();
	m_skillActive.clear();
	m_skillPriorities.clear();
	*/
	updateSprite();
}

/// @brief Rebuilds the automaton's visual sprite from its body-part definitions and component materials.
///        Eye and flame parts are only included when fuel is above zero.
void Automaton::updateSprite()
{
	QString material = g->inv()->materialSID( m_automatonItem );

	auto components = g->inv()->components( m_automatonItem );
	QMap<QString, QString> compMats;
	for ( auto vcomp : components )
	{
		auto comp = vcomp.toMap();
		auto it = comp.value( "ItSID" ).toString();
		auto ma = comp.value( "MaSID" ).toString();

		if ( it.endsWith( "Leg" ) || it.endsWith( "Arm" ) )
		{
			if ( compMats.contains( it + "Left" ) )
			{
				compMats.insert( it + "Right", ma );
			}
			else
			{
				compMats.insert( it + "Left", ma );
			}
		}
		else
		{
			compMats.insert( it, ma );
		}
	}
	QString itemSID = g->inv()->itemSID( m_automatonItem );
	auto parts      = DB::selectRows( "Creature_Parts", itemSID );

	QVariantMap ordered;
	for ( auto pm : parts )
	{
		ordered.insert( pm.value( "Order" ).toString(), pm );
	}

	QVariantMap randTemp;

	QVariantList def;
	for ( auto vpm : ordered )
	{
		auto pm = vpm.toMap();
		if ( pm.value( "BaseSprite" ).toString().startsWith( "#" ) )
		{
			QString bsa = pm.value( "BaseSprite" ).toString();
			bsa.remove( 0, 1 );
			auto bsl = bsa.split( "|" );

			srand( std::chrono::system_clock::now().time_since_epoch().count() );
			int rn = rand() % bsl.size();
			randTemp.insert( pm.value( "Part" ).toString() + "Rand", rn );
			pm.insert( "BaseSprite", bsl[rn] );
		}
		QString idPart = pm.value( "ID" ).toString() + pm.value( "Part" ).toString();
		if ( compMats.contains( idPart ) )
		{
			pm.insert( "Material", compMats.value( idPart ) );
		}
		else
		{
			pm.insert( "Material", material );
		}
		if ( idPart == "AutomatonEye" )
		{
			if ( m_fuel > 0 )
			{
				def.append( pm );
			}
		}
		else
		{
			def.append( pm );
		}
	}

	auto partsBack = DB::selectRows( "Creature_Parts", itemSID + "Back" );

	QVariantMap orderedBack;
	for ( auto pm : partsBack )
	{
		orderedBack.insert( pm.value( "Order" ).toString(), pm );
	}

	QVariantList defBack;
	for ( auto vpm : orderedBack )
	{
		auto pm = vpm.toMap();
		if ( pm.value( "BaseSprite" ).toString().startsWith( "#" ) )
		{
			QString bsa = pm.value( "BaseSprite" ).toString();
			bsa.remove( 0, 1 );
			auto bsl = bsa.split( "|" );

			srand( std::chrono::system_clock::now().time_since_epoch().count() );
			int rn = randTemp.value( pm.value( "Part" ).toString() + "Rand" ).toInt();
			pm.insert( "BaseSprite", bsl[rn] + "Back" );
		}
		QString aid = pm.value( "ID" ).toString();
		aid.chop( 4 );
		QString idPart = aid + pm.value( "Part" ).toString();

		if ( compMats.contains( idPart ) )
		{
			pm.insert( "Material", compMats.value( idPart ) );
		}
		else
		{
			pm.insert( "Material", material );
		}

		if ( idPart == "AutomatonFlame" )
		{
			if ( m_fuel > 0 )
			{
				defBack.append( pm );
			}
		}
		else
		{
			defBack.append( pm );
		}
	}

	m_spriteID = g->sf()->setCreatureSprite( m_id, def, defBack, isDead() )->uID;

	m_renderParamsChanged = true;
}

/// @brief Per-tick update: consumes one unit of fuel, runs the behavior tree, and moves the automaton.
/// @param tickNumber   Current absolute game tick.
/// @param seasonChanged True if the season changed this tick.
/// @param dayChanged    True if the day changed this tick.
/// @param hourChanged   True if the hour changed this tick.
/// @param minuteChanged True if the minute changed this tick.
/// @return NOFUEL if out of fuel; NOFLOOR if standing over void; JOBCHANGED if job changed; OK otherwise.
CreatureTickResult Automaton::onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	if ( m_fuel <= 0 )
	{
		return CreatureTickResult::NOFUEL;
	}
	--m_fuel;
	if ( m_fuel <= 0 )
	{
		updateSprite();
	}

	m_needs.insert( "Fuel", m_fuel );

	processCooldowns( tickNumber );

	m_jobChanged    = false;
	Position oldPos = m_position;

	if ( checkFloor() )
	{
		m_lastOnTick = tickNumber;
		return CreatureTickResult::NOFLOOR;
	}

	if ( m_job && ( m_job->isAborted() || m_job->isCanceled() ) )
	{
		cleanUpJob( false );
		m_behaviorTree->halt();
	}

	if ( m_behaviorTree )
	{
		m_behaviorTree->tick();
	}

	move( oldPos );
	updateLight( oldPos, m_position );

	m_lastOnTick = tickNumber;

	if ( m_jobChanged )
	{
		return CreatureTickResult::JOBCHANGED;
	}
	return CreatureTickResult::OK;
}

/// @brief Installs a new fuel core into the automaton.
///        If a core is already installed it is dropped at the automaton's current position first.
///        Loads the behavior tree and skills defined by the new core's DB row.
/// @param itemID UID of the core item to install, or 0 to only remove the existing core.
void Automaton::installCore( unsigned int itemID )
{
	if ( m_core )
	{
		//drop existing core
		g->inv()->setInJob( m_core, 0 );
		g->inv()->putDownItem( m_core, m_position );
		m_core = 0;

		m_behaviorTree.reset();

		m_skills.clear();
		m_skillActive.clear();
		m_skillPriorities.clear();

		m_uninstallCore = false;
		m_coreType      = "";
	}
	//install core
	if ( itemID )
	{
		QString itemSID = g->inv()->itemSID( itemID );
		if ( itemSID.startsWith( "AutomatonCore" ) )
		{
			m_core = itemID;
			g->inv()->pickUpItem( itemID, m_id );

			auto row = DB::selectRow( "Automaton_Cores", itemSID );

			loadBehaviorTree( row.value( "BehaviorTree" ).toString() );
			for ( auto row2 : DB::selectRows( "Automaton_Cores_Skills", itemSID ) )
			{
				QString skillID = row2.value( "SkillID" ).toString();
				int value       = row2.value( "SkillValue" ).toInt();

				m_skills.insert( skillID, value );
				m_skillActive.insert( skillID, true );
				m_skillPriorities.append( skillID );
			}
		}
	}
}

/// @brief Returns the UID of the currently installed core item, or 0 if none.
/// @return Installed core item UID.
unsigned int Automaton::coreItem()
{
	return m_core;
}

/// @brief Returns whether the automaton should seek refueling when fuel is low.
/// @return True if auto-refuel is enabled.
bool Automaton::getRefuelFlag()
{
	return m_refuel;
}

/// @brief Sets whether the automaton should seek refueling when fuel is low.
/// @param flag True to enable auto-refuel.
void Automaton::setRefuelFlag( bool flag )
{
	m_refuel = flag;
}

/// @brief Sets the desired core type SID.  If the type is cleared while one was set,
///        also raises the uninstall flag so the current core gets removed.
/// @param coreSID Core type string ID, or empty to clear/uninstall.
void Automaton::setCoreType( QString coreSID )
{
	if( !m_coreType.isEmpty() && coreSID.isEmpty() )
	{
		m_uninstallCore = true;
	}
	m_coreType        = coreSID;
	m_maintJobChanged = true;
}

/// @brief Returns the desired core type SID.
/// @return Core type string ID.
QString Automaton::coreType()
{
	return m_coreType;
}

/// @brief Requests or cancels a core uninstall.  Marks the maintenance job as changed.
/// @param uninstall True to schedule uninstall, false to cancel.
void Automaton::uninstallCore( bool uninstall )
{
	m_uninstallCore   = uninstall;
	m_maintJobChanged = true;
}

/// @brief Returns whether a core uninstall has been requested.
/// @return True if uninstall is pending.
bool Automaton::uninstallFlag()
{
	return m_uninstallCore;
}

/// @brief Assigns a maintenance job (core swap / refuel) and marks job-changed.
/// @param job Shared pointer to the maintenance job.
void Automaton::setMaintenanceJob( QSharedPointer<Job> job )
{
	m_maintenaceJob   = job;
	m_maintJobChanged = true;
}

/// @brief Returns the ID of the currently assigned maintenance job, or 0 if none.
/// @return Maintenance job ID.
unsigned int Automaton::maintenanceJobID()
{
	if( m_maintenaceJob )
	{
		auto job = m_maintenaceJob.toStrongRef();
		return job->id();
	}
	return 0;
}

/// @brief Consumes and returns the maintenance-job-changed flag.
/// @return True if the maintenance job changed since the last call; resets the flag.
bool Automaton::maintenanceJobChanged()
{
	bool out          = m_maintJobChanged;
	m_maintJobChanged = false;
	return out;
}

/// @brief Returns the current fuel level.
/// @return Remaining fuel units.
int Automaton::getFuelLevel()
{
	return m_fuel;
}

/// @brief Sets the fuel level to @p burnValue and refreshes the sprite.
/// @param burnValue New fuel amount (maximum capacity of the installed core).
void Automaton::fillUp( int burnValue )
{
	m_fuel = burnValue;
	updateSprite();
}