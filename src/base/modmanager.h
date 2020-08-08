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

#include <QJsonObject>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QTimer>

#include <steam_api.h>
#include <vector>

class ModManager : public QObject
{
	Q_OBJECT

private:
	ModManager();
	// Stop the compiler generating methods of copy the object
	ModManager( ModManager const& copy );            // Not Implemented
	ModManager& operator=( ModManager const& copy ); // Not Implemented

	QStringList m_modsInDir;

	QString m_modFolder;

	QSet<QString> m_activeMods;

	std::vector<quint64> m_subscribedItemVec;

	void onCreateItemResult( CreateItemResult_t* pCallback, bool bIOFailure );
	CCallResult<ModManager, CreateItemResult_t> m_itemCreateResult;

	QJsonObject m_currentWorkshopProject;
	bool m_callbackPending;

public:
	~ModManager();

	static ModManager& getInstance()
	{
		// The only instance
		// Guaranteed to be lazy initialized
		// Guaranteed that it will be destroyed correctly
		static ModManager instance;
		return instance;
	}

	void init();

	void reloadMods();

	void loadMod( QJsonObject jo );
	void processRow( QString tableName, QVariantMap row );

	QStringList& modsInDir();
	QString modFolder()
	{
		return m_modFolder;
	}

	void setModActive( QString mod, bool state );

	std::vector<quint64>& subscribedMods();
	QJsonObject modInfo( quint64 modID );

	QStringList localMods();
	QJsonObject localModInfo( QString folder );

	void uploadModToSteam( QJsonObject jo );
	void updateModOnSteam( QJsonObject jo );

public slots:
	void runSteamAPICallbacks();
};
