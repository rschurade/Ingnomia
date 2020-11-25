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

#include "aggregatoragri.h"
#include "aggregatorcreatureinfo.h"
#include "aggregatordebug.h"
#include "aggregatorinventory.h"
#include "aggregatorpopulation.h"
#include "aggregatorrenderer.h"
#include "aggregatorstockpile.h"
#include "aggregatortileinfo.h"
#include "aggregatorworkshop.h"
#include "aggregatorneighbors.h"
#include "aggregatormilitary.h"
#include "aggregatorsettings.h"

#include <QObject>

class EventConnector : public QObject
{
	Q_OBJECT

private:
	// Private Constructor
	EventConnector( QObject* parent = 0 );
	// Stop the compiler generating methods of copy the object
	EventConnector( EventConnector const& copy );            // Not Implemented
	EventConnector& operator=( EventConnector const& copy ); // Not Implemented

public:
	~EventConnector();

	static EventConnector& getInstance()
	{
		// The only instance
		// Guaranteed to be lazy initialized
		// Guaranteed that it will be destroyed correctly
		static EventConnector instance;
		return instance;
	}

	AggregatorTileInfo* aggregatorTileInfo()
	{
		return m_tiAggregator;
	}
	AggregatorStockpile* aggregatorStockpile()
	{
		return m_spAggregator;
	}
	AggregatorWorkshop* aggregatorWorkshop()
	{
		return m_wsAggregator;
	}
	AggregatorAgri* aggregatorAgri()
	{
		return m_acAggregator;
	}
	AggregatorRenderer* aggregatorRenderer()
	{
		return m_rAggregator;
	}
	AggregatorPopulation* aggregatorPopulation()
	{
		return m_popAggregator;
	}
	AggregatorCreatureInfo* aggregatorCreatureInfo()
	{
		return m_creatureInfoAggregator;
	}
	AggregatorDebug* aggregatorDebug()
	{
		return m_debugAggregator;
	}
	AggregatorNeighbors* aggregatorNeighbors()
	{
		return m_neighborsAggregator;
	}
	AggregatorMilitary* aggregatorMilitary()
	{
		return m_militaryAggregator;
	}
	AggregatorSettings* aggregatorSettings()
	{
		return m_settingsAggregator;
	}
	AggregatorInventory* aggregatorInventory()
	{
		return m_inventoryAggregator;
	}

private:
	AggregatorTileInfo* m_tiAggregator = nullptr;
	AggregatorStockpile* m_spAggregator = nullptr;
	AggregatorWorkshop* m_wsAggregator = nullptr;
	AggregatorAgri* m_acAggregator = nullptr;
	AggregatorRenderer* m_rAggregator = nullptr;
	AggregatorPopulation* m_popAggregator = nullptr;
	AggregatorCreatureInfo* m_creatureInfoAggregator = nullptr;
	AggregatorDebug* m_debugAggregator = nullptr;
	AggregatorNeighbors* m_neighborsAggregator = nullptr;
	AggregatorMilitary* m_militaryAggregator = nullptr;
	AggregatorSettings* m_settingsAggregator = nullptr;
	AggregatorInventory* m_inventoryAggregator = nullptr;

public slots:
	void onExit();
	void onWindowSize( int w, int h );

	void onTimeAndDate( int minute, int hour, int day, QString season, int year, QString sunStatus );
	void onKingdomInfo( QString name, QString info1, QString info2, QString info3 );
	void onViewLevel( int level );

	void onUpdatePause( bool paused );
	void onUpdateGameSpeed( GameSpeed speed );
	void onKeyPress( int key );
	void onPropagateEscape();

	void onBuild();

	void onTerrainCommand( unsigned int tileID, QString cmd );
	void onManageCommand( unsigned int tileID );

signals:
	void signalExit();
	void signalWindowSize( int w, int h );
	void signalTimeAndDate( int minute, int hour, int day, QString season, int year, QString sunStatus );
	void signalKingdomInfo( QString name, QString info1, QString info2, QString info3 );
	void signalViewLevel( int level );
	void signalUpdatePause( bool paused );
	void signalUpdateGameSpeed( GameSpeed speed );
	void signalKeyEsc();
	void signalPropagateKeyEsc();
	void signalBuild();
};
