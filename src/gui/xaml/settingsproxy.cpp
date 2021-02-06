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
	connect( this, &SettingsProxy::signalRequestSettings, Global::eventConnector->aggregatorSettings(), &AggregatorSettings::onRequestSettings, Qt::QueuedConnection );
    connect( this, &SettingsProxy::signalSetUIScale, Global::eventConnector->aggregatorSettings(), &AggregatorSettings::onSetUIScale, Qt::QueuedConnection );
    connect( this, &SettingsProxy::signalSetKeyboardSpeed, Global::eventConnector->aggregatorSettings(), &AggregatorSettings::onSetKeyboardSpeed, Qt::QueuedConnection );
    connect( this, &SettingsProxy::signalSetFullScreen, Global::eventConnector->aggregatorSettings(), &AggregatorSettings::onSetFullScreen, Qt::QueuedConnection );
    connect( this, &SettingsProxy::signalSetLanguage, Global::eventConnector->aggregatorSettings(), &AggregatorSettings::onSetLanguage, Qt::QueuedConnection );
    connect( this, &SettingsProxy::signalSetLightMin, Global::eventConnector->aggregatorSettings(), &AggregatorSettings::onSetLightMin, Qt::QueuedConnection );
    connect( this, &SettingsProxy::signalSetToggleMouseWheel, Global::eventConnector->aggregatorSettings(), &AggregatorSettings::onSetToggleMouseWheel, Qt::QueuedConnection );
	 connect( this, &SettingsProxy::signalSetAudioMasterVolume, Global::eventConnector->aggregatorSettings(), &AggregatorSettings::onSetAudioMasterVolume, Qt::QueuedConnection );
    
    connect( Global::eventConnector->aggregatorSettings(), &AggregatorSettings::signalUpdateSettings, this, &SettingsProxy::onSettings, Qt::QueuedConnection );
}

void SettingsProxy::setParent( IngnomiaGUI::SettingsModel* parent )
{
	m_parent = parent;
}

void SettingsProxy::requestSettings()
{
    emit signalRequestSettings();
}

void SettingsProxy::onSettings( const GuiSettings& settings )
{
    if ( m_parent )
	{
        m_parent->updateSettings( settings );
	}
}

void SettingsProxy::setLanguage( QString language )
{
    emit signalSetLanguage( language );
}
    
void SettingsProxy::setUIScale( float scale )
{
    emit signalSetUIScale( scale );
}

void SettingsProxy::setFullScreen( bool value )
{
    emit signalSetFullScreen( value );
}

void SettingsProxy::setKeyboardSpeed( int value )
{
    emit signalSetKeyboardSpeed( value );
}

void SettingsProxy::setLightMin( int value )
{
    emit signalSetLightMin( value );
}

void SettingsProxy::setToggleMouseWheel( bool value )
{
    emit signalSetToggleMouseWheel( value );
}

void SettingsProxy::setAudioMasterVolume( float value )
{
    emit signalSetAudioMasterVolume( value );
}