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
#include "game.h"

#include "../base/gamestate.h"

#include <QDebug>
#include <QJsonDocument>

WorkshopManager::WorkshopManager( Game* parent ) :
	g( parent ),
	QObject( parent )
{
}

WorkshopManager::~WorkshopManager()
{
	for ( const auto& ws : m_workshops )
	{
		delete ws;
	}
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

	for ( auto& w : m_workshops )
	{
		w->onTick( tick );
	}
}

Workshop* WorkshopManager::addWorkshop( QString type, Position& pos, int rotation )
{
	Workshop* w = new Workshop( type, pos, rotation, g );
	m_workshops.push_back( w );

	return m_workshops.last();
}

void WorkshopManager::addWorkshop( QVariantMap vals )
{
	Workshop* w = new Workshop( vals, g );
	m_workshops.push_back( w );
}

bool WorkshopManager::isWorkshop( Position& pos )
{
	for ( const auto& w : m_workshops )
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
	for ( auto& w : m_workshops )
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
	for ( auto& w : m_workshops )
	{
		if ( w->id() == ID )
		{
			return w;
		}
	}
	return 0;
}

void WorkshopManager::deleteWorkshop( unsigned int workshopID )
{
	m_toDelete.push_back( workshopID );
}

bool WorkshopManager::autoGenCraftJob( QString itemSID, QString materialSID, int amount )
{
	if( !craftJobExists( itemSID, materialSID ) )
	{
		QList<Workshop*> possibles;
		for ( auto&& w : m_workshops )
		{
			if ( w->isAcceptingGenerated() )
			{
				if( w->canCraft( itemSID, materialSID ) )
				{
					possibles.append( w );
				}
			}
		}
		auto compare = []( Workshop* a, Workshop* b) { return a->numJobs() < b->numJobs(); };
		std::sort( std::begin( possibles ), std::end( possibles ), compare );

		for ( auto&& w : possibles )
		{
			if ( w->autoCraft( itemSID, materialSID, amount ) )
			{
				emit signalJobListChanged( w->id() );
				return true;
			}
		}
	}
	return false;
}

bool WorkshopManager::craftJobExists( const QString& itemSID, const QString& materialSID )
{
	for ( const auto& w : m_workshops )
	{
		if ( w->isAcceptingGenerated() )
		{
			if ( w->hasCraftJob( itemSID, materialSID ) )
			{
				return true;
			}
		}
	}
	return false;
}

void WorkshopManager::setPriority( unsigned int workshopID, int prio )
{
	if( prio > 0 && prio < m_workshops.size() )
	{
		int current = 0;
		for ( const auto& w : m_workshops )
		{
			if ( w->id() == workshopID )
			{
				break;
			}
			++current;
		}
		m_workshops.move( current, prio );
	}
}

int WorkshopManager::priority( unsigned int workshopID )
{
	int current = 0;
	for ( const auto& w : m_workshops )
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
	QList<Workshop*> out;
	for ( auto& w : m_workshops )
	{
		if ( w->type() == "MeleeTraining" )
		{
			out.append( w );
		}
	}
	return out;
}

void WorkshopManager::emitJobListChanged( unsigned int workshopID )
{
	emit signalJobListChanged( workshopID );
}