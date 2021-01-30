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

#include "../aggregatorinventory.h"

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
	void requestInventoryUpdate();

	void sendEventAnswer( unsigned int eventID, bool answer );

	void propagateEscape();

	void setGameSpeed( GameSpeed speed );
	void setPaused( bool paused );

	void setRenderOptions( bool designations, bool jobs, bool walls, bool axels );

	void setSelectionAction( QString action );

	void requestBuildItems( BuildSelection buildSelection, QString category );
	void requestCmdBuild( BuildItemType type, QString param, QString item, QStringList mats );

private:
	IngnomiaGUI::GameModel* m_parent = nullptr;

private slots:
	void onTimeAndDate( int minute, int hour, int day, QString season, int year, QString sunStatus );
	void onKingdomInfo( QString name, QString info1, QString info2, QString info3 );
	void onViewLevel( int level );
	
	void onHeartbeat( int value );

	void onUpdatePause( bool value );
	void onUpdateGameSpeed( GameSpeed speed );

	void onShowTileInfo( unsigned int tileID );

	void onBuild();

	void onOpenStockpileWindow( unsigned int stockpileID );
	void onOpenWorkshopWindow( unsigned int workshopID );
	void onOpenAgriWindow( unsigned int ID );

	void onEvent( unsigned int id, QString title, QString msg, bool pause, bool yesno );

	void onKeyEscape();
	void onUpdateRenderOptions( bool designation, bool jobs, bool walls, bool axles );

	void onBuildItems( const QList<GuiBuildItem>& items );

	void onWatchList( const QList<GuiWatchedItem>& watchedItemList );

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
	void signalRequestInventoryUpdate();
	void signalPropagateEscape();
	void signalSetGameSpeed( GameSpeed speed );
	void signalSetPaused( bool paused );
	void signalSetRenderOptions( bool designations, bool jobs, bool walls, bool axles );

	void signalRequestBuildItems( BuildSelection buildSelection, QString category );
	void signalRequestCmdBuild( BuildItemType type, QString param, QString item, QStringList mats );
	void signalSetSelectionAction( QString action );
	
	void signalHeartbeatResponse( int value );
};
