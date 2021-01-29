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
#pragma once

#include <QObject>


struct GuiSettings
{
    bool fullscreen = false;
    float scale = 1.0;
    QList<QString> languages;
    QString language;
    int keyboardSpeed = 20;
    int lightMin = 30;
    bool toggleMouseWheel = false;
    float audioMasterVolume = 100.0;
};

Q_DECLARE_METATYPE( GuiSettings )

class AggregatorSettings : public QObject
{
	Q_OBJECT

public:
	AggregatorSettings( QObject* parent = nullptr );
	~AggregatorSettings();

private:
	GuiSettings m_settings;

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
