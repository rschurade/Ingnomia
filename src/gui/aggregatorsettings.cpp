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

#include <QDebug>

AggregatorSettings::AggregatorSettings( QObject* parent )
{

}

AggregatorSettings::~AggregatorSettings()
{
}

void AggregatorSettings::onRequestSettings()
{
    m_settings.fullscreen = Config::getInstance().get( "fullscreen" ).toBool();
    m_settings.scale = qMax( 1.0f, Config::getInstance().get( "uiscale" ).toFloat() );
    m_settings.keyboardSpeed = qMax( 20, Config::getInstance().get( "keyboardMoveSpeed" ).toInt() );
    m_settings.languages.clear();
    m_settings.languages.append( "en_US" );
    m_settings.languages.append( "fr_FR" );

    m_settings.language = Config::getInstance().get( "language" ).toString();

    m_settings.lightMin = Config::getInstance().get( "lightMin" ).toFloat() * 100; 

    emit signalUpdateSettings( m_settings );
}

void AggregatorSettings::onSetLanguage( QString language )
{
    Config::getInstance().set( "language", language );
    //emit signalSetLanguage( language );
}
    
void AggregatorSettings::onSetUIScale( float scale )
{
    Config::getInstance().set( "uiscale", scale );
    emit signalUIScale( scale );
}

void AggregatorSettings::onSetFullScreen( bool value )
{
    Config::getInstance().set( "fullscreen", value );
    emit signalFullScreen( value );
}

void AggregatorSettings::onSetKeyboardSpeed( int value )
{
    Config::getInstance().set( "keyboardMoveSpeed", value );
}

void AggregatorSettings::onSetLightMin( int value )
{
    Config::getInstance().set( "lightMin", (float)value / 100. );
}
