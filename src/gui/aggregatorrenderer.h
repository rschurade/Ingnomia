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

#include "../base/position.h"
#include "../base/tile.h"

#include <QObject>
#include <QVector>

class Game;

struct TileData
{
	unsigned int flags  = 0;
	unsigned int flags2 = 0;

	unsigned int floorSpriteUID = 0;
	unsigned int wallSpriteUID  = 0;

	unsigned int itemSpriteUID     = 0;
	unsigned int creatureSpriteUID = 0;

	unsigned int jobSpriteFloorUID = 0;
	unsigned int jobSpriteWallUID  = 0;

	unsigned char fluidLevel      = 0;
	unsigned char lightLevel      = 0;
	unsigned char vegetationLevel = 0;
	unsigned char unused3         = 0;
};
Q_DECLARE_TYPEINFO( TileData, Q_PRIMITIVE_TYPE );

struct TileDataUpdate
{
	unsigned int id = 0;
	TileData tile;
};
Q_DECLARE_TYPEINFO( TileDataUpdate, Q_PRIMITIVE_TYPE );

struct TileDataUpdateInfo
{
	QVector<TileDataUpdate> updates;
};
Q_DECLARE_METATYPE( TileDataUpdateInfo );

struct SelectionData
{
	Position pos;
	unsigned short spriteID = 0;
	bool valid              = true;
	bool isFloor            = false;
	quint8 localRot         = 0;
};
//Q_DECLARE_TYPEINFO( SelectionData, Q_PRIMITIVE_TYPE );
Q_DECLARE_METATYPE( SelectionData );

struct ThoughtBubble
{
	Position pos;
	unsigned int sprite;
};
Q_DECLARE_TYPEINFO( ThoughtBubble, Q_PRIMITIVE_TYPE );

struct ThoughtBubbleInfo
{
	QVector<ThoughtBubble> thoughtBubbles;
};
Q_DECLARE_METATYPE( ThoughtBubbleInfo );

struct AxleDataInfo
{
	QHash<unsigned int, AxleData> data;
};
Q_DECLARE_METATYPE( AxleDataInfo );

constexpr size_t TD_SIZE  = sizeof( TileData ) / sizeof( unsigned int );
constexpr size_t ROT_BIT  = 0x10000;
constexpr size_t ANIM_BIT = 0x40000;
constexpr size_t WALL_BIT = 0x80000;

class AggregatorRenderer : public QObject
{
	Q_OBJECT

public:
	AggregatorRenderer( QObject* parent = nullptr );

	void init( Game* game );

private:
	QPointer<Game> g;

	QHash<unsigned int, unsigned int> collectCreatures();
	TileDataUpdate aggregateTile( unsigned int tileID ) const;

public slots:
	void onWorldParametersChanged();
	void onAllTileInfo();
	void onUpdateAnyTileInfo( const QSet<unsigned int>& changeSet );
	void onThoughtBubbleUpdate();
	void onAxleDataUpdate();

signals:
	void signalWorldParametersChanged();
	void signalTileUpdates( const TileDataUpdateInfo& updates );
	void signalThoughtBubbles( const ThoughtBubbleInfo& bubbles );
	void signalAxleData( const AxleDataInfo& data );
};