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
#include "../game/job.h"

class Game;

enum MechanismType
{
	MT_NONE = 0,
	MT_AXLE,
	MT_VERTICALAXLE,
	MT_GEARBOX,
	MT_LEVER,
	MT_ENGINE,
	MT_PUMP,
	MT_WALL,
	MT_PRESSUREPLATE
};

struct MechanismData
{
	MechanismType type = MT_NONE;

	unsigned int itemID    = 0;
	unsigned int networkID = 0;
	Position pos;
	unsigned char rot = 0;

	QString gui;
	QString name;

	bool active       = false;
	bool changeActive = false;

	int producePower = 0;

	int fuel            = 0;
	int maxFuel         = 0;
	int refuelThreshold = 50;
	int consumePower    = 0;
	bool hasPower       = false;

	bool anim = false;

	bool isInvertable   = false;
	bool inverted       = false;
	bool changeInverted = false;

	QList<Position> connectsTo;

	QVariantMap serialize() const;
	void deserialize( QVariantMap in );

	QWeakPointer<Job> job;
};

struct MechanismNetwork
{
	unsigned int id      = 0;
	unsigned int produce = 0;
	unsigned int consume = 0;

	QSet<unsigned int> producers;
	QSet<unsigned int> consumers;
};

class MechanismManager : public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY_MOVE( MechanismManager )
public:
	MechanismManager() = delete;
	MechanismManager( Game* parent );
	~MechanismManager();

	QHash<unsigned int, MechanismData>& mechanisms()
	{
		return m_mechanisms;
	};
	void loadMechanisms( QVariantList data );

	void onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged );

	bool hasMechanism( Position pos );
	bool hasGearBox( Position pos );

	QString mechanismName( Position pos );
	unsigned int mechanismID( Position pos );

	bool hasGUI( unsigned int itemID );
	bool hasGUI( Position pos );
	QString gui( unsigned int itemID );

	bool hasPower( Position pos );

	void installItem( unsigned int itemID, Position pos, int rotation );
	void uninstallItem( unsigned int itemID );

	bool axlesChanged();
	QHash<unsigned int, AxleData> axleData();

	MechanismData mechanismData( unsigned int itemID );

	bool changeActive( unsigned int itemID );
	void toggleActive( unsigned int itemID );

	bool changeInverted( unsigned int itemID );
	void toggleInvert( unsigned int itemID );

	void setRefuelThreshold( unsigned int itemID, int percent );

	void refuel( unsigned int itemID, int burnValue );

	void updateCreaturesAtPos( Position pos, int numCreatures );

private:
	QPointer<Game> g;

	void installItem( MechanismData md );

	void updateNetWorks();

	void updateSpritesAndFlags( MechanismData& md, bool isON );
	void addEffect( Position pos, QString effect );
	void removeEffect( Position pos, QString effect );

	void setActive( unsigned int itemID, bool active );
	void setInverted( unsigned int itemID, bool inv );
	void setConnectsTo( MechanismData& md );

	quint64 m_lastTick       = 0;
	bool m_needNetworkUpdate = false;

	QHash<unsigned int, MechanismData> m_mechanisms;

	QHash<unsigned int, unsigned int> m_floorPositions;
	QHash<unsigned int, unsigned int> m_wallPositions;

	QHash<unsigned int, AxleData> m_axleData;

	bool m_axlesChanged = false;

	QHash<unsigned int, MechanismNetwork> m_networks;

	QHash<QString, MechanismType> m_string2Type;
};
