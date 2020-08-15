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

#include <QMap>
#include <QString>
#include <QStringList>

class DBHelper
{
public:
	//static QStringList getWorkPositions( QString jobID );

	static QString spriteID( QString itemID );
	static bool spriteIsRandom( QString spriteID );

	static int materialToolLevel( QString material );

	static int materialUID( QString material );
	static QString materialSID( int material );

	static int itemUID( QString item );
	static QString itemSID( int item );
	static bool itemIsContainer( int item );

	static int rowID( QString table, QString id );
	static QString id( QString table, int rowID );

	static QString qualitySID( int rank );
	static float qualityMod( int rank );

	static QString itemGroup( QString itemID );

private:
	DBHelper()  = delete;
	~DBHelper() = delete;

	static QMap<QString, QString> m_spriteIDCache;
	static QMap<QString, bool> m_spriteIsRandomCache;

	static QMap<int, QString> m_materialSIDCache;
	static QMap<int, QString> m_itemSIDCache;
	static QMap<QString, int> m_materialUIDCache;
	static QMap<QString, int> m_materialToolLevelCache;
	static QMap<QString, int> m_itemUIDCache;
	static QMap<int, bool> m_itemIsContainerCache;
	static QMap<int, QString> m_qualitySIDCache;
	static QMap<int, float> m_qualityModCache;
	static QMap<QString, QString> m_itemGroupCache;
};
