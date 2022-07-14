#include "keybindings.h"

#include "../base/io.h"

#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QJsonArray>
#include <QJsonDocument>

#include <range/v3/view.hpp>

#include "spdlog/spdlog.h"

KeyBindings::KeyBindings(QObject *parent)
	: QObject(parent)
{
	m_stringToCommand.insert_or_assign( "WorldScrollLeft",     WorldScrollLeft );
	m_stringToCommand.insert_or_assign( "WorldScrollRight",    WorldScrollRight );
	m_stringToCommand.insert_or_assign( "WorldScrollUp",       WorldScrollUp );
	m_stringToCommand.insert_or_assign( "WorldScrollDown",     WorldScrollDown );
	m_stringToCommand.insert_or_assign( "OpenBugReportWindow", OpenBugReportWindow );
	m_stringToCommand.insert_or_assign( "OpenLogWindow",       OpenLogWindow );
	m_stringToCommand.insert_or_assign( "OpenDebugWindow",     OpenDebugWindow );
	m_stringToCommand.insert_or_assign( "PrintDebug",          PrintDebug );
	m_stringToCommand.insert_or_assign( "ToggleDebugOverlay",  ToggleDebugOverlay );
	m_stringToCommand.insert_or_assign( "ToggleDebugMode",     ToggleDebugMode );
	m_stringToCommand.insert_or_assign( "ToggleFullScreen",    ToggleFullScreen );
	m_stringToCommand.insert_or_assign( "OpenGnomeList",       OpenGnomeList );
	m_stringToCommand.insert_or_assign( "ToggleWalls",         ToggleWalls );
	m_stringToCommand.insert_or_assign( "ToggleOverlay",       ToggleOverlay );
	m_stringToCommand.insert_or_assign( "ToggleAxles",         ToggleAxles );
	m_stringToCommand.insert_or_assign( "ReloadShaders",       ReloadShaders );
	m_stringToCommand.insert_or_assign( "ReloadCSS",			 ReloadCSS );
	m_stringToCommand.insert_or_assign( "OpenLastActionWindow",OpenLastActionWindow );
	m_stringToCommand.insert_or_assign( "TogglePause",         TogglePause );
	m_stringToCommand.insert_or_assign( "RotateSelection",     RotateSelection );
	m_stringToCommand.insert_or_assign( "RotateWorldCW",       RotateWorldCW );
	m_stringToCommand.insert_or_assign( "RotateWorldCCW",      RotateWorldCCW );
	m_stringToCommand.insert_or_assign( "ZMinus",              ZMinus );
	m_stringToCommand.insert_or_assign( "ZPlus",               ZPlus );
	m_stringToCommand.insert_or_assign( "ZoomIn",              ZoomIn );
	m_stringToCommand.insert_or_assign( "ZoomOut",             ZoomOut );
	m_stringToCommand.insert_or_assign( "MenuButtonKey1",      MenuButtonKey1 );
	m_stringToCommand.insert_or_assign( "MenuButtonKey2",      MenuButtonKey2 );
	m_stringToCommand.insert_or_assign( "MenuButtonKey3",      MenuButtonKey3 );
	m_stringToCommand.insert_or_assign( "MenuButtonKey4",      MenuButtonKey4 );
	m_stringToCommand.insert_or_assign( "MenuButtonKey5",      MenuButtonKey5 );
	m_stringToCommand.insert_or_assign( "MenuButtonKey6",      MenuButtonKey6 );
	m_stringToCommand.insert_or_assign( "MenuButtonKey7",      MenuButtonKey7 );
	m_stringToCommand.insert_or_assign( "MenuButtonKey8",      MenuButtonKey8 );
	m_stringToCommand.insert_or_assign( "MenuButtonKey9",      MenuButtonKey9 );
	m_stringToCommand.insert_or_assign( "MenuButtonKey0",      MenuButtonKey0 );

	m_stringToCommand.insert_or_assign( "Mine",								ActionMine );
	m_stringToCommand.insert_or_assign( "DigHole",                        	ActionDigHole );
	m_stringToCommand.insert_or_assign( "ExplorativeMine",                	ActionExplorativeMine );
	m_stringToCommand.insert_or_assign( "RemoveRamp",                     	ActionRemoveRamp );
	m_stringToCommand.insert_or_assign( "RemoveFloor",                    	ActionRemoveFloor );
	m_stringToCommand.insert_or_assign( "RemovePlant",                    	ActionRemovePlant );
	m_stringToCommand.insert_or_assign( "MineStairsUp",                   	ActionMineStairsUp );
	m_stringToCommand.insert_or_assign( "DigStairsDown",                  	ActionDigStairsDown );
	m_stringToCommand.insert_or_assign( "DigRampDown",                    	ActionDigRampDown );
	m_stringToCommand.insert_or_assign( "CreateRoom",                     	ActionCreateRoom );
	m_stringToCommand.insert_or_assign( "CreateStockpile",                	ActionCreateStockpile );
	m_stringToCommand.insert_or_assign( "CreateGrove",                    	ActionCreateGrove );
	m_stringToCommand.insert_or_assign( "CreateFarm",                     	ActionCreateFarm );
	m_stringToCommand.insert_or_assign( "CreatePasture",                  	ActionCreatePasture );
	m_stringToCommand.insert_or_assign( "CreateNoPass",                   	ActionCreateNoPass );
	m_stringToCommand.insert_or_assign( "BuildWall",                      	ActionBuildWall );
	m_stringToCommand.insert_or_assign( "ReplaceWall",                    	ActionReplaceWall );
	m_stringToCommand.insert_or_assign( "BuildWallFloor",                 	ActionBuildWallFloor );
	m_stringToCommand.insert_or_assign( "BuildFancyWall",                 	ActionBuildFancyWall );
	m_stringToCommand.insert_or_assign( "BuildFloor",                     	ActionBuildFloor );
	m_stringToCommand.insert_or_assign( "ReplaceFloor",                   	ActionReplaceFloor );
	m_stringToCommand.insert_or_assign( "BuildFancyFloor",                	ActionBuildFancyFloor );
	m_stringToCommand.insert_or_assign( "BuildScaffold",                  	ActionBuildScaffold );
	m_stringToCommand.insert_or_assign( "BuildFence",                     	ActionBuildFence );
	m_stringToCommand.insert_or_assign( "BuildWorkshop",                  	ActionBuildWorkshop );
	m_stringToCommand.insert_or_assign( "BuildStairs",                    	ActionBuildStairs );
	m_stringToCommand.insert_or_assign( "BuildRamp",                      	ActionBuildRamp );
	m_stringToCommand.insert_or_assign( "BuildRampCorner",                	ActionBuildRampCorner );
	m_stringToCommand.insert_or_assign( "CutClipping",                    	ActionCutClipping );
	m_stringToCommand.insert_or_assign( "BuildItem",                      	ActionBuildItem );
	m_stringToCommand.insert_or_assign( "PlantTree",                      	ActionPlantTree );
	m_stringToCommand.insert_or_assign( "FellTree",                       	ActionFellTree );
	m_stringToCommand.insert_or_assign( "Forage",                         	ActionForage );
	m_stringToCommand.insert_or_assign( "HarvestTree",                    	ActionHarvestTree );
	m_stringToCommand.insert_or_assign( "RemoveDesignation",              	ActionRemoveDesignation );
	m_stringToCommand.insert_or_assign( "Deconstruct",                    	ActionDeconstruct );
	m_stringToCommand.insert_or_assign( "CancelJob",                      	ActionCancelJob );
	m_stringToCommand.insert_or_assign( "RaisePrio",                      	ActionRaisePrio );
	m_stringToCommand.insert_or_assign( "LowerPrio",                      	ActionLowerPrio );
	m_stringToCommand.insert_or_assign( "MagicNatureSpeedGrowth",         	ActionMagicNatureSpeedGrowth );
	m_stringToCommand.insert_or_assign( "MagicGeomancyRevealOre",         	ActionMagicGeomancyRevealOre );

	m_stringToCommand.insert_or_assign( "ToggleRenderCreatures",         		ToggleRenderCreatures );

	m_stringToCommand.insert_or_assign( "QuickSave",         					QuickSave );
	m_stringToCommand.insert_or_assign( "QuickLoad",         					QuickLoad );

}

KeyBindings::~KeyBindings()
{
}

void KeyBindings::update()
{
	const fs::path& folder = IO::getDataFolder() / "settings";
	QJsonDocument jd;
	IO::loadFile( folder / "keybindings.json", jd );
	spdlog::debug("Load key bindings...");
	auto groupList = jd.toVariant().toList();
	
	for( auto groupElement : groupList )
	{
		auto eleMap = groupElement.toMap();
		for( auto vKey : eleMap.value( "Keys" ).toList() )
		{
			auto kmap = vKey.toMap();
			QString cmd = kmap.value( "Command" ).toString();
			QVariantMap km1 = kmap.value( "Key1" ).toMap();
			QString keyCode = km1.value( "Key" ).toString() + km1.value( "Ctrl" ).toString() + km1.value( "Alt" ).toString() + km1.value( "Shift" ).toString();
			m_keyCodeToCmd.insert_or_assign( keyCode, m_stringToCommand.at( cmd) );
			QVariantMap km2 = kmap.value( "Key2" ).toMap();
			keyCode = km2.value( "Key" ).toString() + km2.value( "Ctrl" ).toString() + km2.value( "Alt" ).toString() + km2.value( "Shift" ).toString();
			m_keyCodeToCmd.insert_or_assign( keyCode, m_stringToCommand.at( cmd) );
		}
	}
}

UserKeyboardAction KeyBindings::getCommand( QKeyEvent* event )
{
	bool ctrl = event->modifiers() & Qt::CTRL;
	bool alt = event->modifiers() & Qt::ALT;
	bool shift = event->modifiers() & Qt::SHIFT;

	QString keyCode = QKeySequence( event->key() ).toString();
	keyCode += ctrl ? "true" : "false";
	keyCode += alt ? "true" : "false";
	keyCode += shift ? "true" : "false";
	if( m_keyCodeToCmd.contains( keyCode ) )
	{
		return m_keyCodeToCmd.at( keyCode );
	}
	return NoAction;
}

QString KeyBindings::getStringForCommand( UserKeyboardAction cmd )
{
	for( auto key : m_stringToCommand | ranges::views::keys )
	{
		if( m_stringToCommand[key] == cmd )
		{
			return key;
		}
	}
	return "";
}