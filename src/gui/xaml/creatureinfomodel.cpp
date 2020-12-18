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

void CreatureInfoModel::updateEmptySlotImages( const QMap< QString, std::vector<unsigned char> >& pics )
{
	m_bitmapHeadEmpty =  BitmapImage::Create( 32, 32, 96, 96, pics.value( "UIEmptySlotHead" ).data(), 128, BitmapSource::Format::Format_RGBA8 );
	m_bitmapChestEmpty = BitmapImage::Create( 32, 32, 96, 96, pics.value( "UIEmptySlotChest" ).data(), 128, BitmapSource::Format::Format_RGBA8 );
	m_bitmapArmsEmpty =  BitmapImage::Create( 32, 32, 96, 96, pics.value( "UIEmptySlotArms" ).data(), 128, BitmapSource::Format::Format_RGBA8 );
	m_bitmapHandsEmpty = BitmapImage::Create( 32, 32, 96, 96, pics.value( "UIEmptySlotHands" ).data(), 128, BitmapSource::Format::Format_RGBA8 );
	m_bitmapLegsEmpty =  BitmapImage::Create( 32, 32, 96, 96, pics.value( "UIEmptySlotLegs" ).data(), 128, BitmapSource::Format::Format_RGBA8 );
	m_bitmapFeetEmpty =  BitmapImage::Create( 32, 32, 96, 96, pics.value( "UIEmptySlotFeet" ).data(), 128, BitmapSource::Format::Format_RGBA8 );
	m_bitmapLHeldEmpty = BitmapImage::Create( 32, 32, 96, 96, pics.value( "UIEmptySlotShield" ).data(), 128, BitmapSource::Format::Format_RGBA8 );
	m_bitmapRHeldEmpty = BitmapImage::Create( 32, 32, 96, 96, pics.value( "UIEmptySlotWeapon" ).data(), 128, BitmapSource::Format::Format_RGBA8 );
	m_bitmapBackEmpty =  BitmapImage::Create( 32, 32, 96, 96, pics.value( "UIEmptySlotBack" ).data(), 128, BitmapSource::Format::Format_RGBA8 );
	m_bitmapNeckEmpty =  BitmapImage::Create( 32, 32, 96, 96, pics.value( "UIEmptySlotNeck" ).data(), 128, BitmapSource::Format::Format_RGBA8 );
	m_bitmapLRingEmpty = BitmapImage::Create( 32, 32, 96, 96, pics.value( "UIEmptySlotRing" ).data(), 128, BitmapSource::Format::Format_RGBA8 );
	m_bitmapRRingEmpty = BitmapImage::Create( 32, 32, 96, 96, pics.value( "UIEmptySlotRing" ).data(), 128, BitmapSource::Format::Format_RGBA8 );
	m_emptyPicsInitialized = true;
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
	if( !m_emptyPicsInitialized )
	{
		m_proxy->requestEmptySlotImages();
	}

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
		if( info.itemPics.contains( "ArmorHead" ) )
		{
			if( info.itemPics.value( "ArmorHead" ).size() == 8192 )
			{
				m_bitmapHead = BitmapImage::Create( 32, 32, 96, 96, info.itemPics.value( "ArmorHead" ).data(), 128, BitmapSource::Format::Format_RGBA8 );
			}
		}
	}
	if( info.equipment.chest.itemID )
	{
		if( info.itemPics.contains( "ArmorChest" ) )
		{
			if( info.itemPics.value( "ArmorChest" ).size() == 8192 )
			{
				m_bitmapChest = BitmapImage::Create( 32, 32, 96, 96, info.itemPics.value( "ArmorChest" ).data(), 128, BitmapSource::Format::Format_RGBA8 );
			}
		}
	}
	if( info.equipment.arm.itemID )
	{
		if( info.itemPics.contains( "ArmorArms" ) )
		{
			if( info.itemPics.value( "ArmorArms" ).size() == 8192 )
			{
				m_bitmapArms = BitmapImage::Create( 32, 32, 96, 96, info.itemPics.value( "ArmorArms" ).data(), 128, BitmapSource::Format::Format_RGBA8 );
			}
		}
	}
	if( info.equipment.hand.itemID )
	{
		if( info.itemPics.contains( "ArmorHands" ) )
		{
			if( info.itemPics.value( "ArmorHands" ).size() == 8192 )
			{
				m_bitmapHands = BitmapImage::Create( 32, 32, 96, 96, info.itemPics.value( "ArmorHands" ).data(), 128, BitmapSource::Format::Format_RGBA8 );
			}
		}
	}
	if( info.equipment.leg.itemID )
	{
		if( info.itemPics.contains( "ArmorLegs" ) )
		{
			if( info.itemPics.value( "ArmorLegs" ).size() == 8192 )
			{
				m_bitmapLegs = BitmapImage::Create( 32, 32, 96, 96, info.itemPics.value( "ArmorLegs" ).data(), 128, BitmapSource::Format::Format_RGBA8 );
			}
		}
	}
	if( info.equipment.foot.itemID )
	{
		if( info.itemPics.contains( "ArmorFeet" ) )
		{
			if( info.itemPics.value( "ArmorFeet" ).size() == 8192 )
			{
				m_bitmapFeet = BitmapImage::Create( 32, 32, 96, 96, info.itemPics.value( "ArmorFeet" ).data(), 128, BitmapSource::Format::Format_RGBA8 );
			}
		}
	}
	if( info.equipment.leftHandHeld.itemID )
	{
		if( info.itemPics.contains( "LeftHandHeld" ) )
		{
			if( info.itemPics.value( "LeftHandHeld" ).size() == 8192 )
			{
				m_bitmapLHeld = BitmapImage::Create( 32, 32, 96, 96, info.itemPics.value( "LeftHandHeld" ).data(), 128, BitmapSource::Format::Format_RGBA8 );
			}
		}
	}
	if( info.equipment.rightHandHeld.itemID )
	{
		if( info.itemPics.contains( "RightHandHeld" ) )
		{
			if( info.itemPics.value( "RightHandHeld" ).size() == 8192 )
			{
				m_bitmapRHeld = BitmapImage::Create( 32, 32, 96, 96, info.itemPics.value( "RightHandHeld" ).data(), 128, BitmapSource::Format::Format_RGBA8 );
			}
		}
	}
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
	NsProp( "ImgRRing" ,    &CreatureInfoModel::getBitmapSourceRRing );
}
