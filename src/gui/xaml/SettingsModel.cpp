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

#include "SettingsModel.h"
#include "settingsproxy.h"

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
LanguageEntry::LanguageEntry( QString name ) :
	m_name( name.toStdString().c_str() )
{
}

const char* LanguageEntry::getName() const
{
	return m_name.Str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ScaleEntry::ScaleEntry( QString name, float scale ) :
	m_name( name.toStdString().c_str() ),
    m_scale( scale )
{
}

const char* ScaleEntry::getName() const
{
	return m_name.Str();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
SettingsModel::SettingsModel()
{
    m_proxy = new SettingsProxy;
	m_proxy->setParent( this );
     
    m_languages = *new ObservableCollection<LanguageEntry>();
    m_proxy->requestSettings();

    m_scales = *new ObservableCollection<ScaleEntry>();
}

void SettingsModel::updateSettings( const GuiSettings& settings )
{
    m_languages->Clear();  
    for( auto lang : settings.languages )
	{
        m_languages->Add( MakePtr<LanguageEntry>( lang ) );
	}

    if( settings.languages.contains( settings.language ) )
    {
        setLanguage( m_languages->Get( settings.languages.indexOf( settings.language ) ) );
    }
	else
	{
        setLanguage( m_languages->Get( 0 ) );
	}

    m_scale = settings.scale;
    m_scales->Add( MakePtr<ScaleEntry>( "50%", 0.5f ) );
    m_scales->Add( MakePtr<ScaleEntry>( "75%", 0.75f ) );
    m_scales->Add( MakePtr<ScaleEntry>( "100%", 1.f ) );
    m_scales->Add( MakePtr<ScaleEntry>( "150%", 1.5f ) );
    m_scales->Add( MakePtr<ScaleEntry>( "200%", 2.0f ) );

    if( m_scale < 0.75 ) setScale( m_scales->Get( 0 ) );
	else if( m_scale < 1.0 ) setScale( m_scales->Get( 1 ) );
    else if( m_scale < 1.5 ) setScale( m_scales->Get( 2 ) );
    else if( m_scale < 2.0 ) setScale( m_scales->Get( 3 ) );
    else setScale( m_scales->Get( 4 ) );

    m_fullScreen = settings.fullscreen;
    m_keyboardSpeed = settings.keyboardSpeed;
    m_lightMin = settings.lightMin;
    m_toggleMouseWheel = settings.toggleMouseWheel;
	 m_audioMasterVolume = settings.audioMasterVolume;

    OnPropertyChanged( "Languages" );
    OnPropertyChanged( "SelectedLanguage" );
    OnPropertyChanged( "UIScale" );
    OnPropertyChanged( "UIScales" );
    OnPropertyChanged( "KeyboardSpeed" );
    OnPropertyChanged( "FullScreen" );
    OnPropertyChanged( "LightMin" );
    OnPropertyChanged( "MouseWheelBehavior" );
	 OnPropertyChanged( "AudioMasterVolume" );
}

    
Noesis::ObservableCollection<LanguageEntry>* SettingsModel::getLanguages() const
{
    return m_languages;
}

void SettingsModel::setLanguage( LanguageEntry* item )
{
    if( item && m_selectedLanguage != item )
	{
        m_selectedLanguage = item;
        m_proxy->setLanguage( item->getName() );
	}
}

LanguageEntry* SettingsModel::getLanguage() const
{
    return m_selectedLanguage;
}

Noesis::ObservableCollection<ScaleEntry>* SettingsModel::getScales() const
{
    return m_scales;
}
	
void SettingsModel::setScale( ScaleEntry* item )
{
    if( item && m_selectedScale != item )
	{
        m_selectedScale = item;
        m_proxy->setUIScale( item->m_scale );
	}
}

ScaleEntry* SettingsModel::getScale() const
{
    return m_selectedScale;
}
    



bool SettingsModel::getFullScreen() const
{
    return m_fullScreen;
}

void SettingsModel::setFullScreen( bool value )
{
    if( value != m_fullScreen )
	{
	    m_fullScreen = value;
        m_proxy->setFullScreen( value );
    }
}

    
int SettingsModel::getKeyboardSpeed() const
{
    return m_keyboardSpeed;
}

void SettingsModel::setKeyboardSpeed( int value )
{
    if( value != m_keyboardSpeed )
	{
        m_keyboardSpeed = value;
        m_proxy->setKeyboardSpeed( value );
	}
}

int SettingsModel::getLightMin() const
{
    return m_lightMin;
}

void SettingsModel::setLightMin( int value )
{
    if( value != m_lightMin )
	{
        m_lightMin = value;
        m_proxy->setLightMin( value );
	}
}

    
bool SettingsModel::getMouseWheelBehavior() const
{
    return m_toggleMouseWheel;
}

void SettingsModel::setMouseWheelBehavior( bool value )
{
    if( value != m_toggleMouseWheel )
	{
	    m_toggleMouseWheel = value;
        m_proxy->setToggleMouseWheel( value );
    }
}

float SettingsModel::getAudioMasterVolume() const
{
	return m_audioMasterVolume;
}

void SettingsModel::setAudioMasterVolume( float value )
{
	m_audioMasterVolume = value;
	m_proxy->setAudioMasterVolume( value );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION( SettingsModel, "IngnomiaGUI.SettingsModel" )
{
    NsProp( "Languages", &SettingsModel::getLanguages );
    NsProp( "SelectedLanguage", &SettingsModel::getLanguage, &SettingsModel::setLanguage );
    NsProp( "UIScales", &SettingsModel::getScales );
    NsProp( "UIScale", &SettingsModel::getScale, &SettingsModel::setScale );
    NsProp( "KeyboardSpeed", &SettingsModel::getKeyboardSpeed, &SettingsModel::setKeyboardSpeed );
    NsProp( "FullScreen", &SettingsModel::getFullScreen, &SettingsModel::setFullScreen );
    NsProp( "LightMin", &SettingsModel::getLightMin, &SettingsModel::setLightMin );
    NsProp( "MouseWheelBehavior", &SettingsModel::getMouseWheelBehavior, &SettingsModel::setMouseWheelBehavior );
	 NsProp( "AudioMasterVolume", &SettingsModel::getAudioMasterVolume, &SettingsModel::setAudioMasterVolume );
}

NS_IMPLEMENT_REFLECTION( LanguageEntry )
{
	NsProp( "Name", &LanguageEntry::getName );
}

NS_IMPLEMENT_REFLECTION( ScaleEntry )
{
	NsProp( "Name", &ScaleEntry::getName );
}