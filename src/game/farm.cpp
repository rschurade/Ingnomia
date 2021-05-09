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
#include "farm.h"

#include "../base/config.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/util.h"
#include "../game/game.h"
#include "../game/gnomemanager.h"
#include "../game/inventory.h"
#include "../game/jobmanager.h"
#include "../game/plant.h"
#include "../game/world.h"

#include <QDebug>

FarmProperties::FarmProperties( QVariantMap& in )
{
	plantType = in.value( "PlantType" ).toString();
	seedItem  = in.value( "SeedItemID" ).toString();
	onlySeed  = in.value( "OnlySeed" ).toBool();
	harvest   = in.value( "Harvest" ).toBool();
	auto vjpl = in.value( "JobPriorities" ).toList();
	for ( auto vjp : vjpl )
	{
		jobPriorities.append( vjp.value<quint8>() );
	}

	autoHarvestSeed  = in.value( "AutoHarvestSeed" ).toBool();
	autoHarvestItem1 = in.value( "AutoHarvestItem1" ).toBool();
	autoHarvestItem2 = in.value( "AutoHarvestItem2" ).toBool();

	autoHarvestSeedMin  = in.value( "AutoHarvestSeedMin" ).toUInt();
	autoHarvestSeedMax  = in.value( "AutoHarvestSeedMax" ).toUInt();
	autoHarvestItem1Min = in.value( "AutoHarvestItem1Min" ).toUInt();
	autoHarvestItem1Max = in.value( "AutoHarvestItem1Max" ).toUInt();
	autoHarvestItem2Min = in.value( "AutoHarvestItem2Min" ).toUInt();
	autoHarvestItem2Max = in.value( "AutoHarvestItem2Max" ).toUInt();
}

void FarmProperties::serialize( QVariantMap& out ) const
{
	out.insert( "Type", "farm" );
	out.insert( "PlantType", plantType );
	out.insert( "SeedItemID", seedItem );
	out.insert( "OnlySeed", onlySeed );
	out.insert( "Harvest", harvest );
	QVariantList jpl;
	for ( auto jp : jobPriorities )
	{
		jpl.append( jp );
	}
	out.insert( "JobPriorities", jpl );

	out.insert( "AutoHarvestSeed", autoHarvestSeed );
	out.insert( "AutoHarvestItem1", autoHarvestItem1 );
	out.insert( "AutoHarvestItem2", autoHarvestItem2 );

	out.insert( "AutoHarvestSeedMin", autoHarvestSeedMin );
	out.insert( "AutoHarvestSeedMax", autoHarvestSeedMax );
	out.insert( "AutoHarvestItem1Min", autoHarvestItem1Min );
	out.insert( "AutoHarvestItem1Max", autoHarvestItem1Max );
	out.insert( "AutoHarvestItem2Min", autoHarvestItem2Min );
	out.insert( "AutoHarvestItem2Max", autoHarvestItem2Max );
}

Farm::Farm( QList<QPair<Position, bool>> tiles, Game* game ) :
	WorldObject( game )
{
	m_name = "Farm";

	m_properties.jobPriorities.clear();
	m_properties.jobPriorities.push_back( FarmJobs::Harvest );
	m_properties.jobPriorities.push_back( FarmJobs::PlantPlant );
	m_properties.jobPriorities.push_back( FarmJobs::Till );

	for ( auto p : tiles )
	{
		if ( p.second )
		{
			m_fields.insert( p.first.toInt(), FarmField(p.first, nullptr));
		}
	}

	if (!m_fields.empty())
	{
		m_properties.firstPos = m_fields.first().pos;
	}
}

Farm::Farm( QVariantMap vals, Game* game ) :
	WorldObject( vals, game ),
	m_properties( vals )
{
	QVariantList vfl = vals.value( "Fields" ).toList();
	for ( auto vf : vfl )
	{
		FarmField grofi;
		auto gfm = vf.toMap();
		grofi.pos       = Position( gfm.value( "Pos" ).toString() );
		if( gfm.contains( "Job" ) )
		{
			grofi.job = g->jm()->getJob( gfm.value( "Job" ).toUInt() );
		}
		m_fields.insert( grofi.pos.toInt(), std::move(grofi));
	}

	if (!m_fields.empty())
	{
		m_properties.firstPos = m_fields.first().pos;
	}

	QVariantList vjl = vals.value( "Jobs" ).toList();
	for ( auto vj : vjl )
	{
		QSharedPointer<Job> job( new Job( vj.toMap() ) );
	}
}

QVariant Farm::serialize() const
{
	QVariantMap out;
	WorldObject::serialize( out );
	m_properties.serialize( out );

	QVariantList tiles;
	for ( const auto& field : m_fields )
	{
		QVariantMap entry;
		entry.insert( "Pos", field.pos.toString() );
		if( field.job )
		{
			QSharedPointer<Job> spJob = field.job.toStrongRef();
			entry.insert( "Job", spJob->id() );
		}
		tiles.append( entry );
	}
	out.insert( "Fields", tiles );
	return out;
}

Farm::~Farm()
{

}

void Farm::addTile( const Position & pos )
{
	m_fields.insert( pos.toInt(), FarmField(pos, nullptr) );
	g->w()->setTileFlag( pos, TileFlag::TF_FARM );
}

void Farm::onTick( quint64 tick )
{
	if ( !m_active )
		return;

	for( auto& gf : m_fields )
	{
		if( !gf.job )
		{
			Tile& tile = g->w()->getTile( gf.pos );

			if ( g->w()->plants().contains( gf.pos.toInt() ) )
			{
				Plant& plant = g->w()->plants()[gf.pos.toInt()];
				if ( !plant.isPlant() )
				{
					continue;
				}
				if ( plant.harvestable() )
				{
					//harvest
					unsigned int jobID = g->jm()->addJob( "Harvest", gf.pos, 0, true );
					auto job = g->jm()->getJob( jobID );
					if( job )
					{
						//job->setPrio();
						gf.job = job;
					}
				}
			}
			else
			{
				if ( !( tile.flags & TileFlag::TF_TILLED ) )
				{
					//till
					unsigned int jobID = g->jm()->addJob( "Till", gf.pos, 0, true );
					auto job = g->jm()->getJob( jobID );
					if( job )
					{
						//job->setPrio();
						gf.job = job;
					}
				}
				if ( tile.flags & TileFlag::TF_TILLED )
				{
					auto item = g->inv()->getClosestItem( m_fields.first().pos, true, m_properties.seedItem, m_properties.plantType );
					if ( item == 0 )
					{
						continue;
					}
					unsigned int jobID = g->jm()->addJob( "PlantFarm", gf.pos, "Plant", { m_properties.plantType}, 0, true );
					auto job = g->jm()->getJob( jobID );
					if( job )
					{
						job->addRequiredItem( 1, m_properties.seedItem, m_properties.plantType, QStringList() );
						//job->setPrio();
						gf.job = job;
					}
				}
			}
		}
	}

	//updateAutoFarmer();
}

void Farm::updateAutoFarmer()
{
	QString seedMaterialID = DB::select( "Material", "Plants", m_properties.plantType ).toString();

	unsigned int countSeed = g->inv()->itemCount( m_properties.seedItem, seedMaterialID );

	auto hl = DB::selectRows( "Plants_OnHarvest_HarvestedItem", m_properties.plantType );

	QString item1ID;
	QString material1ID;
	QString item2ID;
	QString material2ID;
	for ( auto hi : hl )
	{
		item1ID = hi.value( "ItemID" ).toString();
		if ( item1ID != m_properties.seedItem )
		{
			material1ID = hi.value( "MaterialID" ).toString();
			break;
		}
	}
	unsigned int countItem1 = 0;
	unsigned int countItem2 = 0;
	if ( !item1ID.isEmpty() )
	{
		countItem1 = g->inv()->itemCount( item1ID, material1ID );
	}

	for ( auto hi : hl )
	{
		item2ID = hi.value( "ItemID" ).toString();
		if ( item2ID != m_properties.seedItem && item2ID != item1ID )
		{
			material2ID = hi.value( "MaterialID" ).toString();
			break;
		}
	}
	if ( !item2ID.isEmpty() )
	{
		countItem2 = g->inv()->itemCount( item2ID, material2ID );
	}
	bool harvestOn  = false;
	bool harvestOff = false;

	if ( m_properties.autoHarvestSeed )
	{
		unsigned int count = g->inv()->itemCount( m_properties.seedItem, seedMaterialID );
		unsigned int min   = m_properties.autoHarvestSeedMin;
		unsigned int max   = m_properties.autoHarvestSeedMax;
		if ( count < min )
		{
			harvestOn = true;
		}
		if ( count > max )
		{
			harvestOff = true;
		}
	}
	if ( m_properties.autoHarvestItem1 && !item1ID.isEmpty() )
	{
		unsigned int count = g->inv()->itemCount( item1ID, material1ID );
		unsigned int min   = m_properties.autoHarvestItem1Min;
		unsigned int max   = m_properties.autoHarvestItem1Max;
		if ( count < min )
		{
			harvestOn = true;
		}
		if ( count > max )
		{
			harvestOff = true;
		}
	}
	if ( m_properties.autoHarvestItem2 && !item2ID.isEmpty() )
	{
		unsigned int count = g->inv()->itemCount( item2ID, material2ID );
		unsigned int min   = m_properties.autoHarvestItem2Min;
		unsigned int max   = m_properties.autoHarvestItem2Max;
		if ( count < min )
		{
			harvestOn = true;
		}
		if ( count > max )
		{
			harvestOff = true;
		}
	}

	if ( harvestOn )
	{
		m_properties.harvest = true;
	}
	else
	{
		if ( harvestOff )
		{
			m_properties.harvest = false;
		}
	}
}

bool Farm::canDelete()
{
	return true; //m_jobsOut.isEmpty();
}

bool Farm::removeTile( const Position & pos )
{
	auto id = pos.toInt();
	if( m_fields.contains( id ) )
	{
		auto gf = m_fields.value( id );
		if( gf.job )
		{
			//qDebug() << "farm field still has a job";
			g->jm()->deleteJobAt( pos );
		}
	}
	m_fields.remove( id );
	g->w()->clearTileFlag( pos, TileFlag::TF_FARM );

	if (!m_fields.empty())
	{
		m_properties.firstPos = m_fields.first().pos;
	}
	else
	{
		m_properties.firstPos = Position();
	}

	// if last tile deleted return true
	return m_fields.empty();
}

void Farm::getInfo( int& numPlots, int& tilled, int& planted, int& cropReady )
{
	numPlots  = m_fields.size();
	tilled    = 0;
	planted   = 0;
	cropReady = 0;
	for ( const auto& gf : m_fields )
	{

		Tile& tile = g->w()->getTile( gf.pos );
		if ( tile.flags & TileFlag::TF_TILLED )
		{
			++tilled;
		}
		if ( g->w()->plants().contains( gf.pos.toInt() ) )
		{
			Plant& plant = g->w()->plants()[gf.pos.toInt()];

			if ( plant.isPlant() )
			{
				++planted;
				if ( plant.harvestable() )
				{
					++cropReady;
				}
			}
		}
	}
}

int Farm::countTiles()
{
	return m_fields.size();
}

void Farm::setPlantType( QString plantID )
{
	m_properties.plantType = plantID;

	QString seedItemID    = DB::select( "SeedItemID", "Plants", plantID ).toString();
	m_properties.seedItem = seedItemID;
}

void Farm::setHarvest( bool harvest )
{
	m_properties.harvest = harvest;
}