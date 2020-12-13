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
			FarmField* grofi = new FarmField;
			grofi->pos       = p.first;
			m_fields.insert( p.first.toInt(), grofi );
		}
	}

	if ( m_fields.size() )
	{
		for ( auto field : m_fields )
		{
			m_properties.firstPos = field->pos;
			break;
		}
	}
}

Farm::Farm( QVariantMap vals, Game* game ) :
	WorldObject( vals, game ),
	m_properties( vals )
{
	QVariantList vfl = vals.value( "Fields" ).toList();
	for ( auto vf : vfl )
	{
		FarmField* grofi = new FarmField;
		grofi->pos       = Position( vf.toMap().value( "Pos" ).toString() );
		grofi->hasJob    = vf.toMap().value( "HasJob" ).toBool();
		m_fields.insert( grofi->pos.toInt(), grofi );
	}

	if ( m_fields.size() )
	{
		for ( auto field : m_fields )
		{
			m_properties.firstPos = field->pos;
			break;
		}
	}

	QVariantList vjl = vals.value( "Jobs" ).toList();
	for ( auto vj : vjl )
	{
		Job* job = new Job( vj.toMap() );
		m_jobsOut.insert( job->id(), job );
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
		entry.insert( "Pos", field->pos.toString() );
		entry.insert( "HasJob", field->hasJob );
		tiles.append( entry );
	}
	out.insert( "Fields", tiles );

	QVariantList jobs;
	for ( const auto& job : m_jobsOut )
	{
		jobs.append( job->serialize() );
	}
	out.insert( "Jobs", jobs );

	return out;
}

Farm::~Farm()
{
	for ( const auto& field : m_fields )
	{
		delete field;
	}
	for ( const auto& job : m_jobsOut )
	{
		delete job;
	}
}

void Farm::addTile( const Position & pos )
{
	FarmField* grofi = new FarmField;
	grofi->pos       = pos;
	m_fields.insert( pos.toInt(), grofi );

	g->w()->setTileFlag( pos, TileFlag::TF_FARM );
}

void Farm::onTick( quint64 tick )
{
	if ( !m_active )
		return;
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

unsigned int Farm::getJob( unsigned int gnomeID, QString skillID )
{
	if ( !m_active )
		return 0;
	if ( g->gm()->gnomeCanReach( gnomeID, m_properties.firstPos ) )
	{
		Job* job = nullptr;
		if ( skillID == "Farming" )
		{
			for ( auto p : m_properties.jobPriorities )
			{
				switch ( p )
				{
					case FarmJobs::Till:
						job = getTillJob();
						if ( job )
						{
							m_jobsOut.insert( job->id(), job );
							return job->id();
						}
						break;
					case FarmJobs::PlantPlant:
						job = getPlantJob();
						if ( job )
						{
							m_jobsOut.insert( job->id(), job );
							return job->id();
						}
						break;
					case FarmJobs::Harvest:
						job = getHarvestJob();
						if ( job )
						{
							m_jobsOut.insert( job->id(), job );
							return job->id();
						}
						break;
				}
			}
		}
	}
	return 0;
}

bool Farm::finishJob( unsigned int jobID )
{
	if ( m_jobsOut.contains( jobID ) )
	{
		Job* job = m_jobsOut[jobID];
		m_jobsOut.remove( jobID );
		if ( m_fields.contains( job->pos().toInt() ) )
		{
			m_fields[job->pos().toInt()]->hasJob = false;
		}
		delete job;
		return true;
	}
	return false;
}

bool Farm::giveBackJob( unsigned int jobID )
{
	if ( m_jobsOut.contains( jobID ) )
	{
		Job* job = m_jobsOut[jobID];
		if ( m_fields.contains( job->pos().toInt() ) )
		{
			m_fields[job->pos().toInt()]->hasJob = false;
		}
		m_jobsOut.remove( jobID );
		delete job;
		return true;
	}
	return false;
}

Job* Farm::getJob( unsigned int jobID ) const
{
	if ( m_jobsOut.contains( jobID ) )
	{
		return m_jobsOut[jobID];
	}
	return nullptr;
}

bool Farm::hasJobID( unsigned int jobID ) const
{
	return m_jobsOut.contains( jobID );
}

Job* Farm::getTillJob()
{
	if ( m_fields.empty() )
	{
		return nullptr;
	}
	for ( auto gf : m_fields )
	{
		Tile& tile = g->w()->getTile( gf->pos );
		if ( !gf->hasJob && !g->w()->plants().contains( gf->pos.toInt() ) && !( tile.flags & TileFlag::TF_TILLED ) )
		{
			Job* job = new Job;
			job->setType( "Till" );
			job->setRequiredSkill( Global::util->requiredSkill( "Till" ) );
			job->setPos( gf->pos );
			job->addPossibleWorkPosition( gf->pos );
			job->setNoJobSprite( true );
			gf->hasJob = true;
			return job;
		}
	}
	return nullptr;
}

bool Farm::canDelete()
{
	return m_jobsOut.isEmpty();
}

Job* Farm::getPlantJob()
{
	//any seeds present?
	if ( m_fields.empty() )
	{
		return nullptr;
	}

	auto item      = g->inv()->getClosestItem( m_fields.first()->pos, true, m_properties.seedItem, m_properties.plantType );

	if ( item == 0 )
	{
		return nullptr;
	}

	for ( auto gf : m_fields )
	{
		Tile& tile = g->w()->getTile( gf->pos );
		// tile is empty, we plant something
		if ( !gf->hasJob && !g->w()->plants().contains( gf->pos.toInt() ) && ( tile.flags & TileFlag::TF_TILLED ) )
		{
			Job* job = new Job;
			job->setType( "PlantFarm" );
			job->setRequiredSkill( Global::util->requiredSkill( "PlantFarm" ) );
			job->setPos( gf->pos );
			job->addPossibleWorkPosition( gf->pos );
			job->setItem( "Plant" );
			job->setMaterial( m_properties.plantType );
			job->setNoJobSprite( true );
			job->addRequiredItem( 1, m_properties.seedItem, m_properties.plantType, QStringList() );

			gf->hasJob = true;
			return job;
		}
	}
	return nullptr;
}

Job* Farm::getHarvestJob()
{
	if ( m_fields.empty() )
	{
		return nullptr;
	}
	if ( m_properties.harvest )
	{
		for ( auto gf : m_fields )
		{
			if ( !gf->hasJob && g->w()->plants().contains( gf->pos.toInt() ) )
			{
				Plant& plant = g->w()->plants()[gf->pos.toInt()];
				if ( !plant.isPlant() )
				{
					continue;
				}
				if ( plant.harvestable() )
				{
					Job* job = new Job;
					job->setType( "Harvest" );
					job->setRequiredSkill( Global::util->requiredSkill( "Harvest" ) );
					job->setPos( gf->pos );
					job->addPossibleWorkPosition( gf->pos );
					job->setNoJobSprite( true );
					gf->hasJob = true;
					return job;
				}
			}
		}
	}
	return nullptr;
}

bool Farm::removeTile( const Position & pos )
{
	FarmField* ff = m_fields.value( pos.toInt() );

	m_fields.remove( pos.toInt() );

	g->w()->clearTileFlag( pos, TileFlag::TF_FARM );
	delete ff;

	if ( m_fields.size() )
	{
		for ( auto field : m_fields )
		{
			m_properties.firstPos = field->pos;
			break;
		}
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
	for ( auto gf : m_fields )
	{

		Tile& tile = g->w()->getTile( gf->pos );
		if ( tile.flags & TileFlag::TF_TILLED )
		{
			++tilled;
		}
		if ( g->w()->plants().contains( gf->pos.toInt() ) )
		{
			Plant& plant = g->w()->plants()[gf->pos.toInt()];

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