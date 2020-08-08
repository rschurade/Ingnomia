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

#include "CreatureInfoproxy.h"

#include "../../base/util.h"
#include "../../game/militarymanager.h"

#include <NsApp/Application.h>
#include <NsCore/Log.h>
#include <NsCore/ReflectionImplement.h>
#include <NsCore/ReflectionImplementEnum.h>
#include <NsGui/ObservableCollection.h>
#include <NsGui/UIElement.h>

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

	m_bitmapHead = nullptr;
	m_bitmapChest = nullptr;
	m_bitmapArms =  nullptr;
	m_bitmapHands = nullptr;
	m_bitmapLegs = nullptr; 
	m_bitmapFeet = nullptr; 
	m_bitmapLHeld = nullptr;
	m_bitmapRHeld = nullptr;
	m_bitmapBack = nullptr; 

	if( info.uniform )
	{
		m_bitmapHead = createUniformImg( "HeadArmor", info.uniform );
		m_bitmapChest = createUniformImg( "ChestArmor", info.uniform );
		m_bitmapArms = createUniformImg( "ArmArmor", info.uniform );
		m_bitmapHands = createUniformImg( "HandArmor", info.uniform );
		m_bitmapLegs = createUniformImg( "LegArmor", info.uniform );
		m_bitmapFeet = createUniformImg( "FootArmor", info.uniform );
		m_bitmapLHeld = createUniformImg( "LeftHandHeld", info.uniform );
		m_bitmapRHeld = createUniformImg( "RightHandHeld", info.uniform );
		m_bitmapBack = createUniformImg( "Back", info.uniform );
		//m_bitmapNeck = createUniformImg( "", info.uniform );
		//m_bitmapLRing = createUniformImg( "", info.uniform );
		//m_bitmapRRing = createUniformImg( "", info.uniform );
	}
	OnPropertyChanged( "ImgHead" );
	OnPropertyChanged( "ImgChest" );
	OnPropertyChanged( "ImgArms" );
	OnPropertyChanged( "ImgHands" );
	OnPropertyChanged( "ImgLegs" );
	OnPropertyChanged( "ImgFeet" );
	OnPropertyChanged( "ImgLeftHand" );
	OnPropertyChanged( "ImgRightHand" );
	OnPropertyChanged( "ImgBack" );
}

Noesis::Ptr<Noesis::BitmapSource> CreatureInfoModel::createUniformImg( QString slot, Uniform* uniform )
{
	auto part = uniform->parts.value( slot );
	if( part.item.isEmpty() )
	{
		return nullptr;
	}
	QStringList mats;
	mats.append( part.material );
	QPixmap pm = Util::createItemImage2( part.item, mats );

	std::vector<unsigned char> buffer;

	Util::createBufferForNoesisImage( pm, buffer );

	return BitmapImage::Create( pm.width(), pm.height(), 96, 96, buffer.data(), pm.width() * 4, BitmapSource::Format::Format_RGBA8 );
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
}