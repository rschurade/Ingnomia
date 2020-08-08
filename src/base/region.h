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

#include "../base/position.h"

#include <QMap>
#include <QSet>

class Region
{
public:
	Region( unsigned int id = 0 );
	~Region();

	void addConnectionFrom( unsigned int toRegion, const Position& pos );
	void addConnectionTo( unsigned int toRegion, const Position& pos );
	void addConnectionFrom( unsigned int toRegion, QString pos );
	void addConnectionTo( unsigned int toRegion, QString pos );

	void removeConnectionFrom( unsigned int toRegion, const Position& pos );
	void removeConnectionTo( unsigned int toRegion, const Position& pos );
	void removeConnectionFrom( unsigned int toRegion, QString pos );
	void removeConnectionTo( unsigned int toRegion, QString pos );

	void clearConnectionsFrom()
	{
		m_connectionsFrom.clear();
	}
	void clearConnectionsTo()
	{
		m_connectionsTo.clear();
	}

	void removeAllConnectionsFrom( unsigned int id );
	void removeAllConnectionsTo( unsigned int id );

	QMap<unsigned int, QSet<QString>> connectionSetFrom()
	{
		return m_connectionsFrom;
	};
	QMap<unsigned int, QSet<QString>> connectionSetTo()
	{
		return m_connectionsTo;
	}

	QSet<QString> connectionsToRegion( unsigned int region )
	{
		return m_connectionsTo.value( region );
	}

	unsigned id()
	{
		return m_id;
	}
	QList<unsigned int> connectionsFrom()
	{
		return m_connectionsFrom.keys();
	}
	QList<unsigned int> connectionsTo()
	{
		return m_connectionsTo.keys();
	}

private:
	unsigned int m_id = 0;
	QMap<unsigned int, QSet<QString>> m_connectionsFrom;
	QMap<unsigned int, QSet<QString>> m_connectionsTo;
};
