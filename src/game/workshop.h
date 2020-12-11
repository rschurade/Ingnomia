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
#include "../game/worldobject.h"

#include <QMap>
#include <QVariantMap>

class Job;

#pragma region CraftQueue;
enum class CraftMode : unsigned char
{
	CraftNumber,
	CraftTo,
	Repeat
};

struct InputItem
{
	QString itemSID;
	QString materialSID;
	int amount;
	bool requireSame;
};

struct CraftJob
{
	unsigned int id    = 0; // unique id for this craft job
	unsigned int jobID = 0; // if the job is

	QString craftID;       // craft definition in DB table Crafts
	QString itemSID;       // the item to craft
	int itemsPerCraft = 1; // some recipes produce more than one item per single craft

	int numItemsToCraft = 1;
	CraftMode mode      = CraftMode::CraftNumber;
	int alreadyCrafted  = 0;

	QString skillID; // skill needed to accept this craft job

	QList<InputItem> requiredItems; // defined in DB::Crafts_Components, material is selected in the gui
	QString conversionMaterial;
	QString resultMaterial;

	int productionTime = 50;

	// crafting queue control
	bool moveToBackWhenDone = false; //move this job to the back of the queue when one item is finished
	bool paused             = false;

	CraftJob() {};
	CraftJob( QVariantMap& in );
	void serialize( QVariantMap& out );
};
#pragma endregion CraftQueue;

#pragma region Properties;
struct WorkshopProperties
{
	QString type = "";
	int rotation = 0;
	Position pos;
	Position posIn;
	Position posOut;

	QString gui;

	unsigned int owner           = 0;
	unsigned int linkedStockpile = 0;

	bool toDestroy = false;
	bool canDelete = false;

	bool noAutoGenerate = false;

	bool butcherExcess  = false;
	bool butcherCorpses = true;
	bool fish           = false;
	bool processFish    = false;

	bool showMaterials    = true;
	bool showListControls = true;
	bool acceptGenerated  = true;
	bool craftIngredients = true;

	QVariantList sourceItems;
	QVariantList itemsToSell;

	void serialize( QVariantMap& out );
	WorkshopProperties() {};
	WorkshopProperties( QVariantMap& in );
};
#pragma endregion Properties;

class Workshop : public WorldObject
{
	friend class WorkshopManager;

public:
	Workshop( QString type, Position& pos, int rotation, Game* game );
	Workshop( QVariantMap values, Game* game );
	~Workshop();

	QVariant serialize();

	void onTick( quint64 tick );

	Position pos()
	{
		return m_properties.pos;
	}
	void setPos( Position& pos )
	{
		m_properties.pos = pos;
	}

	bool isOnTile( const Position& pos );
	QString type()
	{
		return m_properties.type;
	}

	void addJob( QString craftID, int mode, int number, QStringList mats );

	void checkAutoGenerate( CraftJob cj );

	void setJobSuspended( unsigned int jobDefID, bool suspended );
	bool moveJob( unsigned int jobDefID, QString moveCmd );
	void moveJob( int pos, int newPos );
	bool autoCraft( QString itemSID, QString materialSID, int amount );

	bool setJobParams( unsigned int craftJobID, int mode, int numToCraft, bool suspended, bool moveBack );

	Job* createJobFromCraftJob( CraftJob& cj );
	Job* createButcherJob();
	Job* createDyeSheepJob();
	Job* createFisherJob();
	Job* createFishButcherJob();

	void setLinkedStockpile( bool link );
	unsigned int linkedStockpile();

	Position inputPos()
	{
		return m_properties.posIn;
	}
	Position outputPos()
	{
		return m_properties.posOut;
	}

	void setSprites( QVariantList sprites )
	{
		m_spriteComposition = sprites;
	}
	QVariantList sprites()
	{
		return m_spriteComposition;
	}

	void destroy();
	bool canDelete();

	bool outputTileFree();

	QMap<unsigned int, Position>& tiles()
	{
		return m_tiles;
	}

	QList<QPair<unsigned int, QString>>& toDyeList()
	{
		return m_toDye;
	}

	void setButcherExcess( bool value )
	{
		m_properties.butcherExcess = value;
	}
	void setButcherCorpses( bool value )
	{
		m_properties.butcherCorpses = value;
	}
	bool butcherExcess()
	{
		return m_properties.butcherExcess;
	}
	bool butcherCorpses()
	{
		return m_properties.butcherCorpses;
	}

	void setFish( bool value )
	{
		m_properties.fish = value;
	}
	void setProcessFish( bool value )
	{
		m_properties.processFish = value;
	}
	bool fish()
	{
		return m_properties.fish;
	}
	bool processFish()
	{
		return m_properties.processFish;
	}

	QList<CraftJob> jobList()
	{
		return m_jobList;
	}

	QString gui()
	{
		return m_properties.gui;
	}

	void setAcceptGenerated( bool accept );
	bool isAcceptingGenerated();

	void setAutoCraftMissing( bool autoCraft );
	bool getAutoCraftMissing();

	void setItemsToSell( QVariantList items );
	QVariantList getItemsToSell();

	void assignGnome( unsigned int gnomeID );
	unsigned int assignedGnome();

	int rotation();

	void setSourceItems( QVariantList items );
	QVariantList sourceItems();

	void checkLinkedStockpile();
	unsigned int getPossibleStockpile();

	bool noAutoGenerate();

	void cancelJob( unsigned int jobDefID );

	bool hasCraftJob( const QString& itemSID, const QString& materialSID );

private:
	//only access through workshop manager
	bool finishJob( unsigned int jobID );
	unsigned int getJob( unsigned int gnomeID, QString skillID );
	bool giveBackJob( unsigned int jobID );
	Job* getJob( unsigned int jobID );
	bool hasJobID( unsigned int jobID );

	WorkshopProperties m_properties;

	QMap<unsigned int, Position> m_tiles;
	QList<CraftJob> m_jobList;

	Job* m_job        = nullptr;
	Job* m_fishingJob = nullptr;

	QVariantList m_spriteComposition;

	bool checkItemsAvailable( Job* job );

	QList<QPair<unsigned int, QString>> m_toDye;
};
