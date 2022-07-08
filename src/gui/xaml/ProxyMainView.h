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

#include "ViewModel.h"

#include <QObject>

#include <sigslot/signal.hpp>

class ProxyMainView : public QObject
{
	Q_OBJECT

public:
	ProxyMainView( QObject* parent = nullptr );
	~ProxyMainView();

	void setParent( IngnomiaGUI::ViewModel* parent );

	void requestLoadScreenUpdate();
	void requestUIScale();
	void requestVersion();

	void startNewGame();
	void continueLastGame();
	void loadGame( const std::string& param );
	void saveGame();
	void setShowMainMenu( bool value );

	void endGame();

private:
	IngnomiaGUI::ViewModel* m_parent = nullptr;

private slots:
	void onWindowSize( int w, int h );
	void onKeyEsc();

	void onUIScale( float value );

	void onResume();
	void onLoadGameDone( bool value );
	void onVersion( const std::string& version );

public: // signals:
	sigslot::signal<> signalRequestLoadScreenUpdate;
	sigslot::signal<> signalRequestUIScale;
	sigslot::signal<> signalRequestVersion;

	sigslot::signal<> signalStartNewGame;
	sigslot::signal<> signalContinueLastGame;
	sigslot::signal<const std::string& /*param*/> signalLoadGame;
	sigslot::signal<> signalSaveGame;
	sigslot::signal<bool /*value*/> signalSetShowMainMenu;
	sigslot::signal<> signalEndGame;
	
};
