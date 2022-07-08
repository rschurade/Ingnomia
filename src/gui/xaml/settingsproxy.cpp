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
#include "settingsproxy.h"

#include "../../base/global.h"
#include "../eventconnector.h"

#include <QDebug>

SettingsProxy::SettingsProxy( QObject* parent ) :
	QObject( parent )
{
	this->signalRequestSettings.connect(&AggregatorSettings::onRequestSettings, Global::eventConnector->aggregatorSettings()); // TODO: Qt::QueuedConnection
    this->signalSetUIScale.connect(&AggregatorSettings::onSetUIScale, Global::eventConnector->aggregatorSettings()); // TODO: Qt::QueuedConnection
    this->signalSetKeyboardSpeed.connect(&AggregatorSettings::onSetKeyboardSpeed, Global::eventConnector->aggregatorSettings()); // TODO: Qt::QueuedConnection
    this->signalSetFullScreen.connect(&AggregatorSettings::onSetFullScreen, Global::eventConnector->aggregatorSettings()); // TODO: Qt::QueuedConnection
    this->signalSetLanguage.connect(&AggregatorSettings::onSetLanguage, Global::eventConnector->aggregatorSettings()); // TODO: Qt::QueuedConnection
    this->signalSetLightMin.connect(&AggregatorSettings::onSetLightMin, Global::eventConnector->aggregatorSettings()); // TODO: Qt::QueuedConnection
    this->signalSetToggleMouseWheel.connect(&AggregatorSettings::onSetToggleMouseWheel, Global::eventConnector->aggregatorSettings()); // TODO: Qt::QueuedConnection
	 this->signalSetAudioMasterVolume.connect(&AggregatorSettings::onSetAudioMasterVolume, Global::eventConnector->aggregatorSettings()); // TODO: Qt::QueuedConnection
    
    Global::eventConnector->aggregatorSettings()->signalUpdateSettings.connect(&SettingsProxy::onSettings, this); // TODO: Qt::QueuedConnection
}

void SettingsProxy::setParent( IngnomiaGUI::SettingsModel* parent )
{
	m_parent = parent;
}

void SettingsProxy::requestSettings()
{
    signalRequestSettings();
}

void SettingsProxy::onSettings( const GuiSettings& settings )
{
    if ( m_parent )
	{
        m_parent->updateSettings( settings );
	}
}

void SettingsProxy::setLanguage( const std::string& language )
{
    signalSetLanguage( language );
}
    
void SettingsProxy::setUIScale( float scale )
{
    signalSetUIScale( scale );
}

void SettingsProxy::setFullScreen( bool value )
{
    signalSetFullScreen( value );
}

void SettingsProxy::setKeyboardSpeed( int value )
{
    signalSetKeyboardSpeed( value );
}

void SettingsProxy::setLightMin( int value )
{
    signalSetLightMin( value );
}

void SettingsProxy::setToggleMouseWheel( bool value )
{
    signalSetToggleMouseWheel( value );
}

void SettingsProxy::setAudioMasterVolume( float value )
{
    signalSetAudioMasterVolume( value );
}