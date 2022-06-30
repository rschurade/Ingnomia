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

#include <QPointer>
#include <QList>

#include "openalwrapper.h"

class Job;
class Game;
struct SoundEffect;

struct ActiveEffect
{
	bool isAbsolute;
	Position pos;
	std::shared_ptr<AL::Source> sound;
};

//TODO THe logic in this class doesn't belong in the "Aggregator" (server side!), it belongs with the UI
class AggregatorSound : public QObject
{
	Q_OBJECT

public:
	AggregatorSound( EventConnector* parent = nullptr );
	~AggregatorSound();

	void init( Game* game );

private:
	QPointer<Game> g;
	QList<ActiveEffect> m_activeEffects;
	absl::btree_map<QString, std::shared_ptr<AL::Buffer>> m_buffers;
	std::shared_ptr<AL::Context> m_audioContext;
	std::shared_ptr<AL::Listener> m_audioListener;
	static constexpr size_t maxOcclusion = 8;
	std::vector<std::shared_ptr<AL::Filter>> m_occlusionFilter;

	int m_viewLevel = 100;
	Position m_viewDirection;

	bool rebalanceSound( ActiveEffect& effect );
	void garbageCollection();
public slots:
	void onCameraPosition( float x, float y, float z, int r, float scale );

	void onPlayEffect( const SoundEffect& effect );
	void onPlayNotify( const SoundEffect& effect );
};
