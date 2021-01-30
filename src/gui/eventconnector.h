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

#include <QObject>

class GameManager;

class AggregatorTileInfo;     
class AggregatorStockpile;    
class AggregatorWorkshop;     
class AggregatorAgri;         
class AggregatorRenderer;
class AggregatorPopulation;
class AggregatorCreatureInfo;
class AggregatorDebug;
class AggregatorNeighbors;
class AggregatorMilitary;   
class AggregatorSettings;     
class AggregatorInventory;    
class AggregatorLoadGame;     
class AggregatorSelection;


class Game;

class EventConnector : public QObject
{
	Q_OBJECT

public:
	// Private Constructor
	EventConnector( GameManager* parent );
	~EventConnector();

	void setGamePtr( Game* game );

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
	AggregatorLoadGame* aggregatorLoadGame()
	{
		return m_loadGameAggregator;
	}
	AggregatorSelection* aggregatorSelection()
	{
		return m_selectionAggregator;
	}

	void emitStartGame();
	void emitStopGame();
	void emitInitView();
	void emitInMenu( bool value );

	void emitPause( bool paused );
	void emitGameSpeed( GameSpeed speed );

	void sendResume();
	void sendLoadGameDone( bool value );

	Game* game();
private:
	GameManager* gm = nullptr;
	QPointer<Game> g;
	

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
	AggregatorLoadGame* m_loadGameAggregator = nullptr;
	AggregatorSelection* m_selectionAggregator = nullptr;

public slots:
	void onExit();
	void onWindowSize( int w, int h );

	void onTimeAndDate( int minute, int hour, int day, QString season, int year, QString sunStatus );
	void onKingdomInfo( QString name, QString info1, QString info2, QString info3 );
	void onViewLevel( int level );
	
	void onHeartbeat( int value);
	void onHeartbeatResponse( int value);

	void onSetPause( bool paused );
	void onSetGameSpeed( GameSpeed speed );

	void onKeyPress( int key );
	void onTogglePause();
	void onPropagateEscape();

	void onBuild();

	void onTerrainCommand( unsigned int tileID, QString cmd );
	void onManageCommand( unsigned int tileID );

	void onSetRenderOptions( bool designations, bool jobs, bool walls, bool axles );
	void onUpdateRenderOptions();

	void onStartNewGame();
	void onContinueLastGame();
	void onLoadGame( QString folder );
	void onSaveGame();
	void onSetShowMainMenu( bool value );
	void onEndGame();

	void onSetSelectionAction( QString action );
	void onSetSelectionItem( QString item );
	void onSetSelectionMaterials( QStringList mats );

	void onCmdBuild( BuildItemType type, QString param, QString item, QStringList mats );

	void onAnswer( unsigned int id, bool answer );
	void onEvent( unsigned int id, QString title, QString msg, bool pause, bool yesno );

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
	void signalUpdateRenderOptions( bool designation, bool jobs, bool walls, bool axles );

	void startGame();
	void stopGame();
	void signalInitView();
	void signalInMenu( bool value );
	void signalResume();
	void signalLoadGameDone( bool value );

	void signalEvent( unsigned int id, QString title, QString msg, bool pause, bool yesno );
	
	void signalHeartbeat( int value );
};
