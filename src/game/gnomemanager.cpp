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
/** @file gnomemanager.cpp
 *  Implementation of GnomeManager for ticking, spawning, profession management, and gnome queries.
 */
#include "gnomemanager.h"
#include "game.h"

#include "../base/config.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/io.h"
#include "../game/creature.h"
#include "../game/gnomefactory.h"
#include "../game/gnometrader.h"
#include "../game/inventory.h"
#include "../game/jobmanager.h"
#include "../game/militarymanager.h"
#include "../game/world.h"
#include "../gfx/spritefactory.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QStandardPaths>

/** @brief Constructs the gnome manager and loads profession definitions.
 *  @param parent Pointer to the owning Game instance.
 */
GnomeManager::GnomeManager( Game* parent ) :
	g( parent ),
	QObject( parent )
{
	loadProfessions();
}

/** @brief Destructor. Deletes all owned gnome, special gnome, dead gnome, and automaton instances. */
GnomeManager::~GnomeManager()
{
	for ( const auto& gnome : m_gnomes )
	{
		delete gnome;
	}
	for ( const auto& gnome : m_specialGnomes )
	{
		delete gnome;
	}
	for ( const auto& gnome : m_deadGnomes )
	{
		delete gnome;
	}
	for ( const auto& gnome : m_automatons )
	{
		delete gnome;
	}
}

/** @brief Checks whether a gnome with the given ID exists in the active gnome list.
 *  @param gnomeID The gnome's unique ID.
 *  @return True if the gnome is found.
 */
bool GnomeManager::contains( unsigned int gnomeID )
{
	for( const auto& gnome : m_gnomes )
	{
		if( gnome->id() == gnomeID )
		{
			return true;
		}
	}
	return false;
}

/** @brief Creates a new gnome at the given position and adds it to the manager.
 *  @param pos World position to spawn the gnome.
 */
void GnomeManager::addGnome( Position pos )
{
	GnomeFactory gf( g );
	m_gnomes.push_back( gf.createGnome( pos ) );
	m_gnomesByID.insert( m_gnomes.last()->id(), m_gnomes.last() );
}

/** @brief Creates a trader gnome at the given position, assigns it to a market stall, and loads its trade inventory.
 *  @param pos World position to spawn the trader.
 *  @param workshopID ID of the market stall workshop.
 *  @param type Trader type ID used to look up available trade items in the DB.
 *  @return The unique ID of the created trader gnome.
 */
unsigned int GnomeManager::addTrader( Position pos, unsigned int workshopID, QString type )
{
	GnomeFactory gf( g );
	GnomeTrader* gnome = gf.createGnomeTrader( pos );
	gnome->setName( "Trader " + gnome->name() );
	gnome->setMarketStall( workshopID );

	auto rows = DB::selectRows( "Traders_Items", type );

	if ( rows.size() )
	{
		QVariantMap vmTrader;
		vmTrader.insert( "ID", type );
		QVariantList items;
		for ( auto row : rows )
		{
			items.append( row );
		}
		vmTrader.insert( "Items", items );
		gnome->setTraderDefinition( vmTrader );
	}

	m_specialGnomes.push_back( gnome );
	m_gnomesByID.insert( gnome->id(), m_specialGnomes.last() );
	return gnome->id();
}

/** @brief Registers an existing automaton with the manager.
 *  @param a Pointer to the Automaton to add.
 */
void GnomeManager::addAutomaton( Automaton* a )
{
	m_automatons.append( a );
	m_gnomesByID.insert( a->id(), m_automatons.last() );

	//a->setSpriteID( Global::sf().setAutomatonSprite( a->id(), g->m_inv->spriteID( a->automatonItem() ) ) );
	//a->updateSprite();
}

/** @brief Creates and registers an automaton from serialized save data.
 *  @param values QVariantMap containing the saved automaton state.
 */
void GnomeManager::addAutomaton( QVariantMap values )
{
	Automaton* a = new Automaton( values, g );
	m_automatons.append( a );
	m_gnomesByID.insert( a->id(), m_automatons.last() );

	//a->setSpriteID( Global::sf().setAutomatonSprite( a->id(), g->m_inv->spriteID( a->automatonItem() ) ) );
	//a->updateSprite();
}

/** @brief Restores a gnome from serialized save data and adds it to the manager.
 *  @param values QVariantMap containing the saved gnome state.
 */
void GnomeManager::addGnome( QVariantMap values )
{
	GnomeFactory gf( g );
	Gnome* gn( gf.createGnome( values ) );
	m_gnomes.push_back( gn );
	m_gnomesByID.insert( gn->id(), m_gnomes.last() );
}

/** @brief Restores a trader gnome from serialized save data and adds it to the special gnomes list.
 *  @param values QVariantMap containing the saved trader gnome state.
 */
void GnomeManager::addTrader( QVariantMap values )
{
	GnomeFactory gf( g );
	GnomeTrader* gt( gf.createGnomeTrader( values ) );
	m_specialGnomes.push_back( gt );
	m_gnomesByID.insert( gt->id(), m_specialGnomes.last() );
}

/** @brief Advances all gnomes, special gnomes, and automatons by one game tick. Handles death, cleanup of expired corpses, and automaton job creation.
 *  @param tickNumber Current game tick number.
 *  @param seasonChanged Whether the season changed this tick.
 *  @param dayChanged Whether the day changed this tick.
 *  @param hourChanged Whether the hour changed this tick.
 *  @param minuteChanged Whether the minute changed this tick.
 */
void GnomeManager::onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	QElapsedTimer timer;
	timer.start();

	//create possible automaton jobs;
	createJobs();

	if ( m_startIndex >= m_gnomes.size() )
	{
		m_startIndex = 0;
	}
	QList<unsigned int> deadGnomes;
	QList<unsigned int> deadOrGoneSpecial;
	for ( int i = m_startIndex; i < m_gnomes.size(); ++i )
	{
		Gnome* gn = m_gnomes[i];
#ifdef CHECKTIME
		QElapsedTimer timer2;
		timer2.start();

		CreatureTickResult tr = g->onTick( tickNumber, seasonChanged, dayChanged, hourChanged, minuteChanged );

		auto elapsed = timer2.elapsed();
		if ( elapsed > 100 )
		{
			qDebug() << g->name() << "just needed" << elapsed << "ms for tick";
			Global::cfg->set( "Pause", true );
			return;
		}
#else
		CreatureTickResult tr = gn->onTick( tickNumber, seasonChanged, dayChanged, hourChanged, minuteChanged );
#endif
		m_startIndex = i + 1;
		switch ( tr )
		{
			case CreatureTickResult::DEAD:
				deadGnomes.append( gn->id() );
				break;
			case CreatureTickResult::OK:
				break;
			case CreatureTickResult::JOBCHANGED:
				emit signalGnomeActivity( gn->id(), gn->getActivity() );
				break;
			case CreatureTickResult::TODESTROY:
				break;
			case CreatureTickResult::NOFLOOR:
				break;
			case CreatureTickResult::LEFTMAP:
				break;
		}

		if ( timer.elapsed() > 5 )
		{
			break;
		}
	}

	for ( auto& gnome : m_specialGnomes )
	{
		CreatureTickResult tr = gnome->onTick( tickNumber, seasonChanged, dayChanged, hourChanged, minuteChanged );
		if ( tr == CreatureTickResult::DEAD || tr == CreatureTickResult::LEFTMAP )
		{
			deadOrGoneSpecial.append( gnome->id() );
		}
	}

	for ( auto& automaton : m_automatons )
	{
		CreatureTickResult tr = automaton->onTick( tickNumber, seasonChanged, dayChanged, hourChanged, minuteChanged );
		switch ( tr )
		{
			case CreatureTickResult::NOFUEL:
				break;
			case CreatureTickResult::NOCORE:
				break;
		}
	}

	if ( deadGnomes.size() )
	{
		for ( auto gid : deadGnomes )
		{
			for ( int i = 0; i < m_gnomes.size(); ++i )
			{
				if ( gid == m_gnomes[i]->id() )
				{
					Gnome* dg = m_gnomes[i];
					m_deadGnomes.append( dg );
					m_gnomesByID.insert( dg->id(), m_deadGnomes.last() );
					m_gnomes.removeAt( i );
					g->mil()->removeGnome( gid );
					emit signalGnomeDeath( dg->id() );
					break;
				}
			}
		}
	}
	if ( deadOrGoneSpecial.size() )
	{
		for ( auto gid : deadOrGoneSpecial )
		{
			for ( int i = 0; i < m_specialGnomes.size(); ++i )
			{
				if ( gid == m_specialGnomes[i]->id() )
				{
					Gnome* dg = m_specialGnomes[i];
					if ( dg->isDead() )
					{
						m_deadGnomes.append( dg );
						m_gnomesByID.insert( dg->id(), m_deadGnomes.last() );
					}
					else
					{
						m_gnomesByID.remove( dg->id() );
						g->m_world->addToUpdateList( dg->getPos() );
						delete dg;
					}
					m_specialGnomes.removeAt( i );
					break;
				}
			}
		}
	}

	if ( m_deadGnomes.size() )
	{
		for ( int i = 0; i < m_deadGnomes.size(); ++i )
		{
			Gnome* dg = m_deadGnomes[i];
			if ( dg->expires() < GameState::tick )
			{
				m_gnomesByID.remove( dg->id() );
				g->m_world->addToUpdateList( dg->getPos() );
				m_deadGnomes.removeAt( i );
				delete dg;
				break;
			}
		}
	}
}

/** @brief Forces all gnomes at a given position to move to a new position, aborting their current jobs.
 *  @param from The source position.
 *  @param to The destination position.
 */
void GnomeManager::forceMoveGnomes( Position from, Position to )
{
	for ( auto& gn : m_gnomes )
	{
		// check gnome position
		if ( gn->getPos().toInt() == from.toInt() )
		{
			//qDebug() << "force move gnome from " << from.toString() << " to " << to.toString();
			// move gnome
			gn->forceMove( to );
			// abort job if he has one
			gn->setJobAborted( "GnomeManager" );
		}
	}
}

/** @brief Returns all gnomes (living, special, automatons, dead) at a given position.
 *  @param pos The world position to query.
 *  @return List of Gnome pointers at that position.
 */
QList<Gnome*> GnomeManager::gnomesAtPosition( Position pos )
{
	QList<Gnome*> out;
	for ( int i = 0; i < m_gnomes.size(); ++i )
	{
		if ( m_gnomes[i]->getPos() == pos && !m_gnomes[i]->goneOffMap() )
		{
			out.push_back( m_gnomes[i] );
		}
	}
	for ( int i = 0; i < m_specialGnomes.size(); ++i )
	{
		if ( m_specialGnomes[i]->getPos() == pos )
		{
			out.push_back( m_specialGnomes[i] );
		}
	}
	for ( int i = 0; i < m_automatons.size(); ++i )
	{
		if ( m_automatons[i]->getPos() == pos )
		{
			out.push_back( m_automatons[i] );
		}
	}
	for ( int i = 0; i < m_deadGnomes.size(); ++i )
	{
		if ( m_deadGnomes[i]->getPos() == pos )
		{
			out.push_back( m_deadGnomes[i] );
		}
	}
	return out;
}

/** @brief Returns all dead gnomes at a given position.
 *  @param pos The world position to query.
 *  @return List of dead Gnome pointers at that position.
 */
QList<Gnome*> GnomeManager::deadGnomesAtPosition( Position pos )
{
	QList<Gnome*> out;
	for ( int i = 0; i < m_deadGnomes.size(); ++i )
	{
		if ( m_deadGnomes[i]->getPos() == pos )
		{
			out.push_back( m_deadGnomes[i] );
		}
	}
	return out;
}

/** @brief Looks up a gnome by its unique ID (searches all gnome lists).
 *  @param gnomeID The gnome's unique ID.
 *  @return Pointer to the Gnome, or nullptr if not found.
 */
Gnome* GnomeManager::gnome( unsigned int gnomeID )
{
	if ( m_gnomesByID.contains( gnomeID ) )
	{
		return m_gnomesByID[gnomeID];
	}
	return nullptr;
}

/** @brief Looks up a trader gnome by its unique ID in the special gnomes list.
 *  @param traderID The trader's unique ID.
 *  @return Pointer to the GnomeTrader, or nullptr if not found.
 */
GnomeTrader* GnomeManager::trader( unsigned int traderID )
{
	if ( m_gnomesByID.contains( traderID ) )
	{
		for ( int i = 0; i < m_specialGnomes.size(); ++i )
		{
			if ( m_specialGnomes[i]->id() == traderID )
			{
				return dynamic_cast<GnomeTrader*>( m_specialGnomes[i] );
			}
		}
	}
	return nullptr;
}

/** @brief Looks up an automaton by its unique ID.
 *  @param automatonID The automaton's unique ID.
 *  @return Pointer to the Automaton, or nullptr if not found.
 */
Automaton* GnomeManager::automaton( unsigned int automatonID )
{
	if ( m_gnomesByID.contains( automatonID ) )
	{
		return dynamic_cast<Automaton*>( m_gnomesByID[automatonID] );
	}
	return nullptr;
}

/** @brief Returns the list of active gnomes sorted by the default creature comparator.
 *  @return Sorted copy of the gnome list.
 */
QList<Gnome*> GnomeManager::gnomesSorted()
{
	QList<Gnome*> out = gnomes();
	std::sort( out.begin(), out.end(), CreatureCompare() );
	return out;
}

/** @brief Serializes profession definitions to the user settings profs.json file. */
void GnomeManager::saveProfessions()
{
	QVariantList pl;
	for ( auto key : m_profs.keys() )
	{
		QVariantMap pm;
		pm.insert( "Name", key );
		pm.insert( "Skills", m_profs[key] );
		pl.append( pm );
	}

	QJsonDocument sd = QJsonDocument::fromVariant( pl );
	IO::saveFile( IO::getDataFolder() + "/settings/profs.json", sd );
}

/** @brief Loads profession definitions from the user settings file, falling back to content/JSON defaults. Ensures the "Gnomad" base profession always exists. */
void GnomeManager::loadProfessions()
{
	QJsonDocument sd;
	if ( !IO::loadFile( IO::getDataFolder() + "/settings/profs.json", sd ) )
	{
		// if it doesn't exist get from /content/JSON
		if ( IO::loadFile( Global::cfg->get( "dataPath" ).toString() + "/JSON/profs.json", sd ) )
		{
			IO::saveFile( IO::getDataFolder() + "/settings/profs.json", sd );
		}
		else
		{
			qDebug() << "Unable to find profession config!";
			return;
		}
	}
	auto profList = sd.toVariant().toList();

	m_profs.clear();

	for ( auto vprof : profList )
	{
		QString name       = vprof.toMap().value( "Name" ).toString();
		QStringList skills = vprof.toMap().value( "Skills" ).toStringList();
		m_profs.insert( name, skills );
	}

	if ( !m_profs.contains( "Gnomad" ) )
	{
		QStringList skills = DB::ids( "Skills" );
		m_profs.insert( "Gnomad", skills );
		saveProfessions();
	}
}

/** @brief Returns the list of all defined profession names.
 *  @return List of profession name strings.
 */
QStringList GnomeManager::professions()
{
	return m_profs.keys();
}

/** @brief Returns the list of skill IDs associated with a profession.
 *  @param profession The profession name to query.
 *  @return List of skill ID strings, or empty list if profession not found.
 */
QStringList GnomeManager::professionSkills( QString profession )
{
	if ( m_profs.contains( profession ) )
	{
		return m_profs.value( profession );
	}
	return QStringList();
}

/** @brief Creates a new profession with a unique auto-generated name and no skills.
 *  @return The name of the newly created profession.
 */
QString GnomeManager::addProfession()
{
	QString name = "NewProfession";
	
	if( !m_profs.contains( name ) )
	{
		m_profs.insert( name, QStringList() );
		saveProfessions();
		return name;
	}
	int suffixNumber = 1;

	while( m_profs.contains( name + QString::number( suffixNumber ) ) )
	{
		++suffixNumber;
	}
	name += QString::number( suffixNumber );
	m_profs.insert( name, QStringList() );
	saveProfessions();
	return name;
}

/** @brief Adds a profession with a specific name and skill list, if it does not already exist.
 *  @param name The profession name.
 *  @param skills List of skill IDs for this profession.
 */
void GnomeManager::addProfession( QString name, QStringList skills )
{
	if ( !m_profs.contains( name ) )
	{
		m_profs.insert( name, skills );
		saveProfessions();
	}
}

/** @brief Removes a profession by name. The "Gnomad" base profession cannot be removed.
 *  @param name The profession name to remove.
 */
void GnomeManager::removeProfession( QString name )
{
	if ( name == "Gnomad" )
		return;

	m_profs.remove( name );
	saveProfessions();
}

/** @brief Renames a profession and/or updates its skill list. The "Gnomad" profession cannot be modified.
 *  @param name Current profession name.
 *  @param newName New profession name (may be same as name).
 *  @param skills Updated list of skill IDs.
 */
void GnomeManager::modifyProfession( QString name, QString newName, QStringList skills )
{
	if ( name == "Gnomad" )
		return;

	m_profs.remove( name );

	if ( m_profs.contains( newName ) )
	{
		m_profs.insert( name, skills );
	}
	else
	{
		m_profs.insert( newName, skills );
	}

	saveProfessions();
}

/** @brief Checks whether a gnome can reach a given position via connected regions.
 *  @param gnomeID The gnome's unique ID.
 *  @param pos Target position.
 *  @return True if the gnome's region is connected to the target position.
 */
bool GnomeManager::gnomeCanReach( unsigned int gnomeID, Position pos )
{
	if ( m_gnomesByID.contains( gnomeID ) )
	{
		return g->m_world->regionMap().checkConnectedRegions( m_gnomesByID[gnomeID]->getPos(), pos );
	}
	return false;
}

/** @brief Creates maintenance jobs (refuel, install core, uninstall core) for automatons that need them. */
void GnomeManager::createJobs()
{
	for ( auto a : m_automatons )
	{
		if ( a->maintenanceJobID() == 0 )
		{
			// has core
			if ( a->coreItem() )
			{
				// remove core
				if ( a->uninstallFlag() )
				{
					getUninstallJob( a );
				}
				else if ( a->getRefuelFlag() && a->getFuelLevel() <= 0 )
				{
					getRefuelJob( a );
				}
			}
			else
			{
				// no core but core type is set, install core
				if ( !a->coreType().isEmpty() )
				{
					getInstallJob( a );
				}
			}
		}
	}
}

/** @brief Creates a refuel job for an automaton that has run out of fuel.
 *  @param a Pointer to the Automaton needing fuel.
 */
void GnomeManager::getRefuelJob( Automaton* a )
{
	auto jobID = g->jm()->addJob( "Refuel", a->getPos(), 0, true );
	auto job = g->jm()->getJob( jobID );
	if( job )
	{
		job->setAutomaton( a->id() );
		job->setRequiredSkill( "Machining" );
		job->addPossibleWorkPosition( a->getPos() );
		job->addRequiredItem( 1, "RawCoal", "any", {} );
		a->setMaintenanceJob( job );
	}
}

/** @brief Creates a core installation job for an automaton that has a core type set but no core installed.
 *  @param a Pointer to the Automaton needing a core.
 */
void GnomeManager::getInstallJob( Automaton* a )
{
	auto jobID = g->jm()->addJob( "Install", a->getPos(), 0, true );
	auto job = g->jm()->getJob( jobID );
	if( job )
	{
		job->setAutomaton( a->id() );
		job->setRequiredSkill( "Machining" );
		job->addPossibleWorkPosition( a->getPos() );
		job->addRequiredItem( 1, a->coreType(), "any", {} );
		a->setMaintenanceJob( job );
	}
}

/** @brief Creates a core uninstallation job for an automaton flagged for core removal.
 *  @param a Pointer to the Automaton whose core should be removed.
 */
void GnomeManager::getUninstallJob( Automaton* a )
{
	auto jobID = g->jm()->addJob( "Uninstall", a->getPos(), 0, true );
	auto job = g->jm()->getJob( jobID );
	if( job )
	{
		job->setAutomaton( a->id() );
		job->setRequiredSkill( "Machining" );
		job->addPossibleWorkPosition( a->getPos() );
		a->setMaintenanceJob( job );
	}
}

/** @brief Assigns a gnome to a military mission.
 *  @param gnomeID The gnome's unique ID.
 *  @param missionID The mission's unique ID.
 */
void GnomeManager::setInMission( unsigned int gnomeID, unsigned int missionID )
{
	if ( m_gnomesByID.contains( gnomeID ) )
	{
		m_gnomesByID[gnomeID]->setMission( missionID );
		m_gnomesByID[gnomeID]->setOnMission( true );
	}
}

/** @brief Returns the display name of a gnome by ID.
 *  @param gnomeID The gnome's unique ID.
 *  @return The gnome's name, or "*no name*" if not found.
 */
QString GnomeManager::name( unsigned int gnomeID )
{
	if ( m_gnomesByID.contains( gnomeID ) )
	{
		return m_gnomesByID[gnomeID]->name();
	}
	return "*no name*";
}

/** @brief Returns the military role ID assigned to a gnome.
 *  @param gnomeID The gnome's unique ID.
 *  @return The role ID, or 0 if not found.
 */
unsigned int GnomeManager::roleID( unsigned int gnomeID )
{
	if ( m_gnomesByID.contains( gnomeID ) )
	{
		return m_gnomesByID[gnomeID]->roleID();
	}
	return 0;
}
	
/** @brief Assigns a military role to a gnome.
 *  @param gnomeID The gnome's unique ID.
 *  @param roleID The role ID to assign.
 */
void GnomeManager::setRoleID( unsigned int gnomeID, unsigned int roleID )
{
	if ( m_gnomesByID.contains( gnomeID ) )
	{
		return m_gnomesByID[gnomeID]->setRole( roleID );
	}
}

/** @brief Returns the number of active (living) gnomes.
 *  @return Count of active gnomes.
 */
int GnomeManager::numGnomes()
{
	return m_gnomes.size();
}