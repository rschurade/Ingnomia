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
#include "farmingmanager.h"

#include "../base/global.h"
#include "../base/position.h"
#include "../game/inventory.h"
#include "../game/job.h"
#include "../game/jobmanager.h"
#include "../game/world.h"

#include <QDebug>
#include <QVariantMap>

void Beehive::serialize( QVariantMap& out )
{
	out.insert( "Type", "beehive" );
	out.insert( "ID", id );
	out.insert( "Pos", pos.toString() );
	out.insert( "Honey", honey );
	out.insert( "Harvest", harvest );
}

Beehive::Beehive( QVariantMap& in )
{
	id      = in.value( "ID" ).toUInt();
	pos     = Position( in.value( "Pos" ) );
	honey   = in.value( "Honey" ).toFloat();
	harvest = in.value( "Harvest" ).toBool();
}

FarmingManager::FarmingManager( QObject* parent ) :
	QObject(parent)
{
}

FarmingManager::~FarmingManager()
{
}

void FarmingManager::reset()
{
	m_groves.clear();
	m_allGroveTiles.clear();
	m_farms.clear();
	m_allFarmTiles.clear();
	m_pastures.clear();
	m_allPastureTiles.clear();
}

bool FarmingManager::load( QVariantMap vm )
{
	QString type = vm.value( "Type" ).toString();
	if ( type == "farm" )
	{
		Farm fa( vm );
		m_farms.insert( fa.id(), fa );
		for ( auto f : vm.value( "Fields" ).toList() )
		{
			QVariantMap fm = f.toMap();
			m_allFarmTiles.insert( Position( fm.value( "Pos" ).toString() ).toInt(), fa.id() );
		}
		return true;
	}
	if ( type == "grove" )
	{
		Grove gr( vm );
		m_groves.insert( gr.id(), gr );
		for ( auto f : vm.value( "Fields" ).toList() )
		{
			QVariantMap fm = f.toMap();
			m_allGroveTiles.insert( Position( fm.value( "Pos" ).toString() ).toInt(), gr.id() );
		}

		return true;
	}
	if ( type == "pasture" )
	{
		Pasture pa( vm );
		m_pastures.insert( pa.id(), pa );
		for ( auto f : vm.value( "Fields" ).toList() )
		{
			QVariantMap fm = f.toMap();
			m_allPastureTiles.insert( Position( fm.value( "Pos" ).toString() ).toInt(), pa.id() );
		}
		return true;
	}

	if ( type == "beehive" )
	{
		Beehive bh( vm );
		m_beehives.insert( bh.pos.toInt(), bh );

		return true;
	}

	return false;
}

void FarmingManager::onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	onTickFarm( tickNumber, seasonChanged, dayChanged, hourChanged, minuteChanged );
	onTickPasture( tickNumber, seasonChanged, dayChanged, hourChanged, minuteChanged );
	onTickGrove( tickNumber, seasonChanged, dayChanged, hourChanged, minuteChanged );
	if ( hourChanged )
	{
		onTickBeeHive( tickNumber, seasonChanged, dayChanged, hourChanged, minuteChanged );
	}
}

void FarmingManager::onTickBeeHive( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	for ( auto& bh : m_beehives )
	{
		bh.honey += (float)( 1.0 / 20. );

		if ( bh.honey >= 1.0 && !bh.hasJob )
		{
			Global::jm().addJob( "Harvest", bh.pos, 0, true );
		}
	}
}

void FarmingManager::onTickGrove( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	for ( auto&& gr : m_groves )
	{
		gr.onTick( tickNumber );
	}
}

void FarmingManager::onTickFarm( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	if ( hourChanged )
	{
		for ( auto&& fa : m_farms )
		{
			if ( fa.countTiles() == 0 && fa.canDelete() )
			{
				removeFarm( fa.id() );
				break;
			}
		}
	}

	for ( auto&& fa : m_farms )
	{
		fa.onTick( tickNumber );
	}
}

void FarmingManager::onTickPasture( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	if ( hourChanged )
	{
		for ( auto& pa : m_pastures )
		{
			if ( pa.countTiles() == 0 && pa.canDelete() )
			{
				removePasture( pa.id() );
				break;
			}
		}
	}

	if ( minuteChanged )
	{
		for ( auto&& pa : m_pastures )
		{
			pa.onTick( tickNumber );
		}
	}
}

void FarmingManager::addGrove( Position firstClick, QList<QPair<Position, bool>> fields )
{
	if ( m_allGroveTiles.contains( firstClick.toInt() ) )
	{
		unsigned int grID = m_allGroveTiles.value( firstClick.toInt() );

		Grove* gr = getGroveAtPos( firstClick );
		if ( gr )
		{
			for ( auto p : fields )
			{
				if ( p.second && !m_allGroveTiles.contains( p.first.toInt() ) )
				{
					m_allGroveTiles.insert( p.first.toInt(), grID );
					gr->addTile( p.first );
				}
			}
			m_lastAdded = grID;
		}
	}
	else
	{
		Grove gr( fields );
		for ( auto p : fields )
		{
			if ( p.second )
			{
				m_allGroveTiles.insert( p.first.toInt(), gr.id() );
			}
		}
		m_groves.insert( gr.id(), gr );
		m_lastAdded = 0;
	}

	World& world = Global::w();
	for ( auto p : fields )
	{
		if ( p.second )
		{
			world.setTileFlag( p.first, TileFlag::TF_GROVE );
		}
	}
}

void FarmingManager::removeGrove( unsigned int id )
{
	for ( int i = 0; i < m_groves.size(); ++i )
	{
		if ( id == m_groves[i].id() )
		{
			m_groves.removeAt( i );
			return;
		}
	}
}

Grove* FarmingManager::getGroveAtPos( Position pos )
{
	for ( auto&& g : m_groves )
	{
		if ( g.id() == m_allGroveTiles[pos.toInt()] )
		{
			return &g;
		}
	}
	return nullptr;
}

Grove* FarmingManager::getGrove( unsigned int id )
{
	for ( auto&& g : m_groves )
	{
		if ( g.id() == id )
		{
			return &g;
		}
	}
	return nullptr;
}

Grove* FarmingManager::getLastAddedGrove()
{
	if ( m_groves.size() )
	{
		return &m_groves.last();
	}
	return nullptr;
}

void FarmingManager::addFarm( Position firstClick, QList<QPair<Position, bool>> fields )
{
	if ( m_allFarmTiles.contains( firstClick.toInt() ) )
	{
		unsigned int faID = m_allFarmTiles.value( firstClick.toInt() );

		Farm* fa = getFarmAtPos( firstClick );
		if ( fa )
		{
			for ( auto p : fields )
			{
				if ( p.second && !m_allFarmTiles.contains( p.first.toInt() ) )
				{
					m_allFarmTiles.insert( p.first.toInt(), faID );
					fa->addTile( p.first );
				}
			}
			m_lastAdded = faID;
			emit signalFarmChanged( faID );
		}
	}
	else
	{
		Farm fa( fields );
		for ( auto p : fields )
		{
			if ( p.second )
			{
				m_allFarmTiles.insert( p.first.toInt(), fa.id() );
			}
		}
		m_farms.insert( fa.id(), fa );
		m_lastAdded = 0;
		emit signalFarmChanged( fa.id() );
	}

	for ( auto p : fields )
	{
		if ( p.second )
		{
			Global::w().setTileFlag( p.first, TileFlag::TF_FARM );
		}
	}
}

void FarmingManager::removeFarm( unsigned int id )
{
	for ( int i = 0; i < m_farms.size(); ++i )
	{
		if ( id == m_farms[i].id() )
		{
			m_farms.removeAt( i );
			return;
		}
	}
}

Farm* FarmingManager::getFarmAtPos( Position pos )
{
	for ( auto&& f : m_farms )
	{
		if ( f.id() == m_allFarmTiles[pos.toInt()] )
		{
			return &f;
		}
	}
	return nullptr;
}

Farm* FarmingManager::getFarm( unsigned int id )
{
	for ( auto&& f : m_farms )
	{
		if ( f.id() == id )
		{
			return &f;
		}
	}
	return nullptr;
}

Farm* FarmingManager::getLastAddedFarm()
{
	if ( m_lastAdded )
	{
		for ( int i = 0; i < m_farms.size(); ++i )
		{
			if ( m_farms[i].id() == m_lastAdded )
			{
				return &m_farms[i];
			}
		}
	}
	if ( m_farms.size() )
	{
		return &m_farms.last();
	}
	return nullptr;
}

void FarmingManager::addPasture( Position firstClick, QList<QPair<Position, bool>> fields )
{
	if ( m_allPastureTiles.contains( firstClick.toInt() ) )
	{
		unsigned int paID = m_allPastureTiles.value( firstClick.toInt() );

		Pasture* pa = getPastureAtPos( firstClick );
		if ( pa )
		{
			for ( auto p : fields )
			{
				if ( p.second && !m_allPastureTiles.contains( p.first.toInt() ) )
				{
					m_allPastureTiles.insert( p.first.toInt(), paID );
					pa->addTile( p.first );
				}
			}
		}
		m_lastAdded = 0;
	}
	else
	{
		Pasture pa( fields );
		for ( auto p : fields )
		{
			if ( p.second )
			{
				m_allPastureTiles.insert( p.first.toInt(), pa.id() );
			}
		}
		m_pastures.insert( pa.id(), pa );
		m_lastAdded = pa.id();
	}

	for ( auto p : fields )
	{
		if ( p.second )
		{
			Global::w().setTileFlag( p.first, TileFlag::TF_PASTURE );
		}
	}
}

void FarmingManager::removePasture( unsigned int id )
{
	for ( int i = 0; i < m_pastures.size(); ++i )
	{
		if ( id == m_pastures[i].id() )
		{
			m_pastures[i].removeAllAnimals();
			m_pastures.removeAt( i );
			return;
		}
	}
}

Pasture* FarmingManager::getPastureAtPos( Position pos )
{
	for ( auto& pa : m_pastures )
	{
		if ( pa.id() == m_allPastureTiles[pos.toInt()] )
		{
			return &pa;
		}
	}
	return nullptr;
}

Pasture* FarmingManager::getPasture( unsigned int id )
{
	for ( auto&& pa : m_pastures )
	{
		if ( pa.id() == id )
		{
			return &pa;
		}
	}
	return nullptr;
}

Pasture* FarmingManager::getLastAddedPasture()
{
	if ( m_lastAdded )
	{
		for ( int i = 0; i < m_pastures.size(); ++i )
		{
			if ( m_pastures[i].id() == m_lastAdded )
			{
				return &m_pastures[i];
			}
		}
	}
	if ( m_pastures.size() )
	{
		return &m_pastures.last();
	}
	return nullptr;
}

unsigned int FarmingManager::getJob( unsigned int gnomeID, QString skillID )
{
	for ( auto&& g : m_groves )
	{
		unsigned int job = g.getJob( gnomeID, skillID );
		if ( job )
		{
			return job;
		}
	}
	for ( auto&& f : m_farms )
	{
		unsigned int job = f.getJob( gnomeID, skillID );
		if ( job )
		{
			return job;
		}
	}
	for ( auto&& pa : m_pastures )
	{
		unsigned int job = pa.getJob( gnomeID, skillID );
		if ( job )
		{
			return job;
		}
	}
	return 0;
}

bool FarmingManager::finishJob( unsigned int jobID )
{
	for ( auto&& g : m_groves )
	{
		if ( g.finishJob( jobID ) )
		{
			return true;
		}
	}
	for ( auto&& f : m_farms )
	{
		if ( f.finishJob( jobID ) )
		{
			emit signalFarmChanged( f.id() );
			return true;
		}
	}
	for ( auto&& pa : m_pastures )
	{
		if ( pa.finishJob( jobID ) )
		{
			emit signalPastureChanged( pa.id() );
			return true;
		}
	}
	return false;
}

bool FarmingManager::giveBackJob( unsigned int jobID )
{
	for ( auto&& g : m_groves )
	{
		if ( g.giveBackJob( jobID ) )
		{
			return true;
		}
	}
	for ( auto&& f : m_farms )
	{
		if ( f.giveBackJob( jobID ) )
		{
			emit signalFarmChanged( f.id() );
			return true;
		}
	}
	for ( auto&& pa : m_pastures )
	{
		if ( pa.giveBackJob( jobID ) )
		{
			emit signalPastureChanged( pa.id() );
			return true;
		}
	}
	return false;
}

Job* FarmingManager::getJob( unsigned int jobID )
{
	for ( auto&& g : m_groves )
	{
		if ( g.hasJobID( jobID ) )
		{
			return g.getJob( jobID );
		}
	}
	for ( auto&& f : m_farms )
	{
		if ( f.hasJobID( jobID ) )
		{
			return f.getJob( jobID );
		}
	}
	for ( auto&& pa : m_pastures )
	{
		if ( pa.hasJobID( jobID ) )
		{
			return pa.getJob( jobID );
		}
	}
	return nullptr;
}

bool FarmingManager::hasJobID( unsigned int jobID )
{
	for ( auto&& g : m_groves )
	{
		if ( g.hasJobID( jobID ) )
		{
			return true;
		}
	}
	for ( auto&& f : m_farms )
	{
		if ( f.hasJobID( jobID ) )
		{
			return true;
		}
	}
	for ( auto&& pa : m_pastures )
	{
		if ( pa.hasJobID( jobID ) )
		{
			return true;
		}
	}
	return false;
}

void FarmingManager::removeTile( Position pos, bool includeFarm, bool includePasture, bool includeGrove )
{
	if ( includeFarm && isFarm( pos ) )
	{
		Farm* farm = getFarmAtPos( pos );
		if ( farm )
		{
			unsigned int id = farm->id();
			m_allFarmTiles.remove( pos.toInt() );
			if ( farm->removeTile( pos ) && farm->canDelete() )
			{
				removeFarm( id );
			}
			emit signalFarmChanged( id );
		}
	}
	else if ( includeGrove && isGrove( pos ) )
	{
		Grove* grove = getGroveAtPos( pos );
		if ( grove )
		{
			m_allGroveTiles.remove( pos.toInt() );
			if ( grove->removeTile( pos ) )
			{
				removeGrove( grove->id() );
			}
		}
	}
	else if ( includePasture && isPasture( pos ) )
	{
		Pasture* pasture = getPastureAtPos( pos );
		if ( pasture )
		{
			m_allPastureTiles.remove( pos.toInt() );
			if ( pasture->removeTile( pos ) && pasture->canDelete() )
			{
				removePasture( pasture->id() );
			}
			emit signalPastureChanged( pasture->id() );
		}
	}
}

bool FarmingManager::hasPlantTreeJob( Position pos )
{
	if ( isGrove( pos ) )
	{
		Grove* grove = getGroveAtPos( pos );
		if ( grove )
		{
			return grove->hasPlantTreeJob( pos );
		}
	}
	return false;
}

bool FarmingManager::addUtil( Position pos, unsigned int itemID )
{
	QString itemSID = Global::inv().itemSID( itemID );

	if ( itemSID == "Shed" )
	{
		return addUtilToPasture( pos, itemID );
	}
	else if ( itemSID == "Trough" )
	{
		return addUtilToPasture( pos, itemID );
	}
	else if ( itemSID == "BeeHive" )
	{
		Beehive bh;
		bh.id  = itemID;
		bh.pos = pos;

		if ( !m_beehives.contains( pos.toInt() ) )
		{
			m_beehives.insert( pos.toInt(), bh );
		}
	}

	return true;
}

bool FarmingManager::removeUtil( Position pos )
{
	if ( m_beehives.contains( pos.toInt() ) )
	{
		m_beehives.remove( pos.toInt() );
		return true;
	}
	else
	{
		auto pasture = getPastureAtPos( pos );
		if ( pasture )
		{
			pasture->removeUtil( pos );
		}
	}

	return false;
}

bool FarmingManager::addUtilToPasture( Position pos, unsigned int itemID )
{
	Pasture* pasture = getPastureAtPos( pos );
	if ( pasture )
	{
		return pasture->addUtil( pos, itemID );
	}

	return false;
}

bool FarmingManager::removeUtilFromPasture( Position pos, unsigned int itemID )
{
	Pasture* pasture = getPastureAtPos( pos );
	if ( pasture )
	{
		return pasture->removeUtil( pos );
	}

	return false;
}

unsigned int FarmingManager::util( Position pos )
{
	Pasture* pasture = getPastureAtPos( pos );
	if ( pasture )
	{
		return pasture->util( pos );
	}

	return 0;
}

int FarmingManager::farmPriority( unsigned int id )
{
	QMutexLocker lock( &m_mutex );
	for ( int i = 0; i < m_farms.size(); ++i )
	{
		if ( m_farms[i].id() == id )
		{
			return i;
		}
	}
	return -1;
}

int FarmingManager::countFarms()
{
	return m_farms.size();
}

void FarmingManager::setFarmPriority( unsigned int id, int prio )
{
	QMutexLocker lock( &m_mutex );
	for ( int i = 0; i < m_farms.size(); ++i )
	{
		if ( m_farms[i].id() == id )
		{
			Farm f = m_farms.takeAt( i );
			m_farms.insert( prio, f );
			break;
		}
	}
}

int FarmingManager::grovePriority( unsigned int id )
{
	QMutexLocker lock( &m_mutex );
	for ( int i = 0; i < m_groves.size(); ++i )
	{
		if ( m_groves[i].id() == id )
		{
			return i;
		}
	}
	return -1;
}

int FarmingManager::countGroves()
{
	return m_groves.size();
}

void FarmingManager::setGrovePriority( unsigned int id, int prio )
{
	QMutexLocker lock( &m_mutex );
	for ( int i = 0; i < m_groves.size(); ++i )
	{
		if ( m_groves[i].id() == id )
		{
			Grove g = m_groves.takeAt( i );
			m_groves.insert( prio, g );
			break;
		}
	}
}

int FarmingManager::pasturePriority( unsigned int id )
{
	QMutexLocker lock( &m_mutex );
	for ( int i = 0; i < m_pastures.size(); ++i )
	{
		if ( m_pastures[i].id() == id )
		{
			return i;
		}
	}
	return -1;
}

int FarmingManager::countPastures()
{
	return m_pastures.size();
}

void FarmingManager::setPasturePriority( unsigned int id, int prio )
{
	QMutexLocker lock( &m_mutex );

	for ( int i = 0; i < m_pastures.size(); ++i )
	{
		if ( m_pastures[i].id() == id )
		{
			Pasture pa = m_pastures.takeAt( i );
			m_pastures.insert( prio, pa );
			break;
		}
	}
}

bool FarmingManager::isBeehive( Position pos )
{
	return m_beehives.contains( pos.toInt() );
}

unsigned int FarmingManager::beehiveID( Position pos )
{
	if ( m_beehives.contains( pos.toInt() ) )
	{
		return m_beehives[pos.toInt()].id;
	}
	return 0;
}

Beehive FarmingManager::beehive( unsigned int id )
{
	for ( auto bh : m_beehives )
	{
		if ( bh.id == id )
		{
			return bh;
		}
	}
	return Beehive();
}

void FarmingManager::setBeehiveHarvest( unsigned int id, bool harvest )
{
	for ( auto& bh : m_beehives )
	{
		if ( bh.id == id )
		{
			bh.harvest = harvest;
		}
	}
}

bool FarmingManager::harvestBeehive( Position pos )
{
	if ( m_beehives.contains( pos.toInt() ) )
	{
		auto& bh  = m_beehives[pos.toInt()];
		bh.hasJob = false;
		bh.honey  = 0.0;
		return true;
	}
	return false;
}

	
void FarmingManager::emitUpdateSignalFarm( unsigned int id )
{
	emit signalFarmChanged( id );
}

void FarmingManager::emitUpdateSignalPasture( unsigned int id )
{
	emit signalPastureChanged( id );
}

void FarmingManager::emitUpdateSignalGrove( unsigned int id )
{
	emit signalGroveChanged( id );
}