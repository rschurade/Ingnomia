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
#include "fluidmanager.h"
#include "game.h"

#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/util.h"
#include "../game/mechanismmanager.h"
#include "../game/world.h"
#include "../gui/strings.h"

#include <QDebug>
#include <QQueue>

QVariantMap NetworkPipe::serialize() const
{
	QVariantMap out;

	out.insert( "ItemID", itemUID );
	out.insert( "Pos", pos.toString() );
	out.insert( "Type", type );

	return out;
}

void NetworkPipe::deserialize( QVariantMap in )
{
	itemUID = in.value( "ItemID" ).toUInt();
	pos     = Position( in.value( "Pos" ) );
	type    = (PipeType)in.value( "Type" ).value<unsigned char>();
}

FluidManager::FluidManager( Game* parent ) :
	g( parent ),
	QObject( parent )
{
}

FluidManager::~FluidManager()
{
}

void FluidManager::loadPipes( QVariantList data )
{
	for ( auto vdata : data )
	{
		NetworkPipe np;
		auto vnp = vdata.toMap();

		np.deserialize( vnp );

		m_allPipes.insert( np.pos.toInt(), np );
		switch ( np.type )
		{
			case PT_PIPE:
				break;
			case PT_INPUT:
				m_inputs.append( np.pos );
				break;
			case PT_OUTPUT:
				m_outputs.append( np.pos );
				break;
		}
		/*
		if( md.jobID )
		{
			QSharedPointer<Job> job = new Job( vmd.value( "Job" ).toMap() );
			m_jobs.insert( md.jobID, job );
		}
		*/
	}
	updateNetwork();
}

void FluidManager::onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	int tickDiff = tickNumber - m_lastTick;

	if ( tickDiff < 20 )
		return;
	m_lastTick = tickNumber;

	bool needUpdate = false;

	QQueue<Position> workQueue;

	// for all outputs
	// empty outputs if possible
	// cue all incoming connections in next order
	for ( auto outPos : m_outputs )
	{
		if ( m_allPipes.contains( outPos.toInt() ) )
		{
			NetworkPipe& nw = m_allPipes[outPos.toInt()];

			unsigned char fl = g->m_world->fluidLevel( nw.pos );

			if ( nw.level > 0 && fl < 10 )
			{
				nw.level -= 1;
				g->m_world->changeFluidLevel( nw.pos, +1 );
			}
			if ( nw.ins.size() )
			{
				for ( auto inPos : nw.ins )
				{
					workQueue.enqueue( inPos );
				}
				auto pos = nw.ins.takeFirst();
				nw.ins.push_back( pos );
			}
		}
		else
		{
			m_allPipes.remove( outPos.toInt() );
		}
	}
	while ( !workQueue.isEmpty() )
	{
		Position pos = workQueue.dequeue();
		if ( m_allPipes.contains( pos.toInt() ) )
		{
			NetworkPipe& nw = m_allPipes[pos.toInt()];

			auto tempList = nw.outs;

			for ( auto outPos : tempList )
			{
				auto pos2 = nw.outs.takeFirst();
				nw.outs.push_back( pos2 );

				NetworkPipe& outPipe = m_allPipes[outPos.toInt()];
				if ( nw.level > 0 && outPipe.level < outPipe.capacity )
				{
					outPipe.level += 1;
					nw.level -= 1;

					break;
				}
			}

			if ( nw.ins.size() )
			{
				for ( auto inPos : nw.ins )
				{
					workQueue.enqueue( inPos );
				}
				auto pos2 = nw.ins.takeFirst();
				nw.ins.push_back( pos2 );
			}
		}
		else
		{
			m_allPipes.remove( pos.toInt() );
		}
	}
	for ( auto inPos : m_inputs )
	{
		if ( m_allPipes.contains( inPos.toInt() ) )
		{
			NetworkPipe& nw = m_allPipes[inPos.toInt()];

			if ( nw.level < nw.capacity )
			{
				if ( g->m_mechanismManager->hasPower( inPos ) )
				{
					unsigned char bfl = g->m_world->fluidLevel( inPos.belowOf() );
					if ( bfl > 0 )
					{
						nw.level += 1;
						g->m_world->changeFluidLevel( inPos.belowOf(), -1 );
					}
				}
			}
		}
	}
}

void FluidManager::addInput( Position pos, unsigned int itemUID )
{
	NetworkPipe pipe;
	pipe.pos     = pos;
	pipe.itemUID = itemUID;
	pipe.type    = PT_INPUT;
	m_allPipes.insert( pos.toInt(), pipe );
	m_inputs.append( pos );
	updateNetwork();
}

void FluidManager::addPipe( Position pos, unsigned int itemUID )
{
	NetworkPipe pipe;
	pipe.pos     = pos;
	pipe.itemUID = itemUID;
	pipe.type    = PT_PIPE;
	m_allPipes.insert( pos.toInt(), pipe );
	updateNetwork();
}

void FluidManager::addOutput( Position pos, unsigned int itemUID )
{
	NetworkPipe pipe;
	pipe.pos     = pos;
	pipe.itemUID = itemUID;
	pipe.type    = PT_OUTPUT;
	m_allPipes.insert( pos.toInt(), pipe );
	m_outputs.append( pos );
	updateNetwork();
}

void FluidManager::removeAt( Position pos )
{
	m_allPipes.remove( pos.toInt() );
	m_inputs.removeAll( pos );
	m_outputs.removeAll( pos );
	updateNetwork();
}

void FluidManager::updateNetwork()
{
	for ( auto& nw : m_allPipes )
	{
		nw.ins.clear();
		nw.outs.clear();
	}
	QQueue<Position> workQueue;
	for ( auto pos : m_outputs )
	{
		Position n = pos.northOf();
		Position e = pos.eastOf();
		Position s = pos.southOf();
		Position w = pos.westOf();

		NetworkPipe& nw = m_allPipes[pos.toInt()];

		if ( m_allPipes.contains( n.toInt() ) )
		{
			nw.ins.append( n );
			workQueue.enqueue( n );

			NetworkPipe& nwn = m_allPipes[n.toInt()];
			nwn.outs.append( pos );
		}
		if ( m_allPipes.contains( e.toInt() ) )
		{
			nw.ins.append( e );
			workQueue.enqueue( e );
			NetworkPipe& nwe = m_allPipes[e.toInt()];
			nwe.outs.append( pos );
		}
		if ( m_allPipes.contains( s.toInt() ) )
		{
			nw.ins.append( s );
			workQueue.enqueue( s );
			NetworkPipe& nws = m_allPipes[s.toInt()];
			nws.outs.append( pos );
		}
		if ( m_allPipes.contains( w.toInt() ) )
		{
			nw.ins.append( w );
			workQueue.enqueue( w );
			NetworkPipe& nww = m_allPipes[w.toInt()];
			nww.outs.append( pos );
		}
	}
	while ( !workQueue.isEmpty() )
	{
		Position pos    = workQueue.dequeue();
		NetworkPipe& nw = m_allPipes[pos.toInt()];

		Position n = pos.northOf();
		Position e = pos.eastOf();
		Position s = pos.southOf();
		Position w = pos.westOf();

		if ( m_allPipes.contains( n.toInt() ) && !nw.outs.contains( n ) )
		{
			nw.ins.append( n );
			workQueue.enqueue( n );

			NetworkPipe& nwn = m_allPipes[n.toInt()];
			nwn.outs.append( pos );
		}
		if ( m_allPipes.contains( e.toInt() ) && !nw.outs.contains( e ) )
		{
			nw.ins.append( e );
			workQueue.enqueue( e );
			NetworkPipe& nwe = m_allPipes[e.toInt()];
			nwe.outs.append( pos );
		}
		if ( m_allPipes.contains( s.toInt() ) && !nw.outs.contains( s ) )
		{
			nw.ins.append( s );
			workQueue.enqueue( s );
			NetworkPipe& nws = m_allPipes[s.toInt()];
			nws.outs.append( pos );
		}
		if ( m_allPipes.contains( w.toInt() ) && !nw.outs.contains( w ) )
		{
			nw.ins.append( w );
			workQueue.enqueue( w );
			NetworkPipe& nww = m_allPipes[w.toInt()];
			nww.outs.append( pos );
		}
	}
}

unsigned int FluidManager::getJob( unsigned int gnomeID, QString skillID )
{

	return 0;
}

bool FluidManager::finishJob( unsigned int jobID )
{
	return false;
}

bool FluidManager::giveBackJob( unsigned int jobID )
{
	return finishJob( jobID );
}

QSharedPointer<Job> FluidManager::getJob( unsigned int jobID )
{
	return nullptr;
}

bool FluidManager::hasJobID( unsigned int jobID ) const
{
	return false;
}
