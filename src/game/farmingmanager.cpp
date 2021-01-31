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
#include "game.h"

#include "../base/global.h"
#include "../base/position.h"
#include "../game/inventory.h"
#include "../game/job.h"
#include "../game/jobmanager.h"
#include "../game/world.h"

#include <QDebug>
#include <QVariantMap>

void Beehive::serialize( QVariantMap& out ) const
{
	out.insert( "Type", "beehive" );
	out.insert( "ID", id );
	out.insert( "Pos", pos.toString() );
	out.insert( "Honey", honey );
	out.insert( "Harvest", harvest );
}

Beehive::Beehive()
{
}

Beehive::~Beehive()
{
}

Beehive::Beehive( QVariantMap& in )
{
	id      = in.value( "ID" ).toUInt();
	pos     = Position( in.value( "Pos" ) );
	honey   = in.value( "Honey" ).toFloat();
	harvest = in.value( "Harvest" ).toBool();
}

FarmingManager::FarmingManager( Game* parent ) :
	g( parent ),
	QObject(parent)
{
}

FarmingManager::~FarmingManager()
{
	for (const auto& pa : m_pastures)
	{
		delete pa;
	}
	m_pastures.clear();
	for (const auto& fa : m_farms)
	{
		delete fa;
	}
	m_farms.clear();
	for (const auto& gr : m_groves)
	{
		delete gr;
	}
	m_groves.clear();
	for ( const auto& bh : m_beehives )
	{
		delete bh;
	}
	m_beehives.clear();
}

bool FarmingManager::load( QVariantMap vm )
{
	QString type = vm.value( "Type" ).toString();
	if ( type == "farm" )
	{
		auto fa = new Farm( vm, g );
		m_farms.insert( fa->id(), fa );
		for ( const auto& f : vm.value( "Fields" ).toList() )
		{
			QVariantMap fm = f.toMap();
			m_allFarmTiles.insert( Position( fm.value( "Pos" ).toString() ), fa->id() );
		}
		return true;
	}
	if ( type == "grove" )
	{
		auto gr = new Grove( vm, g );
		m_groves.insert( gr->id(), gr );
		for ( const auto& f : vm.value( "Fields" ).toList() )
		{
			QVariantMap fm = f.toMap();
			m_allGroveTiles.insert( Position( fm.value( "Pos" ).toString() ), gr->id() );
		}

		return true;
	}
	if ( type == "pasture" )
	{
		auto pa = new Pasture( vm, g );
		m_pastures.insert( pa->id(), pa );
		for ( const auto& f : vm.value( "Fields" ).toList() )
		{
			QVariantMap fm = f.toMap();
			m_allPastureTiles.insert( Position( fm.value( "Pos" ).toString() ), pa->id() );
		}
		return true;
	}

	if ( type == "beehive" )
	{
		auto bh = new Beehive( vm );
		m_beehives.insert( bh->id, bh );
		m_allBeehiveTiles.insert( bh->pos, bh->id );
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
		bh->honey += (float)( 1.0 / 200. );

		if ( bh->honey >= 1.0 && !bh->hasJob )
		{
			g->m_jobManager->addJob( "Harvest", bh->pos, 0, true );
		}
	}
}

void FarmingManager::onTickGrove( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	for ( auto&& gr : m_groves )
	{
		gr->onTick( tickNumber );
	}
}

void FarmingManager::onTickFarm( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	if ( hourChanged )
	{
		for ( auto&& fa : m_farms )
		{
			if ( fa->countTiles() == 0 && fa->canDelete() )
			{
				removeFarm( fa->id() );
				break;
			}
		}
	}

	for ( auto& fa : m_farms )
	{
		fa->onTick( tickNumber );
	}
}

void FarmingManager::onTickPasture( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	if ( hourChanged )
	{
		for ( auto& pa : m_pastures )
		{
			if ( pa->countTiles() == 0 && pa->canDelete() )
			{
				removePasture( pa->id() );
				break;
			}
		}
	}
	
	if ( minuteChanged )
	{
		m_totalCountAnimals = 0;
		int count = 0;
		for ( auto& pa : m_pastures )
		{
			pa->onTick( tickNumber, count );
			m_totalCountAnimals += count;
		}
	}
}

void FarmingManager::addGrove( Position firstClick, QList<QPair<Position, bool>> fields )
{
	if ( m_allGroveTiles.contains( firstClick ) )
	{
		unsigned int grID = m_allGroveTiles.value( firstClick );

		Grove* gr = getGroveAtPos( firstClick );
		if ( gr )
		{
			for ( const auto& p : fields )
			{
				if ( p.second && !m_allGroveTiles.contains( p.first ) )
				{
					m_allGroveTiles.insert( p.first, grID );
					gr->addTile( p.first );
				}
			}
		}
	}
	else
	{
		auto gr = new Grove( fields, g );
		for ( const auto& p : fields )
		{
			if ( p.second && !m_allGroveTiles.contains(p.first) )
			{
				m_allGroveTiles.insert( p.first, gr->id() );
				gr->addTile( p.first );
			}
		}
		m_groves.insert( gr->id(), gr );
	}
}

void FarmingManager::removeGrove( unsigned int id )
{
	auto it = m_groves.find( id );
	if ( it != m_groves.end() )
	{
		for ( auto field = m_allGroveTiles.begin(); field != m_allGroveTiles.end(); )
		{
			if ( field.value() == id )
			{
				it.value()->removeTile( field.key() );
				field = m_allGroveTiles.erase( field );
			}
			else
			{
				++field;
			}
		}
		delete it.value();
		m_groves.erase( it );
	}
}

Grove* FarmingManager::getGroveAtPos( Position pos )
{
	auto it = m_allGroveTiles.find( pos );
	if (it != m_allGroveTiles.end())
	{
		return getGrove( it.value() );
	}
	return nullptr;
}

Grove* FarmingManager::getGrove( unsigned int id )
{
	auto it = m_groves.find( id );
	if (it != m_groves.end())
	{
		return it.value();
	}
	return nullptr;
}

bool FarmingManager::isGrove( Position pos ) const
{
	return m_allGroveTiles.contains( pos );
}

void FarmingManager::addFarm( Position firstClick, QList<QPair<Position, bool>> fields )
{
	if ( m_allFarmTiles.contains( firstClick) )
	{
		unsigned int faID = m_allFarmTiles.value( firstClick );

		Farm* fa = getFarmAtPos( firstClick );
		if ( fa )
		{
			for ( const auto& p : fields )
			{
				if ( p.second && !m_allFarmTiles.contains( p.first ) )
				{
					m_allFarmTiles.insert( p.first, faID );
					fa->addTile( p.first );
				}
			}
			emit signalFarmChanged( faID );
		}
	}
	else
	{
		auto fa = new Farm( fields, g );
		for ( const auto& p : fields )
		{
			if ( p.second && !m_allFarmTiles.contains( p.first ) )
			{
				m_allFarmTiles.insert( p.first, fa->id() );
				fa->addTile( p.first );
			}
		}
		m_farms.insert( fa->id(), fa );
		emit signalFarmChanged( fa->id() );
	}
}

void FarmingManager::removeFarm( unsigned int id )
{
	auto it = m_farms.find( id );
	if ( it != m_farms.end() )
	{
		for ( auto field = m_allFarmTiles.begin(); field != m_allFarmTiles.end(); )
		{
			if ( field.value() == id )
			{
				it.value()->removeTile( field.key() );
				field = m_allFarmTiles.erase( field );
			}
			else
			{
				++field;
			}
		}
		delete it.value();
		m_farms.erase( it );
	}
}

Farm* FarmingManager::getFarmAtPos( Position pos )
{
	auto it = m_allFarmTiles.find( pos );
	if ( it != m_allFarmTiles.end() )
	{
		return getFarm( it.value() );
	}
	return nullptr;
}

Farm* FarmingManager::getFarm( unsigned int id )
{
	auto it = m_farms.find( id );
	if ( it != m_farms.end() )
	{
		return it.value();
	}
	return nullptr;
}

bool FarmingManager::isFarm( Position pos ) const
{
	return m_allFarmTiles.contains( pos );
}

void FarmingManager::addPasture( Position firstClick, QList<QPair<Position, bool>> fields )
{
	if ( m_allPastureTiles.contains( firstClick ) )
	{
		unsigned int paID = m_allPastureTiles.value( firstClick );

		Pasture* pa = getPastureAtPos( firstClick );
		if ( pa )
		{
			for ( const auto& p : fields )
			{
				if ( p.second && !m_allPastureTiles.contains( p.first ) )
				{
					m_allPastureTiles.insert( p.first, paID );
					pa->addTile( p.first );
				}
			}
		}
	}
	else
	{
		auto pa = new Pasture( fields, g );
		for ( const auto& p : fields )
		{
			if ( p.second && !m_allPastureTiles.contains( p.first ) )
			{
				m_allPastureTiles.insert( p.first, pa->id() );
				pa->addTile( p.first );
			}
		}
		m_pastures.insert( pa->id(), pa );
	}
}

void FarmingManager::removePasture( unsigned int id )
{
	auto it = m_pastures.find( id );
	if ( it != m_pastures.end() )
	{
		for ( auto field = m_allFarmTiles.begin(); field != m_allFarmTiles.end(); )
		{
			if ( field.value() == id )
			{
				it.value()->removeTile( field.key() );
				field = m_allFarmTiles.erase( field );
			}
			else
			{
				++field;
			}
		}
		it.value()->removeAllAnimals();
		delete it.value();
		m_pastures.erase( it );
	}
}

Pasture* FarmingManager::getPastureAtPos( Position pos )
{
	auto it = m_allPastureTiles.find( pos );
	if ( it != m_allPastureTiles.end() )
	{
		return getPasture( it.value() );
	}
	return nullptr;
}

Pasture* FarmingManager::getPasture( unsigned int id )
{
	auto it = m_pastures.find( id );
	if ( it != m_pastures.end() )
	{
		return it.value();
	}
	return nullptr;
}

bool FarmingManager::isPasture( Position pos ) const
{
	return m_allPastureTiles.contains( pos );
}

const QHash<unsigned int, Grove*>& FarmingManager::allGroves()
{
	return m_groves;
}

const QHash<unsigned int, Farm*>& FarmingManager::allFarms()
{
	return m_farms;
}

const QHash<unsigned int, Pasture*>& FarmingManager::allPastures()
{
	return m_pastures;
}

const QHash<unsigned int, Beehive*>& FarmingManager::allBeeHives()
{
	return m_beehives;
}

void FarmingManager::removeTile( Position pos, bool includeFarm, bool includePasture, bool includeGrove )
{
	if ( includeFarm && isFarm( pos ) )
	{
		Farm* farm = getFarmAtPos( pos );
		if ( farm )
		{
			unsigned int id = farm->id();
			m_allFarmTiles.remove( pos );
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
			m_allGroveTiles.remove( pos );
			if ( grove->removeTile( pos ) && grove->canDelete() )
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
			m_allPastureTiles.remove( pos );
			if ( pasture->removeTile( pos ) && pasture->canDelete() )
			{
				removePasture( pasture->id() );
			}
			emit signalPastureChanged( pasture->id() );
		}
	}
}

bool FarmingManager::addUtil( Position pos, unsigned int itemID )
{
	QString itemSID = g->m_inv->itemSID( itemID );

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
		if ( !m_allBeehiveTiles.contains( pos ) )
		{
			auto bh = new Beehive;
			bh->id  = itemID;
			bh->pos = pos;

			m_beehives.insert( bh->id, bh );
			m_allBeehiveTiles.insert( bh->pos, bh->id );
		}
		else
		{
			return false;
		}
	}

	return true;
}

bool FarmingManager::removeUtil( Position pos )
{
	if ( auto bh = getBeehiveAtPos(pos) )
	{
		m_beehives.remove( bh->id );
		m_allBeehiveTiles.remove( bh->pos );
		delete bh;
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
	//TODO
	return -1;
}

int FarmingManager::countFarms()
{
	return m_farms.size();
}

void FarmingManager::setFarmPriority( unsigned int id, int prio )
{
	//TODO
}

int FarmingManager::grovePriority( unsigned int id )
{
	//TODO
	return -1;
}

int FarmingManager::countGroves()
{
	return m_groves.size();
}

void FarmingManager::setGrovePriority( unsigned int id, int prio )
{
	//TODO
}

int FarmingManager::pasturePriority( unsigned int id )
{
	//TODO
	return -1;
}

int FarmingManager::countPastures()
{
	return m_pastures.size();
}

void FarmingManager::setPasturePriority( unsigned int id, int prio )
{
	//TODO
}

bool FarmingManager::isBeehive( Position pos )
{
	return m_allBeehiveTiles.contains( pos.toInt() );
}

Beehive* FarmingManager::getBeehiveAtPos( Position pos )
{
	auto it = m_allBeehiveTiles.find( pos );
	if ( it != m_allBeehiveTiles.end() )
	{
		return getBeehive( it.value() );
	}
	return nullptr;
}

Beehive* FarmingManager::getBeehive( unsigned int id )
{
	auto it = m_beehives.find( id );
	if ( it != m_beehives.end() )
	{
		return it.value();
	}
	return nullptr;
}

void FarmingManager::setBeehiveHarvest( unsigned int id, bool harvest )
{
	auto bh = getBeehive( id );
	if (bh)
	{
		bh->harvest = harvest;
	}
}

bool FarmingManager::harvestBeehive( Position pos )
{
	auto bh = getBeehiveAtPos( pos );
	if (bh)
	{
		bh->hasJob = false;
		bh->honey  = 0.0;
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