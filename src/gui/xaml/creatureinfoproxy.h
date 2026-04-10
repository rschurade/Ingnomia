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
/** @file creatureinfoproxy.h
 *  @brief Qt-side proxy bridging CreatureInfoModel with AggregatorCreatureInfo.
 */
#pragma once

#include "../aggregatorcreatureinfo.h"
#include "creatureinfomodel.h"

#include <QObject>

/// @brief Forwards user actions from CreatureInfoModel into AggregatorCreatureInfo and pushes
///        incoming GuiCreatureInfo updates back into the model.
class CreatureInfoProxy : public QObject
{
	Q_OBJECT

public:
	CreatureInfoProxy( QObject* parent = nullptr );
	~CreatureInfoProxy();

	void setParent( IngnomiaGUI::CreatureInfoModel* parent );
	void requestProfessionList();
	void setProfession( unsigned int gnomeID, QString profession );

	void requestEmptySlotImages();


private:
	IngnomiaGUI::CreatureInfoModel* m_parent = nullptr;  ///< View model the proxy pushes updates into.

private slots:
	void onUpdateInfo( const GuiCreatureInfo& info );
	void onProfessionList( const QStringList& profs );
	void onEmptyPics( const QMap< QString, std::vector<unsigned char> >& emptyPics );
signals:
	void signalRequestCreatureUpdate( unsigned int creatureID );
	void signalRequestProfessionList();
	void signalSetProfession( unsigned int gnomeID, QString profession );
	void signalRequestEmptySlotImages();
};
