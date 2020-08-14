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
#ifndef __CreatureInfoModel_H__
#define __CreatureInfoModel_H__

#include "PopulationModel.h"

#include "../aggregatorcreatureinfo.h"

#include <NsApp/DelegateCommand.h>
#include <NsApp/NotifyPropertyChangedBase.h>
#include <NsCore/Noesis.h>
#include <NsCore/Ptr.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/ReflectionDeclareEnum.h>
#include <NsCore/String.h>
#include <NsGui/Collection.h>
#include <NsGui/ImageSource.h>
#include <NsGui/BitmapSource.h>

#include <QString>

class CreatureInfoProxy;
struct UniformItem;
struct EquipmentItem;

namespace Noesis
{
template <class T>
class ObservableCollection;
}

namespace IngnomiaGUI
{

////////////////////////////////////////////////////////////////////////////////////////////////////
class CreatureInfoModel final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	CreatureInfoModel();

	void updateInfo( const GuiCreatureInfo& info );
	void updateProfessionList( const QStringList& professions );

private:
	CreatureInfoProxy* m_proxy = nullptr;
	unsigned int m_id = 0;

	Noesis::String m_str;
	Noesis::String m_con;
	Noesis::String m_dex;
	Noesis::String m_int;
	Noesis::String m_wis;
	Noesis::String m_cha;
	Noesis::String m_hunger;
	Noesis::String m_thirst;
	Noesis::String m_sleep;
	Noesis::String m_happiness;
	Noesis::String m_activity;

	const char* GetName() const;
	const char* GetStr() const;
	const char* GetCon() const;
	const char* GetDex() const;
	const char* GetInt() const;
	const char* GetWis() const;
	const char* GetCha() const;
	const char* GetHunger() const;
	const char* GetThirst() const;
	const char* GetSleep() const;
	const char* GetHappiness() const;

	const char* GetActivity() const;

	Noesis::String m_name;

	Noesis::ObservableCollection<ProfItem>* GetProfessions() const;
	void SetProfession( ProfItem* item );
	ProfItem* GetProfession() const;
	Noesis::Ptr<Noesis::ObservableCollection<ProfItem>> m_professions;
	ProfItem* m_selectedProfession = nullptr;

	const Noesis::ImageSource* getBitmapSourceHead()  const { return m_bitmapHead; };
	const Noesis::ImageSource* getBitmapSourceChest() const { return m_bitmapChest; };
	const Noesis::ImageSource* getBitmapSourceArms()  const { return m_bitmapArms; };
	const Noesis::ImageSource* getBitmapSourceHands() const { return m_bitmapHands; };
	const Noesis::ImageSource* getBitmapSourceLegs()  const { return m_bitmapLegs; };
	const Noesis::ImageSource* getBitmapSourceFeet()  const { return m_bitmapFeet; };
	const Noesis::ImageSource* getBitmapSourceLHeld() const { return m_bitmapLHeld; };
	const Noesis::ImageSource* getBitmapSourceRHeld() const { return m_bitmapRHeld; };
	const Noesis::ImageSource* getBitmapSourceBack()  const { return m_bitmapBack; };
	const Noesis::ImageSource* getBitmapSourceNeck()  const { return m_bitmapNeck; };
	const Noesis::ImageSource* getBitmapSourceLRing() const { return m_bitmapLRing; };
	const Noesis::ImageSource* getBitmapSourceRRing() const { return m_bitmapRRing; };
	
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapHead;
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapChest;
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapArms;
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapHands;
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapLegs;
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapFeet;
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapLHeld;
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapRHeld;
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapBack;
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapNeck;
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapLRing;
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapRRing;

	Noesis::Ptr<Noesis::BitmapSource> m_bitmapHeadEmpty;
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapChestEmpty;
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapArmsEmpty;
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapHandsEmpty;
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapLegsEmpty;
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapFeetEmpty;
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapLHeldEmpty;
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapRHeldEmpty;
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapBackEmpty;
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapNeckEmpty;
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapLRingEmpty;
	Noesis::Ptr<Noesis::BitmapSource> m_bitmapRRingEmpty;
	bool m_emptySlotsInitialized = false;

	Noesis::Ptr<Noesis::BitmapSource> createUniformImg( QString slot, const UniformItem& uItem, const EquipmentItem& eItem );
	Noesis::Ptr<Noesis::BitmapSource> createEmptyUniformImg( QString spriteID );


	NS_DECLARE_REFLECTION( CreatureInfoModel, NotifyPropertyChangedBase )
};

} // namespace IngnomiaGUI

#endif
