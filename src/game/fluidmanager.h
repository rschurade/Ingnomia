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
#include "../game/job.h"

#include <QSet>

class Game;

enum PipeType : unsigned char
{
	PT_NONE,
	PT_INPUT,
	PT_OUTPUT,
	PT_PIPE
};

struct NetworkPipe
{
	Position pos;

	unsigned int itemUID = 0;

	PipeType type = PT_NONE;

	unsigned char capacity = 1;
	unsigned char level    = 0;

	QList<Position> ins;
	QList<Position> outs;

	QVariantMap serialize() const;
	void deserialize( QVariantMap in );
};

class FluidManager : public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY_MOVE( FluidManager )
public:
	FluidManager( Game* parent );
	~FluidManager();

	QHash<unsigned int, NetworkPipe>& pipes()
	{
		return m_allPipes;
	};
	void loadPipes( QVariantList data );

	void addInput( Position pos, unsigned int itemUID );
	void addPipe( Position pos, unsigned int itemUID );
	void addOutput( Position pos, unsigned int itemUID );
	void removeAt( Position pos );
	void updateNetwork();

	void onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged );

	unsigned int getJob( unsigned int gnomeID, QString skillID );

	bool finishJob( unsigned int jobID );
	bool giveBackJob( unsigned int jobID );
	QSharedPointer<Job> getJob( unsigned int jobID );
	bool hasJobID( unsigned int jobID ) const;

private:
	QPointer<Game> g;

	quint64 m_lastTick = 0;

	QList<Position> m_inputs;
	QList<Position> m_outputs;

	QHash<unsigned int, NetworkPipe> m_allPipes;
};
