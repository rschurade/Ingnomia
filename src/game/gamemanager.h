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
class EventConnector;
class NewGameSettings;
class SpriteFactory;

class GameManager : public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY_MOVE( GameManager )
public:
	GameManager( QObject* parent = nullptr );
	~GameManager();

	void startNewGame();
	void setUpNewGame();
	void continueLastGame();
	void loadGame( QString folder );
	void saveGame();

	void setShowMainMenu( bool value );
	void endCurrentGame();

	GameSpeed gameSpeed();
	void setGameSpeed( GameSpeed speed );

	bool paused();
	void setPaused( bool value );
	
	void setHeartbeatResponse( int value );

	EventConnector* eventConnector();

	Game* game();
	
private:
	QPointer<EventConnector> m_eventConnector;
	//SpriteFactory* m_sf = nullptr;

	QPointer<Game> m_game;
	
	void init();
	void createNewGame();

	void postCreationInit();
	
signals:

public slots:
	void onGeneratorMessage( QString message );
};
