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

#include <QMutex>
#include <QString>
#include <absl/types/variant.h>
#include <absl/container/flat_hash_map.h>

#include "containersHelper.h"

using ConfigVariant = absl::variant<bool, double, QString>;
using ConfigMap = absl::flat_hash_map<QString, ConfigVariant>;

class Config
{
private:
	ConfigMap m_settings;
	bool m_valid = false;
	QMutex m_mutex;

	ConfigVariant get_variant(const QString &key);

public:
	Config();
	~Config();

	template<class T>
	T get( const QString& key )
	{
		return std::get<T>( get_variant( key ) );
	}

	template<class T>
	T get_or_default( const QString& key, const T& defaultValue )
	{
		return maps::get_or_default<T>( get_variant( key ), defaultValue );
	}

	void set( const QString& key, int value );
	void set( const QString& key, ConfigVariant value );

	ConfigMap& object()
	{
		return m_settings;
	}

	void setObject( ConfigMap obj )
	{
		m_settings = obj;
	}

	bool valid()
	{
		return m_valid;
	}
};

#endif /* CONFIG_H_ */
