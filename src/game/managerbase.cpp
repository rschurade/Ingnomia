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
#include "managerbase.h"

ManagerBase::ManagerBase( QObject* parent ) :
    QObject( parent )
{
}

ManagerBase::~ManagerBase()
{
}

void ManagerBase::setInventory( Inventory* inv )
{
    m_inv = inv;
}
void ManagerBase::setWorld( World* world )
{
    m_world = world;
}
void ManagerBase::setSpriteFactory( SpriteFactory* sf )
{
    m_sf = sf;
}

void ManagerBase::setDB( DB* db )
{
    m_db = db;
}

void ManagerBase::setPathFinder( PathFinder* pf )
{
    m_pf = pf;
}
	
void ManagerBase::setJobManager( JobManager* jobManager )
{
    m_jobManager = jobManager;
}
void ManagerBase::setStockpileManager( StockpileManager* stockpileManager )
{
    m_spm = stockpileManager;
}
void ManagerBase::setFarmingManager( FarmingManager* farmingManager )
{
    m_farmingManager = farmingManager;
}
void ManagerBase::setWorkshopManager( WorkshopManager* workshopManager )
{
    m_workshopManager = workshopManager;
}
void ManagerBase::setRoomManager( RoomManager* roomManager )
{
    m_roomManager = roomManager;
}
void ManagerBase::setCreaturemanager( CreatureManager* creatureManager )
{
    m_creatureManager = creatureManager;
}
void ManagerBase::setGnomeManager( GnomeManager* gnomeManager )
{
    m_gnomeManager = gnomeManager;
}
void ManagerBase::setMechanismManager( MechanismManager* mechanismManager )
{
    m_mechanismManager = mechanismManager;
}
void ManagerBase::setFluidManager( FluidManager* fluidManager )
{
    m_fluidManager = fluidManager;
}
	
void ManagerBase::setMilitaryManager( MilitaryManager* militaryManager )
{
    m_militaryManager = militaryManager;
}
void ManagerBase::setNeighborManager( NeighborManager* neighborManager )
{
    m_neighborManager = neighborManager;
}
void ManagerBase::setEventManager( EventManager* eventManager )
{
    m_eventManager = eventManager;
}