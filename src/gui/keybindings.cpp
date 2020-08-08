#include "keybindings.h"

#include "../base/io.h"

#include <QDebug>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QJsonArray>
#include <QJsonDocument>

KeyBindings::KeyBindings(QObject *parent)
	: QObject(parent)
{
	m_stringToCommand.insert( "WorldScrollLeft",     WorldScrollLeft );
	m_stringToCommand.insert( "WorldScrollRight",    WorldScrollRight );
	m_stringToCommand.insert( "WorldScrollUp",       WorldScrollUp );
	m_stringToCommand.insert( "WorldScrollDown",     WorldScrollDown );
	m_stringToCommand.insert( "OpenBugReportWindow", OpenBugReportWindow );
	m_stringToCommand.insert( "OpenLogWindow",       OpenLogWindow );
	m_stringToCommand.insert( "OpenDebugWindow",     OpenDebugWindow );
	m_stringToCommand.insert( "PrintDebug",          PrintDebug );
	m_stringToCommand.insert( "ToggleDebugOverlay",  ToggleDebugOverlay );
	m_stringToCommand.insert( "ToggleDebugMode",     ToggleDebugMode );
	m_stringToCommand.insert( "ToggleFullScreen",    ToggleFullScreen );
	m_stringToCommand.insert( "OpenGnomeList",       OpenGnomeList );
	m_stringToCommand.insert( "ToggleWalls",         ToggleWalls );
	m_stringToCommand.insert( "ToggleOverlay",       ToggleOverlay );
	m_stringToCommand.insert( "ToggleAxles",         ToggleAxles );
	m_stringToCommand.insert( "ReloadShaders",       ReloadShaders );
	m_stringToCommand.insert( "ReloadCSS",			 ReloadCSS );
	m_stringToCommand.insert( "OpenLastActionWindow",OpenLastActionWindow );
	m_stringToCommand.insert( "TogglePause",         TogglePause );
	m_stringToCommand.insert( "RotateSelection",     RotateSelection );
	m_stringToCommand.insert( "RotateWorldCW",       RotateWorldCW );
	m_stringToCommand.insert( "RotateWorldCCW",      RotateWorldCCW );
	m_stringToCommand.insert( "ZMinus",              ZMinus );
	m_stringToCommand.insert( "ZPlus",               ZPlus );
	m_stringToCommand.insert( "ZoomIn",              ZoomIn );
	m_stringToCommand.insert( "ZoomOut",             ZoomOut );
	m_stringToCommand.insert( "MenuButtonKey1",      MenuButtonKey1 );
	m_stringToCommand.insert( "MenuButtonKey2",      MenuButtonKey2 );
	m_stringToCommand.insert( "MenuButtonKey3",      MenuButtonKey3 );
	m_stringToCommand.insert( "MenuButtonKey4",      MenuButtonKey4 );
	m_stringToCommand.insert( "MenuButtonKey5",      MenuButtonKey5 );
	m_stringToCommand.insert( "MenuButtonKey6",      MenuButtonKey6 );
	m_stringToCommand.insert( "MenuButtonKey7",      MenuButtonKey7 );
	m_stringToCommand.insert( "MenuButtonKey8",      MenuButtonKey8 );
	m_stringToCommand.insert( "MenuButtonKey9",      MenuButtonKey9 );
	m_stringToCommand.insert( "MenuButtonKey0",      MenuButtonKey0 );

	m_stringToCommand.insert( "Mine",								ActionMine );
	m_stringToCommand.insert( "DigHole",                        	ActionDigHole );
	m_stringToCommand.insert( "ExplorativeMine",                	ActionExplorativeMine );
	m_stringToCommand.insert( "RemoveRamp",                     	ActionRemoveRamp );
	m_stringToCommand.insert( "RemoveFloor",                    	ActionRemoveFloor );
	m_stringToCommand.insert( "RemovePlant",                    	ActionRemovePlant );
	m_stringToCommand.insert( "MineStairsUp",                   	ActionMineStairsUp );
	m_stringToCommand.insert( "DigStairsDown",                  	ActionDigStairsDown );
	m_stringToCommand.insert( "DigRampDown",                    	ActionDigRampDown );
	m_stringToCommand.insert( "CreateRoom",                     	ActionCreateRoom );
	m_stringToCommand.insert( "CreateStockpile",                	ActionCreateStockpile );
	m_stringToCommand.insert( "CreateGrove",                    	ActionCreateGrove );
	m_stringToCommand.insert( "CreateFarm",                     	ActionCreateFarm );
	m_stringToCommand.insert( "CreatePasture",                  	ActionCreatePasture );
	m_stringToCommand.insert( "CreateNoPass",                   	ActionCreateNoPass );
	m_stringToCommand.insert( "BuildWall",                      	ActionBuildWall );
	m_stringToCommand.insert( "ReplaceWall",                    	ActionReplaceWall );
	m_stringToCommand.insert( "BuildWallFloor",                 	ActionBuildWallFloor );
	m_stringToCommand.insert( "BuildFancyWall",                 	ActionBuildFancyWall );
	m_stringToCommand.insert( "BuildFloor",                     	ActionBuildFloor );
	m_stringToCommand.insert( "ReplaceFloor",                   	ActionReplaceFloor );
	m_stringToCommand.insert( "BuildFancyFloor",                	ActionBuildFancyFloor );
	m_stringToCommand.insert( "BuildScaffold",                  	ActionBuildScaffold );
	m_stringToCommand.insert( "BuildFence",                     	ActionBuildFence );
	m_stringToCommand.insert( "BuildWorkshop",                  	ActionBuildWorkshop );
	m_stringToCommand.insert( "BuildStairs",                    	ActionBuildStairs );
	m_stringToCommand.insert( "BuildRamp",                      	ActionBuildRamp );
	m_stringToCommand.insert( "BuildRampCorner",                	ActionBuildRampCorner );
	m_stringToCommand.insert( "CutClipping",                    	ActionCutClipping );
	m_stringToCommand.insert( "BuildItem",                      	ActionBuildItem );
	m_stringToCommand.insert( "PlantTree",                      	ActionPlantTree );
	m_stringToCommand.insert( "FellTree",                       	ActionFellTree );
	m_stringToCommand.insert( "Forage",                         	ActionForage );
	m_stringToCommand.insert( "HarvestTree",                    	ActionHarvestTree );
	m_stringToCommand.insert( "RemoveDesignation",              	ActionRemoveDesignation );
	m_stringToCommand.insert( "Deconstruct",                    	ActionDeconstruct );
	m_stringToCommand.insert( "CancelJob",                      	ActionCancelJob );
	m_stringToCommand.insert( "RaisePrio",                      	ActionRaisePrio );
	m_stringToCommand.insert( "LowerPrio",                      	ActionLowerPrio );
	m_stringToCommand.insert( "MagicNatureSpeedGrowth",         	ActionMagicNatureSpeedGrowth );
	m_stringToCommand.insert( "MagicGeomancyRevealOre",         	ActionMagicGeomancyRevealOre );

	m_stringToCommand.insert( "ToggleRenderCreatures",         		ToggleRenderCreatures );

	m_stringToCommand.insert( "QuickSave",         					QuickSave );
	m_stringToCommand.insert( "QuickLoad",         					QuickLoad );

}

KeyBindings::~KeyBindings()
{
}

void KeyBindings::update()
{
	QString folder = QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/My Games/Ingnomia/settings/";
	QJsonDocument jd;
	IO::loadFile( folder + "keybindings.json", jd );
	qDebug() << "Load key bindings...";
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
			m_keyCodeToCmd.insert( keyCode, m_stringToCommand.value( cmd) );
			QVariantMap km2 = kmap.value( "Key2" ).toMap();
			keyCode = km2.value( "Key" ).toString() + km2.value( "Ctrl" ).toString() + km2.value( "Alt" ).toString() + km2.value( "Shift" ).toString();
			m_keyCodeToCmd.insert( keyCode, m_stringToCommand.value( cmd) );
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
		return m_keyCodeToCmd.value( keyCode );
	}
	return NoAction;
}

QString KeyBindings::getStringForCommand( UserKeyboardAction cmd )
{
	for( auto key : m_stringToCommand.keys() )
	{
		if( m_stringToCommand[key] == cmd )
		{
			return key;
		}
	}
	return "";
}