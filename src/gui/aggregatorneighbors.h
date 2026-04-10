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
/** @file aggregatorneighbors.h
 *  @brief Data types and aggregator bridging the Neighbors (diplomacy) XAML window with
 *         NeighborManager. Exposes kingdom info, available-gnome lists for missions, and
 *         mission lifecycle signals.
 */
#pragma once

#include "../game/creature.h"
#include "../game/eventmanager.h"

#include <QObject>

/// @brief Summary of one neighbouring kingdom shown in the diplomacy list. Undiscovered
///        kingdoms carry placeholder strings for all fields.
struct GuiNeighborInfo
{
	unsigned int id          = 0;            ///< Kingdom UID.
	bool discovered          = false;        ///< True once the kingdom has been discovered.
	QString distance = "undiscovered";       ///< Travel distance label.
	QString name = "undiscovered";           ///< Kingdom name.
	QString type = "undiscovered";           ///< Kingdom race / culture type.
	QString attitude = "undiscovered";       ///< Current diplomatic attitude.
	QString wealth = "undiscovered";         ///< Wealth rating label.
	QString economy = "undiscovered";        ///< Economy rating label.
	QString military = "undiscovered";       ///< Military strength label.

	bool spyMission = false;                 ///< True if a spy mission is currently active against this kingdom.
	bool sabotageMission = false;            ///< True if a sabotage mission is active.
	bool raidMission = false;                ///< True if a raid mission is active.
	bool diploMission = false;               ///< True if an emissary mission is active.
};
Q_DECLARE_METATYPE( GuiNeighborInfo )

/// @brief Gnome eligible to be sent on a mission (not assigned elsewhere, not in a squad).
struct GuiAvailableGnome
{
	unsigned int id = 0;    ///< Creature UID.
	QString name;           ///< Display name.
};
Q_DECLARE_METATYPE( GuiAvailableGnome )



/// @brief Routes neighbour kingdom info and diplomacy mission state between NeighborManager
///        and the Neighbors XAML window.
class AggregatorNeighbors : public QObject
{
	Q_OBJECT

public:
	AggregatorNeighbors( QObject* parent = nullptr );
	~AggregatorNeighbors();

	void init( Game* game );

private:
	QPointer<Game> g;                           ///< Game instance (weak ownership).

	QList<GuiNeighborInfo> m_neighborsInfo;     ///< Cached kingdom list for the GUI.
	QList<GuiAvailableGnome> m_availableGnomes; ///< Cached eligible-gnome list for mission dispatch.

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
