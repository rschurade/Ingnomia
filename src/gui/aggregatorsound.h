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
/** @file aggregatorsound.h
 *  @brief OpenAL-based spatial sound aggregator. Loads effect buffers on demand, spawns
 *         per-effect sources, tracks their position relative to the camera, and applies
 *         low-pass occlusion filters based on how many z-levels separate listener and source.
 */
#pragma once

#include "../base/position.h"

#include <QList>

#include "openalwrapper.h"

class Job;
class Game;
struct SoundEffect;

/// @brief One currently playing sound effect (positioned in world space or absolute).
struct ActiveEffect
{
	bool isAbsolute;                   ///< True for HUD sounds (ignore camera position).
	Position pos;                      ///< World-space source position.
	std::shared_ptr<AL::Source> sound; ///< OpenAL source handle.
};

//TODO THe logic in this class doesn't belong in the "Aggregator" (server side!), it belongs with the UI
/// @brief GUI-thread aggregator that owns the OpenAL context and plays positional sound
///        effects on demand.
class AggregatorSound : public QObject
{
	Q_OBJECT

public:
	AggregatorSound( QObject* parent = nullptr );
	~AggregatorSound();

	void init( Game* game );

private:
	QPointer<Game> g;                                      ///< Game instance (weak ownership).
	QList<ActiveEffect> m_activeEffects;                   ///< Currently playing sources.
	QMap<QString, std::shared_ptr<AL::Buffer>> m_buffers;  ///< Buffer cache keyed by sound name.
	std::shared_ptr<AL::Context> m_audioContext;           ///< OpenAL context.
	std::shared_ptr<AL::Listener> m_audioListener;         ///< OpenAL listener (the camera).
	static constexpr size_t maxOcclusion = 8;              ///< Highest occlusion filter index.
	std::vector<std::shared_ptr<AL::Filter>> m_occlusionFilter; ///< Pre-built low-pass filters by occlusion depth.

	int m_viewLevel = 100;                                 ///< Current camera z-level.
	Position m_viewDirection;                              ///< Current camera world-space position.

	bool rebalanceSound( ActiveEffect& effect );
	void garbageCollection();
public slots:
	void onCameraPosition( float x, float y, float z, int r, float scale );

	void onPlayEffect( const SoundEffect& effect );
	void onPlayNotify( const SoundEffect& effect );
};
