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

#include <QMap>
#include <QVariantMap>

class Game;
struct Mission;

enum class KingdomType
{
	NONE,
	GNOME,
	GOBLIN,
	OTHER
};

enum class KingdomWealth
{
	VERYPOOR,
	POOR,
	AVERAGE,
	RICH,
	VERYRICH
};

enum class KingdomEconomy
{
	TRADING,
	FARMING,
	LOGGING,
	MINING,
	ANIMALBREEDING
};

enum class KingdomMilitary
{
	VERYWEAK,
	WEAK,
	AVERAGE,
	STRONG,
	VERYSTRONG
};

struct NeighborKingdom
{
	unsigned int id          = 0;
	bool discovered          = false;
	bool discoverMission     = false;
	int distance             = 0; // travel time in hours
	QString name             = "undiscovered";
	KingdomType type         = KingdomType::NONE;
	float attitude           = 0;
	KingdomWealth wealth     = KingdomWealth::AVERAGE;
	KingdomEconomy economy   = KingdomEconomy::FARMING;
	KingdomMilitary military = KingdomMilitary::AVERAGE;
	quint64 nextRaid         = 0;
	quint64 nextTrader       = 0;

	QVariantMap serialize();
	void deserialize( QVariantMap in );
};

class NeighborManager : public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY_MOVE( NeighborManager )
public:
	NeighborManager( Game* parent );
	~NeighborManager();

	void addRandomKingdom( KingdomType type );

	QVariantList serialize();
	void deserialize( QVariantList in );

	void onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged );

	QList<NeighborKingdom>& kingdoms();

	int countDiscovered();

	void discoverKingdom( unsigned int id );

	int distance( unsigned int kingdomID );

	void spy( Mission* mission );

	void sabotage( Mission* mission );

	void raid( Mission* mission );

	void emissary( Mission* mission );

private:
	QPointer<Game> g;

	QList<NeighborKingdom> m_kingdoms;

signals:

public slots:
};
