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
#include "config.h"

#include "../base/io.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QCoreApplication>

#include "containersHelper.h"

void to_json(json& j, const ConfigVariant& item) {
	if (const bool *itemPtr = std::get_if<bool>(&item)) {
		j = *itemPtr;
	} else if (const int *itemPtr = std::get_if<int>(&item)) {
		j = *itemPtr;
	} else if (const double *itemPtr = std::get_if<double>(&item)) {
		j = *itemPtr;
	} else if (const std::string *itemPtr = std::get_if<std::string>(&item)) {
		j = *itemPtr;
	} else {
		throw std::runtime_error("Cannot convert ConfigVariant to JSON");
	}
}

ConfigVariant jsonToConfigVariant(const json& v) {
	if (v.type() == nlohmann::detail::value_t::string) {
		qDebug() << "Type string";
		return { v.get<std::string>() };
	} else if (v.type() == nlohmann::detail::value_t::number_float) {
		qDebug() << "Type double";
		return { v.get<double>() };
	} else if (v.type() == nlohmann::detail::value_t::number_integer || v.type() == nlohmann::detail::value_t::number_unsigned) {
		qDebug() << "Type int or long";
		return { v.get<int>() };
	} else if (v.type() == nlohmann::detail::value_t::boolean) {
		qDebug() << "Type bool";
		return { v.get<bool>() };
	} else {
		qDebug() << "Fatal error: Cannot convert QVariant to ConfigVariant!";
		abort();
	}
}

Config::Config()
{
	QMutexLocker lock( &m_mutex );
	IO::createFolders();

	//check if Ingnomia folder in /Documents/My Games exist
	const fs::path& folder = IO::getDataFolder();
	bool ok        = true;
	json jd;

	if ( !IO::loadFile( folder / "settings" / "config.json", jd ) )
	{
		if( !IO::loadOriginalConfig( jd ) )
		{
			return;
		}
	}

	for ( const auto& entry : jd.items() ) {
		m_settings.insert_or_assign(entry.key(), jsonToConfigVariant(entry.value()));
	}

	if ( m_settings.empty() )
	{
		return;
	}


	/*
	if ( !IO::loadFile( folder / "settings" / "keybindings.json", jd ) )
	{
		if ( QFile::exists( "keybindings.json" ) )
		{
			QFile::copy( "keybindings.json", folder / "settings" / "keybindings.json" );
		}
	}
	*/
	// add values to exisiting confings
	if ( !m_settings.contains( "XpMod" ) )
	{
		m_settings.insert_or_assign( "XpMod", 250.0 );
	}

	if ( !m_settings.contains( "fow" ) )
	{
		m_settings.insert_or_assign( "fow", true );
	}

	if ( !m_settings.contains( "AutoSaveInterval" ) )
	{
		m_settings.insert_or_assign( "AutoSaveInterval", 3 );
	}
	if ( !m_settings.contains( "uiscale" ) )
	{
		m_settings.insert_or_assign( "uiscale", 1.0 );
	}
	m_settings.insert_or_assign( "dataPath", fs::path(QCoreApplication::applicationDirPath().toStdString()) / "content" );

	m_valid = true;

}

Config::~Config()
{
}

ConfigVariant Config::get_variant( const QString& key )
{
	QMutexLocker lock( &m_mutex );

	ConfigVariant v;
	maps::try_at(m_settings, key.toStdString(), v);
	return v;
}

template<class> inline constexpr bool always_false_v = false;

std::string variantToString( ConfigVariant v )
{
	return std::visit( []( auto&& arg )
					   {
		using T = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<T, std::string>)
			return arg;
		else if constexpr (std::is_same_v<T, bool>)
			return std::string(arg ? "true" : "false");
		else if constexpr (std::is_arithmetic_v<T>)
			return std::to_string(arg);
		else
			static_assert(always_false_v<T>, "non-exhaustive visitor!"); },
					   v );
}

void Config::set( const QString& key, const ConfigVariant& value )
{
	QMutexLocker lock( &m_mutex );
	const auto oldValue = m_settings[key.toStdString()];
	if (oldValue != value)
	{
		m_settings[key.toStdString()] = value;
		IO::saveConfig();

		qDebug() << "Update config" << key << "=" << QString::fromStdString(variantToString(value)) << "(was" << QString::fromStdString(variantToString(oldValue)) << ")";
	}
}
