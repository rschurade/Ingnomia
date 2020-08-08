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

#include "GameModel.h"

#include <QObject>

class ProxyGameView : public QObject
{
	Q_OBJECT

public:
	ProxyGameView( QObject* parent = nullptr );
	~ProxyGameView();

	void setParent( IngnomiaGUI::GameModel* parent );

	void closeStockpileWindow();
	void closeWorkshopWindow();
	void closeAgricultureWindow();
	void requestPopulationUpdate();
	void requestNeighborsUpdate();
	void requestMissionsUpdate();
	void requestCreatureUpdate( unsigned int id );
	void requestMilitaryUpdate();

	void sendEventAnswer( unsigned int eventID, bool answer );

private:
	IngnomiaGUI::GameModel* m_parent = nullptr;

private slots:
	void onTimeAndDate( int minute, int hour, int day, QString season, int year, QString sunStatus );
	void onViewLevel( int level );

	void onUpdatePause();
	void onUpdateGameSpeed();

	void onShowTileInfo( unsigned int tileID );

	void onBuild();

	void onOpenStockpileWindow( unsigned int stockpileID );
	void onOpenWorkshopWindow( unsigned int workshopID );
	void onOpenAgriWindow( unsigned int ID );

	void onEvent( unsigned int id, QString title, QString msg, bool pause, bool yesno );

signals:
	void signalCloseStockpileWindow();
	void signalCloseWorkshopWindow();
	void signalCloseAgricultureWindow();
	void signalRequestPopulationUpdate();
	void signalRequestNeighborsUpdate();
	void signalRequestMissionsUpdate();
	void signalRequestCreatureUpdate( unsigned int id );
	void signalEventAnswer( unsigned int eventID, bool answer );
	void signalRequestMilitaryUpdate();
};
