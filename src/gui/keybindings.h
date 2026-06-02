/** @file keybindings.h
 *  @brief Enumeration of every user-triggerable keyboard action and the KeyBindings class
 *         that maps configured keys onto those actions.
 */
#pragma once

#include <QKeyEvent>
#include <QObject>
#include <QHash>

/// @brief Every hotkey action the user can bind. Grouped loosely by category
///        (camera, windows, build actions, debug, magic).
enum UserKeyboardAction : unsigned int {
	NoAction,
	WorldScrollLeft,
	WorldScrollRight,
	WorldScrollUp,
	WorldScrollDown,
	RotateWorldCW,
	RotateWorldCCW,
	ZMinus,
	ZPlus,
	ZoomIn,
	ZoomOut,

	
	ToggleFullScreen,
	OpenGnomeList,
	ToggleWalls,
	ToggleOverlay,
	OpenLastActionWindow,
	TogglePause,
	RotateSelection,
	ToggleAxles,
	
	MenuButtonKey1,
	MenuButtonKey2,
	MenuButtonKey3,
	MenuButtonKey4,
	MenuButtonKey5,
	MenuButtonKey6,
	MenuButtonKey7,
	MenuButtonKey8,
	MenuButtonKey9,
	MenuButtonKey0,
	
	OpenBugReportWindow,
	OpenLogWindow,
	OpenDebugWindow,
	PrintDebug,
	ToggleDebugOverlay,
	ToggleDebugMode,
	ReloadShaders,
	ReloadCSS,

	QuickSave,
	QuickLoad,

	ActionMine,
	ActionDigHole,
	ActionExplorativeMine,
	ActionRemoveRamp,
	ActionRemoveFloor,
	ActionRemovePlant,
	ActionMineStairsUp,
	ActionDigStairsDown,
	ActionDigRampDown,
	ActionCreateRoom,
	ActionCreateStockpile,
	ActionCreateGrove,
	ActionCreateFarm,
	ActionCreatePasture,
	ActionCreateNoPass,
	ActionBuildWall,
	ActionReplaceWall,
	ActionBuildWallFloor,
	ActionBuildFancyWall,
	ActionBuildFloor,
	ActionReplaceFloor,
	ActionBuildFancyFloor,
	ActionBuildScaffold,
	ActionBuildFence,
	ActionBuildWorkshop,
	ActionBuildStairs,
	ActionBuildRamp,
	ActionBuildRampCorner,
	ActionCutClipping,
	ActionBuildItem,
	ActionPlantTree,
	ActionFellTree,
	ActionForage,
	ActionHarvestTree,
	ActionRemoveDesignation,
	ActionDeconstruct,
	ActionCancelJob,
	ActionRaisePrio,
	ActionLowerPrio,
	ActionMagicNatureSpeedGrowth,
	ActionMagicGeomancyRevealOre,

	ToggleRenderCreatures
	

};


/// @brief Loads the user's key bindings from config and resolves Qt key events to
///        UserKeyboardAction values. Also provides reverse lookup for the settings GUI.
class KeyBindings : public QObject
{
	Q_OBJECT

public:
	KeyBindings(QObject *parent = 0);
	~KeyBindings();

	void update();
	UserKeyboardAction getCommand( QKeyEvent* event );

	QString getStringForCommand( UserKeyboardAction cmd );

private:
	QHash<QString, UserKeyboardAction>m_stringToCommand; ///< Config string → action enum.
	QHash<QString, UserKeyboardAction>m_keyCodeToCmd;    ///< Encoded key combo → action enum.
};
