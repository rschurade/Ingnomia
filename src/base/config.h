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

#ifndef CONFIG_H_
#define CONFIG_H_

#include <QHash>
#include <QMutex>
#include <QString>
#include <QVariant>
#include <QVariantMap>

class Config
{
private:
	// Private Constructor
	Config();
	// Stop the compiler generating methods of copy the object
	Config( Config const& copy );            // Not Implemented
	Config& operator=( Config const& copy ); // Not Implemented

	QVariantMap m_settings;

	QMutex m_mutex;

public:
	~Config();

	static Config& getInstance()
	{
		// The only instance
		// Guaranteed to be lazy initialized
		// Guaranteed that it will be destroyed correctly
		static Config instance;
		return instance;
	}

	bool init();
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
};

#endif /* CONFIG_H_ */
