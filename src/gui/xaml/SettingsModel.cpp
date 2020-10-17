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
SettingsModel::SettingsModel()
{
    m_proxy = new SettingsProxy;
	m_proxy->setParent( this );
     
    m_languages = *new ObservableCollection<LanguageEntry>();
    m_proxy->requestSettings();
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

    m_fullScreen = settings.fullscreen;
    m_scale = settings.scale * 10;

    OnPropertyChanged( "Languages" );
    OnPropertyChanged( "SelectedLanguage" );
    OnPropertyChanged( "UIScale" );
    OnPropertyChanged( "FullScreen" );
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
        m_proxy->setLanguage( item->ToString().Str() );
	}
}

LanguageEntry* SettingsModel::getLanguage() const
{
    return m_selectedLanguage;
}

    
int SettingsModel::getScale() const
{
    return m_scale;
}

void SettingsModel::setScale( int value )
{
    if( value != m_scale )
	{
        m_scale = value;
        m_proxy->setUIScale( (float)value / 10. );
	}
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


////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION( SettingsModel, "IngnomiaGUI.SettingsModel" )
{
    NsProp( "Languages", &SettingsModel::getLanguages );
    NsProp( "SelectedLanguage", &SettingsModel::getLanguage, &SettingsModel::setLanguage );
    NsProp( "UIScale", &SettingsModel::getScale, &SettingsModel::setScale );
    NsProp( "FullScreen", &SettingsModel::getFullScreen, &SettingsModel::setFullScreen );
}

NS_IMPLEMENT_REFLECTION( LanguageEntry )
{
	NsProp( "Name", &LanguageEntry::getName );
}