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
#include "global.h"

#include "../base/config.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/io.h"
#include "../base/logger.h"
#include "../base/util.h"
#include "spdlog/spdlog.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>

EventConnector* Global::eventConnector = nullptr;
Util* Global::util = nullptr;
Selection* Global::sel = nullptr;
NewGameSettings* Global::newGameSettings = nullptr;
Config* Global::cfg = nullptr;
fs::path Global::exePath;

Logger Global::m_logger;

bool Global::wallsLowered = false;
bool Global::showAxles    = false;
bool Global::showDesignations = true;
bool Global::showJobs = true;

unsigned int Global::waterSpriteUID  = 0;
unsigned int Global::undiscoveredUID = 0;

QVariantMap Global::copiedStockpileSettings;

absl::btree_map<QString, QVariantMap> Global::m_windowParams;

absl::btree_map<QString, absl::btree_set<QString>> Global::allowedInContainer;

absl::flat_hash_map<Qt::Key, Noesis::Key> Global::keyConvertMap;

int Global::dimX     = 100;
int Global::dimY     = 100;
int Global::dimZ     = 100;

int Global::zWeight = 20;

double Global::xpMod = 250.;

bool Global::debugMode = false;
bool Global::debugOpenGL = false;
bool Global::debugSound = false;

absl::btree_map<QString, pugi::xml_node> Global::m_behaviorTrees;

QStringList Global::needIDs;
absl::btree_map<QString, float> Global::needDecays;

unsigned int Global::dirtUID = 0;

absl::btree_map<QString, CreaturePart> Global::creaturePartLookUp;
absl::btree_map<CreaturePart, QString> Global::creaturePartToString;

absl::btree_set<QString> Global::craftable;

void Global::reset()
{
	spdlog::debug("*** Global reset");

	GameState::stockOverlay.clear();
	GameState::squads.clear();

	Global::xpMod = Global::cfg->get<double>( "XpMod" );

	m_logger.reset();

	wallsLowered = false;
	showAxles    = false;

	//m_keyBindings.update();

	if ( !loadBehaviorTrees() )
	{
		spdlog::critical("failed to load behavior trees");
		abort();
	}

	needIDs.clear();
	needDecays.clear();

	for ( auto row : DB::selectRows( "Needs" ) )
	{
		if ( row.value( "Creature" ).toString() == "Gnome" )
		{
			auto need = row.value( "ID" ).toString();
			needIDs.append( need );
			needDecays.insert_or_assign( need, row.value( "DecayPerMinute" ).toFloat() );
		}
	}

	Global::dirtUID = DBH::materialUID( "Dirt" );

	Global::cfg->set( "renderCreatures", true );

	creaturePartLookUp.insert_or_assign( "Head", CP_HEAD );
	creaturePartLookUp.insert_or_assign( "Torso", CP_TORSO );
	creaturePartLookUp.insert_or_assign( "LeftArm", CP_LEFT_ARM );
	creaturePartLookUp.insert_or_assign( "RightArm", CP_RIGHT_ARM );
	creaturePartLookUp.insert_or_assign( "LeftHand", CP_LEFT_HAND );
	creaturePartLookUp.insert_or_assign( "RightHand", CP_RIGHT_HAND );
	creaturePartLookUp.insert_or_assign( "LeftLeg", CP_LEFT_LEG );
	creaturePartLookUp.insert_or_assign( "RightLeg", CP_RIGHT_LEG );
	creaturePartLookUp.insert_or_assign( "LeftFoot", CP_LEFT_FOOT );
	creaturePartLookUp.insert_or_assign( "RightFoot", CP_RIGHT_FOOT );

	creaturePartLookUp.insert_or_assign( "LeftFrontLeg", CP_LEFT_FRONT_LEG );
	creaturePartLookUp.insert_or_assign( "LegFrontLeg", CP_RIGHT_FRONT_LEG );
	creaturePartLookUp.insert_or_assign( "LeftFrontFoot", CP_LEFT_FRONT_FOOT );
	creaturePartLookUp.insert_or_assign( "RightFrontFoot", CP_RIGHT_FRONT_FOOT );

	creaturePartLookUp.insert_or_assign( "LeftWing", CP_RIGHT_WING );
	creaturePartLookUp.insert_or_assign( "RightWing", CP_RIGHT_WING );

	creaturePartLookUp.insert_or_assign( "Brain", CP_BRAIN );
	creaturePartLookUp.insert_or_assign( "LeftEye", CP_LEFT_EYE );
	creaturePartLookUp.insert_or_assign( "RightEye", CP_RIGHT_EYE );
	creaturePartLookUp.insert_or_assign( "Heart", CP_HEART );
	creaturePartLookUp.insert_or_assign( "LeftLung", CP_LEFT_LUNG );
	creaturePartLookUp.insert_or_assign( "RightLung", CP_RIGHT_LUNG );

	creaturePartLookUp.insert_or_assign( "Hair", CP_HAIR );
	creaturePartLookUp.insert_or_assign( "Facial", CP_FACIAL_HAIR );
	creaturePartLookUp.insert_or_assign( "Clothing", CP_CLOTHING );
	creaturePartLookUp.insert_or_assign( "Boots", CP_BOOTS );
	creaturePartLookUp.insert_or_assign( "Hat", CP_HAT );
	creaturePartLookUp.insert_or_assign( "Back", CP_BACK );

	creaturePartLookUp.insert_or_assign( "HeadArmor", CP_ARMOR_HEAD );
	creaturePartLookUp.insert_or_assign( "ChestArmor", CP_ARMOR_TORSO );
	creaturePartLookUp.insert_or_assign( "LeftArmArmor", CP_ARMOR_LEFT_ARM );
	creaturePartLookUp.insert_or_assign( "RightArmArmor", CP_ARMOR_RIGHT_ARM );
	creaturePartLookUp.insert_or_assign( "LeftHandArmor", CP_ARMOR_LEFT_HAND );
	creaturePartLookUp.insert_or_assign( "RightHandArmor", CP_ARMOR_RIGHT_HAND );
	creaturePartLookUp.insert_or_assign( "LeftFootArmor", CP_ARMOR_LEFT_FOOT );
	creaturePartLookUp.insert_or_assign( "RightFootArmor", CP_ARMOR_RIGHT_FOOT );
	creaturePartLookUp.insert_or_assign( "LeftHandHeld", CP_LEFT_HAND_HELD );
	creaturePartLookUp.insert_or_assign( "RightHandHeld", CP_RIGHT_HAND_HELD );

	creaturePartLookUp.insert_or_assign( "ArmArmor", CP_ARMOR_ARM );
	creaturePartLookUp.insert_or_assign( "HandArmor", CP_ARMOR_HAND );
	creaturePartLookUp.insert_or_assign( "FootArmor", CP_ARMOR_FOOT );
	creaturePartLookUp.insert_or_assign( "LegArmor", CP_ARMOR_LEG );

	creaturePartToString.insert_or_assign( CP_HEAD, "Head" );
	creaturePartToString.insert_or_assign( CP_TORSO, "Torso" );
	creaturePartToString.insert_or_assign( CP_LEFT_ARM, "LeftArm" );
	creaturePartToString.insert_or_assign( CP_RIGHT_ARM, "RightArm" );
	creaturePartToString.insert_or_assign( CP_LEFT_HAND, "LeftHand" );
	creaturePartToString.insert_or_assign( CP_RIGHT_HAND, "RightHand" );
	creaturePartToString.insert_or_assign( CP_LEFT_LEG, "LeftLeg" );
	creaturePartToString.insert_or_assign( CP_RIGHT_LEG, "LegLeg" );
	creaturePartToString.insert_or_assign( CP_LEFT_FOOT, "LeftFoot" );
	creaturePartToString.insert_or_assign( CP_RIGHT_FOOT, "RightFoot" );

	creaturePartToString.insert_or_assign( CP_LEFT_FRONT_LEG, "LeftFrontLeg" );
	creaturePartToString.insert_or_assign( CP_RIGHT_FRONT_LEG, "LegFrontLeg" );
	creaturePartToString.insert_or_assign( CP_LEFT_FRONT_FOOT, "LeftFrontFoot" );
	creaturePartToString.insert_or_assign( CP_RIGHT_FRONT_FOOT, "RightFrontFoot" );

	creaturePartToString.insert_or_assign( CP_LEFT_WING, "LeftWing" );
	creaturePartToString.insert_or_assign( CP_RIGHT_WING, "RightWing" );

	creaturePartToString.insert_or_assign( CP_BRAIN, "Brain" );
	creaturePartToString.insert_or_assign( CP_LEFT_EYE, "LeftEye" );
	creaturePartToString.insert_or_assign( CP_RIGHT_EYE, "RightEye" );
	creaturePartToString.insert_or_assign( CP_HEART, "Heart" );
	creaturePartToString.insert_or_assign( CP_LEFT_LUNG, "LeftLung" );
	creaturePartToString.insert_or_assign( CP_RIGHT_LUNG, "RightLung" );

	creaturePartToString.insert_or_assign( CP_HAIR, "Hair" );
	creaturePartToString.insert_or_assign( CP_FACIAL_HAIR, "Facial" );
	creaturePartToString.insert_or_assign( CP_CLOTHING, "Clothing" );
	creaturePartToString.insert_or_assign( CP_BOOTS, "Boots" );
	creaturePartToString.insert_or_assign( CP_HAT, "Hat" );
	creaturePartToString.insert_or_assign( CP_BACK, "Back" );

	creaturePartToString.insert_or_assign( CP_ARMOR_HEAD, "HeadArmor" );
	creaturePartToString.insert_or_assign( CP_ARMOR_TORSO, "ChestArmor" );
	creaturePartToString.insert_or_assign( CP_ARMOR_LEFT_ARM, "LeftArmArmor" );
	creaturePartToString.insert_or_assign( CP_ARMOR_RIGHT_ARM, "RightArmArmor" );
	creaturePartToString.insert_or_assign( CP_ARMOR_LEFT_HAND, "LeftHandArmor" );
	creaturePartToString.insert_or_assign( CP_ARMOR_RIGHT_HAND, "RightHandArmor" );
	creaturePartToString.insert_or_assign( CP_ARMOR_LEFT_FOOT, "LeftFootArmor" );
	creaturePartToString.insert_or_assign( CP_ARMOR_RIGHT_FOOT, "RightFootArmor" );
	creaturePartToString.insert_or_assign( CP_LEFT_HAND_HELD, "LeftHandHeld" );
	creaturePartToString.insert_or_assign( CP_RIGHT_HAND_HELD, "RightHandHeld" );

	creaturePartToString.insert_or_assign( CP_ARMOR_ARM, "ArmArmor" );
	creaturePartToString.insert_or_assign( CP_ARMOR_HAND, "HandArmor" );
	creaturePartToString.insert_or_assign( CP_ARMOR_FOOT, "FootArmor" );
	creaturePartToString.insert_or_assign( CP_ARMOR_LEG, "LegArmor" );

	craftable.clear();
	auto rows = DB::selectRows( "Crafts" );
	for( const auto& row : rows )
	{
		craftable.insert( row.value( "ItemID" ).toString() );
	}
}

Logger& Global::logger()
{
	return m_logger;
}

/*
KeyBindings& Global::keyBindings()
{
	return m_keyBindings;
}
*/

bool Global::loadBehaviorTrees()
{
	m_behaviorTrees.clear();

	for ( const auto& id : DB::ids( "AI" ) )
	{
		QString xmlName = DB::select( "BehaviorTree", "AI", id ).toString();

		pugi::xml_document xml;
		const auto& xmlPath = fs::path( Global::cfg->get<std::string>( "dataPath" ) ) / "ai" / xmlName.toStdString();
		xml.load_file( xmlPath.c_str() );

		const auto& root = xml.document_element();
		m_behaviorTrees.insert_or_assign( id, root );
	}
	return true;
}

const pugi::xml_node& Global::behaviorTree( QString id )
{
	return m_behaviorTrees.at( id );
}

bool Global::addBehaviorTree( QString id, QString path )
{
	pugi::xml_document xml;
	xml.load_file( path.toStdString().c_str() );

	const auto& root = xml.document_element();
	m_behaviorTrees.insert_or_assign( id, root );

	return true;
}

void Global::initKeyConvert()
{
	keyConvertMap.clear();

	//keyConvertMap.insert_or_assign( Qt::Key_, Noesis::Key_None );

	keyConvertMap.insert_or_assign( Qt::Key_Cancel, Noesis::Key_Cancel );
	keyConvertMap.insert_or_assign( Qt::Key_Backspace, Noesis::Key_Back );
	keyConvertMap.insert_or_assign( Qt::Key_Tab, Noesis::Key_Tab );
	//keyConvertMap.insert_or_assign( Qt::Key_, Noesis::Key_LineFeed );
	keyConvertMap.insert_or_assign( Qt::Key_Clear, Noesis::Key_Clear );
	keyConvertMap.insert_or_assign( Qt::Key_Return, Noesis::Key_Return );
	keyConvertMap.insert_or_assign( Qt::Key_Enter, Noesis::Key_Enter );
	keyConvertMap.insert_or_assign( Qt::Key_Pause, Noesis::Key_Pause );
	//keyConvertMap.insert_or_assign( Qt::Key_, Noesis::Key_Capital );
	keyConvertMap.insert_or_assign( Qt::Key_CapsLock, Noesis::Key_CapsLock );

	keyConvertMap.insert_or_assign( Qt::Key_Escape, Noesis::Key_Escape );

	//keyConvertMap.insert_or_assign( Qt::Key_, Noesis::Key_ImeConvert );
	//keyConvertMap.insert_or_assign( Qt::Key_, Noesis::Key_ImeNonConvert );
	//keyConvertMap.insert_or_assign( Qt::Key_, Noesis::Key_ImeAccept );
	//keyConvertMap.insert_or_assign( Qt::Key_, Noesis::Key_ImeModeChange );

	keyConvertMap.insert_or_assign( Qt::Key_Space, Noesis::Key_Space );

	//keyConvertMap.insert_or_assign( Qt::Key_, Noesis::Key_Prior );
	keyConvertMap.insert_or_assign( Qt::Key_PageUp, Noesis::Key_PageUp );
	//keyConvertMap.insert_or_assign( Qt::Key_, Noesis::Key_Next );
	keyConvertMap.insert_or_assign( Qt::Key_PageDown, Noesis::Key_PageDown );
	keyConvertMap.insert_or_assign( Qt::Key_End, Noesis::Key_End );
	keyConvertMap.insert_or_assign( Qt::Key_Home, Noesis::Key_Home );

	keyConvertMap.insert_or_assign( Qt::Key_Left, Noesis::Key_Left );
	keyConvertMap.insert_or_assign( Qt::Key_Up, Noesis::Key_Up );
	keyConvertMap.insert_or_assign( Qt::Key_Right, Noesis::Key_Right );
	keyConvertMap.insert_or_assign( Qt::Key_Down, Noesis::Key_Down );

	keyConvertMap.insert_or_assign( Qt::Key_Select, Noesis::Key_Select );
	keyConvertMap.insert_or_assign( Qt::Key_Print, Noesis::Key_Print );
	keyConvertMap.insert_or_assign( Qt::Key_Execute, Noesis::Key_Execute );
	//keyConvertMap.insert_or_assign( Qt::Key_, Noesis::Key_Snapshot );
	//keyConvertMap.insert_or_assign( Qt::Key_, Noesis::Key_PrintScreen );
	keyConvertMap.insert_or_assign( Qt::Key_Insert, Noesis::Key_Insert );
	keyConvertMap.insert_or_assign( Qt::Key_Delete, Noesis::Key_Delete );
	keyConvertMap.insert_or_assign( Qt::Key_Help, Noesis::Key_Help );

	keyConvertMap.insert_or_assign( Qt::Key_0, Noesis::Key_D0 );
	keyConvertMap.insert_or_assign( Qt::Key_1, Noesis::Key_D1 );
	keyConvertMap.insert_or_assign( Qt::Key_2, Noesis::Key_D2 );
	keyConvertMap.insert_or_assign( Qt::Key_3, Noesis::Key_D3 );
	keyConvertMap.insert_or_assign( Qt::Key_4, Noesis::Key_D4 );
	keyConvertMap.insert_or_assign( Qt::Key_5, Noesis::Key_D5 );
	keyConvertMap.insert_or_assign( Qt::Key_6, Noesis::Key_D6 );
	keyConvertMap.insert_or_assign( Qt::Key_7, Noesis::Key_D7 );
	keyConvertMap.insert_or_assign( Qt::Key_8, Noesis::Key_D8 );
	keyConvertMap.insert_or_assign( Qt::Key_9, Noesis::Key_D9 );

	keyConvertMap.insert_or_assign( Qt::Key_A, Noesis::Key_A );
	keyConvertMap.insert_or_assign( Qt::Key_B, Noesis::Key_B );
	keyConvertMap.insert_or_assign( Qt::Key_C, Noesis::Key_C );
	keyConvertMap.insert_or_assign( Qt::Key_D, Noesis::Key_D );
	keyConvertMap.insert_or_assign( Qt::Key_E, Noesis::Key_E );
	keyConvertMap.insert_or_assign( Qt::Key_F, Noesis::Key_F );
	keyConvertMap.insert_or_assign( Qt::Key_G, Noesis::Key_G );
	keyConvertMap.insert_or_assign( Qt::Key_H, Noesis::Key_H );
	keyConvertMap.insert_or_assign( Qt::Key_I, Noesis::Key_I );
	keyConvertMap.insert_or_assign( Qt::Key_J, Noesis::Key_J );
	keyConvertMap.insert_or_assign( Qt::Key_K, Noesis::Key_K );
	keyConvertMap.insert_or_assign( Qt::Key_L, Noesis::Key_L );
	keyConvertMap.insert_or_assign( Qt::Key_M, Noesis::Key_M );
	keyConvertMap.insert_or_assign( Qt::Key_N, Noesis::Key_N );
	keyConvertMap.insert_or_assign( Qt::Key_O, Noesis::Key_O );
	keyConvertMap.insert_or_assign( Qt::Key_P, Noesis::Key_P );
	keyConvertMap.insert_or_assign( Qt::Key_Q, Noesis::Key_Q );
	keyConvertMap.insert_or_assign( Qt::Key_R, Noesis::Key_R );
	keyConvertMap.insert_or_assign( Qt::Key_S, Noesis::Key_S );
	keyConvertMap.insert_or_assign( Qt::Key_T, Noesis::Key_T );
	keyConvertMap.insert_or_assign( Qt::Key_U, Noesis::Key_U );
	keyConvertMap.insert_or_assign( Qt::Key_V, Noesis::Key_V );
	keyConvertMap.insert_or_assign( Qt::Key_W, Noesis::Key_W );
	keyConvertMap.insert_or_assign( Qt::Key_X, Noesis::Key_X );
	keyConvertMap.insert_or_assign( Qt::Key_Y, Noesis::Key_Y );
	keyConvertMap.insert_or_assign( Qt::Key_Z, Noesis::Key_Z );

	keyConvertMap.insert_or_assign( Qt::Key_ApplicationLeft, Noesis::Key_LWin );
	keyConvertMap.insert_or_assign( Qt::Key_ApplicationRight, Noesis::Key_RWin );
	//keyConvertMap.insert_or_assign( Qt::Key_, Noesis::Key_Apps );
	keyConvertMap.insert_or_assign( Qt::Key_Sleep, Noesis::Key_Sleep );
	/*
	keyConvertMap.insert_or_assign( Qt::Key_0, Noesis::Key_NumPad0 );
	keyConvertMap.insert_or_assign( Qt::Key_1, Noesis::Key_NumPad1 );
	keyConvertMap.insert_or_assign( Qt::Key_2, Noesis::Key_NumPad2 );
	keyConvertMap.insert_or_assign( Qt::Key_3, Noesis::Key_NumPad3 );
	keyConvertMap.insert_or_assign( Qt::Key_4, Noesis::Key_NumPad4 );
	keyConvertMap.insert_or_assign( Qt::Key_5, Noesis::Key_NumPad5 );
	keyConvertMap.insert_or_assign( Qt::Key_6, Noesis::Key_NumPad6 );
	keyConvertMap.insert_or_assign( Qt::Key_7, Noesis::Key_NumPad7 );
	keyConvertMap.insert_or_assign( Qt::Key_8, Noesis::Key_NumPad8 );
	keyConvertMap.insert_or_assign( Qt::Key_9, Noesis::Key_NumPad9 );
	*/
	keyConvertMap.insert_or_assign( Qt::Key_multiply, Noesis::Key_Multiply );
	keyConvertMap.insert_or_assign( Qt::Key_Plus, Noesis::Key_Add );
	//keyConvertMap.insert_or_assign( Qt::Key_, Noesis::Key_Separator );
	keyConvertMap.insert_or_assign( Qt::Key_Minus, Noesis::Key_Subtract );
	keyConvertMap.insert_or_assign( Qt::Key_Colon, Noesis::Key_Decimal );
	keyConvertMap.insert_or_assign( Qt::Key_division, Noesis::Key_Divide );

	keyConvertMap.insert_or_assign( Qt::Key_F1, Noesis::Key_F1 );
	keyConvertMap.insert_or_assign( Qt::Key_F2, Noesis::Key_F2 );
	keyConvertMap.insert_or_assign( Qt::Key_F3, Noesis::Key_F3 );
	keyConvertMap.insert_or_assign( Qt::Key_F4, Noesis::Key_F4 );
	keyConvertMap.insert_or_assign( Qt::Key_F5, Noesis::Key_F5 );
	keyConvertMap.insert_or_assign( Qt::Key_F6, Noesis::Key_F6 );
	keyConvertMap.insert_or_assign( Qt::Key_F7, Noesis::Key_F7 );
	keyConvertMap.insert_or_assign( Qt::Key_F8, Noesis::Key_F8 );
	keyConvertMap.insert_or_assign( Qt::Key_F9, Noesis::Key_F9 );
	keyConvertMap.insert_or_assign( Qt::Key_F10, Noesis::Key_F10 );
	keyConvertMap.insert_or_assign( Qt::Key_F11, Noesis::Key_F11 );
	keyConvertMap.insert_or_assign( Qt::Key_F12, Noesis::Key_F12 );
	keyConvertMap.insert_or_assign( Qt::Key_F13, Noesis::Key_F13 );
	keyConvertMap.insert_or_assign( Qt::Key_F14, Noesis::Key_F14 );
	keyConvertMap.insert_or_assign( Qt::Key_F15, Noesis::Key_F15 );
	keyConvertMap.insert_or_assign( Qt::Key_F16, Noesis::Key_F16 );
	keyConvertMap.insert_or_assign( Qt::Key_F17, Noesis::Key_F17 );
	keyConvertMap.insert_or_assign( Qt::Key_F18, Noesis::Key_F18 );
	keyConvertMap.insert_or_assign( Qt::Key_F19, Noesis::Key_F19 );
	keyConvertMap.insert_or_assign( Qt::Key_F20, Noesis::Key_F20 );
	keyConvertMap.insert_or_assign( Qt::Key_F21, Noesis::Key_F21 );
	keyConvertMap.insert_or_assign( Qt::Key_F22, Noesis::Key_F22 );
	keyConvertMap.insert_or_assign( Qt::Key_F23, Noesis::Key_F23 );
	keyConvertMap.insert_or_assign( Qt::Key_F24, Noesis::Key_F24 );

	keyConvertMap.insert_or_assign( Qt::Key_NumLock, Noesis::Key_NumLock );
	keyConvertMap.insert_or_assign( Qt::Key_ScrollLock, Noesis::Key_Scroll );

	keyConvertMap.insert_or_assign( Qt::Key_Shift, Noesis::Key_LeftShift );
	//keyConvertMap.insert_or_assign( Qt::Key_, Noesis::Key_RightShift );
	keyConvertMap.insert_or_assign( Qt::Key_Control, Noesis::Key_LeftCtrl );
	//keyConvertMap.insert_or_assign( Qt::Key_, Noesis::Key_RightCtrl );
	keyConvertMap.insert_or_assign( Qt::Key_Alt, Noesis::Key_LeftAlt );
	keyConvertMap.insert_or_assign( Qt::Key_AltGr, Noesis::Key_RightAlt );

	//keyConvertMap.insert_or_assign( Qt::Key_, Noesis::Key_PageLeft );
	//keyConvertMap.insert_or_assign( Qt::Key_, Noesis::Key_PageRight );
}

Noesis::Key Global::keyConvert( Qt::Key key )
{
	if ( keyConvertMap.empty() )
	{
		initKeyConvert();
	}
	if ( keyConvertMap.contains( key ) )
	{
		return keyConvertMap.at( key );
	}
	return Noesis::Key_None;
}