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
#include "modmanager.h"

#include "../base/config.h"
#include "../base/db.h"
#include "../base/global.h"
#include "../base/io.h"
#include "../gfx/sprite.h"
#include "../gfx/spritefactory.h"
#include "../gui/strings.h"

#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTimer>

ModManager::ModManager() :
	m_callbackPending( false )
{
	m_modFolder = QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/My Games/Ingnomia/mods/";
}

ModManager::~ModManager()
{
}

void ModManager::init()
{
	m_modFolder = QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation ) + "/My Games/Ingnomia/mods/";
	QJsonDocument jd;

	if ( IO::loadFile( m_modFolder + "activemods.json", jd ) )
	{

		QJsonArray ja = jd.array();
		for ( auto entry : ja.toVariantList() )
		{
			m_activeMods.insert( entry.toString() );
		}
	}
	m_callbackPending = false;
}

void ModManager::reloadMods()
{
	std::vector<quint64>& subscribedMods = ModManager::getInstance().subscribedMods();

	for ( auto modID : subscribedMods )
	{
		QJsonObject jo = ModManager::getInstance().modInfo( modID );

		if ( !jo.value( "Active" ).toBool() )
		{
			continue;
		}

		qDebug() << jo.value( "Title" ).toString();

		ModManager::getInstance().loadMod( jo );
	}

	for ( auto folder : ModManager::getInstance().localMods() )
	{
		QJsonObject jo = ModManager::getInstance().localModInfo( m_modFolder + "/" + folder );

		if ( !jo.value( "Active" ).toBool() )
		{
			continue;
		}

		qDebug() << jo.value( "Title" ).toString();

		ModManager::getInstance().loadMod( jo );
	}
}

void ModManager::loadMod( QJsonObject jo )
{
	QString folder = jo.value( "Folder" ).toString();
	QString type   = jo.value( "Type" ).toString();
	qDebug() << "loding mod: " << jo.value( "Title" ).toString();
	qDebug() << "folder: " << folder;
	qDebug() << "type: " << type;

	bool isTranslation = ( type == "Translation" );

	
	QJsonDocument jd;
	IO::loadFile( folder + "/files.json", jd );

	if ( jd.array().size() == 1 )
	{
		QJsonObject jo = jd.array().first().toObject();

		for ( auto entry : jo.value( "Tables" ).toArray().toVariantList() )
		{
			QString tableName = entry.toString();

			QString fileName = folder + "/tables/" + tableName + ".json";

			if ( !fileName.isEmpty() )
			{
				QJsonDocument jd;
				bool ok = IO::loadFile( fileName, jd );
				if ( !ok )
				{

					qDebug() << "Mod " << jo.value( "Title" ).toString() << " contains at least one error in json files.";
					qDebug() << "Skipping load.";
					qDebug() << fileName;

					QMessageBox msgBox;
					msgBox.setText( "Mod " + jo.value( "Title" ).toString() + " contains at least one error in json files.\nSkipping load.\n " + fileName );
					msgBox.exec();

					return;
				}
			}
			else
			{
				qDebug() << "Mod " << jo.value( "Title" ).toString() << " referenced table file doesn't exist.";
				qDebug() << fileName;

				QMessageBox msgBox;
				msgBox.setText( "Mod " + jo.value( "Title" ).toString() + " contains at least one error in json files.\nSkipping load.\n " + fileName );
				msgBox.exec();

				return;
			}
		}

		for ( auto entry : jo.value( "Tables" ).toArray().toVariantList() )
		{
			QString tableName = entry.toString();

			QString fileName = folder + "/tables/" + tableName + ".json";

			QJsonDocument jd;
			bool ok = IO::loadFile( fileName, jd );

			QJsonArray ja = jd.array();
			for ( auto rowVariant : ja.toVariantList() )
			{
				QVariantMap row = rowVariant.toMap();
				if ( isTranslation )
				{
					DB::updateRow( tableName, row );
				}
				else
				{
					processRow( tableName, row );
				}
			}
		}
		for ( auto entry : jo.value( "Tilesheets" ).toArray().toVariantList() )
		{
			QString tilesheet = entry.toString();
			QString fileName  = folder + "/tilesheets/" + tilesheet;
			Global::sf().addPixmapSource( tilesheet, fileName );
		}
		for ( auto entry : jo.value( "AI" ).toArray().toVariantList() )
		{
			auto btm         = entry.toMap();
			QString id       = btm.value( "ID" ).toString();
			QString fn       = btm.value( "File" ).toString();
			QString fileName = folder + "/ai/" + fn;
			Global::addBehaviorTree( id, fn );
		}
	}
}

void ModManager::processRow( QString tableName, QVariantMap row )
{
	QString mod = row.value( "Mod" ).toString();
	if ( mod.isEmpty() || mod == "Add" )
	{
		DB::addRow( tableName, row );
	}
	else if ( mod == "Remove" )
	{
		DB::removeRows( tableName, row.value( "ID" ).toString() );
	}
	else if ( mod == "Update" )
	{
		DB::updateRow( tableName, row );
	}
}

QStringList& ModManager::modsInDir()
{
	// iterate over mod directory
	QDir dir( m_modFolder );
	dir.setFilter( QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks );
	m_modsInDir = dir.entryList();

	return m_modsInDir;
}

void ModManager::setModActive( QString mod, bool state )
{
	qDebug() << mod << state;
	if ( state )
	{
		m_activeMods.insert( mod );
	}
	else
	{
		m_activeMods.remove( mod );
	}

	QJsonArray ja;
	for ( auto m : m_activeMods )
	{
		QJsonValue jv = m;
		ja.append( jv );
	}
	IO::saveFile( m_modFolder + "activemods.json", ja );
}

std::vector<quint64>& ModManager::subscribedMods()
{
	m_subscribedItemVec.clear();
	ISteamUGC* steam = SteamUGC();
	if ( steam )
	{
		int numSubscribedItems = steam->GetNumSubscribedItems();

		std::vector<PublishedFileId_t> itemVec;
		itemVec.resize( numSubscribedItems );
		steam->GetSubscribedItems( &itemVec[0], numSubscribedItems );

		for ( auto item : itemVec )
		{
			m_subscribedItemVec.push_back( item );
		}
	}
	return m_subscribedItemVec;
}

QJsonObject ModManager::modInfo( quint64 modID )
{
	ISteamUGC* steam = SteamUGC();
	if ( !steam )
	{
		return QJsonObject();
	}

	uint32 unItemState = steam->GetItemState( modID );
	if ( unItemState & k_EItemStateDownloading )
	{
		// indicates the item is currently downloading to the client
		qDebug() << "item downloading";
	}
	else if ( ( unItemState & k_EItemStateInstalled ) && ( unItemState & k_EItemStateNeedsUpdate ) )
	{
		// indicates the item is installed but needs to be updated
		qDebug() << "item installed, needs update";
	}
	else if ( ( unItemState & k_EItemStateInstalled ) )
	{
		// indicates the item is installed
		qDebug() << "item installed";
		uint64 punSizeOnDisk;
		char pchFolder[255];
		uint32 cchFolderSize = 255;
		uint32 punTimeStamp;

		bool success = false;
		success      = steam->GetItemInstallInfo( modID, &punSizeOnDisk, pchFolder, cchFolderSize, &punTimeStamp );

		if ( success )
		{
			qDebug() << "size on disk: " << punSizeOnDisk;
			qDebug() << "folder: " << pchFolder;
			qDebug() << "folder size: " << cchFolderSize;
			qDebug() << "time stamp: " << punTimeStamp;

			QJsonDocument jd;
			QString folder( pchFolder );
			QString metaFilename = folder + "/meta.json";
			if ( IO::loadFile( metaFilename, jd ) )
			{
				QJsonObject jo = jd.object();
				jo.insert( "Folder", folder );
				jo.insert( "Active", m_activeMods.contains( QString::number( modID ) ) );
				QString previewFile = folder + "/preview.png";
				jo.insert( "Preview", previewFile );

				return jo;
			}
		}
	}
	return QJsonObject();
}

QStringList ModManager::localMods()
{
	QDir dir( m_modFolder );
	dir.setFilter( QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks );
	m_modsInDir = dir.entryList();

	return m_modsInDir;
}

QJsonObject ModManager::localModInfo( QString folder )
{
	QJsonDocument jd;
	QString metaFilename = folder + "/meta.json";
	if ( IO::loadFile( metaFilename, jd ) )
	{
		QJsonObject jo = jd.object();
		jo.insert( "Folder", folder );
		jo.insert( "Active", m_activeMods.contains( jo.value( "Title" ).toString() ) );
		QString previewFile = folder + "/preview.png";
		jo.insert( "Preview", previewFile );
		return jo;
	}
	return QJsonObject();
}

void ModManager::uploadModToSteam( QJsonObject jo )
{
	if ( m_callbackPending )
	{
		qDebug() << "Cannot upload to steam while other operation ist still in progress.";
		return;
	}
	ISteamUGC* ugc = SteamUGC();
	if ( ugc )
	{
		m_currentWorkshopProject = jo;

		qDebug() << "upload mod " << jo.value( "Title" ).toString();
		SteamAPICall_t hSteamAPICall = ugc->CreateItem( SteamUtils()->GetAppID(), k_EWorkshopFileTypeCommunity );
		m_itemCreateResult.Set( hSteamAPICall, this, &ModManager::onCreateItemResult );

		m_callbackPending = true;
		QTimer::singleShot( 100, this, &ModManager::runSteamAPICallbacks );
	}
}

void ModManager::runSteamAPICallbacks()
{
	SteamAPI_RunCallbacks();
	if ( m_callbackPending )
	{
		QTimer::singleShot( 100, this, &ModManager::runSteamAPICallbacks );
	}
}

void ModManager::onCreateItemResult( CreateItemResult_t* pCallback, bool bIOFailure )
{
	m_callbackPending = false;
	if ( bIOFailure || !pCallback->m_eResult )
	{
		qDebug() << "NumberOfCurrentPlayers_t failed!";
		return;
	}

	qDebug() << "mod created with id " << pCallback->m_nPublishedFileId;

	ISteamUGC* ugc = SteamUGC();
	if ( ugc )
	{
		m_currentWorkshopProject.insert( "PublishedFileId", QString::number( pCallback->m_nPublishedFileId ) );
		IO::saveFile( m_currentWorkshopProject.value( "Folder" ).toString() + "/meta.json", QJsonDocument( m_currentWorkshopProject ) );

		UGCUpdateHandle_t handle = ugc->StartItemUpdate( SteamUtils()->GetAppID(), pCallback->m_nPublishedFileId );
		ugc->SetItemTitle( handle, m_currentWorkshopProject.value( "Title" ).toString().toStdString().c_str() );
		ugc->SetItemDescription( handle, m_currentWorkshopProject.value( "Description" ).toString().toStdString().c_str() );
		ugc->SetItemVisibility( handle, k_ERemoteStoragePublishedFileVisibilityPrivate );
		ugc->SetItemContent( handle, m_currentWorkshopProject.value( "Folder" ).toString().toStdString().c_str() );
		ugc->SetItemPreview( handle, m_currentWorkshopProject.value( "Preview" ).toString().toStdString().c_str() );

		SteamAPICall_t hSteamAPICall = ugc->SubmitItemUpdate( handle, "initial content upload" );
	}
}

void ModManager::updateModOnSteam( QJsonObject jo )
{

	quint64 uid = jo.value( "PublishedFileId" ).toString().toULongLong();

	qDebug() << "update mod " << uid << jo.value( "Title" ).toString();

	ISteamUGC* ugc = SteamUGC();
	if ( ugc )
	{
		UGCUpdateHandle_t handle = ugc->StartItemUpdate( SteamUtils()->GetAppID(), uid );
		ugc->SetItemTitle( handle, jo.value( "Title" ).toString().toStdString().c_str() );
		ugc->SetItemDescription( handle, jo.value( "Description" ).toString().toStdString().c_str() );
		//ugc->SetItemVisibility( handle, k_ERemoteStoragePublishedFileVisibilityPublic );
		ugc->SetItemContent( handle, jo.value( "Folder" ).toString().toStdString().c_str() );
		ugc->SetItemPreview( handle, jo.value( "Preview" ).toString().toStdString().c_str() );

		SteamAPICall_t hSteamAPICall = ugc->SubmitItemUpdate( handle, jo.value( "Changenote" ).toString().toStdString().c_str() );
	}
}