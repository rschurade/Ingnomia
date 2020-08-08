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
#include "workshopmanager.h"

#include "../base/gamestate.h"

#include <QDebug>
#include <QJsonDocument>

WorkshopManager::WorkshopManager( QObject* parent ) :
	QObject( parent )
{
}

WorkshopManager::~WorkshopManager()
{
}

void WorkshopManager::reset()
{
	m_workshops.clear();
}

void WorkshopManager::onTick( quint64 tick )
{
	if ( !m_toDelete.isEmpty() )
	{
		int tds = m_toDelete.size();
		for ( int i = 0; i < tds; ++i )
		{
			unsigned int id = m_toDelete.dequeue();
			Workshop* ws    = workshop( id );
			if ( ws->canDelete() )
			{
				for ( int k = 0; k < m_workshops.size(); ++k )
				{
					if ( m_workshops[k]->id() == id )
					{
						m_workshops.removeAt( k );
						break;
					}
				}
			}
			else
			{
				m_toDelete.enqueue( id );
			}
		}
	}

	for ( auto&& w : m_workshops )
	{
		w->onTick( tick );
	}
}

Workshop* WorkshopManager::addWorkshop( QString type, Position& pos, int rotation )
{
	Workshop* w = new Workshop();
	w->init( type, pos, rotation );
	m_workshops.push_back( w );

	return m_workshops.last();
}

void WorkshopManager::addWorkshop( QVariantMap vals )
{
	QMutexLocker lock( &m_mutex );
	Workshop* w = new Workshop( vals );
	m_workshops.push_back( w );
}

bool WorkshopManager::isWorkshop( Position& pos )
{
	QMutexLocker lock( &m_mutex );
	for ( auto&& w : m_workshops )
	{
		if ( w->isOnTile( pos ) )
		{
			return true;
		}
	}
	return false;
}

Workshop* WorkshopManager::workshopAt( const Position& pos )
{
	QMutexLocker lock( &m_mutex );
	for ( auto&& w : m_workshops )
	{
		if ( w->isOnTile( pos ) )
		{
			return w;
		}
	}
	return 0;
}

Workshop* WorkshopManager::workshop( unsigned int ID )
{
	QMutexLocker lock( &m_mutex );
	for ( auto&& w : m_workshops )
	{
		if ( w->id() == ID )
		{
			return w;
		}
	}
	return 0;
}

unsigned int WorkshopManager::getJob( unsigned int gnomeID, QString skillID )
{
	QMutexLocker lock( &m_mutex );
	//first check if that gnome is assigned to a workshop
	for ( auto&& w : m_workshops )
	{
		if ( w->assignedGnome() == gnomeID )
		{
			unsigned int jobID = w->getJob( gnomeID, skillID );
			if ( jobID != 0 )
			{
				return jobID;
			}
		}
	}
	// now check all other workshops
	for ( auto&& w : m_workshops )
	{
		if ( w->assignedGnome() == 0 )
		{
			unsigned int jobID = w->getJob( gnomeID, skillID );
			if ( jobID != 0 )
			{
				return jobID;
			}
		}
	}
	return 0;
}

bool WorkshopManager::finishJob( unsigned int jobID )
{
	QMutexLocker lock( &m_mutex );
	for ( auto&& w : m_workshops )
	{
		if ( w->finishJob( jobID ) )
		{
			//GameState::addChange( NetworkCommand::WORKSHOPINFO, { QString::number( w->id() ), QJsonDocument::fromVariant( w->serializeJobQueue() ).toJson() } );
			emit signalJobListChanged( w->id() );
			return true;
		}
	}
	return false;
}

bool WorkshopManager::giveBackJob( unsigned int jobID )
{
	QMutexLocker lock( &m_mutex );
	for ( auto&& w : m_workshops )
	{
		if ( w->giveBackJob( jobID ) )
		{
			return true;
		}
	}
	return false;
}

Job* WorkshopManager::getJob( unsigned int jobID )
{
	QMutexLocker lock( &m_mutex );
	for ( auto&& w : m_workshops )
	{
		if ( w->hasJobID( jobID ) )
		{
			return w->getJob( jobID );
		}
	}
	return nullptr;
}

bool WorkshopManager::hasJobID( unsigned int jobID )
{
	QMutexLocker lock( &m_mutex );
	for ( auto&& w : m_workshops )
	{
		if ( w->hasJobID( jobID ) )
		{
			return true;
		}
	}
	return false;
}

void WorkshopManager::deleteWorkshop( unsigned int workshopID )
{
	m_toDelete.push_back( workshopID );
}

void WorkshopManager::autoGenCraftJob( QString itemSID, QString materialSID, int amount )
{
	//QMutexLocker lock( &m_mutex );
	for ( auto&& w : m_workshops )
	{
		if ( w->isAcceptingGenerated() )
		{
			if ( w->autoCraft( itemSID, materialSID, amount ) )
			{
				emit signalJobListChanged( w->id() );
				break;
			}
		}
	}
}

void WorkshopManager::setPriority( unsigned int workshopID, int prio )
{
	QMutexLocker lock( &m_mutex );
	int current = 0;
	for ( auto w : m_workshops )
	{
		if ( w->id() == workshopID )
		{
			break;
		}
		++current;
	}
	auto ws = m_workshops.takeAt( current );
	m_workshops.insert( prio, ws );
}

int WorkshopManager::priority( unsigned int workshopID )
{
	QMutexLocker lock( &m_mutex );
	int current = 0;
	for ( auto w : m_workshops )
	{
		if ( w->id() == workshopID )
		{
			break;
		}
		++current;
	}
	return current;
}

QList<Workshop*> WorkshopManager::getTrainingGrounds()
{
	QMutexLocker lock( &m_mutex );
	QList<Workshop*> out;
	for ( auto&& w : m_workshops )
	{
		if ( w->type() == "MeleeTraining" )
		{
			out.append( w );
		}
	}
	return out;
}
