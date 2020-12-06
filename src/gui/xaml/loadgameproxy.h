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

#include "../aggregatorloadgame.h"
#include "LoadGameModel.h"

#include <QObject>

class LoadGameProxy : public QObject
{
	Q_OBJECT

public:
	LoadGameProxy( QObject* parent = nullptr );
	void setParent( IngnomiaGUI::LoadGameModel* parent );

	void requestKingdoms();
	void requestSaveGames( const QString path );

private:
	IngnomiaGUI::LoadGameModel* m_parent = nullptr;

private slots:
	void onKingdoms( const QList<GuiSaveInfo>& kingdoms );
	void onSaveGames( const QList<GuiSaveInfo>& saveGames );

signals:
	void signalRequestKingdoms();
	void signalRequestSaveGames( const QString path );
};
