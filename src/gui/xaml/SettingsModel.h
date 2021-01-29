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

#ifndef __SETTINGSMODEL_H__
#define __SETTINGSMODEL_H__

#include "../aggregatorsettings.h"

#include <NsApp/DelegateCommand.h>
#include <NsApp/NotifyPropertyChangedBase.h>
#include <NsCore/Noesis.h>
#include <NsCore/Ptr.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/ReflectionDeclareEnum.h>
#include <NsCore/String.h>
#include <NsGui/Collection.h>

#include <QString>

class SettingsProxy;

namespace Noesis
{
template <class T>
class ObservableCollection;
}

namespace IngnomiaGUI
{

struct LanguageEntry : public Noesis::BaseComponent
{
public:
	LanguageEntry( QString name );

	Noesis::String m_name;

    const char* getName() const;
	
	NS_DECLARE_REFLECTION( LanguageEntry, BaseComponent )
};

struct ScaleEntry : public Noesis::BaseComponent
{
public:
	ScaleEntry( QString name, float scale );

	Noesis::String m_name;
    float m_scale;

    const char* getName() const;
	
	NS_DECLARE_REFLECTION( ScaleEntry, BaseComponent )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class SettingsModel final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	SettingsModel();

    void updateSettings( const GuiSettings& settings );

private:
    SettingsProxy* m_proxy = nullptr;

    Noesis::ObservableCollection<LanguageEntry>* getLanguages() const;
	void setLanguage( LanguageEntry* item );
	LanguageEntry* getLanguage() const;

    Noesis::ObservableCollection<ScaleEntry>* getScales() const;
	void setScale( ScaleEntry* item );
	ScaleEntry* getScale() const;

    bool getFullScreen() const;
	void setFullScreen( bool value );

    int getKeyboardSpeed() const;
	void setKeyboardSpeed( int value );

    int getLightMin() const;
	void setLightMin( int value );

    bool getMouseWheelBehavior() const;
	void setMouseWheelBehavior( bool value );
	
	float getAudioMasterVolume() const;
	void setAudioMasterVolume( float value );

    Noesis::Ptr<Noesis::ObservableCollection<LanguageEntry>> m_languages;
    LanguageEntry* m_selectedLanguage = nullptr;;

    Noesis::Ptr<Noesis::ObservableCollection<ScaleEntry>> m_scales;
    ScaleEntry* m_selectedScale = nullptr;;

    float m_scale = 1.0f;
    bool m_fullScreen = false;
    bool m_toggleMouseWheel = false;
    int m_keyboardSpeed = 20;
    int m_lightMin = 30;
	 float m_audioMasterVolume = 100.0f;

	NS_DECLARE_REFLECTION( SettingsModel, NotifyPropertyChangedBase )
};

} // namespace IngnomiaGUI

#endif
