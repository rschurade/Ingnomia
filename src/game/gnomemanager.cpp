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

GnomeManager::GnomeManager( Game* parent ) :
	g( parent ),
	QObject( parent )
{
	loadProfessions();
}

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

void GnomeManager::addGnome( Position pos )
{
	GnomeFactory gf( g );
	m_gnomes.push_back( gf.createGnome( pos ) );
	m_gnomesByID.insert( m_gnomes.last()->id(), m_gnomes.last() );
}

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

void GnomeManager::addAutomaton( Automaton* a )
{
	m_automatons.append( a );
	m_gnomesByID.insert( a->id(), m_automatons.last() );

	//a->setSpriteID( Global::sf().setAutomatonSprite( a->id(), g->m_inv->spriteID( a->automatonItem() ) ) );
	//a->updateSprite();
}

void GnomeManager::addAutomaton( QVariantMap values )
{
	Automaton* a = new Automaton( values, g );
	m_automatons.append( a );
	m_gnomesByID.insert( a->id(), m_automatons.last() );

	//a->setSpriteID( Global::sf().setAutomatonSprite( a->id(), g->m_inv->spriteID( a->automatonItem() ) ) );
	//a->updateSprite();
}

void GnomeManager::addGnome( QVariantMap values )
{
	GnomeFactory gf( g );
	Gnome* gn( gf.createGnome( values ) );
	m_gnomes.push_back( gn );
	m_gnomesByID.insert( gn->id(), m_gnomes.last() );
}

void GnomeManager::addTrader( QVariantMap values )
{
	GnomeFactory gf( g );
	GnomeTrader* gt( gf.createGnomeTrader( values ) );
	m_specialGnomes.push_back( gt );
	m_gnomesByID.insert( gt->id(), m_specialGnomes.last() );
}

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

Gnome* GnomeManager::gnome( unsigned int gnomeID )
{
	if ( m_gnomesByID.contains( gnomeID ) )
	{
		return m_gnomesByID[gnomeID];
	}
	return nullptr;
}

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

Automaton* GnomeManager::automaton( unsigned int automatonID )
{
	if ( m_gnomesByID.contains( automatonID ) )
	{
		return dynamic_cast<Automaton*>( m_gnomesByID[automatonID] );
	}
	return nullptr;
}

QList<Gnome*> GnomeManager::gnomesSorted()
{
	QList<Gnome*> out = gnomes();
	std::sort( out.begin(), out.end(), CreatureCompare() );
	return out;
}

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
	IO::saveFile( QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/My Games/Ingnomia/settings/profs.json", sd );
}

void GnomeManager::loadProfessions()
{
	QJsonDocument sd;
	if ( !IO::loadFile( QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/My Games/Ingnomia/settings/profs.json", sd ) )
	{
		// if it doesn't exist get from /content/JSON
		if ( IO::loadFile( Global::cfg->get( "dataPath" ).toString() + "/JSON/profs.json", sd ) )
		{
			IO::saveFile( QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/My Games/Ingnomia/settings/profs.json", sd );
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

QStringList GnomeManager::professions()
{
	return m_profs.keys();
}

QStringList GnomeManager::professionSkills( QString profession )
{
	if ( m_profs.contains( profession ) )
	{
		return m_profs.value( profession );
	}
	return QStringList();
}

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

void GnomeManager::addProfession( QString name, QStringList skills )
{
	if ( !m_profs.contains( name ) )
	{
		m_profs.insert( name, skills );
		saveProfessions();
	}
}

void GnomeManager::removeProfession( QString name )
{
	if ( name == "Gnomad" )
		return;

	m_profs.remove( name );
	saveProfessions();
}

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

bool GnomeManager::gnomeCanReach( unsigned int gnomeID, Position pos )
{
	if ( m_gnomesByID.contains( gnomeID ) )
	{
		return g->m_world->regionMap().checkConnectedRegions( m_gnomesByID[gnomeID]->getPos(), pos );
	}
	return false;
}

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

void GnomeManager::setInMission( unsigned int gnomeID, unsigned int missionID )
{
	if ( m_gnomesByID.contains( gnomeID ) )
	{
		m_gnomesByID[gnomeID]->setMission( missionID );
		m_gnomesByID[gnomeID]->setOnMission( true );
	}
}

QString GnomeManager::name( unsigned int gnomeID )
{
	if ( m_gnomesByID.contains( gnomeID ) )
	{
		return m_gnomesByID[gnomeID]->name();
	}
	return "*no name*";
}

unsigned int GnomeManager::roleID( unsigned int gnomeID )
{
	if ( m_gnomesByID.contains( gnomeID ) )
	{
		return m_gnomesByID[gnomeID]->roleID();
	}
	return 0;
}
	
void GnomeManager::setRoleID( unsigned int gnomeID, unsigned int roleID )
{
	if ( m_gnomesByID.contains( gnomeID ) )
	{
		return m_gnomesByID[gnomeID]->setRole( roleID );
	}
}

int GnomeManager::numGnomes()
{
	return m_gnomes.size();
}