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
#ifndef GAME_H_
#define GAME_H_

#include <QObject>

class Config;

class QTimer;
class Grass;
class Gnome;
class Animal;

class Game : public QObject
{
	Q_OBJECT

public:
	Game();
	Game( bool isLoaded );
	virtual ~Game();

	void save();

private:
	QTimer* m_timer = nullptr;

	int m_speed            = 0;
	int m_millisecondsSlow = 50;
	int m_millisecondsFast = 5;

	int m_maxLoopTime = 0;

	void processPlants();

	void sendClock();
	void calcDaylight();
	int timeToInt( QString time );
	QString intToTime( int time );

	void autoSave();

public slots:
	void loop();
	void start();
	void stop();
	void sendTime();

signals:
	void sendOverlayMessage( int id, QString text );
	void signalTimeAndDate( int minute, int hour, int day, QString season, int year, QString sunStatus );
	void signalPause( bool pause );
	void signalEvent( unsigned int id, QString title, QString msg, bool pause, bool yesno );
	void signalStartAutoSave();
	void signalEndAutoSave();
	void signalUpdateTileInfo( QSet<unsigned int> changeSet );
	void signalUpdateStockpile();
};

#endif /* GAME_H_ */
