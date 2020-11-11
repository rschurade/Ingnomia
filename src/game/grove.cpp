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
#include "grove.h"

#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/util.h"
#include "../game/gnomemanager.h"
#include "../game/inventory.h"
#include "../game/jobmanager.h"
#include "../game/plant.h"
#include "../game/world.h"

#include <QDebug>

Grove::Grove()
{
}
Grove::~Grove()
{
}

GroveProperties::GroveProperties( QVariantMap& in )
{
	auto vjpl = in.value( "JobPriorities" ).toList();
	for ( auto vjp : vjpl )
	{
		jobPriorities.append( vjp.value<quint8>() );
	}

	treeType  = in.value( "TreeType" ).toString();
	plant     = in.value( "Plant" ).toBool();
	pickFruit = in.value( "PickFruit" ).toBool();
	fell      = in.value( "Fell" ).toBool();

	autoPick = in.value( "AutoPick" ).toBool();
	autoFell = in.value( "AutoFell" ).toBool();

	autoPickMin = in.value( "AutoPickMin" ).toInt();
	autoPickMax = in.value( "AutoPickMax" ).toInt();
	autoFellMin = in.value( "AutoFellMin" ).toInt();
	autoFellMax = in.value( "AutoFellMax" ).toInt();
}

void GroveProperties::serialize( QVariantMap& out )
{
	out.insert( "Type", "grove" );

	out.insert( "TreeType", treeType );
	out.insert( "Plant", plant );
	out.insert( "PickFruit", pickFruit );
	out.insert( "Fell", fell );

	out.insert( "AutoPick", autoPick );
	out.insert( "AutoFell", autoFell );

	out.insert( "AutoPickMin", autoPickMin );
	out.insert( "AutoPickMax", autoPickMax );
	out.insert( "AutoFellMin", autoFellMin );
	out.insert( "AutoFellMax", autoFellMax );

	QVariantList jpl;
	for ( auto jp : jobPriorities )
	{
		jpl.append( jp );
	}
	out.insert( "JobPriorities", jpl );
}

Grove::Grove( QList<QPair<Position, bool>> tiles ) :
	WorldObject()
{
	m_name = "Grove";

	m_properties.jobPriorities.clear();
	m_properties.jobPriorities.push_back( GroveJobs::PlantTree );
	m_properties.jobPriorities.push_back( GroveJobs::PickFruit );
	m_properties.jobPriorities.push_back( GroveJobs::FellTree );

	for ( auto p : tiles )
	{
		if ( p.second )
		{
			GroveField* grofi = new GroveField;
			grofi->pos        = p.first;
			m_fields.insert( p.first.toInt(), grofi );
		}
	}
}

Grove::Grove( QVariantMap vals ) :
	WorldObject( vals ),
	m_properties( vals )
{
	QVariantList vfl = vals.value( "Fields" ).toList();
	for ( auto vf : vfl )
	{
		auto vfm          = vf.toMap();
		GroveField* grofi = new GroveField;
		grofi->pos        = Position( vfm.value( "Pos" ).toString() );
		grofi->hasJob     = vfm.value( "HasJob" ).toBool();
		grofi->harvested  = vfm.value( "Harvested" ).toBool();
		m_fields.insert( grofi->pos.toInt(), grofi );
	}

	QVariantList vjl = vals.value( "Jobs" ).toList();
	for ( auto vj : vjl )
	{
		Job* job = new Job( vj.toMap() );
		m_jobsOut.insert( job->id(), job );
	}
}

QVariant Grove::serialize()
{
	QVariantMap out;
	WorldObject::serialize( out );
	m_properties.serialize( out );
	QVariantList tiles;
	for ( auto field : m_fields )
	{
		QVariantMap entry;
		entry.insert( "Pos", field->pos.toString() );
		entry.insert( "HasJob", field->hasJob );
		entry.insert( "Harvested", field->harvested );
		tiles.append( entry );
	}
	out.insert( "Fields", tiles );

	QVariantList jobs;
	for ( auto job : m_jobsOut )
	{
		jobs.append( job->serialize() );
	}
	out.insert( "Jobs", jobs );

	return out;
}

void Grove::onTick( quint64 tick )
{
	if ( !m_active )
		return;
	m_prioValues.clear();
	for ( auto p : m_properties.jobPriorities )
	{
		m_prioValues.insert( p, m_prioValues.size() );
	}

	updateAutoForester();
}

void Grove::updateAutoForester()
{
	if ( m_properties.autoPick )
	{
		unsigned int count = Global::inv().itemCount( "Fruit", m_properties.treeType );
		unsigned int min   = m_properties.autoPickMin;
		unsigned int max   = m_properties.autoPickMax;
		if ( count < min )
		{
			m_properties.pickFruit = true;
		}
		if ( count > max )
		{
			m_properties.pickFruit = false;
		}
	}
	if ( m_properties.autoFell )
	{
		unsigned int count = Global::inv().itemCount( "RawWood", m_properties.treeType );
		unsigned int min   = m_properties.autoFellMin;
		unsigned int max   = m_properties.autoFellMax;
		if ( count < min )
		{
			m_properties.fell = true;
		}
		if ( count > max )
		{
			m_properties.fell = false;
		}
	}
}

unsigned int Grove::getJob( unsigned int gnomeID, QString skillID )
{
	if ( !m_active )
		return 0;
	Job* job = 0;

	for ( auto p : m_properties.jobPriorities )
	{
		switch ( p )
		{
			case PlantTree:
				if ( skillID == "Horticulture" )
				{
					job = getPlantJob();
				}
				break;
			case PickFruit:
				if ( skillID == "Horticulture" )
				{
					job = getPickJob();
				}
				break;
			case FellTree:
				if ( skillID == "Woodcutting" )
				{
					job = getFellJob();
				}
				break;
		}
		if ( job )
		{
			m_jobsOut.insert( job->id(), job );
			return job->id();
		}
	}
	return 0;
}

bool Grove::finishJob( unsigned int jobID )
{
	if ( m_jobsOut.contains( jobID ) )
	{
		Job* job          = m_jobsOut[jobID];
		GroveField* grofi = m_fields[job->pos().toInt()];

		if ( job->type() == "PlantTreeGrove" )
		{
			grofi->harvested = false;
		}
		else if ( job->type() == "Harvest" )
		{
			grofi->harvested = true;
		}
		else if ( job->type() == "FellTree" )
		{
			grofi->harvested = false;
		}

		m_jobsOut.remove( jobID );
		m_fields[job->pos().toInt()]->hasJob = false;

		delete job;

		return true;
	}
	return false;
}

bool Grove::giveBackJob( unsigned int jobID )
{
	if ( m_jobsOut.contains( jobID ) )
	{
		Job* job                             = m_jobsOut[jobID];
		m_fields[job->pos().toInt()]->hasJob = false;
		m_jobsOut.remove( jobID );
		delete job;
		return true;
	}
	return false;
}

Job* Grove::getJob( unsigned int jobID )
{
	if ( m_jobsOut.contains( jobID ) )
	{
		return m_jobsOut[jobID];
	}
	return nullptr;
}

bool Grove::hasJobID( unsigned int jobID )
{
	return m_jobsOut.contains( jobID );
}

bool Grove::hasPlantTreeJob( Position pos )
{
	for ( auto job : m_jobsOut )
	{
		if ( job->pos() == pos )
		{
			return true;
		}
	}
	return false;
}

Job* Grove::getPlantJob()
{
	Job* job = 0;
	for ( auto gf : m_fields )
	{
		// tile is empty, we plant something
		if ( !gf->hasJob && m_properties.plant && !Global::w().plants().contains( gf->pos.toInt() ) && Global::w().noTree( gf->pos, 2, 2 ) && Global::w().isWalkable( gf->pos ) )
		{
			QString mat    = DB::select( "Material", "Plants", m_properties.treeType ).toString();
			QString seedID = DB::select( "SeedItemID", "Plants", m_properties.treeType ).toString();

			if ( Global::inv().itemCount( seedID, mat ) > 0 )
			{
				Job* job = new Job();
				job->setType( "PlantTree" );
				job->setRequiredSkill( Util::requiredSkill( "PlantTree" ) );
				job->setPos( gf->pos );
				job->addPossibleWorkPosition( gf->pos );
				job->setItem( m_properties.treeType );
				job->addRequiredItem( 1, seedID, mat, QStringList() );

				gf->hasJob = true;
				return job;
			}
			else
			{
				return nullptr;
			}
		}
	}
	return nullptr;
}

Job* Grove::getPickJob()
{
	Job* job = new Job();
	for ( auto gf : m_fields )
	{
		if ( !gf->hasJob && Global::w().plants().contains( gf->pos.toInt() ) )
		{
			Plant& tree = Global::w().plants()[gf->pos.toInt()];
			if ( !tree.isTree() )
			{
				continue;
			}
			if ( m_properties.pickFruit && tree.harvestable() )
			{
				bool hasWorkPos = false;
				if ( Global::w().isWalkable( gf->pos.northOf() ) )
				{
					job->addPossibleWorkPosition( gf->pos.northOf() );
					hasWorkPos = true;
				}
				if ( Global::w().isWalkable( gf->pos.eastOf() ) )
				{
					job->addPossibleWorkPosition( gf->pos.eastOf() );
					hasWorkPos = true;
				}
				if ( Global::w().isWalkable( gf->pos.southOf() ) )
				{
					job->addPossibleWorkPosition( gf->pos.southOf() );
					hasWorkPos = true;
				}
				if ( Global::w().isWalkable( gf->pos.westOf() ) )
				{
					job->addPossibleWorkPosition( gf->pos.westOf() );
					hasWorkPos = true;
				}
				if ( !hasWorkPos )
				{
					continue;
				}

				job->setType( "HarvestTree" );
				job->setRequiredSkill( Util::requiredSkill( "HarvestTree" ) );
				job->setPos( gf->pos );

				job->setNoJobSprite( true );
				gf->hasJob = true;
				return job;
			}
		}
	}
	delete job;
	return nullptr;
}

Job* Grove::getFellJob()
{
	Job* job = new Job();
	for ( auto gf : m_fields )
	{
		if ( !gf->hasJob && Global::w().plants().contains( gf->pos.toInt() ) )
		{
			Plant& tree = Global::w().plants()[gf->pos.toInt()];
			if ( !tree.isTree() )
			{
				continue;
			}

			if ( tree.isFruitTree() && m_properties.pickFruit && ( m_prioValues.value( PickFruit ) < m_prioValues.value( FellTree ) ) && !gf->harvested )
			{
				continue;
			}

			if ( m_properties.fell && tree.matureWood() )
			{
				bool hasWorkPos = false;
				if ( Global::w().isWalkable( gf->pos.northOf() ) )
				{
					job->addPossibleWorkPosition( gf->pos.northOf() );
					hasWorkPos = true;
				}
				if ( Global::w().isWalkable( gf->pos.eastOf() ) )
				{
					job->addPossibleWorkPosition( gf->pos.eastOf() );
					hasWorkPos = true;
				}
				if ( Global::w().isWalkable( gf->pos.southOf() ) )
				{
					job->addPossibleWorkPosition( gf->pos.southOf() );
					hasWorkPos = true;
				}
				if ( Global::w().isWalkable( gf->pos.westOf() ) )
				{
					job->addPossibleWorkPosition( gf->pos.westOf() );
					hasWorkPos = true;
				}
				if ( !hasWorkPos )
				{
					continue;
				}

				job->setType( "FellTree" );
				job->setRequiredSkill( Util::requiredSkill( "FellTree" ) );
				job->setPos( gf->pos );
				job->setRequiredTool( "FellingAxe", 1 );
				job->setNoJobSprite( true );
				gf->hasJob = true;
				return job;
			}
		}
	}
	delete job;
	return nullptr;
}

bool Grove::removeTile( Position& pos )
{
	GroveField* gf = m_fields.value( pos.toInt() );

	m_fields.remove( pos.toInt() );

	Global::w().clearTileFlag( pos, TileFlag::TF_GROVE );
	delete gf;
	// if last tile deleted return true
	return m_fields.empty();
}

void Grove::addTile( Position& pos )
{
	GroveField* grofi = new GroveField;
	grofi->pos        = pos;
	m_fields.insert( pos.toInt(), grofi );
}
