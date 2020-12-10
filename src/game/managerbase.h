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
#pragma once

#include <QObject>

class PathFinder;
class SpriteFactory;
class World;
class DB;
class PathFinder;

class Inventory; 
class ItemHistory;
class JobManager;
class StockpileManager;
class FarmingManager;
class WorkshopManager;
class RoomManager;
class GnomeManager;
class CreatureManager;
class EventManager;
class MechanismManager;
class FluidManager;
class NeighborManager;
class MilitaryManager;



class ManagerBase : public QObject
{
	Q_OBJECT

public:
    ManagerBase( QObject* parent );
    ~ManagerBase();

	void setInventory( Inventory* inv );
	void setWorld( World* world );
	void setSpriteFactory( SpriteFactory* sf );
	void setDB( DB* db );
	void setPathFinder( PathFinder* pf );

	void setJobManager( JobManager* jobManager );
	void setStockpileManager( StockpileManager* stockpileManager );
	void setFarmingManager( FarmingManager* farmingManager );
	void setWorkshopManager( WorkshopManager* workshopManager );
	void setRoomManager( RoomManager* roomManager );
	void setCreaturemanager( CreatureManager* creatureManager );
	void setGnomeManager( GnomeManager* gnomeManager );
	void setMechanismManager( MechanismManager* mechanismManager );
	void setFluidManager( FluidManager* fluidManager );
	
	void setMilitaryManager( MilitaryManager* militaryManager );
	void setNeighborManager( NeighborManager* neighborManager );
	void setEventManager( EventManager* eventManager );

protected:
    Inventory* m_inv = nullptr;
	World* m_world = nullptr;
	SpriteFactory* m_sf = nullptr;
	DB* m_db = nullptr;
	PathFinder* m_pf = nullptr;

	JobManager* m_jobManager = nullptr;
	StockpileManager* m_spm = nullptr;
	FarmingManager* m_farmingManager = nullptr;
	WorkshopManager* m_workshopManager = nullptr;
	RoomManager* m_roomManager = nullptr;
	CreatureManager* m_creatureManager = nullptr;
	GnomeManager* m_gnomeManager = nullptr;
	MechanismManager* m_mechanismManager = nullptr;
	FluidManager* m_fluidManager = nullptr;

	MilitaryManager* m_militaryManager = nullptr;
	NeighborManager* m_neighborManager = nullptr;
	EventManager* m_eventManager = nullptr;
};