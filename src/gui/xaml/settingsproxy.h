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
/** @file settingsproxy.h
 *  @brief Qt-side proxy that bridges the SettingsModel view model to
 *         AggregatorSettings via queued signal/slot connections.
 */
#pragma once

//#include "../aggregatorsettings.h"
#include "SettingsModel.h"
#include "../aggregatorsettings.h"

#include <QObject>

/// @brief Bridges SettingsModel to AggregatorSettings. Forwards setter calls as signals
///        onto the game thread and relays settings snapshots back to the view model.
class SettingsProxy : public QObject
{
	Q_OBJECT

public:
	SettingsProxy( QObject* parent = nullptr );
	void setParent( IngnomiaGUI::SettingsModel* parent );

	void requestSettings();

    void setLanguage( QString language );
    void setUIScale( float scale );
    void setFullScreen( bool value );
    void setKeyboardSpeed( int value );
    void setLightMin( int value );
    void setToggleMouseWheel( bool value );
	void setAudioMasterVolume( float value );

private:
	IngnomiaGUI::SettingsModel* m_parent = nullptr; ///< Owning view model for relaying updates.



private slots:
	void onSettings( const GuiSettings& settings );

signals:
	void signalRequestSettings();

    void signalSetLanguage( QString language );
    void signalSetUIScale( float scale );
    void signalSetFullScreen( bool value );
    void signalSetKeyboardSpeed( int value );
    void signalSetLightMin( int value );
    void signalSetToggleMouseWheel( bool value );
	void signalSetAudioMasterVolume( float value );
};
