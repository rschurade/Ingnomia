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
/** @file aggregatorsettings.cpp
 *  @brief AggregatorSettings implementation: reads values from Global::cfg for the GUI and
 *         writes edits back. Emits a few specialised signals for changes that the renderer
 *         needs to react to immediately (UI scale, fullscreen).
 */
#include "aggregatorsettings.h"

#include "../base/config.h"

#include <QDebug>

/// @brief Constructs the AggregatorSettings.
/// @param parent Qt parent object.
AggregatorSettings::AggregatorSettings( QObject* parent ) :
	QObject(parent)
{

}

/// @brief Destructor.
AggregatorSettings::~AggregatorSettings()
{
}

/// @brief Snapshots the current config values into m_settings, populates the language list,
///        and emits signalUpdateSettings for the GUI to bind to.
void AggregatorSettings::onRequestSettings()
{
    m_settings.fullscreen = Global::cfg->get( "fullscreen" ).toBool();
    m_settings.scale = qMax( 0.5f, Global::cfg->get( "uiscale" ).toFloat() );
    m_settings.keyboardSpeed = qMax( 0, qMin( Global::cfg->get( "keyboardMoveSpeed" ).toInt(), 200) );
    m_settings.languages.clear();
    m_settings.languages.append( "en_US" );
    m_settings.languages.append( "fr_FR" );
    m_settings.languages.append( "pt_BR" );
    m_settings.languages.append( "zh_CN" );

    m_settings.language = Global::cfg->get( "language" ).toString();

    m_settings.lightMin = Global::cfg->get( "lightMin" ).toFloat() * 100;

    m_settings.toggleMouseWheel = Global::cfg->get( "toggleMouseWheel" ).toBool();

	 m_settings.audioMasterVolume = Global::cfg->get( "AudioMasterVolume" ).toFloat() * 100;

    emit signalUpdateSettings( m_settings );
}

/// @brief Persists the selected language in the config.
/// @param language Language ID (e.g. "en_US").
void AggregatorSettings::onSetLanguage( QString language )
{
    Global::cfg->set( "language", language );
    //emit signalSetLanguage( language );
}

/// @brief Persists a new UI scale factor and notifies the GUI shell to reflow layouts.
/// @param scale New scale factor.
void AggregatorSettings::onSetUIScale( float scale )
{
    Global::cfg->set( "uiscale", scale );
    emit signalUIScale( scale );
}

/// @brief Persists the fullscreen flag and notifies the main window.
/// @param value True for fullscreen.
void AggregatorSettings::onSetFullScreen( bool value )
{
    Global::cfg->set( "fullscreen", value );
    emit signalFullScreen( value );
}

/// @brief Persists the keyboard camera pan speed.
/// @param value New pan speed in pixels/tick.
void AggregatorSettings::onSetKeyboardSpeed( int value )
{
    Global::cfg->set( "keyboardMoveSpeed", value );
}

/// @brief Persists the minimum light level, stored as a 0–1 float after dividing by 100.
/// @param value Light-min value in percent (0–100).
void AggregatorSettings::onSetLightMin( int value )
{
    Global::cfg->set( "lightMin", (float)value / 100. );
}

/// @brief Persists whether the mouse wheel cycles z-levels (true) or zooms (false).
/// @param value New toggle state.
void AggregatorSettings::onSetToggleMouseWheel( bool value )
{
    Global::cfg->set( "toggleMouseWheel", value );
}

/// @brief Emits the current UI scale so the GUI can re-bind to it during startup.
void AggregatorSettings::onRequestUIScale()
{
    float scale = Global::cfg->get( "uiscale" ).toFloat();
    emit signalUIScale( scale );
}

/// @brief Emits the current game version string to the GUI.
void AggregatorSettings::onRequestVersion()
{
    QString version = Global::cfg->get( "CurrentVersion" ).toString();
    emit signalVersion( version );
}

/// @brief Persists the master audio volume.
/// @param value Volume in percent (0–100).
void AggregatorSettings::onSetAudioMasterVolume( float value )
{
	Global::cfg->set( "AudioMasterVolume", (float)value);
}
