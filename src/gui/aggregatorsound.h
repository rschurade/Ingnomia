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

#include <QList>

#include <SFML/Audio.hpp>

class Job;
class Game;
struct SoundEffect;

struct ActiveEffect
{
	bool isAbsolute;
	Position pos;
	sf::Sound sound;
};

//TODO THe logic in this class doesn't belong in the "Aggregator" (server side!), it belongs with the UI
class AggregatorSound : public QObject
{
	Q_OBJECT

public:
	AggregatorSound( QObject* parent = nullptr );
	~AggregatorSound();

	void init( Game* game );

	void setVolume( float newvol );

private:
	QPointer<Game> g;
	QList<ActiveEffect> m_activeEffects;
	QMap<QString, sf::SoundBuffer> m_buffers;

	int m_viewLevel;
	Position m_viewDirection;

	void rebalanceSound( ActiveEffect& effect );
	void garbageCollection();
public slots:
	void onCameraPosition( float x, float y, float z, int r, float scale );

	void onPlayEffect( const SoundEffect& effect );
	void onPlayNotify( const SoundEffect& effect );
};
