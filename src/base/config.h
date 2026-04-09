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

/** @file config.h
 * @brief Thread-safe application configuration store backed by JSON on disk.
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <QHash>
#include <QMutex>
#include <QString>
#include <QVariant>
#include <QVariantMap>

/**
 * @brief Thread-safe key-value configuration store.
 *
 * Loads settings from config.json in the user data folder (Documents/My Games/Ingnomia).
 * Ensures default values for XpMod, fow, AutoSaveInterval, uiscale, and dataPath.
 * All get/set operations are mutex-protected. set() persists changes to disk immediately.
 */
class Config
{
private:
	QVariantMap m_settings; ///< Key-value map of all configuration settings.
	bool m_valid = false;   ///< Whether the config was loaded successfully.
	QMutex m_mutex;         ///< Mutex protecting concurrent access to m_settings.

public:
	Config();
	~Config();

	QVariant get( QString key );
	void set( QString key, QVariant value );

	QVariantMap& object()
	{
		return m_settings;
	}
	void setObject( QVariantMap obj )
	{
		m_settings = obj;
	}

	bool valid()
	{
		return m_valid;
	}
};

#endif /* CONFIG_H_ */
