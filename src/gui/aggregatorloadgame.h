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
/** @file aggregatorloadgame.h
 *  @brief Aggregator that enumerates kingdoms and save games on disk for the Load Game GUI.
 */
#pragma once

#include <QObject>
#include <QDateTime>

/// @brief Metadata for one kingdom or save-game slot shown in the Load Game list.
struct GuiSaveInfo
{
	QString name;          ///< Display name (kingdom name or save file name).
	QString folder;        ///< Folder basename under the save root.
	QString dir;           ///< Absolute directory path.
	QString version;       ///< Version string read from the save file.
	QDateTime date;        ///< Last-modified timestamp.
	bool compatible = true;///< False if the save was produced by an incompatible game version.
};

Q_DECLARE_METATYPE( GuiSaveInfo )

/// @brief Scans the save-game folder layout and emits kingdom/save lists to the Load Game GUI.
class AggregatorLoadGame : public QObject
{
	Q_OBJECT

public:
	AggregatorLoadGame( QObject* parent = nullptr );
	~AggregatorLoadGame();

private:
	QList<GuiSaveInfo> m_kingdomList;  ///< Cached list of kingdoms in the save root.
	QList<GuiSaveInfo> m_gameList;     ///< Cached list of saves for the currently selected kingdom.

public slots:
	void onRequestKingdoms();
	void onRequestSaveGames( const QString path );

signals:
	void signalKingdoms( const QList<GuiSaveInfo>& kingdoms );
	void signalSaveGames( const QList<GuiSaveInfo>& kingdoms );
};
