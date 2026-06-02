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
/** @file aggregatorrenderer.h
 *  @brief Data types and aggregator that feeds the MainWindowRenderer with tile sprite updates,
 *         thought bubbles, axle power data, and camera control events.
 */
#pragma once

#include "../base/position.h"
#include "../base/tile.h"

#include <QObject>
#include <QVector>

class Game;

/// @brief GPU-bound representation of one world tile. Four sprite UIDs (floor/wall/item/
///        creature) plus two job-overlay sprite UIDs and packed state bytes. Laid out so it
///        can be uploaded as a small array of uints (see TD_SIZE).
struct TileData
{
	unsigned int flags  = 0;                ///< Primary tile flags (walkable, occupied, …).
	unsigned int flags2 = 0;                ///< Secondary flags overflow.

	unsigned int floorSpriteUID = 0;        ///< Floor sprite UID.
	unsigned int wallSpriteUID  = 0;        ///< Wall sprite UID.

	unsigned int itemSpriteUID     = 0;     ///< Top-most item sprite UID.
	unsigned int creatureSpriteUID = 0;     ///< Creature sprite UID (if any).

	unsigned int jobSpriteFloorUID = 0;     ///< Floor-aligned job overlay sprite.
	unsigned int jobSpriteWallUID  = 0;     ///< Wall-aligned job overlay sprite.

	unsigned char fluidLevel      = 0;      ///< Fluid level 0–10.
	unsigned char lightLevel      = 0;      ///< Light level 0–255.
	unsigned char vegetationLevel = 0;      ///< Grass/vegetation growth 0–255.
	unsigned char unused3         = 0;      ///< Padding / reserved.
};
Q_DECLARE_TYPEINFO( TileData, Q_PRIMITIVE_TYPE );

/// @brief One tile update packet: tile ID plus its new TileData.
struct TileDataUpdate
{
	unsigned int id = 0;   ///< Tile ID (Position::toInt()).
	TileData tile;         ///< New tile data.
};
Q_DECLARE_TYPEINFO( TileDataUpdate, Q_PRIMITIVE_TYPE );

/// @brief Batch of tile updates emitted to the renderer per frame.
struct TileDataUpdateInfo
{
	QVector<TileDataUpdate> updates;
};
Q_DECLARE_METATYPE( TileDataUpdateInfo );

/// @brief One selection/construction preview tile (used by the placement cursor).
struct SelectionData
{
	Position pos;           ///< Tile position.
	unsigned short spriteID = 0; ///< Preview sprite UID.
	bool valid              = true; ///< True if this tile is a valid placement target.
	bool isFloor            = false;///< True if the preview sprite is a floor (vs wall).
	quint8 localRot         = 0;    ///< Rotation index for the preview sprite.
};
//Q_DECLARE_TYPEINFO( SelectionData, Q_PRIMITIVE_TYPE );
Q_DECLARE_METATYPE( SelectionData );

/// @brief Position + sprite for one thought bubble rendered above a gnome.
struct ThoughtBubble
{
	Position pos;           ///< Gnome position.
	unsigned int sprite;    ///< Thought bubble sprite UID.
};
Q_DECLARE_TYPEINFO( ThoughtBubble, Q_PRIMITIVE_TYPE );

/// @brief Batch of thought bubbles emitted to the renderer.
struct ThoughtBubbleInfo
{
	QVector<ThoughtBubble> thoughtBubbles;
};
Q_DECLARE_METATYPE( ThoughtBubbleInfo );

/// @brief Per-axle power/rotation data, keyed by axle UID, sent to the renderer.
struct AxleDataInfo
{
	QHash<unsigned int, AxleData> data;
};
Q_DECLARE_METATYPE( AxleDataInfo );

constexpr size_t TD_SIZE  = sizeof( TileData ) / sizeof( unsigned int ); ///< Number of uints in a TileData upload.
constexpr size_t ROT_BIT  = 0x10000;  ///< Encoded rotation flag (bit 16 of a sprite UID).
constexpr size_t ANIM_BIT = 0x40000;  ///< Encoded animation flag (bit 18).
constexpr size_t WALL_BIT = 0x80000;  ///< Encoded wall flag (bit 19).

/// @brief Collects tile/creature/thought-bubble/axle state from the game and emits batched
///        update packets to the MainWindowRenderer over Qt signals.
class AggregatorRenderer : public QObject
{
	Q_OBJECT

public:
	AggregatorRenderer( QObject* parent = nullptr );

	void init( Game* game );

private:
	QPointer<Game> g;   ///< Game instance (weak ownership).

	QHash<unsigned int, unsigned int> collectCreatures();
	TileDataUpdate aggregateTile( unsigned int tileID ) const;

public slots:
	void onWorldParametersChanged();
	void onAllTileInfo();
	void onUpdateAnyTileInfo( const QSet<unsigned int>& changeSet );
	void onThoughtBubbleUpdate();
	void onAxleDataUpdate();
	void onCenterCamera( const Position& location );

signals:
	void signalWorldParametersChanged();
	void signalTileUpdates( const TileDataUpdateInfo& updates );
	void signalThoughtBubbles( const ThoughtBubbleInfo& bubbles );
	void signalAxleData( const AxleDataInfo& data );
	void signalCenterCamera( const Position& location );
};