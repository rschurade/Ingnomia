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

#include "../game/gnome.h"
#include <QVariantMap>

struct TraderItem {
	QString type;
	QString itemSID;
	QString materialSID;
	QString gender;
	unsigned char quality = 0;
	int value = 0;
	int amount = 0;
	int reserved = 0;
};

struct TraderDefinition {
	QString id;
	QList<TraderItem> items;

	TraderDefinition() {};
	TraderDefinition( QVariantMap& in );
	void serialize( QVariantMap& out );
};

class GnomeTrader : public Gnome
{
public:
	GnomeTrader( Position& pos, QString name, Gender gender, Game* game );
	GnomeTrader( QVariantMap& in, Game* game );
	~GnomeTrader();

	virtual void serialize( QVariantMap& out );

	void init();

	void setMarketStall( unsigned int id );
	void setTraderDefinition( QVariantMap map );

	CreatureTickResult onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged );

	void setInventory( QVariantList items );

	QList<TraderItem>& inventory();

	void initTaskMapTrader();

	BT_RESULT conditionIsTimeToLeave( bool halt = false );
	BT_RESULT actionGetMarketStallPosition( bool halt );
	BT_RESULT actionTrade( bool halt );

private:
	quint64 m_leavesOnTick     = 0;
	unsigned int m_marketStall = 0;
	TraderDefinition m_traderDefinition;
};
