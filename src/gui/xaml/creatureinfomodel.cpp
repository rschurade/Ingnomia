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
#include "creatureinfomodel.h"

#include "creatureinfoproxy.h"

#include "../../base/util.h"
#include "../../game/militarymanager.h"
#include "../../gfx/spritefactory.h"

#include <NsApp/Application.h>
#include <NsCore/Log.h>
#include <NsCore/ReflectionImplement.h>
#include <NsCore/ReflectionImplementEnum.h>
#include <NsGui/ObservableCollection.h>
#include <NsGui/UIElement.h>
#include <NsGui/BitmapImage.h>

#include <QDebug>

using namespace IngnomiaGUI;
using namespace Noesis;
using namespace NoesisApp;






////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
CreatureInfoModel::CreatureInfoModel()
{
	m_proxy = new CreatureInfoProxy;
	m_proxy->setParent( this );

	m_professions = *new ObservableCollection<ProfItem>();
	m_proxy->requestProfessionList();
}

void CreatureInfoModel::updateProfessionList( const QStringList& professions )
{
	m_professions->Clear();
	for( const auto& prof : professions )
	{
		m_professions->Add( MakePtr<ProfItem>( prof.toStdString().c_str() ) );
	}
	OnPropertyChanged( "Professions" );
}

void CreatureInfoModel::updateInfo( const GuiCreatureInfo& info )
{
	m_name = info.name.toStdString().c_str();
	m_id = info.id;

	m_str = std::to_string( info.str ).c_str();
	m_con = std::to_string( info.con ).c_str();
	m_dex = std::to_string( info.dex ).c_str();
	m_int = std::to_string( info.intel ).c_str();
	m_wis = std::to_string( info.wis ).c_str();
	m_cha = std::to_string( info.cha ).c_str();

	m_hunger = std::to_string( info.hunger ).c_str();
	m_thirst = std::to_string( info.thirst ).c_str();
	m_sleep = std::to_string( info.sleep ).c_str();
	m_happiness = std::to_string( info.happiness ).c_str();

	for( int i = 0; i < m_professions->Count(); ++i )
	{
		if( m_professions->Get( i )->GetName() == info.profession )
		{
			SetProfession( m_professions->Get( i ) );
			break;
		}
	}

	m_activity = info.activity.toStdString().c_str();

	OnPropertyChanged( "Name" );
	OnPropertyChanged( "Str" );
	OnPropertyChanged( "Con" );
	OnPropertyChanged( "Dex" );
	OnPropertyChanged( "Int" );
	OnPropertyChanged( "Wis" );
	OnPropertyChanged( "Cha" );
	OnPropertyChanged( "Hunger" );
	OnPropertyChanged( "Thirst" );
	OnPropertyChanged( "Sleep" );
	OnPropertyChanged( "Happiness" );
	OnPropertyChanged( "Activity" );

	if( ! m_emptySlotsInitialized )
	{
		m_bitmapHeadEmpty = createEmptyUniformImg( "UIEmptySlotHead" );
		m_bitmapChestEmpty = createEmptyUniformImg( "UIEmptySlotChest" );
		m_bitmapArmsEmpty = createEmptyUniformImg( "UIEmptySlotArms" );
		m_bitmapHandsEmpty = createEmptyUniformImg( "UIEmptySlotHands" );
		m_bitmapLegsEmpty = createEmptyUniformImg( "UIEmptySlotLegs" );
		m_bitmapFeetEmpty = createEmptyUniformImg( "UIEmptySlotFeet" );
		m_bitmapLHeldEmpty = createEmptyUniformImg( "UIEmptySlotShield" );
		m_bitmapRHeldEmpty = createEmptyUniformImg( "UIEmptySlotWeapon" );
		//m_bitmapBackEmpty = createEmptyUniformImg( "UIEmptySlot" );
		m_bitmapNeckEmpty = createEmptyUniformImg( "UIEmptySlotNeck" );
		m_bitmapLRingEmpty = createEmptyUniformImg( "UIEmptySlotRing" );
		m_bitmapRRingEmpty = createEmptyUniformImg( "UIEmptySlotRing" );
		m_emptySlotsInitialized = true;
	}

	m_bitmapHead  = m_bitmapHeadEmpty;
	m_bitmapChest = m_bitmapChestEmpty;
	m_bitmapArms  = m_bitmapArmsEmpty;
	m_bitmapHands = m_bitmapHandsEmpty;
	m_bitmapLegs  = m_bitmapLegsEmpty;
	m_bitmapFeet  = m_bitmapFeetEmpty;
	m_bitmapLHeld = m_bitmapLHeldEmpty;
	m_bitmapRHeld = m_bitmapRHeldEmpty;
	m_bitmapBack  = m_bitmapBackEmpty;
	m_bitmapNeck  = m_bitmapNeckEmpty;
	m_bitmapLRing =	m_bitmapLRingEmpty;
	m_bitmapRRing =	m_bitmapRRingEmpty;

	if( info.equipment.head.itemID )
	{
		m_bitmapHead = createUniformImg( "ArmorHead", info.uniform.parts.value( "HeadArmor" ), info.equipment.head );
	}
	if( info.equipment.chest.itemID )
	{
		m_bitmapChest = createUniformImg( "ArmorChest", info.uniform.parts.value( "ChestArmor" ), info.equipment.chest );
	}
	if( info.equipment.arm.itemID )
	{
		m_bitmapArms = createUniformImg( "ArmorArms", info.uniform.parts.value( "ArmArmor" ), info.equipment.arm );
	}
	if( info.equipment.hand.itemID )
	{
		m_bitmapHands = createUniformImg( "ArmorHands", info.uniform.parts.value( "HandArmor" ), info.equipment.hand );
	}
	if( info.equipment.leg.itemID )
	{
		m_bitmapLegs = createUniformImg( "ArmorLegs", info.uniform.parts.value( "LegArmor" ), info.equipment.leg );
	}
	if( info.equipment.foot.itemID )
	{
		m_bitmapFeet = createUniformImg( "ArmorFeet", info.uniform.parts.value( "FootArmor" ), info.equipment.foot );
	}
	//m_bitmapLHeld = createUniformImg( "LeftHandHeld", info.uniform->parts.value( "LeftHandHeld" ) );
	//m_bitmapRHeld = createUniformImg( "RightHandHeld", info.uniform->parts.value( "RightHandHeld" ) );
	//m_bitmapBack = createUniformImg( "Back", info.uniform );
	//m_bitmapNeck = createUniformImg( "", info.uniform );
	//m_bitmapLRing = createUniformImg( "", info.uniform );
	//m_bitmapRRing = createUniformImg( "", info.uniform );
	
	OnPropertyChanged( "ImgHead" );
	OnPropertyChanged( "ImgChest" );
	OnPropertyChanged( "ImgArms" );
	OnPropertyChanged( "ImgHands" );
	OnPropertyChanged( "ImgLegs" );
	OnPropertyChanged( "ImgFeet" );
	OnPropertyChanged( "ImgLeftHand" );
	OnPropertyChanged( "ImgRightHand" );
	OnPropertyChanged( "ImgBack" );
	OnPropertyChanged( "ImgNeck" );
	OnPropertyChanged( "ImgLRing" );
	OnPropertyChanged( "ImgRRing" );
}

Noesis::Ptr<Noesis::BitmapSource> CreatureInfoModel::createUniformImg( QString slot, const UniformItem& uItem, const EquipmentItem& eItem )
{
	if( uItem.item.isEmpty() || eItem.itemID == 0 )
	{
		return nullptr;
	}
	QStringList mats;
	mats.append( eItem.material );

	auto sprite = Global::sf().createSprite( "UI" + uItem.type + slot, mats );
	if( sprite )
	{
		QPixmap pm = sprite->pixmap( "Spring", 0, 0 );

		std::vector<unsigned char> buffer;

		Util::createBufferForNoesisImage( pm, buffer );

		return BitmapImage::Create( pm.width(), pm.height(), 96, 96, buffer.data(), pm.width() * 4, BitmapSource::Format::Format_RGBA8 );
	}
	return nullptr;
}

Noesis::Ptr<Noesis::BitmapSource> CreatureInfoModel::createEmptyUniformImg( QString spriteID )
{
	QStringList mats; 
	mats.append( "any" );
	
	auto sprite = Global::sf().createSprite( spriteID, mats );
	if( sprite )
	{
		QPixmap pm = sprite->pixmap( "Spring", 0, 0 );

		std::vector<unsigned char> buffer;

		Util::createBufferForNoesisImage( pm, buffer );

		return BitmapImage::Create( pm.width(), pm.height(), 96, 96, buffer.data(), pm.width() * 4, BitmapSource::Format::Format_RGBA8 );
	}
	return nullptr;
}

const char* CreatureInfoModel::GetName() const { return m_name.Str(); }
const char* CreatureInfoModel::GetStr() const { return m_str.Str(); }
const char* CreatureInfoModel::GetCon() const { return m_con.Str(); }
const char* CreatureInfoModel::GetDex() const { return m_dex.Str(); }
const char* CreatureInfoModel::GetInt() const { return m_int.Str(); }
const char* CreatureInfoModel::GetWis() const { return m_wis.Str(); }
const char* CreatureInfoModel::GetCha() const { return m_cha.Str(); }
const char* CreatureInfoModel::GetHunger() const { return m_hunger.Str(); }
const char* CreatureInfoModel::GetThirst() const { return m_thirst.Str(); }
const char* CreatureInfoModel::GetSleep() const { return m_sleep.Str(); }
const char* CreatureInfoModel::GetHappiness() const { return m_happiness.Str(); }
const char* CreatureInfoModel::GetActivity() const { return m_activity.Str(); }


Noesis::ObservableCollection<ProfItem>* CreatureInfoModel::GetProfessions() const
{
	return m_professions;
}
	
void CreatureInfoModel::SetProfession( ProfItem* item )
{
	if ( item && m_selectedProfession != item )
	{
		m_selectedProfession = item;
		m_proxy->setProfession( m_id, item->GetName() );

		OnPropertyChanged( "Profession" );
	}
}

ProfItem* CreatureInfoModel::GetProfession() const
{
	return m_selectedProfession;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION


NS_IMPLEMENT_REFLECTION( CreatureInfoModel, "IngnomiaGUI.CreatureInfoModel" )
{
	NsProp( "Name", &CreatureInfoModel::GetName );
	NsProp( "Professions", &CreatureInfoModel::GetProfessions );
	NsProp( "Profession", &CreatureInfoModel::GetProfession, &CreatureInfoModel::SetProfession );

	NsProp( "Str", &CreatureInfoModel::GetStr );
	NsProp( "Con", &CreatureInfoModel::GetCon );
	NsProp( "Dex", &CreatureInfoModel::GetDex );
	NsProp( "Int", &CreatureInfoModel::GetInt );
	NsProp( "Wis", &CreatureInfoModel::GetWis );
	NsProp( "Cha", &CreatureInfoModel::GetCha );
	NsProp( "Hunger", &CreatureInfoModel::GetHunger );
	NsProp( "Thirst", &CreatureInfoModel::GetThirst );
	NsProp( "Sleep", &CreatureInfoModel::GetSleep );
	NsProp( "Happiness", &CreatureInfoModel::GetHappiness );

	NsProp( "Activity", &CreatureInfoModel::GetActivity );

	NsProp( "ImgHead",      &CreatureInfoModel::getBitmapSourceHead );
	NsProp( "ImgChest",     &CreatureInfoModel::getBitmapSourceChest );
	NsProp( "ImgArms" ,     &CreatureInfoModel::getBitmapSourceArms );
	NsProp( "ImgHands",     &CreatureInfoModel::getBitmapSourceHands );
	NsProp( "ImgLegs" ,     &CreatureInfoModel::getBitmapSourceLegs );
	NsProp( "ImgFeet" ,     &CreatureInfoModel::getBitmapSourceFeet );
	NsProp( "ImgLeftHand",  &CreatureInfoModel::getBitmapSourceLHeld );
	NsProp( "ImgRightHand", &CreatureInfoModel::getBitmapSourceRHeld );
	NsProp( "ImgBack" ,     &CreatureInfoModel::getBitmapSourceBack );
	NsProp( "ImgNeck" ,     &CreatureInfoModel::getBitmapSourceNeck );
	NsProp( "ImgLRing",     &CreatureInfoModel::getBitmapSourceLRing );
	NsProp( "ImgRRing" ,     &CreatureInfoModel::getBitmapSourceRRing );
}
