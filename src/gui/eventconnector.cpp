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
/** @file eventconnector.cpp
 *  @brief EventConnector implementation: owns all Aggregator instances, bridges game-thread
 *         and GUI-thread signals, and implements the various high-level commands that the
 *         GUI issues (start/stop/load/save game, build, terrain commands, selection, etc.).
 */
#include "eventconnector.h"

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
#include "aggregatorloadgame.h"
#include "aggregatorselection.h"
#include "aggregatorsound.h"

#include "../base/db.h"
#include "../base/config.h"
#include "../base/global.h"
#include "../base/selection.h"
#include "../game/gamemanager.h"
#include "../game/game.h"
#include "../game/job.h"
#include "../game/jobmanager.h"
#include "../game/soundmanager.h"
#include "../game/plant.h"
#include "../game/world.h"

#include <QDebug>

/// @brief Constructs the EventConnector and instantiates every Aggregator. Wires the
///        Selection aggregator's tile-select signal directly into the TileInfo aggregator
///        so clicking the map opens the Tile Info window without going through the main menu.
/// @param parent Owning GameManager.
EventConnector::EventConnector( GameManager* parent ) :
	gm( parent ),
	QObject( parent )
{
	m_tiAggregator           = new AggregatorTileInfo( this );
	m_spAggregator           = new AggregatorStockpile( this );
	m_wsAggregator           = new AggregatorWorkshop( this );
	m_acAggregator           = new AggregatorAgri( this );
	m_rAggregator            = new AggregatorRenderer( this );
	m_popAggregator          = new AggregatorPopulation( this );
	m_creatureInfoAggregator = new AggregatorCreatureInfo( this );
	m_debugAggregator        = new AggregatorDebug( this );
	m_neighborsAggregator    = new AggregatorNeighbors( this );
	m_militaryAggregator     = new AggregatorMilitary( this );
	m_settingsAggregator	 = new AggregatorSettings( this );
	m_inventoryAggregator    = new AggregatorInventory( this );
	m_loadGameAggregator	 = new AggregatorLoadGame( this );
	m_selectionAggregator	 = new AggregatorSelection( this );
	m_soundAggregator	 = new AggregatorSound( this );

	connect( m_selectionAggregator, &AggregatorSelection::signalSelectTile, m_tiAggregator, &AggregatorTileInfo::onShowTileInfo );
}

/// @brief Updates the stored Game pointer; called on new game / load game.
/// @param game New Game instance (or nullptr when the game is unloaded).
void EventConnector::setGamePtr( Game* game )
{
	g = game;
}

/// @brief Destructor.
EventConnector::~EventConnector()
{
}

/// @brief Emits signalExit to ask the GUI shell to quit the application.
void EventConnector::onExit()
{
	emit signalExit();
}

/// @brief Forwards a window-size change from the GUI shell to the renderer.
/// @param w New width in pixels.
/// @param h New height in pixels.
void EventConnector::onWindowSize( int w, int h )
{
	emit signalWindowSize( w, h );
}

/// @brief Relays the in-game clock to the GUI status bar.
/// @param minute    Current minute (0–59).
/// @param hour      Current hour (0–23).
/// @param day       Current day of month.
/// @param season    Localised season name.
/// @param year      Current year.
/// @param sunStatus "Day" / "Night" / "Twilight" string.
void EventConnector::onTimeAndDate( int minute, int hour, int day, QString season, int year, QString sunStatus )
{
	emit signalTimeAndDate( minute, hour, day, season, year, sunStatus );
}

/// @brief Relays kingdom stats (name plus three info strings) to the GUI top bar.
/// @param name  Kingdom name.
/// @param info1 First info line (usually population).
/// @param info2 Second info line.
/// @param info3 Third info line.
void EventConnector::onKingdomInfo( QString name, QString info1, QString info2, QString info3 )
{
	emit signalKingdomInfo( name, info1, info2, info3 );
}

/// @brief Forwards a heartbeat pulse to the GUI (used to detect frozen simulation).
/// @param value Heartbeat counter value.
void EventConnector::onHeartbeat( int value)
{
	emit signalHeartbeat( value );
}

/// @brief Records a heartbeat acknowledgment from the GUI back in GameManager.
/// @param value Acknowledgment value.
void EventConnector::onHeartbeatResponse( int value)
{
	gm->setHeartbeatResponse( value );
}

/// @brief Relays the current view-level z-coordinate to the GUI.
/// @param level Z-level index.
void EventConnector::onViewLevel( int level )
{
	emit signalViewLevel( level );
}

/// @brief Sets the simulation paused state via GameManager.
/// @param paused True to pause.
void EventConnector::onSetPause( bool paused )
{
	gm->setPaused( paused );
}

/// @brief Sets the simulation speed via GameManager.
/// @param speed New speed enum.
void EventConnector::onSetGameSpeed( GameSpeed speed )
{
	gm->setGameSpeed( speed );
}

/// @brief Handles a GUI key press. Currently only forwards Escape as signalKeyEsc.
/// @param key Qt key code.
void EventConnector::onKeyPress( int key )
{
	switch ( key )
	{
		case Qt::Key_Escape:
			emit signalKeyEsc();
			break;
	}
}

/// @brief Toggles the paused flag and emits the updated state to the GUI.
void EventConnector::onTogglePause()
{
	emit signalUpdatePause( !gm->paused() );
}

/// @brief Propagates an Escape key press through the GUI stack (close top window).
void EventConnector::onPropagateEscape()
{
	emit signalPropagateKeyEsc();
}

/// @brief Emits signalBuild so the GUI opens the build menu.
void EventConnector::onBuild()
{
	emit signalBuild();
}

/// @brief Turns a terrain context-menu command into a job queued on the target tile.
///        Supports Mine, Remove, Fell, Destroy, and Harvest (trees vs crops).
/// @param tileID Target tile UID.
/// @param cmd    Command keyword.
void EventConnector::onTerrainCommand( unsigned int tileID, QString cmd )
{
	if ( cmd == "Mine" )
		g->jm()->addJob( "Mine", Position( tileID ), 0 );
	else if ( cmd == "Remove" )
		g->jm()->addJob( "RemoveFloor", Position( tileID ), 0 );
	else if ( cmd == "Fell" )
		g->jm()->addJob( "FellTree", Position( tileID ), 0 );
	else if ( cmd == "Destroy" )
		g->jm()->addJob( "RemovePlant", Position( tileID ), 0 );
	else if ( cmd == "Harvest" )
	{
		if ( gm->game() && gm->game()->world() && gm->game()->world()->plants().contains( tileID ) )
		{
			Plant& plant = gm->game()->world()->plants()[tileID];
			if ( plant.isTree() )
			{
				g->jm()->addJob( "HarvestTree", Position( tileID ), 0 );
			}
			else
			{
				g->jm()->addJob( "Harvest", Position( tileID ), 0 );
			}
		}
	}
}

/// @brief Opens the appropriate management window for a tile's designation (workshop,
///        stockpile, farm/pasture/grove, or room) based on its tile flags.
/// @param tileID Target tile UID.
void EventConnector::onManageCommand( unsigned int tileID )
{
	if ( gm->game() && gm->game()->world() )
	{
		Tile& tile = gm->game()->world()->getTile( tileID );

		TileFlag designation = tile.flags - ~( TileFlag::TF_WORKSHOP + TileFlag::TF_STOCKPILE + TileFlag::TF_GROVE + TileFlag::TF_FARM + TileFlag::TF_PASTURE + TileFlag::TF_ROOM );
		switch ( designation )
		{
			case TileFlag::TF_WORKSHOP:
				m_wsAggregator->onOpenWorkshopInfoOnTile( tileID );
				break;
			case TileFlag::TF_STOCKPILE:
			{
				m_spAggregator->onOpenStockpileInfoOnTile( tileID );
			}
			break;
			case TileFlag::TF_GROVE:
			case TileFlag::TF_FARM:
			case TileFlag::TF_PASTURE:
			{
				m_acAggregator->onOpen( designation, tileID );
			}
			break;
			case TileFlag::TF_ROOM:
				break;
		}
	}
}


/// @brief Applies render-option checkboxes to the Global toggles that control overlays.
/// @param designations True to show designation overlays.
/// @param jobs         True to show job overlays.
/// @param walls        True to lower walls (walls become half-height).
/// @param axles        True to show mechanism axles.
void EventConnector::onSetRenderOptions( bool designations, bool jobs, bool walls, bool axles )
{

	Global::wallsLowered = walls;
	Global::showDesignations = designations;
	Global::showJobs = jobs;
	Global::showAxles = axles;


}

/// @brief Emits the current render-option Globals so the GUI can reflect them.
void EventConnector::onUpdateRenderOptions()
{
	emit signalUpdateRenderOptions( Global::showDesignations, Global::showJobs, Global::wallsLowered, Global::showAxles );
}

/// @brief Relays a sound effect request (QVariantMap form) to the sound aggregator.
/// @param effect Sound-effect descriptor.
void EventConnector::onPlayEffect( QVariantMap effect)
{
	emit signalPlayEffect( effect);
}

/// @brief Relays the renderer's current camera position to any listeners (e.g. sound).
/// @param x     Camera X.
/// @param y     Camera Y.
/// @param z     Camera Z.
/// @param r     Rotation index.
/// @param scale Zoom factor.
void EventConnector::onCameraPosition( float x, float y, float z, int r, float scale )
{
	emit signalCameraPosition( x, y, z, r, scale );
}

/// @brief Emits startGame so GameManager can switch the GUI into gameplay state.
void EventConnector::emitStartGame()
{
	emit startGame();
}

/// @brief Emits stopGame so GameManager can unload the current game and return to the menu.
void EventConnector::emitStopGame()
{
	emit stopGame();
}

/// @brief Asks the GUI to initialise the gameplay viewport.
void EventConnector::emitInitView()
{
	emit signalInitView();
}

/// @brief Toggles in-menu mode on the GUI (shows/hides the main menu overlay).
/// @param value True when the main menu is active.
void EventConnector::emitInMenu( bool value )
{
	emit signalInMenu( value );
}

/// @brief Starts a new game via GameManager.
void EventConnector::onStartNewGame()
{
	gm->startNewGame();
}

/// @brief Continues the most recent save via GameManager.
void EventConnector::onContinueLastGame()
{
	gm->continueLastGame();
}

/// @brief Loads a specific save folder via GameManager.
/// @param folder Absolute path to the save folder.
void EventConnector::onLoadGame( QString folder )
{
	gm->loadGame( folder );
}

/// @brief Saves the current game via GameManager.
void EventConnector::onSaveGame()
{
	gm->saveGame();
}

/// @brief Forwards the show-main-menu flag to GameManager.
/// @param value True to show the main menu overlay.
void EventConnector::onSetShowMainMenu( bool value )
{
	gm->setShowMainMenu( value );
}

/// @brief Ends the current game via GameManager (returns to main menu).
void EventConnector::onEndGame()
{
	gm->endCurrentGame();
}

/// @brief Asks the GUI to resume from a paused state.
void EventConnector::sendResume()
{
	emit signalResume();
}

/// @brief Notifies the GUI that a load-game operation finished.
/// @param value True on success.
void EventConnector::sendLoadGameDone( bool value )
{
	emit signalLoadGameDone( value );
}

/// @brief Emits a paused-state change so the GUI can update the pause indicator.
/// @param paused New paused flag.
void EventConnector::emitPause( bool paused )
{
	emit signalUpdatePause( paused );
}

/// @brief Emits a speed change so the GUI can update the speed indicator.
/// @param speed New speed enum.
void EventConnector::emitGameSpeed( GameSpeed speed )
{
	emit signalUpdateGameSpeed( speed );
}

/// @brief Sets the active Global::sel action string (e.g. "DigFloor").
/// @param action Action keyword.
void EventConnector::onSetSelectionAction( QString action )
{
	Global::sel->setAction( action );
}

/// @brief Sets the active Global::sel item ID (e.g. the sprite to preview).
/// @param item Item string ID.
void EventConnector::onSetSelectionItem( QString item )
{
	Global::sel->setItemID( item );
}

/// @brief Sets the active Global::sel material list.
/// @param mats Materials to use for the current action.
void EventConnector::onSetSelectionMaterials( QStringList mats )
{
	Global::sel->setMaterials( mats );
}

/// @brief Entry point for build menu clicks. Picks the correct action string based on
///        BuildItemType and the DB construction type, then initialises Global::sel with
///        the chosen materials and item and emits signalBuild so the GUI enters placement mode.
/// @param type  Build kind (workshop/terrain/item).
/// @param param Optional action prefix (e.g. "FillHole" or "BuildFancy").
/// @param item  Item/construction/workshop ID.
/// @param mats  Materials selected per component.
void EventConnector::onCmdBuild( BuildItemType type, QString param, QString item, QStringList mats )
{
	switch ( type )
	{
		case BuildItemType::Workshop:
		{
			Global::sel->setAction( "BuildWorkshop" );
		}
		break;
		case BuildItemType::Terrain:
		{
			QString type = DB::select( "Type", "Constructions", item ).toString();
			
			if( !param.isEmpty() )
			{
				if( param == "FillHole" )
				{
					Global::sel->setAction( param );
				}
				else
				{
					Global::sel->setAction( param + type );
				}
			}
			else
			{
				if( type == "Stairs" && item == "Scaffold" )
				{
					Global::sel->setAction( "BuildScaffold" );
				}
				else
				{
					Global::sel->setAction( "Build" + type );
				}
			}
		}
		break;
		case BuildItemType::Item:
		{
			Global::sel->setAction( "BuildItem" );
		}
		break;
	}

	Global::sel->setMaterials( mats );
	Global::sel->setItemID( item );

	emit signalBuild();
}

/// @brief Relays a game event notification (e.g. trader arrival, invasion warning) to the GUI.
/// @param id    Event UID.
/// @param title Event title.
/// @param msg   Body message.
/// @param pause True if the event should pause the game when shown.
/// @param yesno True if the event needs a yes/no answer.
void EventConnector::onEvent( unsigned int id, QString title, QString msg, bool pause, bool yesno )
{
	emit signalEvent( id, title, msg, pause, yesno );
}

/// @brief Forwards a user's yes/no answer to the event back to EventManager.
/// @param id     Event UID.
/// @param answer User's answer (true = yes).
void EventConnector::onAnswer( unsigned int id, bool answer )
{
	g->em()->onAnswer( id, answer );
}

/// @brief Returns the current Game instance pointer.
/// @return Game pointer, or nullptr if no game is loaded.
Game* EventConnector::game()
{
	return g;
}