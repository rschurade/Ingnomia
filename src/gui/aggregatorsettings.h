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
/** @file aggregatorsettings.h
 *  @brief Data type and aggregator for the in-game Settings XAML window. Mirrors
 *         Global::cfg values and pushes changes back into the config when edited.
 */
#pragma once

#include <QObject>


/// @brief Settings payload sent to the GUI.
struct GuiSettings
{
    bool fullscreen = false;          ///< Fullscreen toggle.
    float scale = 1.0;                ///< UI scaling factor.
    QList<QString> languages;         ///< Available language IDs.
    QString language;                 ///< Currently active language ID.
    int keyboardSpeed = 20;           ///< Camera pan speed in pixels/tick when holding a key.
    int lightMin = 30;                ///< Minimum tile light level (ambient floor).
    bool toggleMouseWheel = false;    ///< True if the mouse wheel cycles z-levels (else zooms).
    float audioMasterVolume = 50.0;   ///< Master audio volume 0–100.
};

Q_DECLARE_METATYPE( GuiSettings )

/// @brief Reads settings from Global::cfg, exposes them to the Settings window, and writes
///        edits back to the config.
class AggregatorSettings : public QObject
{
	Q_OBJECT

public:
	AggregatorSettings( QObject* parent = nullptr );
	~AggregatorSettings();

private:
	GuiSettings m_settings;  ///< Cached settings payload.

public slots:
	void onRequestSettings();

    void onRequestUIScale();
    void onRequestVersion();

    void onSetLanguage( QString language );
    void onSetUIScale( float scale );
    void onSetKeyboardSpeed( int value );
    void onSetFullScreen( bool value );
    void onSetLightMin( int value );
    void onSetToggleMouseWheel( bool value );
	void onSetAudioMasterVolume( float value );

signals:
	void signalUpdateSettings( const GuiSettings& info );

    void signalFullScreen( bool value );
    void signalUIScale( float value );
    void signalSetLanguage( QString language );
    void signalVersion( QString version );
};
