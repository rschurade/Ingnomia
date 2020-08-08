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
#include "aggregatorrenderer.h"

#include "../base/global.h"
#include "../game/creature.h"
#include "../game/creaturemanager.h"
#include "../game/gnomemanager.h"
#include "../game/mechanismmanager.h"
#include "../game/world.h"
#include "../gfx/sprite.h"
#include "../gfx/spritefactory.h"

AggregatorRenderer::AggregatorRenderer( QObject* parent ) :
	QObject( parent )
{
	qRegisterMetaType<TileDataUpdateInfo>();
	qRegisterMetaType<ThoughtBubbleInfo>();
	qRegisterMetaType<AxleDataInfo>();
}

TileDataUpdate AggregatorRenderer::aggregateTile( unsigned int tileID ) const
{
	const auto& tile = Global::w().world()[tileID];
	TileData td;
	if ( tile.floorSpriteUID )
	{
		unsigned int rot  = ( tile.floorRotation % 4 ) * ROT_BIT;
		td.floorSpriteUID = tile.floorSpriteUID + rot;
	}

	if ( tile.wallSpriteUID )
	{
		unsigned int rot = ( tile.wallRotation % 4 ) * ROT_BIT;
		td.wallSpriteUID = tile.wallSpriteUID + rot;
		if ( tile.wallType & WallType::WT_SOLIDWALL )
		{
			td.wallSpriteUID += WALL_BIT;
		}
	}

	if ( tile.itemSpriteUID )
	{
		td.itemSpriteUID = tile.itemSpriteUID;
	}

	if ( Global::w().hasJob( Position( tileID ) ) )
	{
		QVariantMap job      = Global::w().jobSprite( Position( tileID ) );
		QVariantMap floorJob = job.value( "Floor" ).toMap();
		QVariantMap wallJob  = job.value( "Wall" ).toMap();

		if ( !floorJob.isEmpty() )
		{
			unsigned int suid    = floorJob.value( "SpriteUID" ).toUInt();
			unsigned int rot     = floorJob.value( "Rot" ).toUInt() * ROT_BIT;
			td.jobSpriteFloorUID = suid + rot;
		}
		if ( !wallJob.isEmpty() )
		{
			unsigned int suid   = wallJob.value( "SpriteUID" ).toUInt();
			unsigned int rot    = wallJob.value( "Rot" ).toUInt() * ROT_BIT;
			td.jobSpriteWallUID = suid + rot;
		}
	}

	td.flags  = (quint64)tile.flags;
	td.flags2 = (quint64)tile.flags >> 32;

	td.lightLevel      = qMin( tile.lightLevel, (unsigned char)20 );
	td.fluidLevel      = qMin( tile.fluidLevel, (unsigned char)10 );
	td.vegetationLevel = qMin( tile.vegetationLevel, (unsigned char)100 );

	return TileDataUpdate { tileID, td };
}

QHash<unsigned int, unsigned int> AggregatorRenderer::collectCreatures()
{
	QHash<unsigned int, unsigned int> creatures;

	SpriteFactory& sf = Global::sf();
	Sprite* sprite    = nullptr;

	unsigned int posID = 0;

	//TODO remove when we create gnome corpses
	for ( auto g : Global::gm().deadGnomes() )
	{
		posID = g->getPos().toInt();
		{
			unsigned int spriteID = 0;
			sprite                = sf.getCreatureSprite( g->id(), spriteID );
			if ( sprite )
			{
				spriteID += g->facing() * ROT_BIT;
			}
			creatures[posID] = spriteID;
		}
	}

	for ( auto g : Global::gm().gnomes() )
	{
		if ( !g->goneOffMap() )
		{
			posID = g->getPos().toInt();
			{
				unsigned int spriteID = 0;
				sprite                = sf.getCreatureSprite( g->id(), spriteID );
				if ( sprite )
				{
					spriteID += g->facing() * ROT_BIT;
				}
				creatures[posID] = spriteID;
			}
		}
	}
	for ( auto g : Global::gm().specialGnomes() )
	{
		posID = g->getPos().toInt();
		{
			unsigned int spriteID = 0;
			sprite                = sf.getCreatureSprite( g->id(), spriteID );
			if ( sprite )
			{
				spriteID += g->facing() * ROT_BIT;
			}
			creatures[posID] = spriteID;
		}
	}

	for ( auto a : Global::gm().automatons() )
	{
		posID = a->getPos().toInt();
		{
			unsigned int spriteID = 0;
			sprite                = sf.getCreatureSprite( a->id(), spriteID );
			if ( sprite )
			{
				spriteID += a->facing() * ROT_BIT;
			}
			creatures[posID] = spriteID;
		}
	}

	auto& creatureList = Global::cm().creatures();

	for( auto creature : creatureList )
	{
		switch( creature->type() )
		{
			case CreatureType::ANIMAL:
			{
				auto a = dynamic_cast<Animal*>( creature );
				unsigned int posID = a->getPos().toInt();
				if ( !creatures.contains( posID ) && !a->isDead() )
				{
					if ( a->isMulti() )
					{
						auto sprites = a->multiSprites();
						for ( auto def : sprites )
						{
							posID                 = def.first.toInt();
							unsigned int spriteID = def.second;
							creatures[posID]      = spriteID;
						}
					}
					else
					{
						Sprite* sprite        = sf.getSprite( a->spriteUID() );
						unsigned int spriteID = 0;
						if ( sprite )
						{
							spriteID = sprite->uID;
							spriteID += a->facing() * ROT_BIT;
						}
						creatures[posID] = spriteID;
					}
				}
			}
			break;
			case CreatureType::MONSTER:
			{
				auto m = dynamic_cast<Monster*>( creature );
				unsigned int posID = m->getPos().toInt();
				if ( !creatures.contains( posID ) && !m->isDead() )
				{
					unsigned int spriteID = 0;
					sprite                = sf.getCreatureSprite( m->id(), spriteID );
					if ( sprite )
					{
						spriteID += m->facing() * ROT_BIT;
					}
					creatures[posID] = spriteID;
				}
			}
		}
	}
	
	return creatures;
}

void AggregatorRenderer::onAllTileInfo()
{
	// Bake tile updates
	auto creatures = collectCreatures();
	for ( auto tile = creatures.keyBegin(); tile != creatures.keyEnd(); ++tile )
	{
		//tiles.insert(*tile);
	}
	constexpr size_t batchSize = 1 << 16;
	TileDataUpdateInfo tileUpdates;
	tileUpdates.updates.reserve( batchSize );
	const unsigned int worldSize = (unsigned int)Global::w().world().size();
	for ( unsigned int tileUID = 0; tileUID < worldSize; ++tileUID )
	{
		auto update = aggregateTile( tileUID );

		const auto creatureSprite = creatures.find( tileUID );
		if ( creatureSprite != creatures.end() )
		{
			update.tile.creatureSpriteUID = creatureSprite.value();
		}

		tileUpdates.updates.push_back( update );

		if ( tileUpdates.updates.size() >= batchSize )
		{
			{
				emit signalTileUpdates( tileUpdates );
				tileUpdates.updates.clear();
			}
			tileUpdates.updates.reserve( batchSize );
		}
	}
	if ( !tileUpdates.updates.empty() )
	{
		emit signalTileUpdates( tileUpdates );
	}
}

void AggregatorRenderer::onUpdateAnyTileInfo( const QSet<unsigned int>& changeSet )
{
	// Bake tile updates
	auto creatures = collectCreatures();
	for ( auto tile = creatures.keyBegin(); tile != creatures.keyEnd(); ++tile )
	{
		//tiles.insert(*tile);
	}
	constexpr size_t batchSize = 1 << 16;
	TileDataUpdateInfo tileUpdates;
	tileUpdates.updates.reserve( batchSize );
	for ( auto tileUID : changeSet )
	{
		auto update = aggregateTile( tileUID );

		const auto creatureSprite = creatures.find( tileUID );
		if ( creatureSprite != creatures.end() )
		{
			update.tile.creatureSpriteUID = creatureSprite.value();
		}

		tileUpdates.updates.push_back( update );

		if ( tileUpdates.updates.size() >= batchSize )
		{
			{
				emit signalTileUpdates( tileUpdates );
				tileUpdates.updates.clear();
			}
			tileUpdates.updates.reserve( batchSize );
		}
	}
	if ( !tileUpdates.updates.empty() )
	{
		emit signalTileUpdates( tileUpdates );
	}

	if ( Global::mcm().axlesChanged() )
	{
		onAxleDataUpdate();
	}
	onThoughtBubbleUpdate();
}

void AggregatorRenderer::onThoughtBubbleUpdate()
{
	ThoughtBubbleInfo info;
	for ( const auto& g : Global::gm().gnomes() )
	{
		QString thoughtBubble = g->thoughtBubble();
		if ( !thoughtBubble.isEmpty() )
		{
			info.thoughtBubbles.push_back( { g->getPos(), Global::sf().thoughtBubbleID( thoughtBubble ) } );
		}
	}

	for ( const auto& g : Global::cm().animals() )
	{
		QString thoughtBubble = g->thoughtBubble();
		if ( !thoughtBubble.isEmpty() )
		{
			info.thoughtBubbles.push_back( { g->getPos(), Global::sf().thoughtBubbleID( thoughtBubble ) } );
		}
	}
	emit signalThoughtBubbles( info );
}

void AggregatorRenderer::onAxleDataUpdate()
{
	AxleDataInfo data;
	data.data = Global::mcm().axleData();
	emit signalAxleData( data );
}

void AggregatorRenderer::onWorldParametersChanged()
{
	emit signalWorldParametersChanged();
}
