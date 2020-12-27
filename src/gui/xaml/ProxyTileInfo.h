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

#include "../aggregatortileinfo.h"
#include "TileInfoModel.h"

#include <QObject>

class ProxyTileInfo : public QObject
{
	Q_OBJECT

public:
	ProxyTileInfo( QObject* parent = nullptr );
	~ProxyTileInfo();

	void setParent( IngnomiaGUI::TileInfoModel* parent );

	void sendTerrainCommand( unsigned int tileID, QString cmd );
	void sendManageCommand( unsigned int tileID );

	void requestStockpileItems( unsigned int tileID );

	void setTennant( unsigned int designationID, unsigned int gnomeID );
	void setAlarm( unsigned int designationID, bool value );

	void toggleMechActive( unsigned int id );
	void toggleMechInvert( unsigned int id );

	void setAutomatonRefuel( unsigned int id, bool refuel );
	void setAutomatonCore( unsigned int id, QString core );

private:
	IngnomiaGUI::TileInfoModel* m_parent = nullptr;

private slots:
	void onUpdateTileInfo( const GuiTileInfo& info );
	void onUpdateStockpileInfo( const GuiStockpileInfo& info );

signals:
	void signalTerrainCommand( unsigned int tileID, QString cmd );
	void signalManageCommand( unsigned int tileID );
	void signalRequestStockpileItems( unsigned int tileID );
	void signalSetTennant( unsigned int designationID, unsigned int gnomeID );
	void signalSetAlarm( unsigned int designationID, bool value );
	void signalToggleMechActive( unsigned int id );
	void signalToggleMechInvert( unsigned int id );

	void signalSetAutomatonRefuel( unsigned int id, bool refuel );
	void signalSetAutomatonCore( unsigned int id, QString core );
};
