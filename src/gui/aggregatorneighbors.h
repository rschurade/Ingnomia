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

#include "../game/creature.h"
#include "../game/eventmanager.h"

#include <QObject>

struct GuiNeighborInfo
{
	unsigned int id          = 0;
	bool discovered          = false;
	QString distance = "undiscovered";
	QString name = "undiscovered";
	QString type = "undiscovered";
	QString attitude = "undiscovered";
	QString wealth = "undiscovered";
	QString economy = "undiscovered";
	QString military = "undiscovered";

	bool spyMission = false;
	bool sabotageMission = false;
	bool raidMission = false;
	bool diploMission = false;
};
Q_DECLARE_METATYPE( GuiNeighborInfo )

struct GuiAvailableGnome
{
	unsigned int id = 0;
	QString name;
};
Q_DECLARE_METATYPE( GuiAvailableGnome )



class AggregatorNeighbors : public QObject
{
	Q_OBJECT

public:
	AggregatorNeighbors( QObject* parent = nullptr );
	~AggregatorNeighbors();

	void init( Game* game );

private:
	QPointer<Game> g;

	QList<GuiNeighborInfo> m_neighborsInfo;
	QList<GuiAvailableGnome> m_availableGnomes;

public slots:
	void onRequestNeighborsUpdate();
	void onRequestMissions();
	void onRequestAvailableGnomes();
	void onStartMission( MissionType type, MissionAction action, unsigned int targetKingdom, unsigned int gnomeID );
	void onUpdateMission( const Mission& mission );

signals:
	void signalNeighborsUpdate( const QList<GuiNeighborInfo>& infos );
	void signalMissions( const QList<Mission>& missions );
	void signalAvailableGnomes( const QList<GuiAvailableGnome>& gnomes );
	void signalUpdateMission( const Mission& mission );
};
