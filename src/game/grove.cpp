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
#include "../game/game.h"
#include "../game/gnomemanager.h"
#include "../game/inventory.h"
#include "../game/jobmanager.h"
#include "../game/plant.h"
#include "../game/world.h"

#include <QDebug>

Grove::~Grove()
{
	for ( const auto& field : m_fields )
	{
		delete field;
	}
}

GroveProperties::GroveProperties( QVariantMap& in )
{
	auto vjpl = in.value( "JobPriorities" ).toList();
	for ( const auto& vjp : vjpl )
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

void GroveProperties::serialize( QVariantMap& out ) const
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

Grove::Grove( QList<QPair<Position, bool>> tiles, Game* game ) :
	WorldObject( game )
{
	m_name = "Grove";

	m_properties.jobPriorities.clear();
	m_properties.jobPriorities.push_back( GroveJobs::PlantTree );
	m_properties.jobPriorities.push_back( GroveJobs::PickFruit );
	m_properties.jobPriorities.push_back( GroveJobs::FellTree );

	for ( const auto& p : tiles )
	{
		if ( p.second )
		{
			GroveField* grofi = new GroveField;
			grofi->pos        = p.first;
			m_fields.insert( p.first.toInt(), grofi );
		}
	}
}

Grove::Grove( QVariantMap vals, Game* game ) :
	WorldObject( vals, game ),
	m_properties( vals )
{
	QVariantList vfl = vals.value( "Fields" ).toList();
	for ( const auto& vf : vfl )
	{
		auto vfm          = vf.toMap();
		GroveField* grofi = new GroveField;
		grofi->pos        = Position( vfm.value( "Pos" ).toString() );
		if( vfm.contains( "Job" ) )
		{
			grofi->job = g->jm()->getJob( vfm.value( "Job" ).toUInt() );
		}
		m_fields.insert( grofi->pos.toInt(), grofi );
	}
}

QVariant Grove::serialize() const
{
	QVariantMap out;
	WorldObject::serialize( out );
	m_properties.serialize( out );
	QVariantList tiles;
	for ( auto field : m_fields )
	{
		QVariantMap entry;
		entry.insert( "Pos", field->pos.toString() );
		if( field->job )
		{
			QSharedPointer<Job> spJob = field->job.toStrongRef();
			entry.insert( "Job", spJob->id() );
		}
		tiles.append( entry );
	}
	out.insert( "Fields", tiles );

	return out;
}

void Grove::onTick( quint64 tick )
{
	if ( !m_active )
		return;
	
	for( auto& gf : m_fields )
	{
		if( !gf->job )
		{
			Tile& tile = g->w()->getTile( gf->pos );

			if( !g->w()->plants().contains( gf->pos.toInt() ) )
			{
				if ( m_properties.plant && g->w()->noTree( gf->pos, 2, 2 ) && g->w()->isWalkable( gf->pos ) )
				{
					QString mat    = DB::select( "Material", "Plants", m_properties.treeType ).toString();
					QString seedID = DB::select( "SeedItemID", "Plants", m_properties.treeType ).toString();

					auto item = g->inv()->getClosestItem( m_fields.first()->pos, true, seedID, mat );
					if ( item == 0 )
					{
						continue;
					}
					unsigned int jobID = g->jm()->addJob( "PlantTree", gf->pos, 0, true );
					auto job = g->jm()->getJob( jobID );
					if( job )
					{
						job->setItem( m_properties.treeType );
						job->addRequiredItem( 1, seedID, mat, QStringList() );
						gf->job = job;
					}
				}
			}
			else
			{
				Plant& tree = g->w()->plants()[gf->pos.toInt()];
				if ( !tree.isTree() )
				{
					continue;
				}
				if ( m_properties.pickFruit && tree.harvestable() )
				{
					unsigned int jobID = g->jm()->addJob( "HarvestTree", gf->pos, 0, true );
					auto job = g->jm()->getJob( jobID );
					if( job )
					{
						gf->job = job;
					}
						continue;
				}
				if ( m_properties.fell && tree.matureWood() )
				{
					unsigned int jobID = g->jm()->addJob( "FellTree", gf->pos, 0, true );
					auto job = g->jm()->getJob( jobID );
					if( job )
					{
						gf->job = job;
					}
					continue;
				}
			}
		}
	}


	updateAutoForester();
}

void Grove::updateAutoForester()
{
	if ( m_properties.autoPick )
	{
		unsigned int count = g->inv()->itemCount( "Fruit", m_properties.treeType );
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
		unsigned int count = g->inv()->itemCount( "RawWood", m_properties.treeType );
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



bool Grove::canDelete() const
{
	return true;
}

bool Grove::removeTile( const Position & pos )
{
	GroveField* gf = m_fields.value( pos.toInt() );

	m_fields.remove( pos.toInt() );

	g->w()->clearTileFlag( pos, TileFlag::TF_GROVE );
	delete gf;
	// if last tile deleted return true
	return m_fields.empty();
}

void Grove::addTile( const Position & pos )
{
	GroveField* grofi = new GroveField;
	grofi->pos        = pos;
	m_fields.insert( pos.toInt(), grofi );

	g->w()->setTileFlag( pos, TileFlag::TF_GROVE );
}

int Grove::numTrees()
{
	int numTrees = 0;
	for( auto& gf : m_fields )
	{
		if( g->w()->plants().contains( gf->pos.toInt() ) )
		{
			if( g->w()->plants()[gf->pos.toInt()].isTree() )
			{
				++numTrees;
			}
		}
	}
	return numTrees;
}