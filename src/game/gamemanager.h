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

#include "../base/enums.h"

#include <QObject>
#include <QString>
#include <QThread>

class Game;

class GameManager : public QObject
{
	Q_OBJECT

private:
	// Private Constructor
	GameManager( QObject* parent = nullptr );
	// Stop the compiler generating methods of copy the object
	GameManager( GameManager const& copy );            // Not Implemented
	GameManager& operator=( GameManager const& copy ); // Not Implemented

public:
	~GameManager();

	static GameManager& getInstance()
	{
		// The only instance
		// Guaranteed to be lazy initialized
		// Guaranteed that it will be destroyed correctly
		static GameManager instance;
		return instance;
	}

	void startNewGame( std::function<void( void )> callback );
	void setUpNewGame();
	void continueLastGame( std::function<void( bool )> callback );
	void loadGame( QString folder, std::function<void( bool )> callback );
	void saveGame();

	bool showMainMenu()
	{
		return m_showMainMenu;
	}
	void setShowMainMenu( bool value );

	GameSpeed gameSpeed()
	{
		return m_gameSpeed;
	}
	void setGameSpeed( GameSpeed speed )
	{
		m_gameSpeed = speed;
	}

	bool paused()
	{
		return m_paused;
	}
	void setPaused( bool value )
	{
		m_paused = value;
		emit signalUpdatePaused();
		emit signalUpdateGameSpeed();
	}

private:
	bool m_showMainMenu = true;

	Game* m_game = nullptr;
	QThread m_gameThread;

	GameSpeed m_gameSpeed = GameSpeed::Normal;
	bool m_paused         = true;

	void init();
	void createNewGame();

signals:
	void startGame();
	void stopGame();
	void signalUpdateGameSpeed();
	void signalUpdatePaused();

	void signalInitView();

public slots:
	void onGeneratorMessage( QString message );
};
