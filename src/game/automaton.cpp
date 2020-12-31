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
#include "automaton.h"
#include "game.h"

#include "../base/db.h"
#include "../base/global.h"
#include "../game/inventory.h"
#include "../gfx/spritefactory.h"

#include <QDebug>

Automaton::Automaton( Position pos, unsigned int automatonItem, Game* game ) :
	Gnome( pos, "Automaton", Gender::UNDEFINED, game ),
	m_automatonItem( automatonItem )
{
	m_type = CreatureType::AUTOMATON;

	init();
}

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

Automaton::~Automaton()
{
}

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

unsigned int Automaton::coreItem()
{
	return m_core;
}

bool Automaton::getRefuelFlag()
{
	return m_refuel;
}

void Automaton::setRefuelFlag( bool flag )
{
	m_refuel = flag;
}

void Automaton::setCoreType( QString coreSID )
{
	if( !m_coreType.isEmpty() && coreSID.isEmpty() )
	{
		m_uninstallCore = true;
	}
	m_coreType        = coreSID;
	m_maintJobChanged = true;
}

QString Automaton::coreType()
{
	return m_coreType;
}

void Automaton::uninstallCore( bool uninstall )
{
	m_uninstallCore   = uninstall;
	m_maintJobChanged = true;
}

bool Automaton::uninstallFlag()
{
	return m_uninstallCore;
}

void Automaton::setMaintenanceJob( QSharedPointer<Job> job )
{
	m_maintenaceJob   = job;
	m_maintJobChanged = true;
}

unsigned int Automaton::maintenanceJobID()
{
	if( m_maintenaceJob )
	{
		auto job = m_maintenaceJob.toStrongRef();
		return job->id();
	}
	return 0;
}

bool Automaton::maintenanceJobChanged()
{
	bool out          = m_maintJobChanged;
	m_maintJobChanged = false;
	return out;
}

int Automaton::getFuelLevel()
{
	return m_fuel;
}

void Automaton::fillUp( int burnValue )
{
	m_fuel = burnValue;
	updateSprite();
}