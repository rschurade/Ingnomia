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
#include <QDateTime>

struct GuiSaveInfo
{
	QString name;
	QString folder;
	QString dir;
	QString version;
	QDateTime date;
	bool compatible = true;
};

Q_DECLARE_METATYPE( GuiSaveInfo )

class AggregatorLoadGame : public QObject
{
	Q_OBJECT

public:
	AggregatorLoadGame( QObject* parent = nullptr );
	~AggregatorLoadGame();

private:
	QList<GuiSaveInfo> m_kingdomList;
	QList<GuiSaveInfo> m_gameList;

public slots:
	void onRequestKingdoms();
	void onRequestSaveGames( const QString path );

signals:
	void signalKingdoms( const QList<GuiSaveInfo>& kingdoms );
	void signalSaveGames( const QList<GuiSaveInfo>& kingdoms );
};
