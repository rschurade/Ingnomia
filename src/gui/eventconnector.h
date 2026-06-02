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
/** @file eventconnector.h
 *  @brief The central GUI-side event bus. Owns all Aggregator* instances, forwards
 *         game-thread signals to the GUI thread, and routes input/command signals from
 *         the GUI back into the game.
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
class AggregatorSound;


class Game;

/// @brief Central event-routing object accessible as Global::eventConnector. Holds one
///        instance of every Aggregator and exposes them to the GUI; acts as the glue between
///        the game-simulation thread and the Qt GUI thread.
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
	AggregatorSound* aggregatorSound()
	{
		return m_soundAggregator;
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
	GameManager* gm = nullptr;        ///< Owning GameManager.
	QPointer<Game> g;                 ///< Current Game instance (weak ownership).


	AggregatorTileInfo* m_tiAggregator = nullptr;                   ///< Tile Info window aggregator.
	AggregatorStockpile* m_spAggregator = nullptr;                  ///< Stockpile window aggregator.
	AggregatorWorkshop* m_wsAggregator = nullptr;                   ///< Workshop window aggregator.
	AggregatorAgri* m_acAggregator = nullptr;                       ///< Agriculture (farm/pasture/grove) aggregator.
	AggregatorRenderer* m_rAggregator = nullptr;                    ///< Renderer tile/creature data aggregator.
	AggregatorPopulation* m_popAggregator = nullptr;                ///< Population / schedule aggregator.
	AggregatorCreatureInfo* m_creatureInfoAggregator = nullptr;     ///< Creature Info window aggregator.
	AggregatorDebug* m_debugAggregator = nullptr;                   ///< Debug window aggregator.
	AggregatorNeighbors* m_neighborsAggregator = nullptr;           ///< Neighbors / diplomacy aggregator.
	AggregatorMilitary* m_militaryAggregator = nullptr;             ///< Military window aggregator.
	AggregatorSettings* m_settingsAggregator = nullptr;             ///< Settings window aggregator.
	AggregatorInventory* m_inventoryAggregator = nullptr;           ///< Inventory/Build menu aggregator.
	AggregatorLoadGame* m_loadGameAggregator = nullptr;             ///< Load Game window aggregator.
	AggregatorSelection* m_selectionAggregator = nullptr;           ///< Placement cursor / selection aggregator.
	AggregatorSound* m_soundAggregator = nullptr;                   ///< OpenAL sound effect aggregator.

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
	
	void onPlayEffect( QVariantMap effect);
	void onCameraPosition( float x, float y, float z, int r, float scale );

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
	void signalPlayEffect( QVariantMap effect );
	
	void signalCameraPosition( float x, float y, float z, int r, float scale );
};
