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

#include <QPoint>
#include <QPointer>
#include <QSize>
#include <QString>
#include <QVariantMap>

class Game;

class WorldObject
{
	Q_DISABLE_COPY_MOVE( WorldObject )
public:
	WorldObject( Game* game );
	WorldObject( QVariantMap vals, Game* game );
	virtual ~WorldObject();

	void serialize( QVariantMap& out ) const;

	QString name()
	{
		return m_name;
	}
	void setName( QString name )
	{
		m_name = name;
	}

	unsigned int id()
	{
		return m_id;
	}
	void setId( unsigned int id )
	{
		m_id = id;
	}

	void setActive( bool state )
	{
		m_active = state;
	}
	bool active()
	{
		return m_active;
	}

	void setSuspended( bool state )
	{
		m_active = !state;
	}
	bool suspended()
	{
		return !m_active;
	}

protected:
	QPointer<Game> g;
	unsigned int m_id = 0;
	QString m_name    = "";
	bool m_active     = true;

	quint64 m_lastUpdateTick = 0;
};