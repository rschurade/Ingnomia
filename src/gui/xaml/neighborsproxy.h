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

#include "../aggregatorneighbors.h"
#include "neighborsmodel.h"

#include <QObject>

class NeighborsProxy : public QObject
{
	Q_OBJECT

public:
	NeighborsProxy( QObject* parent = nullptr );
	void setParent( IngnomiaGUI::NeighborsModel* parent );

	void requestAvailableGnomes();

	void startMission( MissionType type, MissionAction action, unsigned int targetKingdom, unsigned int gnomeID );

private:
	IngnomiaGUI::NeighborsModel* m_parent = nullptr;



private slots:
	void onNeighborsUpdate( const QList<GuiNeighborInfo>& infos );
	void onMissions( const QList<Mission>& missions );
	void onAvailableGnomes( const QList<GuiAvailableGnome>& gnomes );
	void onUpdateMission( const Mission& mission );

signals:
	void signalRequestAvailableGnomes();
	void signalStartMission( MissionType type, MissionAction action, unsigned int targetKingdom, unsigned int gnomeID );
};
