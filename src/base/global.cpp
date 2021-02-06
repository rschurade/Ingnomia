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

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>

EventConnector* Global::eventConnector = nullptr;
Util* Global::util = nullptr;
Selection* Global::sel = nullptr;
NewGameSettings* Global::newGameSettings = nullptr;
Config* Global::cfg = nullptr;

Logger Global::m_logger;

bool Global::wallsLowered = false;
bool Global::showAxles    = false;
bool Global::showDesignations = true;
bool Global::showJobs = true;

unsigned int Global::waterSpriteUID  = 0;
unsigned int Global::undiscoveredUID = 0;

QVariantMap Global::copiedStockpileSettings;

QMap<QString, QVariantMap> Global::m_windowParams;

QMap<QString, QSet<QString>> Global::allowedInContainer;

QHash<Qt::Key, Noesis::Key> Global::keyConvertMap;

int Global::dimX     = 100;
int Global::dimY     = 100;
int Global::dimZ     = 100;

int Global::zWeight = 20;

double Global::xpMod = 250.;

bool Global::debugMode = false;
bool Global::debugOpenGL = false;
bool Global::debugSound = false;

QMap<QString, QDomElement> Global::m_behaviorTrees;

QStringList Global::needIDs;
QMap<QString, float> Global::needDecays;

unsigned int Global::dirtUID = 0;

QMap<QString, CreaturePart> Global::creaturePartLookUp;
QMap<CreaturePart, QString> Global::creaturePartToString;

QSet<QString> Global::craftable;

void Global::reset()
{
	qDebug() << "*** Global reset";

	GameState::stockOverlay.clear();
	GameState::squads.clear();

	Global::xpMod = Global::cfg->get( "XpMod" ).toDouble();

	m_logger.reset();

	wallsLowered = false;
	showAxles    = false;

	//m_keyBindings.update();

	if ( !loadBehaviorTrees() )
	{
		qCritical() << "failed to load behavior trees";
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
			needDecays.insert( need, row.value( "DecayPerMinute" ).toFloat() );
		}
	}

	Global::dirtUID = DBH::materialUID( "Dirt" );

	Global::cfg->set( "renderCreatures", true );

	creaturePartLookUp.insert( "Head", CP_HEAD );
	creaturePartLookUp.insert( "Torso", CP_TORSO );
	creaturePartLookUp.insert( "LeftArm", CP_LEFT_ARM );
	creaturePartLookUp.insert( "RightArm", CP_RIGHT_ARM );
	creaturePartLookUp.insert( "LeftHand", CP_LEFT_HAND );
	creaturePartLookUp.insert( "RightHand", CP_RIGHT_HAND );
	creaturePartLookUp.insert( "LeftLeg", CP_LEFT_LEG );
	creaturePartLookUp.insert( "RightLeg", CP_RIGHT_LEG );
	creaturePartLookUp.insert( "LeftFoot", CP_LEFT_FOOT );
	creaturePartLookUp.insert( "RightFoot", CP_RIGHT_FOOT );

	creaturePartLookUp.insert( "LeftFrontLeg", CP_LEFT_FRONT_LEG );
	creaturePartLookUp.insert( "LegFrontLeg", CP_RIGHT_FRONT_LEG );
	creaturePartLookUp.insert( "LeftFrontFoot", CP_LEFT_FRONT_FOOT );
	creaturePartLookUp.insert( "RightFrontFoot", CP_RIGHT_FRONT_FOOT );

	creaturePartLookUp.insert( "LeftWing", CP_RIGHT_WING );
	creaturePartLookUp.insert( "RightWing", CP_RIGHT_WING );

	creaturePartLookUp.insert( "Brain", CP_BRAIN );
	creaturePartLookUp.insert( "LeftEye", CP_LEFT_EYE );
	creaturePartLookUp.insert( "RightEye", CP_RIGHT_EYE );
	creaturePartLookUp.insert( "Heart", CP_HEART );
	creaturePartLookUp.insert( "LeftLung", CP_LEFT_LUNG );
	creaturePartLookUp.insert( "RightLung", CP_RIGHT_LUNG );

	creaturePartLookUp.insert( "Hair", CP_HAIR );
	creaturePartLookUp.insert( "Facial", CP_FACIAL_HAIR );
	creaturePartLookUp.insert( "Clothing", CP_CLOTHING );
	creaturePartLookUp.insert( "Boots", CP_BOOTS );
	creaturePartLookUp.insert( "Hat", CP_HAT );
	creaturePartLookUp.insert( "Back", CP_BACK );

	creaturePartLookUp.insert( "HeadArmor", CP_ARMOR_HEAD );
	creaturePartLookUp.insert( "ChestArmor", CP_ARMOR_TORSO );
	creaturePartLookUp.insert( "LeftArmArmor", CP_ARMOR_LEFT_ARM );
	creaturePartLookUp.insert( "RightArmArmor", CP_ARMOR_RIGHT_ARM );
	creaturePartLookUp.insert( "LeftHandArmor", CP_ARMOR_LEFT_HAND );
	creaturePartLookUp.insert( "RightHandArmor", CP_ARMOR_RIGHT_HAND );
	creaturePartLookUp.insert( "LeftFootArmor", CP_ARMOR_LEFT_FOOT );
	creaturePartLookUp.insert( "RightFootArmor", CP_ARMOR_RIGHT_FOOT );
	creaturePartLookUp.insert( "LeftHandHeld", CP_LEFT_HAND_HELD );
	creaturePartLookUp.insert( "RightHandHeld", CP_RIGHT_HAND_HELD );

	creaturePartLookUp.insert( "ArmArmor", CP_ARMOR_ARM );
	creaturePartLookUp.insert( "HandArmor", CP_ARMOR_HAND );
	creaturePartLookUp.insert( "FootArmor", CP_ARMOR_FOOT );
	creaturePartLookUp.insert( "LegArmor", CP_ARMOR_LEG );

	creaturePartToString.insert( CP_HEAD, "Head" );
	creaturePartToString.insert( CP_TORSO, "Torso" );
	creaturePartToString.insert( CP_LEFT_ARM, "LeftArm" );
	creaturePartToString.insert( CP_RIGHT_ARM, "RightArm" );
	creaturePartToString.insert( CP_LEFT_HAND, "LeftHand" );
	creaturePartToString.insert( CP_RIGHT_HAND, "RightHand" );
	creaturePartToString.insert( CP_LEFT_LEG, "LeftLeg" );
	creaturePartToString.insert( CP_RIGHT_LEG, "LegLeg" );
	creaturePartToString.insert( CP_LEFT_FOOT, "LeftFoot" );
	creaturePartToString.insert( CP_RIGHT_FOOT, "RightFoot" );

	creaturePartToString.insert( CP_LEFT_FRONT_LEG, "LeftFrontLeg" );
	creaturePartToString.insert( CP_RIGHT_FRONT_LEG, "LegFrontLeg" );
	creaturePartToString.insert( CP_LEFT_FRONT_FOOT, "LeftFrontFoot" );
	creaturePartToString.insert( CP_RIGHT_FRONT_FOOT, "RightFrontFoot" );

	creaturePartToString.insert( CP_LEFT_WING, "LeftWing" );
	creaturePartToString.insert( CP_RIGHT_WING, "RightWing" );

	creaturePartToString.insert( CP_BRAIN, "Brain" );
	creaturePartToString.insert( CP_LEFT_EYE, "LeftEye" );
	creaturePartToString.insert( CP_RIGHT_EYE, "RightEye" );
	creaturePartToString.insert( CP_HEART, "Heart" );
	creaturePartToString.insert( CP_LEFT_LUNG, "LeftLung" );
	creaturePartToString.insert( CP_RIGHT_LUNG, "RightLung" );

	creaturePartToString.insert( CP_HAIR, "Hair" );
	creaturePartToString.insert( CP_FACIAL_HAIR, "Facial" );
	creaturePartToString.insert( CP_CLOTHING, "Clothing" );
	creaturePartToString.insert( CP_BOOTS, "Boots" );
	creaturePartToString.insert( CP_HAT, "Hat" );
	creaturePartToString.insert( CP_BACK, "Back" );

	creaturePartToString.insert( CP_ARMOR_HEAD, "HeadArmor" );
	creaturePartToString.insert( CP_ARMOR_TORSO, "ChestArmor" );
	creaturePartToString.insert( CP_ARMOR_LEFT_ARM, "LeftArmArmor" );
	creaturePartToString.insert( CP_ARMOR_RIGHT_ARM, "RightArmArmor" );
	creaturePartToString.insert( CP_ARMOR_LEFT_HAND, "LeftHandArmor" );
	creaturePartToString.insert( CP_ARMOR_RIGHT_HAND, "RightHandArmor" );
	creaturePartToString.insert( CP_ARMOR_LEFT_FOOT, "LeftFootArmor" );
	creaturePartToString.insert( CP_ARMOR_RIGHT_FOOT, "RightFootArmor" );
	creaturePartToString.insert( CP_LEFT_HAND_HELD, "LeftHandHeld" );
	creaturePartToString.insert( CP_RIGHT_HAND_HELD, "RightHandHeld" );

	creaturePartToString.insert( CP_ARMOR_ARM, "ArmArmor" );
	creaturePartToString.insert( CP_ARMOR_HAND, "HandArmor" );
	creaturePartToString.insert( CP_ARMOR_FOOT, "FootArmor" );
	creaturePartToString.insert( CP_ARMOR_LEG, "LegArmor" );

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

	for ( auto id : DB::ids( "AI" ) )
	{
		QString xmlName = DB::select( "BehaviorTree", "AI", id ).toString();

		QDomDocument xml;
		// Load xml file as raw data
		QFile f( Global::cfg->get( "dataPath" ).toString() + "/ai/" + xmlName );
		if ( !f.open( QIODevice::ReadOnly ) )
		{
			// Error while loading file
			qDebug() << "Error while loading file xmlName";
			return false;
		}
		// Set data into the QDomDocument before processing
		xml.setContent( &f );
		f.close();

		QDomElement root = xml.documentElement();
		m_behaviorTrees.insert( id, root );
	}
	return true;
}

QDomElement Global::behaviorTree( QString id )
{
	return m_behaviorTrees.value( id );
}

bool Global::addBehaviorTree( QString id, QString path )
{
	QDomDocument xml;
	// Load xml file as raw data
	QFile f( path );
	if ( !f.open( QIODevice::ReadOnly ) )
	{
		// Error while loading file
		qDebug() << "Error while loading file xmlName";
		return false;
	}
	// Set data into the QDomDocument before processing
	xml.setContent( &f );
	f.close();

	QDomElement root = xml.documentElement();
	m_behaviorTrees.insert( id, root );

	return true;
}

void Global::initKeyConvert()
{
	keyConvertMap.clear();

	//keyConvertMap.insert( Qt::Key_, Noesis::Key_None );

	keyConvertMap.insert( Qt::Key_Cancel, Noesis::Key_Cancel );
	keyConvertMap.insert( Qt::Key_Backspace, Noesis::Key_Back );
	keyConvertMap.insert( Qt::Key_Tab, Noesis::Key_Tab );
	//keyConvertMap.insert( Qt::Key_, Noesis::Key_LineFeed );
	keyConvertMap.insert( Qt::Key_Clear, Noesis::Key_Clear );
	keyConvertMap.insert( Qt::Key_Return, Noesis::Key_Return );
	keyConvertMap.insert( Qt::Key_Enter, Noesis::Key_Enter );
	keyConvertMap.insert( Qt::Key_Pause, Noesis::Key_Pause );
	//keyConvertMap.insert( Qt::Key_, Noesis::Key_Capital );
	keyConvertMap.insert( Qt::Key_CapsLock, Noesis::Key_CapsLock );

	keyConvertMap.insert( Qt::Key_Escape, Noesis::Key_Escape );

	//keyConvertMap.insert( Qt::Key_, Noesis::Key_ImeConvert );
	//keyConvertMap.insert( Qt::Key_, Noesis::Key_ImeNonConvert );
	//keyConvertMap.insert( Qt::Key_, Noesis::Key_ImeAccept );
	//keyConvertMap.insert( Qt::Key_, Noesis::Key_ImeModeChange );

	keyConvertMap.insert( Qt::Key_Space, Noesis::Key_Space );

	//keyConvertMap.insert( Qt::Key_, Noesis::Key_Prior );
	keyConvertMap.insert( Qt::Key_PageUp, Noesis::Key_PageUp );
	//keyConvertMap.insert( Qt::Key_, Noesis::Key_Next );
	keyConvertMap.insert( Qt::Key_PageDown, Noesis::Key_PageDown );
	keyConvertMap.insert( Qt::Key_End, Noesis::Key_End );
	keyConvertMap.insert( Qt::Key_Home, Noesis::Key_Home );

	keyConvertMap.insert( Qt::Key_Left, Noesis::Key_Left );
	keyConvertMap.insert( Qt::Key_Up, Noesis::Key_Up );
	keyConvertMap.insert( Qt::Key_Right, Noesis::Key_Right );
	keyConvertMap.insert( Qt::Key_Down, Noesis::Key_Down );

	keyConvertMap.insert( Qt::Key_Select, Noesis::Key_Select );
	keyConvertMap.insert( Qt::Key_Print, Noesis::Key_Print );
	keyConvertMap.insert( Qt::Key_Execute, Noesis::Key_Execute );
	//keyConvertMap.insert( Qt::Key_, Noesis::Key_Snapshot );
	//keyConvertMap.insert( Qt::Key_, Noesis::Key_PrintScreen );
	keyConvertMap.insert( Qt::Key_Insert, Noesis::Key_Insert );
	keyConvertMap.insert( Qt::Key_Delete, Noesis::Key_Delete );
	keyConvertMap.insert( Qt::Key_Help, Noesis::Key_Help );

	keyConvertMap.insert( Qt::Key_0, Noesis::Key_D0 );
	keyConvertMap.insert( Qt::Key_1, Noesis::Key_D1 );
	keyConvertMap.insert( Qt::Key_2, Noesis::Key_D2 );
	keyConvertMap.insert( Qt::Key_3, Noesis::Key_D3 );
	keyConvertMap.insert( Qt::Key_4, Noesis::Key_D4 );
	keyConvertMap.insert( Qt::Key_5, Noesis::Key_D5 );
	keyConvertMap.insert( Qt::Key_6, Noesis::Key_D6 );
	keyConvertMap.insert( Qt::Key_7, Noesis::Key_D7 );
	keyConvertMap.insert( Qt::Key_8, Noesis::Key_D8 );
	keyConvertMap.insert( Qt::Key_9, Noesis::Key_D9 );

	keyConvertMap.insert( Qt::Key_A, Noesis::Key_A );
	keyConvertMap.insert( Qt::Key_B, Noesis::Key_B );
	keyConvertMap.insert( Qt::Key_C, Noesis::Key_C );
	keyConvertMap.insert( Qt::Key_D, Noesis::Key_D );
	keyConvertMap.insert( Qt::Key_E, Noesis::Key_E );
	keyConvertMap.insert( Qt::Key_F, Noesis::Key_F );
	keyConvertMap.insert( Qt::Key_G, Noesis::Key_G );
	keyConvertMap.insert( Qt::Key_H, Noesis::Key_H );
	keyConvertMap.insert( Qt::Key_I, Noesis::Key_I );
	keyConvertMap.insert( Qt::Key_J, Noesis::Key_J );
	keyConvertMap.insert( Qt::Key_K, Noesis::Key_K );
	keyConvertMap.insert( Qt::Key_L, Noesis::Key_L );
	keyConvertMap.insert( Qt::Key_M, Noesis::Key_M );
	keyConvertMap.insert( Qt::Key_N, Noesis::Key_N );
	keyConvertMap.insert( Qt::Key_O, Noesis::Key_O );
	keyConvertMap.insert( Qt::Key_P, Noesis::Key_P );
	keyConvertMap.insert( Qt::Key_Q, Noesis::Key_Q );
	keyConvertMap.insert( Qt::Key_R, Noesis::Key_R );
	keyConvertMap.insert( Qt::Key_S, Noesis::Key_S );
	keyConvertMap.insert( Qt::Key_T, Noesis::Key_T );
	keyConvertMap.insert( Qt::Key_U, Noesis::Key_U );
	keyConvertMap.insert( Qt::Key_V, Noesis::Key_V );
	keyConvertMap.insert( Qt::Key_W, Noesis::Key_W );
	keyConvertMap.insert( Qt::Key_X, Noesis::Key_X );
	keyConvertMap.insert( Qt::Key_Y, Noesis::Key_Y );
	keyConvertMap.insert( Qt::Key_Z, Noesis::Key_Z );

	keyConvertMap.insert( Qt::Key_ApplicationLeft, Noesis::Key_LWin );
	keyConvertMap.insert( Qt::Key_ApplicationRight, Noesis::Key_RWin );
	//keyConvertMap.insert( Qt::Key_, Noesis::Key_Apps );
	keyConvertMap.insert( Qt::Key_Sleep, Noesis::Key_Sleep );
	/*
	keyConvertMap.insert( Qt::Key_0, Noesis::Key_NumPad0 );
	keyConvertMap.insert( Qt::Key_1, Noesis::Key_NumPad1 );
	keyConvertMap.insert( Qt::Key_2, Noesis::Key_NumPad2 );
	keyConvertMap.insert( Qt::Key_3, Noesis::Key_NumPad3 );
	keyConvertMap.insert( Qt::Key_4, Noesis::Key_NumPad4 );
	keyConvertMap.insert( Qt::Key_5, Noesis::Key_NumPad5 );
	keyConvertMap.insert( Qt::Key_6, Noesis::Key_NumPad6 );
	keyConvertMap.insert( Qt::Key_7, Noesis::Key_NumPad7 );
	keyConvertMap.insert( Qt::Key_8, Noesis::Key_NumPad8 );
	keyConvertMap.insert( Qt::Key_9, Noesis::Key_NumPad9 );
	*/
	keyConvertMap.insert( Qt::Key_multiply, Noesis::Key_Multiply );
	keyConvertMap.insert( Qt::Key_Plus, Noesis::Key_Add );
	//keyConvertMap.insert( Qt::Key_, Noesis::Key_Separator );
	keyConvertMap.insert( Qt::Key_Minus, Noesis::Key_Subtract );
	keyConvertMap.insert( Qt::Key_Colon, Noesis::Key_Decimal );
	keyConvertMap.insert( Qt::Key_division, Noesis::Key_Divide );

	keyConvertMap.insert( Qt::Key_F1, Noesis::Key_F1 );
	keyConvertMap.insert( Qt::Key_F2, Noesis::Key_F2 );
	keyConvertMap.insert( Qt::Key_F3, Noesis::Key_F3 );
	keyConvertMap.insert( Qt::Key_F4, Noesis::Key_F4 );
	keyConvertMap.insert( Qt::Key_F5, Noesis::Key_F5 );
	keyConvertMap.insert( Qt::Key_F6, Noesis::Key_F6 );
	keyConvertMap.insert( Qt::Key_F7, Noesis::Key_F7 );
	keyConvertMap.insert( Qt::Key_F8, Noesis::Key_F8 );
	keyConvertMap.insert( Qt::Key_F9, Noesis::Key_F9 );
	keyConvertMap.insert( Qt::Key_F10, Noesis::Key_F10 );
	keyConvertMap.insert( Qt::Key_F11, Noesis::Key_F11 );
	keyConvertMap.insert( Qt::Key_F12, Noesis::Key_F12 );
	keyConvertMap.insert( Qt::Key_F13, Noesis::Key_F13 );
	keyConvertMap.insert( Qt::Key_F14, Noesis::Key_F14 );
	keyConvertMap.insert( Qt::Key_F15, Noesis::Key_F15 );
	keyConvertMap.insert( Qt::Key_F16, Noesis::Key_F16 );
	keyConvertMap.insert( Qt::Key_F17, Noesis::Key_F17 );
	keyConvertMap.insert( Qt::Key_F18, Noesis::Key_F18 );
	keyConvertMap.insert( Qt::Key_F19, Noesis::Key_F19 );
	keyConvertMap.insert( Qt::Key_F20, Noesis::Key_F20 );
	keyConvertMap.insert( Qt::Key_F21, Noesis::Key_F21 );
	keyConvertMap.insert( Qt::Key_F22, Noesis::Key_F22 );
	keyConvertMap.insert( Qt::Key_F23, Noesis::Key_F23 );
	keyConvertMap.insert( Qt::Key_F24, Noesis::Key_F24 );

	keyConvertMap.insert( Qt::Key_NumLock, Noesis::Key_NumLock );
	keyConvertMap.insert( Qt::Key_ScrollLock, Noesis::Key_Scroll );

	keyConvertMap.insert( Qt::Key_Shift, Noesis::Key_LeftShift );
	//keyConvertMap.insert( Qt::Key_, Noesis::Key_RightShift );
	keyConvertMap.insert( Qt::Key_Control, Noesis::Key_LeftCtrl );
	//keyConvertMap.insert( Qt::Key_, Noesis::Key_RightCtrl );
	keyConvertMap.insert( Qt::Key_Alt, Noesis::Key_LeftAlt );
	keyConvertMap.insert( Qt::Key_AltGr, Noesis::Key_RightAlt );

	//keyConvertMap.insert( Qt::Key_, Noesis::Key_PageLeft );
	//keyConvertMap.insert( Qt::Key_, Noesis::Key_PageRight );
}

Noesis::Key Global::keyConvert( Qt::Key key )
{
	if ( keyConvertMap.isEmpty() )
	{
		initKeyConvert();
	}
	if ( keyConvertMap.contains( key ) )
	{
		return keyConvertMap.value( key );
	}
	return Noesis::Key_None;
}