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

#include "../game/creature.h"
#include "../game/eventmanager.h"

#include <QObject>

class Game;

class AggregatorDebug : public QObject
{
	Q_OBJECT

public:
	AggregatorDebug( QObject* parent = nullptr );
	~AggregatorDebug();

	void init( Game* game );

public slots:
	void onSpawnCreature( QString type );
	void onSetWindowSize( int width, int height );
	void onSetNeed( unsigned int gnomeID, QString need, float value );
	void onKillGnome( unsigned int gnomeID );
	void onSpawnItem( QString itemSID, QString materialSID, int count, int x, int y, int z );
	void onSpawnCompositeItem( QString itemSID, QStringList materialSIDs, int count, int x, int y, int z );
	void onRequestGnomeList();
	void onRequestItemGroups();
	void onRequestItems( QString group );
	void onRequestMaterials( QString itemSID );
	void onSetNeedDecayMultiplier( float value );
	void onSetDisableNeedDecay( QString need, bool disable );

signals:
	void signalTriggerEvent( EventType type, QVariantMap args );
	void signalSetWindowSize( int width, int height );
	void signalGnomeList( const QList<QPair<QString, unsigned int>>& gnomes );
	void signalItemGroups( const QStringList& groups );
	void signalItems( const QStringList& items );
	void signalMaterials( int componentCount, const QStringList& mats1, const QStringList& mats2 );

private:
	QPointer<Game> g;
};
