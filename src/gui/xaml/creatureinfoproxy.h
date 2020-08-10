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

#include "../aggregatorcreatureinfo.h"
#include "creatureinfomodel.h"

#include <QObject>

class CreatureInfoProxy : public QObject
{
	Q_OBJECT

public:
	CreatureInfoProxy( QObject* parent = nullptr );
	~CreatureInfoProxy();

	void setParent( IngnomiaGUI::CreatureInfoModel* parent );
	void requestProfessionList();
	void setProfession( unsigned int gnomeID, QString profession );


private:
	IngnomiaGUI::CreatureInfoModel* m_parent = nullptr;

private slots:
	void onUpdateInfo( const GuiCreatureInfo& info );
	void onProfessionList( const QStringList& profs );
	
signals:
	void signalRequestCreatureUpdate( unsigned int creatureID );
	void signalRequestProfessionList();
	void signalSetProfession( unsigned int gnomeID, QString profession );
};
