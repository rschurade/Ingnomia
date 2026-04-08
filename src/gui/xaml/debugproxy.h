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

#include "../aggregatordebug.h"
#include "debugmodel.h"

#include <QObject>

class DebugProxy : public QObject
{
	Q_OBJECT

public:
	DebugProxy( QObject* parent = nullptr );
	void setParent( IngnomiaGUI::DebugModel* parent );

	void spawnCreature( QString type );
	void setWindowSize( int width, int height );
	void setNeed( unsigned int gnomeID, QString need, float value );
	void killGnome( unsigned int gnomeID );
	void spawnItem( QString itemSID, QString materialSID, int count, int x, int y, int z );
	void spawnCompositeItem( QString itemSID, QStringList materialSIDs, int count, int x, int y, int z );
	void requestGnomeList();
	void requestItemGroups();
	void requestItems( QString group );
	void requestMaterials( QString itemSID );
	void setNeedDecayMultiplier( float value );
	void setDisableNeedDecay( QString need, bool disable );

private:
	IngnomiaGUI::DebugModel* m_parent = nullptr;

public slots:
	void onGnomeList( const QList<QPair<QString, unsigned int>>& gnomes );
	void onItemGroups( const QStringList& groups );
	void onItems( const QStringList& items );
	void onMaterials( int componentCount, const QStringList& mats1, const QStringList& mats2 );

signals:
	void signalSpawnCreature( QString type );
	void signalSetWindowSize( int width, int height );
	void signalSetNeed( unsigned int gnomeID, QString need, float value );
	void signalKillGnome( unsigned int gnomeID );
	void signalSpawnItem( QString itemSID, QString materialSID, int count, int x, int y, int z );
	void signalSpawnCompositeItem( QString itemSID, QStringList materialSIDs, int count, int x, int y, int z );
	void signalRequestGnomeList();
	void signalRequestItemGroups();
	void signalRequestItems( QString group );
	void signalRequestMaterials( QString itemSID );
	void signalSetNeedDecayMultiplier( float value );
	void signalSetDisableNeedDecay( QString need, bool disable );
};
