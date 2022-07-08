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
#include "aggregatorsettings.h"

#include "../base/config.h"
#include "../base/containersHelper.h"
#include "../base/global.h"

#include <QDebug>

#include <variant>

AggregatorSettings::AggregatorSettings( QObject* parent ) :
	QObject(parent)
{

}

AggregatorSettings::~AggregatorSettings()
{
}

void AggregatorSettings::onRequestSettings()
{
    m_settings.fullscreen = Global::cfg->get_or_default<bool>( "fullscreen" , false );
    m_settings.scale = qMax( 0.5, Global::cfg->get<double>( "uiscale" ) );
    m_settings.keyboardSpeed = qMax( 0.0, qMin( Global::cfg->get_or_default<double>( "keyboardMoveSpeed" , 0.0 ), 200.0) );
    m_settings.languages.clear();
    m_settings.languages.append( "en_US" );
    m_settings.languages.append( "fr_FR" );

    m_settings.language = Global::cfg->get_or_default<std::string>( "language" , "en_US" );

    m_settings.lightMin = Global::cfg->get_or_default<double>( "lightMin" , 0 ) * 100;

    m_settings.toggleMouseWheel = Global::cfg->get_or_default<bool>( "toggleMouseWheel" , false );

    m_settings.audioMasterVolume = Global::cfg->get_or_default<double>( "AudioMasterVolume" , 1.0 ) * 100.0;

    signalUpdateSettings( m_settings );
}

void AggregatorSettings::onSetLanguage( const std::string& language )
{
    Global::cfg->set( "language", language );
    //emit signalSetLanguage( language );
}
    
void AggregatorSettings::onSetUIScale( float scale )
{
    Global::cfg->set( "uiscale", scale );
    signalUIScale( scale );
}

void AggregatorSettings::onSetFullScreen( bool value )
{
    Global::cfg->set( "fullscreen", value );
    signalFullScreen( value );
}

void AggregatorSettings::onSetKeyboardSpeed( int value )
{
    Global::cfg->set( "keyboardMoveSpeed", value );
}

void AggregatorSettings::onSetLightMin( int value )
{
    Global::cfg->set( "lightMin", (float)value / 100. );
}

void AggregatorSettings::onSetToggleMouseWheel( bool value )
{
    Global::cfg->set( "toggleMouseWheel", value );
}

void AggregatorSettings::onRequestUIScale()
{
	float scale = Global::cfg->get<double>( "uiscale" );
    signalUIScale( scale );
}
void AggregatorSettings::onRequestVersion()
{
    const auto& version = Global::cfg->get<std::string>( "CurrentVersion" );
    signalVersion( version );
}

void AggregatorSettings::onSetAudioMasterVolume( float value )
{
	Global::cfg->set( "AudioMasterVolume", value );
}
