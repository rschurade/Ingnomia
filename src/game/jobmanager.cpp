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
#include "jobmanager.h"

#include "../base/db.h"
#include "../base/enums.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/util.h"
#include "../game/farmingmanager.h"
#include "../game/gnomemanager.h"
#include "../game/inventory.h"
#include "../game/mechanismmanager.h"
#include "../game/stockpilemanager.h"
#include "../game/workshopmanager.h"
#include "../game/world.h"
#include "../gfx/sprite.h"
#include "../gfx/spritefactory.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QVariantMap>

JobManager::JobManager() :
	m_startIndex( 0 )
{
}

JobManager::~JobManager()
{
}

void JobManager::reset()
{
	m_jobList.clear();
	m_jobsPerType.clear();
	m_jobPositions.clear();
	m_returnedJobQueue.clear();
	m_skillToInt.clear();

	for ( auto job : DB::ids( "Jobs" ) )
	{
		m_jobsPerType.insert( job, QMultiMap<int, unsigned int>() );
	}

	m_jobIDs.clear();

	m_skillToInt.insert( "Mining", SK_Mining );
	m_skillToInt.insert( "Masonry", SK_Masonry );
	m_skillToInt.insert( "Stonecarving", SK_Stonecarving );
	m_skillToInt.insert( "Woodcutting", SK_Woodcutting );
	m_skillToInt.insert( "Carpentry", SK_Carpentry );
	m_skillToInt.insert( "Woodcarving", SK_Woodcarving );
	m_skillToInt.insert( "Smelting", SK_Smelting );
	m_skillToInt.insert( "Blacksmithing", SK_Blacksmithing );
	m_skillToInt.insert( "Metalworking", SK_Metalworking );
	m_skillToInt.insert( "WeaponCrafting", SK_WeaponCrafting );
	m_skillToInt.insert( "ArmorCrafting", SK_ArmorCrafting );
	m_skillToInt.insert( "Gemcutting", SK_Gemcutting );
	m_skillToInt.insert( "JewelryMaking", SK_JewelryMaking );
	m_skillToInt.insert( "Weaving", SK_Weaving );
	m_skillToInt.insert( "Tailoring", SK_Tailoring );
	m_skillToInt.insert( "Dyeing", SK_Dyeing );
	m_skillToInt.insert( "Pottery", SK_Pottery );
	m_skillToInt.insert( "Leatherworking", SK_Leatherworking );
	m_skillToInt.insert( "Bonecarving", SK_Bonecarving );
	m_skillToInt.insert( "Prospecting", SK_Prospecting );
	m_skillToInt.insert( "Tinkering", SK_Tinkering );
	m_skillToInt.insert( "Machining", SK_Machining );
	m_skillToInt.insert( "Engineering", SK_Engineering );
	m_skillToInt.insert( "Mechanic", SK_Mechanic );
	m_skillToInt.insert( "AnimalHusbandry", SK_AnimalHusbandry );
	m_skillToInt.insert( "Butchery", SK_Butchery );
	m_skillToInt.insert( "Fishing", SK_Fishing );
	m_skillToInt.insert( "Horticulture", SK_Horticulture );
	m_skillToInt.insert( "Farming", SK_Farming );
	m_skillToInt.insert( "Cooking", SK_Cooking );
	m_skillToInt.insert( "Brewing", SK_Brewing );
	m_skillToInt.insert( "Construction", SK_Construction );
	m_skillToInt.insert( "Hauling", SK_Hauling );
	m_skillToInt.insert( "Unarmed", SK_Unarmed );
	m_skillToInt.insert( "Melee", SK_Melee );
	m_skillToInt.insert( "Ranged", SK_Ranged );
	m_skillToInt.insert( "Thrown", SK_Thrown );
	m_skillToInt.insert( "Dodge", SK_Dodge );
	m_skillToInt.insert( "Block", SK_Block );
	m_skillToInt.insert( "Armor", SK_Armor );
	m_skillToInt.insert( "Crossbow", SK_Crossbow );
	m_skillToInt.insert( "Gun", SK_Gun );
	m_skillToInt.insert( "Medic", SK_Medic );
	m_skillToInt.insert( "Caretaking", SK_Caretaking );
	m_skillToInt.insert( "MagicNature", SK_MagicNature );
	m_skillToInt.insert( "MagicGeomancy", SK_MagicGeomancy );

	for ( auto skillID : DB::ids( "Skills" ) )
	{
		QStringList jobs;
		for ( auto job : DB::select2( "ID", "Jobs", "SkillID", skillID ) )
		{
			jobs.append( job.toString() );
		}
		m_jobIDs.insert( skillID, jobs );
	}
}

void JobManager::onTick()
{
	QElapsedTimer timer;
	timer.start();

	Inventory& inv     = Global::inv();
	unsigned int jobID = 0;

	QMutexLocker lock( &m_mutex );
	int queueSize = m_returnedJobQueue.size();

	QQueue<unsigned int> skippedJobs;

	while ( !m_returnedJobQueue.empty() )
	{
		//QMutexLocker lock( &m_mutex );
		jobID = m_returnedJobQueue.dequeue();
		//lock.unlock();
		--queueSize;
		Job& job = m_jobList[jobID];

		if ( workPositionWalkable( job.id() ) && requiredToolExists( job.id() ) )
		{
			if ( requiredItemsExist( jobID ) )
			{
				m_jobsPerType[job.type()].insert( job.priority(), job.id() );
			}
			else
			{
				job.setComponentMissing( true );
				skippedJobs.enqueue( jobID );
			}
		}
		else
		{
			//QMutexLocker lock( &m_mutex );
			skippedJobs.enqueue( jobID );
			//lock.unlock();
		}

		if ( timer.elapsed() > 3 )
			return;
	}
	m_returnedJobQueue += skippedJobs;
}

bool JobManager::requiredItemsExist( unsigned int jobID )
{
	Job& job = m_jobList[jobID];

	for ( auto rim : job.requiredItems() )
	{
		bool found = false;
		for ( auto pos : job.possibleWorkPositions() )
		{
			if ( Global::inv().checkReachableItems( pos, true, rim.count, rim.itemSID, rim.materialSID ) )
			{
				found = true;
				break;
			}
		}
		if ( !found )
		{
			return false;
		}
	}
	return true;
}

bool JobManager::workPositionWalkable( unsigned int jobID )
{
	if ( m_jobList.contains( jobID ) )
	{
		Job& job     = m_jobList[jobID];
		Position pos = job.pos();
		job.clearPossibleWorkPositions();
		// jobs on same tile
		auto wpl = job.origWorkPosOffsets();
		for ( auto spos : wpl )
		{
			Position offset( spos );
			Position testPos( pos + offset );
			if ( Global::w().isWalkable( testPos ) )
			{
				job.addPossibleWorkPosition( testPos );
			}
		}
		return job.possibleWorkPositions().size() > 0;
	}
	return false;
}

bool JobManager::requiredToolExists( unsigned int jobID )
{
	Job& job = m_jobList[jobID];
	auto rt  = job.requiredTool();

	if ( rt.type.isEmpty() )
	{
		return true;
	}

	QMap<QString, int> mc = Global::inv().materialCountsForItem( rt.type, false );
	QStringList keys      = mc.keys();

	for ( auto key : keys )
	{
		if ( mc[key] > 0 )
		{
			if ( rt.level == 0 )
			{
				return true;
			}
			int tl = DBH::materialToolLevel( key );
			if ( tl >= rt.level )
			{
				return true;
			}
		}
	}
	return false;
}

bool JobManager::insertIntoPositionHash( unsigned int jobID )
{
	if ( m_jobList.contains( jobID ) )
	{
		Job& job = m_jobList[jobID];
		if ( m_jobPositions.contains( job.pos().toInt() ) )
		{
			return false;
		}
		else
		{
			m_jobPositions.insert( job.pos().toInt(), job.id() );
		}
	}
	return true;
}

void JobManager::removeFromPositionHash( unsigned int jobID )
{
	if ( m_jobList.contains( jobID ) )
	{
		Job& job = m_jobList[jobID];
		if ( m_jobPositions.contains( job.pos().toInt() ) )
		{
			m_jobPositions.remove( job.pos().toInt() );
		}
	}
}

void JobManager::addLoadedJob( QVariant vals )
{
	Job job( vals.toMap() );

	m_jobList.insert( job.id(), job );

	if ( !insertIntoPositionHash( job.id() ) )
	{
		return;
	}

	if ( isReachable( job.id(), 0 ) )
	{
		m_jobsPerType[job.type()].insert( job.priority(), job.id() );
	}
	else
	{
		QMutexLocker lock( &m_mutex );
		m_returnedJobQueue.enqueue( job.id() );
	}

	setJobSprites( job.id(), job.isWorked(), false );
}

unsigned int JobManager::addJob( QString type, Position pos, int rotation, bool noJobSprite )
{
	//qDebug() << "jobManager() addJob simple" << type << pos.toString();
	if ( Global::w().hasJob( pos ) )
	{
		return 0;
	}
	QMutexLocker lock( &m_mutex );
	Job job;
	job.setType( type );
	job.setPos( pos );
	job.setRotation( rotation );
	job.setNoJobSprite( noJobSprite );

	job.setMayTrap( DB::select( "MayTrapGnome", "Jobs", type ).toBool() );
	job.setRequiredSkill( Util::requiredSkill( type ) );
	job.setRequiredTool( Util::requiredTool( type ), Util::requiredToolLevel( type, pos ) );

	QString wps = DB::select( "WorkPosition", "Jobs", job.type() ).toString();
	job.setOrigWorkPosOffsets( wps );

	m_jobList.insert( job.id(), job );

	insertIntoPositionHash( job.id() );
#if 0
	if ( isReachable( job.id(), 0 ) )
	{
		m_jobsPerType[type].push_back( job.id() );
	}
	else
	{
		QMutexLocker lock( &m_mutex );
		m_returnedJobQueue.enqueue( job.id() );
	}
#else
	m_returnedJobQueue.enqueue( job.id() );
#endif
	if ( !noJobSprite )
	{
		setJobSprites( job.id(), false, false );
	}

	return job.id();
}

unsigned int JobManager::addJob( QString type, Position pos, QString item, QList<QString> materials, int rotation, bool noJobSprite )
{
	//qDebug() << "JobManager::addJob " << type << item << materials << pos.toString();

	if ( m_jobPositions.contains( pos.toInt() ) )
	{
		return 0;
	}

	Job job;
	job.setType( type );
	job.setRequiredSkill( Util::requiredSkill( type ) );
	job.setPos( pos );
	job.setItem( item );
	job.setMaterial( materials.first() );
	job.setRotation( rotation );
	job.setNoJobSprite( noJobSprite );

	QString wps = DB::select( "WorkPosition", "Jobs", job.type() ).toString();
	job.setOrigWorkPosOffsets( wps );

	QString constructionID = DB::select( "ConstructionType", "Jobs", type ).toString();

	if ( !constructionID.isEmpty() )
	{
		QList<QVariantMap> components;
		if ( constructionID == "Workshop" )
		{
			components = DB::selectRows( "Workshops_Components", "ID", item );
		}
		else if ( constructionID == "Item" )
		{
			// items that exist as different base items before being constructed i.e. VerticleAxle is an Axle
			if ( DB::select( "Type", "Constructions", item ).toString() == "Item" )
			{
				components = DB::selectRows( "Constructions_Components", "ID", item );
			}
			else
			{
				QVariantMap comp;
				comp.insert( "ItemID", item );
				comp.insert( "Type", constructionID );
				components.push_back( comp );
			}
		}
		else
		{
			components = DB::selectRows( "Constructions_Components", "ID", item );
		}

		int cID = 0;
		for ( auto comp : components )
		{
			QString itemID = comp.value( "ItemID" ).toString();
			if ( !itemID.isEmpty() )
			{
				int amount = comp.value( "Amount" ).toInt();
				if ( amount == 0 )
				{
					amount = 1;
				}
				//if( itemID.startsWith( "$" ) )
				{
					// not used right now
				}
				//qDebug() << "require item " << itemID << materials[cID];
				int itemCount = Global::inv().itemCount( itemID, materials[cID] );
				if ( itemCount - amount < 0 )
				{
					//qDebug() << "require " << amount << " items " << itemID << materials[cID] << " there are " << itemCount << " in the world";
					Global::wsm().autoGenCraftJob( itemID, materials[cID], amount - itemCount );
				}

				job.addRequiredItem( amount, itemID, materials[cID], comp.value( "MaterialTypes" ).toStringList() );
				job.setComponentMissing( true );
				++cID;
			}
		}
	}

	else if ( type == "PlantTree" )
	{
		QVariantMap row = DB::selectRow( "Plants", item );
		//qDebug() << row.value("SeedItemID").toString() << row.value("Material").toString();
		job.addRequiredItem( 1, row.value( "SeedItemID" ).toString(), row.value( "Material" ).toString(), QStringList() );
		job.setComponentMissing( true );
	}

	m_jobList.insert( job.id(), job );

	insertIntoPositionHash( job.id() );

	QMutexLocker lock( &m_mutex );
	m_returnedJobQueue.enqueue( job.id() );

	if ( !noJobSprite )
	{
		setJobSprites( job.id(), false, false );
	}
	return job.id();
}

void JobManager::setJobAvailable( unsigned int jobID )
{
	setJobSprites( jobID, false, false );
	if ( m_jobList.contains( jobID ) )
	{
		Job& job = m_jobList[jobID];
		job.setIsWorked( false );
	}
}

void JobManager::setJobBeingWorked( unsigned int jobID, bool hasNeededTool )
{
	if ( m_jobList.contains( jobID ) )
	{
		Job& job = m_jobList[jobID];

		if ( !job.noJobSprite() )
		{
			if ( hasNeededTool )
			{
				setJobSprites( jobID, true, false );
			}
		}
		job.setIsWorked( true );
	}
}

unsigned int JobManager::getJob( QStringList skills, unsigned int gnomeID, Position& gnomePos )
{
	QMutexLocker lock( &m_mutex );

	for ( auto skillID : skills )
	{
		unsigned int jobID = 0;

		int skillInt = m_skillToInt.value( skillID );

		switch ( skillInt )
		{
			case SK_Hauling:
			{
				jobID = Global::spm().getJob();
				{
					if ( jobID )
					{
						return jobID;
					}
				}
			}
			break;
			case SK_Farming:
			case SK_AnimalHusbandry:
			case SK_Horticulture:
			case SK_Woodcutting:
			{
				jobID = Global::fm().getJob( gnomeID, { skillID } );
				{
					if ( jobID )
					{
						return jobID;
					}
				}
			}
			break;
			case SK_Machining:
			{
				jobID = Global::gm().getJob( gnomeID, skillID );
				if ( jobID )
				{
					return jobID;
				}
				jobID = Global::mcm().getJob( gnomeID, skillID );
				if ( jobID )
				{
					return jobID;
				}
			}
			break;
		}
		QElapsedTimer et;
		et.start();
		jobID = Global::wsm().getJob( gnomeID, skillID );
		if ( jobID )
		{
			return jobID;
		}
		if ( Global::debugMode )
		{
			auto ela = et.elapsed();
			if ( ela > 20 )
			{
				if ( jobID )
				{
					qDebug() << "JM WSM GETJOB" << ela << "ms"
							 << "job:" << skillID;
				}
			}
		}

		auto possibleJobIDs   = m_jobIDs.value( skillID ); //  DB::select2( "ID", "Jobs", "SkillID", skillID );
		unsigned int regionID = Global::w().regionMap().regionID( gnomePos );
		for ( int prio = 9; prio >= 0; --prio )
		{
			for ( auto jobID : possibleJobIDs )
			{
				if ( m_jobsPerType.contains( jobID ) )
				{
					QList<unsigned int> jobs = m_jobsPerType[jobID].values( prio );
					PriorityQueue<unsigned int, int> pq;

					if ( jobID == "RemoveFloor" || jobID == "BuildWall" || jobID == "DigHole" )
					{
						int walkableNeighbors = 0;
						for ( int i = 0; i < jobs.size(); ++i )
						{
							walkableNeighbors = Global::w().walkableNeighbors( m_jobList[jobs[i]].pos() );
							pq.put( jobs[i], walkableNeighbors );
						}
						while ( !pq.empty() )
						{
							Job& j = m_jobList[pq.get()];
							if ( !j.isWorked() && !j.isCanceled() )
							{
								if ( requiredItemsExist( j.id() ) && requiredToolExists( j.id() ) )
								{
									if ( isReachable( j.id(), regionID ) && !isEnclosedBySameType( j.id() ) )
									{
										//qDebug() << "getJob " <<  j->id();
										return j.id();
									}
								}
								else
								{
									for ( auto& type : m_jobsPerType )
									{
										type.remove( prio, j.id() );
									}
									m_returnedJobQueue.enqueue( j.id() );
								}
							}
						}
					}
					else
					{
						int dist = 0;
						for ( int i = 0; i < jobs.size(); ++i )
						{
							dist = m_jobList[jobs[i]].distanceSquare( gnomePos, 10 );

							if ( dist == 1 )
							{
								Job& j = m_jobList[jobs[i]];
								if ( !j.isWorked() && !j.isCanceled() )
								{
									if ( requiredItemsExist( j.id() ) && requiredToolExists( j.id() ) )
									{
										if ( isReachable( j.id(), 0 ) )
										{
											//qDebug() << "getJob " <<  j->id();
											return j.id();
										}
									}
									else
									{
										for ( auto& type : m_jobsPerType )
										{
											type.remove( prio, j.id() );
										}
										m_returnedJobQueue.enqueue( j.id() );
									}
								}
								continue;
							}
							pq.put( jobs[i], dist );
						}

						while ( !pq.empty() )
						{
							Job& j = m_jobList[pq.get()];
							if ( !j.isWorked() && !j.isCanceled() )
							{
								if ( requiredItemsExist( j.id() ) && requiredToolExists( j.id() ) )
								{
									if ( isReachable( j.id(), regionID ) )
									{
										//qDebug() << "getJob " <<  j->id();
										return j.id();
									}
								}
								else
								{
									for ( auto& type : m_jobsPerType )
									{
										type.remove( prio, j.id() );
									}
									m_returnedJobQueue.enqueue( j.id() );
								}
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

Job* JobManager::getJob( unsigned int jobID )
{
	if ( m_jobList.contains( jobID ) )
	{
		return &m_jobList[jobID];
	}
	if ( Global::spm().hasJobID( jobID ) )
	{
		return &Global::spm().getJob( jobID );
	}
	if ( Global::wsm().hasJobID( jobID ) )
	{
		return Global::wsm().getJob( jobID );
	}
	if ( Global::fm().hasJobID( jobID ) )
	{
		return Global::fm().getJob( jobID );
	}
	if ( Global::gm().hasJobID( jobID ) )
	{
		return Global::gm().getJob( jobID );
	}
	if ( Global::mcm().hasJobID( jobID ) )
	{
		return Global::mcm().getJob( jobID );
	}
	return nullptr;
}

Job* JobManager::getJobAtPos( Position pos )
{
	if ( m_jobPositions.contains( pos.toInt() ) )
	{
		unsigned jobID = m_jobPositions.value( pos.toInt() );
		return &m_jobList[jobID];
	}

	return nullptr;
}

bool JobManager::isEnclosedBySameType( unsigned int jobID )
{
	if ( m_jobList.contains( jobID ) )
	{
		Job& job     = m_jobList[jobID];
		Position pos = job.pos();
		QString type = job.type();
		if ( m_jobPositions.contains( pos.northOf().toInt() ) )
		{
			if ( m_jobPositions.contains( pos.southOf().toInt() ) )
			{
				if ( m_jobPositions.contains( pos.eastOf().toInt() ) )
				{
					if ( m_jobPositions.contains( pos.westOf().toInt() ) )
					{
						if ( m_jobList[m_jobPositions[pos.northOf().toInt()]].type() == type )
						{
							if ( m_jobList[m_jobPositions[pos.southOf().toInt()]].type() == type )
							{
								if ( m_jobList[m_jobPositions[pos.eastOf().toInt()]].type() == type )
								{
									if ( m_jobList[m_jobPositions[pos.westOf().toInt()]].type() == type )
									{
										return true;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return false;
}

bool JobManager::isReachable( unsigned int jobID, unsigned int regionID )
{
	if ( m_jobList.contains( jobID ) )
	{
		Job& job     = m_jobList[jobID];
		Position pos = job.pos();
		job.clearPossibleWorkPositions();
		// jobs on same tile
		auto wpl = job.origWorkPosOffsets();
		//qDebug() << "### get staging for " << pos.toString();
		for ( auto offset : wpl )
		{
			Position testPos( pos + offset );
			if ( Global::w().isWalkable( testPos ) )
			{
				if ( regionID == 0 )
				{
					job.addPossibleWorkPosition( testPos );
				}
				else
				{
					unsigned int workRegionID = Global::w().regionMap().regionID( testPos );
					if ( Global::w().regionMap().checkConnectedRegions( regionID, workRegionID ) )
					{
						job.addPossibleWorkPosition( testPos );
					}
				}
			}
		}
		//if ( job.possibleWorkPositions().empty() ) qDebug() << "not reachable";
		return job.possibleWorkPositions().size() > 0;
	}
	return false;
}

void JobManager::finishJob( unsigned int jobID )
{
	if ( m_jobList.contains( jobID ) )
	{
		Job& job = m_jobList[jobID];

		int rotation = job.rotation();
		//qDebug() << "finishJob " << job.id() << job.type();
		QString type = job.type();

		setJobSprites( jobID, false, true );

		std::vector<Position> neighs;
		Position pos = job.pos();

		neighs.push_back( pos.northOf() );
		neighs.push_back( pos.eastOf() );
		neighs.push_back( pos.southOf() );
		neighs.push_back( pos.westOf() );
		neighs.push_back( pos.aboveOf() );
		neighs.push_back( pos.belowOf() );

		for ( auto p : neighs )
		{
			if ( m_jobPositions.contains( p.toInt() ) )
			{
				unsigned int j = m_jobPositions.value( p.toInt() );

				if ( isReachable( j, 0 ) && !m_jobList[j].componenentMissing() )
				{
					m_jobsPerType[m_jobList[j].type()].insert( m_jobList[j].priority(), j );
				}
			}
		}

		removeFromPositionHash( jobID );

		for ( auto& type : m_jobsPerType )
		{
			type.remove( job.priority(), jobID );
		}
		m_jobList.remove( jobID );
	}
	if ( Global::wsm().finishJob( jobID ) )
	{
		return;
	}

	if ( Global::spm().finishJob( jobID ) )
	{
		return;
	}

	if ( Global::fm().finishJob( jobID ) )
	{
		return;
	}
	if ( Global::gm().finishJob( jobID ) )
	{
		return;
	}

	if ( Global::mcm().finishJob( jobID ) )
	{
		return;
	}
}

void JobManager::setJobSprites( unsigned int jobID, bool busy, bool remove )
{
	if ( m_jobList.contains( jobID ) )
	{
		Job& job     = m_jobList[jobID];
		QString type = job.type();
		int rotation = job.rotation();
		QList<QPair<Sprite*, Position>> sprites;
		QList<QPair<Sprite*, Position>> spritesInv;

		if ( type == "BuildWall" || type == "BuildFloor" || type == "BuildRamp" || type == "BuildRampCorner" || type == "BuildWorkshop" || type == "BuildItem" || type == "BuildStairs" )
		{
			QList<QVariantMap> components;
			if ( type == "BuildWall" || type == "BuildFloor" || type == "BuildRamp" || type == "BuildRampCorner" || type == "BuildStairs" )
			{
				components = DB::selectRows( "Constructions_Sprites", "ID", job.item() );
			}
			else if ( type == "BuildWorkshop" )
			{
				components = DB::selectRows( "Workshops_Components", "ID", job.item() );
			}
			else if ( type == "BuildItem" )
			{
				QVariantMap sprite;
				sprite.insert( "SpriteID", DBH::spriteID( job.item() ) );
				sprite.insert( "Offset", "0 0 0" );
				sprite.insert( "Type", "Item" );
				components.push_back( sprite );
			}
			for ( auto component : components )
			{
				auto cm = component;
				Position offset( cm.value( "Offset" ).toString() );
				int rotX = offset.x;
				int rotY = offset.y;
				switch ( rotation )
				{
					case 1:
						offset.x = -1 * rotY;
						offset.y = rotX;
						break;
					case 2:
						offset.x = -1 * rotX;
						offset.y = -1 * rotY;
						break;
					case 3:
						offset.x = rotY;
						offset.y = -1 * rotX;
						break;
				}

				if ( !cm.value( "SpriteID" ).toString().isEmpty() )
				{
					QString mat = job.material();
					if ( mat == "any" )
					{
						mat = "None";
					}
					if ( !cm.value( "SpriteIDOverride" ).toString().isEmpty() )
					{
						cm.insert( "SpriteID", cm.value( "SpriteIDOverride" ).toString() );
					}
					Sprite* sprite  = Global::sf().createSprite( cm["SpriteID"].toString(), { mat } );
					sprite->opacity = 0.5;

					bool isFloor = false;
					if ( cm.contains( "Type" ) )
					{
						if ( cm["Type"].toString() == "Floor" || cm["Type"].toString() == "StairsTop" )
						{
							isFloor = true;
						}
					}

					unsigned int spriteUID = sprite->uID;
					/*
					if( sprite->anim )
					{
						spriteUID += 2048;
					}
					*/
					if ( isFloor )
					{
						if ( remove )
						{
							Global::w().clearJobSprite( job.pos() + offset, true );
						}
						else
						{
							Global::w().setJobSprite( job.pos() + offset, spriteUID, rotation, isFloor, jobID, busy );
						}
					}
					else
					{
						QString rots = cm["WallRotation"].toString();
						int rot      = 0;
						if ( rots == "FL" )
							rot = 1;
						else if ( rots == "BL" )
							rot = 2;
						else if ( rots == "BR" )
							rot = 3;
						if ( remove )
						{
							Global::w().clearJobSprite( job.pos() + offset, false );
						}
						else
						{
							Global::w().setJobSprite( job.pos() + offset, spriteUID, ( rotation + rot ) % 4, isFloor, jobID, busy );
						}
					}
				}
				else
				{
					Sprite* sprite  = Global::sf().createSprite( "SolidSelectionFloor", { "None" } );
					sprite->opacity = 0.5;
					if ( remove )
					{
						Global::w().clearJobSprite( job.pos() + offset, true );
					}
					else
					{
						Global::w().setJobSprite( job.pos() + offset, sprite->uID, rotation, true, jobID, busy );
					}
				}
			}
		}
		else
		{

			QList<QVariantMap> spriteIds = DB::selectRows( "Jobs_SpriteID", "ID", type );

			if ( !spriteIds.empty() )
			{
				for ( auto asi : spriteIds )
				{
					QVariantMap entry = asi;
					if ( !entry.value( "SpriteID" ).toString().isEmpty() )
					{
						QString spriteID = entry["SpriteID"].toString();
						Sprite* sprite   = Global::sf().createSprite( spriteID, { "None" } );
						sprite->opacity  = 0.5;
						Position offset( 0, 0, 0 );
						if ( entry.contains( "Offset" ) )
						{
							offset = Position( entry["Offset"].toString() );
						}
						bool floor = false;
						if ( entry.contains( "Type" ) )
						{
							if ( entry["Type"].toString() == "Floor" )
							{
								floor = true;
							}
						}
						unsigned int spriteUID = sprite->uID;
						/*
						if( sprite->anim )
						{
							spriteUID += 2048;
						}
						*/
						if ( remove )
						{
							Global::w().clearJobSprite( job.pos() + offset, floor );
						}
						else
						{
							Global::w().setJobSprite( job.pos() + offset, spriteUID, rotation, floor, jobID, busy );
						}
					}
				}
			}
		}
	}
}

void JobManager::giveBackJob( unsigned int jobID )
{
	if ( m_jobList.contains( jobID ) )
	{
		Job& job = m_jobList[jobID];

		if ( job.isCanceled() || job.destroyOnAbort() )
		{
			job.setIsWorked( false );
			cancelJob( job.pos() );
			return;
		}

		//TODO clear stockpile reserve
		job.setIsWorked( false );
		job.setWorkedBy( 0 );
		job.clearPossibleWorkPositions();
		job.setComponentMissing( true );

		for ( auto& type : m_jobsPerType )
		{
			type.remove( job.priority(), jobID );
		}
		//QMutexLocker lock( &m_mutex );
		setJobSprites( jobID, false, false );
		m_returnedJobQueue.enqueue( jobID );
		//lock.unlock();
		return;
	}
	if ( Global::wsm().giveBackJob( jobID ) )
	{
		return;
	}

	if ( Global::spm().giveBackJob( jobID ) )
	{
		return;
	}

	if ( Global::fm().giveBackJob( jobID ) )
	{
		return;
	}
	if ( Global::gm().giveBackJob( jobID ) )
	{
		return;
	}

	if ( Global::mcm().giveBackJob( jobID ) )
	{
		return;
	}
}

void JobManager::cancelJob( const Position& pos )
{
	unsigned int jobID = 0;
	if ( m_jobPositions.contains( pos.toInt() ) )
	{
		jobID = m_jobPositions.value( pos.toInt() );
	}
	else
	{
		jobID = Global::w().jobSprite( pos ).value( "JobID" ).toUInt();
	}

	if ( jobID != 0 )
	{
		if ( m_jobList[jobID].isWorked() )
		{
			m_jobList[jobID].setCanceled();
		}
		else
		{
			Job& job = m_jobList[jobID];

			if ( job.type() == "SoundAlarm" )
			{
				GameState::alarm       = 0;
				GameState::alarmRoomID = 0;
			}

			setJobSprites( jobID, false, true );

			QMutexLocker lock( &m_mutex );
			removeFromPositionHash( jobID );
			for ( auto& type : m_jobsPerType )
			{
				type.remove( job.priority(), jobID );
			}
			m_jobList.remove( jobID );
		}
	}
}

QString JobManager::jobManagerInfo()
{
	QString out;

	out += QString::number( m_jobList.size() );
	return out;
}

void JobManager::raisePrio( Position& pos )
{
	unsigned int jobID = 0;
	if ( m_jobPositions.contains( pos.toInt() ) )
	{
		jobID = m_jobPositions.value( pos.toInt() );
	}
	else
	{
		jobID = Global::w().jobSprite( pos ).value( "JobID" ).toUInt();
	}

	if ( jobID != 0 )
	{
		Job& job = m_jobList[jobID];
		if ( job.priority() < 9 )
		{
			if ( m_jobsPerType[job.type()].count( job.priority(), job.id() ) )
			{
				//job is in m_jobsPerType so move it
				m_jobsPerType[job.type()].remove( job.priority(), job.id() );
				m_jobsPerType[job.type()].insert( job.priority() + 1, job.id() );
			}
			job.raisePrio();
		}
	}
}

void JobManager::lowerPrio( Position& pos )
{
	unsigned int jobID = 0;
	if ( m_jobPositions.contains( pos.toInt() ) )
	{
		jobID = m_jobPositions.value( pos.toInt() );
	}
	else
	{
		jobID = Global::w().jobSprite( pos ).value( "JobID" ).toUInt();
	}

	if ( jobID != 0 )
	{
		Job& job = m_jobList[jobID];
		if ( job.priority() > 0 )
		{
			if ( m_jobsPerType[job.type()].count( job.priority(), job.id() ) )
			{
				//job is in m_jobsPerType so move it
				m_jobsPerType[job.type()].remove( job.priority(), job.id() );
				m_jobsPerType[job.type()].insert( job.priority() - 1, job.id() );
			}
			job.lowerPrio();
		}
	}
}