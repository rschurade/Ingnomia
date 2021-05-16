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

struct SoundEffect
{
	QString type;
	QString material;
	Position origin;
};
Q_DECLARE_METATYPE( SoundEffect )

class Job;
class Game;

class SoundManager : public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY_MOVE( SoundManager )
public:
	SoundManager( Game* parent = 0 );
	~SoundManager();

	void onTick( quint64 tick );
	void playEffect( QString type, Position& pos, QString material );
	void playEffect( QString type, Position& pos, QString material, int duration );

private:
	QPointer<Game> g;

signals:
	void signalPlayEffect( const SoundEffect& effect );
};
