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

#include <sigslot/signal.hpp>

struct GuiSaveInfo
{
	QString name;
	std::string folder;
	QString dir;
	std::string version;
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
	void onRequestSaveGames( const std::string& path );

public: // signals:
	sigslot::signal<const QList<GuiSaveInfo>& /*kingdoms*/> signalKingdoms;
	sigslot::signal<const QList<GuiSaveInfo>& /*kingdoms*/> signalSaveGames;
};
