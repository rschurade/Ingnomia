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
#include <QSoundEffect>

#define SOUNDS_MAX 8

// struct EffectStruct
// {
// QString id;
// float zvolume;
// };
// Q_DECLARE_METATYPE(EffectStruct)
class Job;
class Game;

class AggregatorSound : public QObject
{
	Q_OBJECT

public:
	AggregatorSound( QObject* parent = nullptr );
	~AggregatorSound();

	void init( Game* game );

	void onTick( quint64 tick );
	void playEffect( QString type, Position& pos);
	void onPlayEffect( QVariantMap effect);
	void setVolume( float newvol );
	float getVolume( );
	

private:
	QPointer<Game> g;
	QMap<QString, QSoundEffect *> m_effects;
	
	QList<QVariantMap> m_playQue;

	float m_volume = 1.0f;
	int m_viewLevel;
	
	
public slots:
	void changeViewLevel( int input);

signals:
	void signalPlayEffect( QVariantMap effect  );
};
