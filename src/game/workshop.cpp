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
/** @file workshop.cpp
 *  Implementation of Workshop, WorkshopProperties, and CraftJob: crafting queue management, job creation, butchering, fishing, and auto-craft logic.
 */
#include "workshop.h"

#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/util.h"

#include "../game/game.h"
#include "../game/inventory.h"
#include "../game/stockpilemanager.h"
#include "../game/farmingmanager.h"
#include "../game/creaturemanager.h"
#include "../game/gnomemanager.h"

#include "../game/workshopmanager.h"

#include "../game/job.h"
#include "../game/pasture.h"
#include "../game/stockpile.h"
#include "../game/world.h"
#include "../gui/strings.h"

#include <QDebug>
#include <QJsonDocument>

/**
 * @brief Deserializes workshop properties from a saved variant map.
 * @param in Variant map containing serialized workshop properties.
 */
WorkshopProperties::WorkshopProperties( QVariantMap& in )
{
	type     = in.value( "Type" ).toString();
	rotation = in.value( "Rotation" ).toInt();
	pos      = Position( in.value( "Pos" ) );
	posIn    = Position( in.value( "PosIn" ) );
	posOut   = Position( in.value( "PosOut" ) );

	owner           = in.value( "Owner" ).toUInt();
	linkedStockpile = in.value( "LinkedStockpile" ).toUInt();

	toDestroy = in.value( "ToDestroy" ).toBool();
	canDelete = in.value( "CanDelete" ).toBool();

	butcherExcess  = in.value( "ButcherExcess" ).toBool();
	butcherCorpses = in.value( "ButcherCorpses" ).toBool();
	fish           = in.value( "Fish" ).toBool();
	processFish    = in.value( "ProcessFish" ).toBool();

	showMaterials    = in.value( "ShowMaterials" ).toBool();
	showListControls = in.value( "ShowListControls" ).toBool();
	acceptGenerated  = in.value( "AcceptGenerated" ).toBool();
	craftIngredients = in.value( "CraftIngredients" ).toBool();

	sourceMaterials = SourceMaterial::deserializeList( in.value( "SourceMaterials" ).toList() );
	itemsToSell = in.value( "ItemsToSell" ).toList();

	auto dbws = DB::workshop( type );
	if( dbws )
	{
		gui = dbws->GUI;
		noAutoGenerate = dbws->NoAutoGenerate;
		crafts = dbws->Crafts;
	}
}

/**
 * @brief Serializes workshop properties into a variant map for saving.
 * @param out Variant map to write properties into.
 */
void WorkshopProperties::serialize( QVariantMap& out )
{
	out.insert( "Type", type );
	out.insert( "Rotation", rotation );
	out.insert( "Pos", pos.toString() );
	out.insert( "PosIn", posIn.toString() );
	out.insert( "PosOut", posOut.toString() );

	out.insert( "Owner", owner );
	out.insert( "LinkedStockpile", linkedStockpile );

	out.insert( "ToDestroy", toDestroy );
	out.insert( "CanDelete", canDelete );

	out.insert( "ButcherExcess", butcherExcess );
	out.insert( "ButcherCorpses", butcherCorpses );
	out.insert( "Fish", fish );
	out.insert( "ProcessFish", processFish );

	out.insert( "ShowMaterials", showMaterials );
	out.insert( "ShowListControls", showListControls );
	out.insert( "AcceptGenerated", acceptGenerated );
	out.insert( "CraftIngredients", craftIngredients );

	out.insert( "SourceMaterials", SourceMaterial::serializeList( sourceMaterials ) );
	out.insert( "ItemsToSell", itemsToSell );
}

/**
 * @brief Deserializes a craft job from a saved variant map, restoring IDs, mode, required items, and fetching DB config.
 * @param in Variant map containing serialized craft job data.
 */
CraftJob::CraftJob( QVariantMap& in )
{
	// Restore IDs
	id            = in.value( "JobDefID" ).toUInt();
	craftID       = in.value( "CraftID" ).toString();

	// Restore user config

	QString sMode   = in.value( "Mode" ).toString();
	if ( sMode == "Craft#" )
		mode = CraftMode::CraftNumber;
	else if ( sMode == "CraftTo" )
		mode = CraftMode::CraftTo;
	else
		mode = CraftMode::Repeat;

	numItemsToCraft = in.value( "Amount" ).toInt();

	alreadyCrafted = in.value( "CraftNumberValue" ).toInt();

	auto vReqItems = in.value( "RequiredItems" ).toList();

	for ( auto vri : vReqItems )
	{
		auto vrm = vri.toMap();
		InputItem ii { vrm.value( "ItemID" ).toString(), vrm.value( "MaterialID" ).toString(), vrm.value( "Amount" ).toInt(), vrm.value( "RequireSame" ).toBool() };
		requiredItems.append( ii );
	}

	moveToBackWhenDone = in.value( "MoveToBackWhenDone" ).toBool();
	paused             = in.value( "Suspended" ).toBool();

	// Fetch config state from DB
	auto row           = DB::selectRow( "Crafts", craftID );
	itemSID            = row.value( "ItemID" ).toString();
	conversionMaterial = row.value( "ConversionMaterial" ).toString();
	resultMaterial     = row.value( "ResultMaterial" ).toString();
	itemsPerCraft      = row.value( "Amount" ).toInt();
	productionTime     = row.value( "ProductionTime" ).toInt();
	skillID            = row.value( "SkillID" ).toString();
}

/**
 * @brief Serializes the craft job into a variant map for saving.
 * @param out Variant map to write craft job data into.
 */
void CraftJob::serialize( QVariantMap& out )
{
	out.insert( "JobDefID", id );
	out.insert( "CraftID", craftID );

	out.insert( "Amount", numItemsToCraft );
	switch ( mode )
	{
		case CraftMode::CraftNumber:
			out.insert( "Mode", "Craft#" );
			break;
		case CraftMode::CraftTo:
			out.insert( "Mode", "CraftTo" );
			break;
		case CraftMode::Repeat:
			out.insert( "Mode", "Repeat" );
			break;
	}

	out.insert( "CraftNumberValue", alreadyCrafted );

	QVariantList vReqItems;
	for ( auto ri : requiredItems )
	{
		QVariantMap vmi;
		vmi.insert( "ItemID", ri.itemSID );
		vmi.insert( "MaterialID", ri.materialSID );
		vmi.insert( "Amount", ri.amount );
		vmi.insert( "RequireSame", ri.requireSame );
		vReqItems.append( vmi );
	}
	out.insert( "RequiredItems", vReqItems );

	out.insert( "MoveToBackWhenDone", moveToBackWhenDone );
	out.insert( "Suspended", paused );
}

/**
 * @brief Constructs a new workshop from a type definition, computing tile positions and input/output positions based on rotation.
 * @param type Workshop type string ID matching the DB Workshops table.
 * @param pos World position of the workshop's origin tile.
 * @param rotation Rotation (0-3) applied to component and I/O offsets.
 * @param game Pointer to the owning Game instance.
 */
Workshop::Workshop( QString type, Position& pos, int rotation, Game* game ) :
	WorldObject( game )
{
	m_properties.type     = type;
	m_properties.rotation = rotation;
	m_properties.pos      = pos;
	m_name                = S::s( "$WorkshopName_" + type );

	auto dbws = DB::workshop( type );
	if( dbws )
	{
		auto& spl = dbws->components;

		for ( auto sp : spl )
		{
			Position offset;
			offset   = sp.Offset;
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

			Position constrPos( pos + offset );
			m_tiles.insert( constrPos.toInt(), constrPos );
		}

		m_properties.gui = dbws->GUI;
		m_properties.noAutoGenerate = dbws->NoAutoGenerate;
		m_properties.crafts = dbws->Crafts;

		Position posIn  = dbws->InputTile;
		Position posOut = dbws->OutputTile;

		int inX  = posIn.x;
		int inY  = posIn.y;
		int outX = posOut.x;
		int outY = posOut.y;
		switch ( rotation )
		{
			case 1:
				posIn.x  = -1 * inY;
				posIn.y  = inX;
				posOut.x = -1 * outY;
				posOut.y = outX;
				break;
			case 2:
				posIn.x  = -1 * inX;
				posIn.y  = -1 * inY;
				posOut.x = -1 * outX;
				posOut.y = -1 * outY;
				break;
			case 3:
				posIn.x  = inY;
				posIn.y  = -1 * inX;
				posOut.x = outY;
				posOut.y = -1 * outX;
				break;
		}

		posIn  = pos + posIn;
		posOut = pos + posOut;

		m_properties.posIn  = posIn;
		m_properties.posOut = posOut;
	}
	else
	{
		// this should never be reached
	}
}

Workshop::~Workshop()
{
}

/**
 * @brief Constructs a workshop from saved game data, restoring tiles, craft queue, and active job references.
 * @param vals Variant map containing serialized workshop state.
 * @param game Pointer to the owning Game instance.
 */
Workshop::Workshop( QVariantMap vals, Game* game ) :
	WorldObject( vals, game ),
	m_properties( vals )
{
	QVariantList vtl = vals.value( "Tiles" ).toList();
	for ( auto vt : vtl )
	{
		Position pos( vt );
		m_tiles.insert( pos.toInt(), pos );
	}

	QVariantList vjl = vals.value( "JobQueue" ).toList();
	for ( auto vj : vjl )
	{
		QVariantMap vjm = vj.toMap();
		CraftJob cj( vjm );
		m_jobList.append( cj );
	}
	m_currentJobID = vals.value( "CurrentJobID" ).toUInt();
	if( m_currentJobID )
	{
		auto job = g->jm()->getJob( m_currentJobID );
		if( job )
		{
			m_currentCraftJobID = job->craftJobID();
			m_job = job;
		}
		else
		{
			m_currentJobID = 0;
			m_currentCraftJobID = 0;
		}
	}

	m_spriteComposition = vals.value( "Sprites" ).toList();
}

/**
 * @brief Serializes the entire workshop state (properties, tiles, job queue, sprites) into a QVariant for saving.
 * @return QVariant containing the serialized workshop data.
 */
QVariant Workshop::serialize()
{
	QVariantMap out;
	WorldObject::serialize( out );
	m_properties.serialize( out );

	QVariantList tiles;
	for ( auto tp : m_tiles )
	{
		QString ps = tp.toString();
		tiles.append( ps );
	}
	out.insert( "Tiles", tiles );

	QVariantList VLJobQueue;
	for ( auto cj : m_jobList )
	{
		QVariantMap vmj;
		cj.serialize( vmj );
		VLJobQueue.append( vmj );
	}
	out.insert( "JobQueue", VLJobQueue );

	out.insert( "Sprites", m_spriteComposition );

	out.insert( "CurrentJobID", m_currentJobID );

	return out;
}

/**
 * @brief Per-tick update: handles destruction, checks linked stockpile, finishes completed jobs, processes craft queue, and creates butcher/fisher jobs.
 * @param tick Current game tick number.
 */
void Workshop::onTick( quint64 tick )
{
	if ( m_properties.toDestroy )
	{
		if ( !m_job && !m_fishingJob )
		{
			m_properties.canDelete = true;
			//destroy
		}
		return;
	}

	if ( !m_active )
	{
		QSharedPointer<Job> spJob = m_job.toStrongRef();
		if ( spJob )
		{
			g->jm()->deleteJob( spJob->id() );
			m_currentCraftJobID = 0;
		}
		return;
	}

	checkLinkedStockpile();

	// job has been finished in the job manager 
	if( !m_job && m_currentCraftJobID )
	{
		if( finishJob( m_currentCraftJobID ) )
		{
			g->wsm()->emitJobListChanged( m_id );
		}
		m_currentCraftJobID = 0;
		m_currentJobID = 0;
	}

	for ( auto& cj : m_autoCraftList )
	{
		m_jobList.push_front( cj );
	}
	m_autoCraftList.clear();

	// check if the emitted job is still possible
	// ressources may have been consumed by 3rd party
	{
		QSharedPointer<Job> spJob = m_job.toStrongRef();
		if ( spJob && !spJob->isWorked() )
		{
			for ( auto& cj : m_jobList )
			{
				if ( cj.id == m_currentCraftJobID )
				{
					if ( !checkItemsAvailable( cj ) )
					{
						g->jm()->deleteJob( spJob->id() );
						m_currentCraftJobID = 0;
					}
				}
			}
		}
	}

	if ( !m_job && !m_jobList.empty() && outputTileFree() )
	{
		for ( auto& cj : m_jobList )
		{
			if ( cj.mode == CraftMode::CraftTo )
			{
				QString materialID = "any";

				if ( cj.requiredItems.size() )
				{
					auto ri    = cj.requiredItems.first();
					materialID = ri.materialSID;

					if ( !cj.conversionMaterial.isEmpty() && !cj.conversionMaterial.startsWith( "$" ) )
					{
						materialID = cj.conversionMaterial;
					}
				}

				int existing = g->inv()->itemCountWithInJob( cj.itemSID, materialID );
				if ( existing >= cj.numItemsToCraft )
				{
					cj.paused = true;
				}
				else
				{
					cj.paused = false;
				}
			}

			if ( !cj.paused )
			{
				if( checkItemsAvailable( cj ) )
				{
					if( createJobFromCraftJob( cj ) )
					{
						return;
					}
				}
			}
		}
	}

	if ( !m_job && m_properties.type == "Butcher" && outputTileFree() )
	{
		m_job = createButcherJob();
	}
	
	if ( !m_fishingJob && m_properties.type == "Fishery" && outputTileFree() && m_properties.fish )
	{
		m_fishingJob = createFisherJob();
	}

	if ( !m_job && m_properties.type == "Fishery" && outputTileFree() && m_properties.processFish )
	{
		m_job = createFishButcherJob();
	}
}

/**
 * @brief Checks whether the given position is one of this workshop's tiles.
 * @param pos Position to test.
 * @return True if the workshop occupies this tile.
 */
bool Workshop::isOnTile( const Position& pos )
{
	return m_tiles.contains( pos.toInt() );
}

/**
 * @brief Adds a new craft job to the workshop's queue, looking up recipe details from the DB.
 * @param craftID Craft recipe ID from the DB Crafts table.
 * @param mode Crafting mode (CraftNumber, CraftTo, or Repeat).
 * @param number Number of items to craft or target count.
 * @param mats List of material SIDs, one per component in the recipe.
 */
void Workshop::addJob( QString craftID, int mode, int number, QStringList mats )
{
	CraftJob cj;
	cj.id              = GameState::createID();
	cj.craftID         = craftID;
	cj.mode            = (CraftMode)mode;
	cj.numItemsToCraft = number;

	auto row              = DB::selectRow( "Crafts", craftID );
	cj.itemSID            = row.value( "ItemID" ).toString();
	cj.conversionMaterial = row.value( "ConversionMaterial" ).toString();
	cj.resultMaterial     = row.value( "ResultMaterial" ).toString();
	cj.itemsPerCraft      = row.value( "Amount" ).toInt();
	cj.productionTime     = row.value( "ProductionTime" ).toInt();
	cj.skillID            = row.value( "SkillID" ).toString();

	auto rows = DB::selectRows( "Crafts_Components", craftID );
	if ( mats.size() != rows.size() )
	{
		//something went wrong, this should never happen, but if we are here it happened
		qDebug() << "error creating craft job";
		return;
	}
	for ( int i = 0; i < rows.size(); ++i )
	{
		auto compRow = rows[i];
		InputItem ii;
		ii.itemSID     = compRow.value( "ItemID" ).toString();
		ii.amount      = compRow.value( "Amount" ).toInt();
		ii.materialSID = mats[i];
		ii.requireSame = compRow.value( "RequireSame" ).toBool();
		cj.requiredItems.append( ii );
	}
	m_jobList.append( cj );
}

/**
 * @brief Checks whether required input items for a craft job are available, and auto-generates craft orders for missing ingredients.
 * @param cj The craft job whose required items to check.
 */
void Workshop::checkAutoGenerate( CraftJob cj )
{
	if ( !m_active || m_properties.noAutoGenerate )
		return;
	auto reqItems = cj.requiredItems;
	//int numToBuild = jm.value( "CraftNumberValue" ).toInt();
	for ( auto ri : reqItems )
	{
		int avail = g->inv()->itemCount( ri.itemSID, ri.materialSID );
		if ( avail >= ri.amount )
		{
			continue;
		}
		else
		{
			g->wsm()->autoGenCraftJob( ri.itemSID, ri.materialSID, ri.amount - avail );
		}
	}
}

/**
 * @brief Moves a craft job within the queue by command string (Top, Up, Down, Bottom).
 * @param jobDefID Unique ID of the craft job to move.
 * @param moveCmd Direction command: "Top", "Up", "Down", or "Bottom".
 * @return True if the job was found and moved.
 */
bool Workshop::moveJob( unsigned int jobDefID, QString moveCmd )
{
	for ( int i = 0; i < m_jobList.size(); ++i )
	{
		if ( m_jobList[i].id == jobDefID )
		{
			if ( moveCmd == "Top" )
			{
				if ( i > 0 )
				{
					auto cj = m_jobList.takeAt( i );
					m_jobList.push_front( cj );
					return true;
				}
			}
			else if ( moveCmd == "Up" )
			{
				if ( i > 0 )
				{
					m_jobList.move( i, i - 1 );
					return true;
				}
			}
			else if ( moveCmd == "Down" )
			{
				if ( i < m_jobList.size() - 1 )
				{
					m_jobList.move( i, i + 1 );
					return true;
				}
			}
			else if ( moveCmd == "Bottom" )
			{
				if ( i < m_jobList.size() - 1 )
				{
					auto cj = m_jobList.takeAt( i );
					m_jobList.push_back( cj );
					return true;
				}
			}
		}
	}
	return false;
}

/**
 * @brief Moves a craft job from one queue position to another by index.
 * @param pos Current index in the job list.
 * @param newPos Target index in the job list.
 */
void Workshop::moveJob( int pos, int newPos )
{
	m_jobList.move( pos, newPos );
}

/**
 * @brief Updates parameters of an existing craft job in the queue.
 * @param craftJobID Unique ID of the craft job.
 * @param mode New crafting mode.
 * @param numToCraft New target item count.
 * @param suspended Whether the job should be paused.
 * @param moveBack Whether to move the job to the back of the queue when one item finishes.
 * @return True if the craft job was found and updated.
 */
bool Workshop::setJobParams( unsigned int craftJobID, int mode, int numToCraft, bool suspended, bool moveBack )
{
	for ( auto& cj : m_jobList )
	{
		if ( cj.id == craftJobID )
		{
			cj.mode               = (CraftMode)mode;
			cj.numItemsToCraft    = numToCraft;
			cj.paused             = suspended;
			cj.moveToBackWhenDone = moveBack;
			return true;
		}
	}
	return false;
}

/**
 * @brief Sets the suspended (paused) state of a craft job.
 * @param jobDefID Unique ID of the craft job.
 * @param suspended True to pause, false to resume.
 */
void Workshop::setJobSuspended( unsigned int jobDefID, bool suspended )
{
	for ( auto& cj : m_jobList )
	{
		if ( cj.id == jobDefID )
		{
			cj.paused = suspended;
			break;
		}
	}
}

/**
 * @brief Cancels and removes a craft job from the queue, also canceling any active gnome job for it.
 * @param jobDefID Unique ID of the craft job to cancel.
 */
void Workshop::cancelJob( unsigned int jobDefID )
{
	for ( int i = 0; i < m_jobList.size(); ++i )
	{
		if ( m_jobList[i].id == jobDefID )
		{
			m_jobList.removeAt( i );
			break;
		}
	}
	if ( m_job )
	{
		QSharedPointer<Job> spJob = m_job.toStrongRef();
		if ( spJob->craftJobID() == jobDefID )
		{
			QSharedPointer<Job> spJob = m_job.toStrongRef();
			g->jm()->deleteJob( spJob->id() );
			m_currentCraftJobID = 0;
		}
	}
}

/**
 * @brief Checks whether this workshop can craft the specified item with the given material.
 * @param itemSID Item string ID to craft.
 * @param materialSID Material string ID, or "any" for any material.
 * @return True if the workshop type supports crafting this item/material combination.
 */
bool Workshop::canCraft( QString itemSID, QString materialSID )
{
	if ( !m_active )
		return false;
	//test if workshop is able to build item
	const auto possibleCrafts = DBH::workshopPossibleCraftResults( type() );
	if ( !possibleCrafts.contains(itemSID) )
	{
		return false;
	}

	const auto& possibleMaterials = possibleCrafts[itemSID];

	QString craftID;
	if ( materialSID != "any" )
	{
		QString matType = Global::util->materialType( materialSID );
		if ( !possibleMaterials.contains(matType ) )
		{
			return false;
		}
		craftID = possibleMaterials.value( matType );
	}
	else
	{
		craftID = possibleMaterials.first();
	}

	if ( craftID.isEmpty() )
	{
		return false;
	}

	return true;
}

/**
 * @brief Creates an auto-generated craft job for the specified item, queued separately and merged on next tick.
 * @param itemSID Item string ID to craft.
 * @param materialSID Material string ID, or "any".
 * @param amount Number of items to craft.
 * @return True if the job was successfully created.
 */
bool Workshop::autoCraft( QString itemSID, QString materialSID, int amount )
{
	if ( !m_active )
		return false;
	//test if workshop is able to build item
	const auto possibleCrafts = DBH::workshopPossibleCraftResults( type() );
	if ( !possibleCrafts.contains(itemSID) )
	{
		return false;
	}

	const auto& possibleMaterials = possibleCrafts[itemSID];

	QString craftID;
	if ( materialSID != "any" )
	{
		QString matType = Global::util->materialType( materialSID );
		if ( !possibleMaterials.contains(matType ) )
		{
			return false;
		}
		craftID = possibleMaterials.value( matType );
	}
	else
	{
		craftID = possibleMaterials.first();
	}

	if ( craftID.isEmpty() )
	{
		return false;
	}

	// create job
	CraftJob cj;
	cj.id				  = GameState::createID();
	cj.craftID            = craftID;
	cj.mode               = CraftMode::CraftNumber;
	cj.numItemsToCraft    = amount;
	cj.moveToBackWhenDone = false;
	cj.paused             = false;

	QVariantMap cm        = DB::selectRow( "Crafts", craftID );
	cj.itemSID            = cm.value( "ItemID" ).toString();
	cj.itemsPerCraft      = cm.value( "Amount" ).toInt();
	cj.conversionMaterial = cm.value( "ConversionMaterial" ).toString();
	cj.skillID            = cm.value( "SkillID" ).toString();

	auto components = DB::selectRows( "Crafts_Components", craftID );

	for ( int i = 0; i < components.size(); ++i )
	{
		auto comp = components[i];

		QString materialID = materialSID;
		if ( i > 0 )
		{
			materialID = "any";
		}
		InputItem reqItem;
		reqItem.itemSID     = comp.value( "ItemID" ).toString();
		reqItem.materialSID = materialID;
		reqItem.amount      = qMax( 1, comp.value( "Amount" ).toInt() );
		reqItem.requireSame = comp.value( "RequireSame" ).toBool();

		cj.requiredItems.push_back( reqItem );
	}

	checkAutoGenerate( cj );

	m_autoCraftList.push_back( cj );

	return true;
}

/**
 * @brief Checks whether this workshop already has a craft job (queued or auto-generated) for the given item and material.
 * @param itemSID Item string ID to check.
 * @param materialSID Material string ID, or "any" to match any material.
 * @return True if a matching craft job exists in the queue or auto-craft list.
 */
bool Workshop::hasCraftJob( const QString& itemSID, const QString& materialSID )
{
	if( materialSID == "any" )
	{
		for( const auto& cj : m_jobList )
		{
			if( cj.itemSID == itemSID )
			{
				return true;
			}
		}
		for( const auto& cj : m_autoCraftList )
		{
			if( cj.itemSID == itemSID )
			{
				return true;
			}
		}
	}
	else
	{
		for( const auto& cj : m_jobList )
		{
			if( cj.itemSID == itemSID )
			{
				if( cj.requiredItems.size() )
				{
					const auto& reqItem = cj.requiredItems.first();
					if( reqItem.materialSID == materialSID )
					{
						return true;
					}
				}
			}
		}
		for( const auto& cj : m_autoCraftList )
		{
			if( cj.itemSID == itemSID )
			{
				if( cj.requiredItems.size() )
				{
					const auto& reqItem = cj.requiredItems.first();
					if( reqItem.materialSID == materialSID )
					{
						return true;
					}
				}
			}
		}
	}
	return false;
}

/**
 * @brief Creates an actual gnome job from a craft job definition and submits it to the job manager.
 * @param cj The craft job to create a gnome job from.
 * @return True if the job was successfully created.
 */
bool Workshop::createJobFromCraftJob( CraftJob& cj )
{
	auto jobID = g->jm()->addJob( "CraftAtWorkshop", m_properties.pos, m_properties.rotation, true );
	auto job = g->jm()->getJob( jobID );
	if( job )
	{
		m_currentJobID = jobID;
		m_currentCraftJobID = cj.id;
		job->setItem( cj.itemSID );
		job->setConversionMaterial( cj.conversionMaterial );
		job->setMaterial( cj.resultMaterial );
		job->setAmount( cj.itemsPerCraft );
		job->setRequiredSkill( cj.skillID );
		job->setPosItemInput( m_properties.posIn );
		job->setPosItemOutput( m_properties.posOut );
		job->setDescription( Global::util->itemName( cj.itemSID ) );
		job->setCraftID( cj.craftID );
		job->setCraftJobID( cj.id );
		for ( auto ri : cj.requiredItems )
		{
			job->addRequiredItem( ri.amount, ri.itemSID, ri.materialSID, {}, ri.requireSame );
		}
		m_job = job;
		return true;
	}
	return false;
}

/**
 * @brief Creates a butcher job for animals marked for butchering in connected pastures, excess tame animals, or corpses.
 * @return Shared pointer to the created job, or nullptr if no butcherable target was found.
 */
QSharedPointer<Job> Workshop::createButcherJob()
{
	for ( auto& pasture : g->fm()->allPastures() )
	{
		if ( g->pf()->checkConnectedRegions( pasture->firstPos(), m_properties.pos ) )
		{
			for ( const auto& animalID : pasture->animals() )
			{
				auto animal = g->cm()->animal( animalID );
				if ( animal )
				{
					if ( animal->toButcher() && !animal->inJob() )
					{
						if ( g->pf()->checkConnectedRegions( animal->getPos(), m_properties.pos ) )
						{
							auto jobID = g->jm()->addJob( "ButcherAnimal", animal->getPos(), 0, true );
							auto job = g->jm()->getJob( jobID );
							if( job )
							{
								job->setAnimal( animalID );
								animal->setInJob( job->id() );
								job->setPos( animal->getPos() );
								job->addPossibleWorkPosition( m_properties.pos );
								job->setPosItemInput( m_properties.posIn );
								job->setPosItemOutput( m_properties.posOut );
								return job;
							}
						}
					}
				}
			}
		}
	}

	if ( m_properties.butcherExcess )
	{
		for ( auto& a : g->cm()->animals() )
		{
			if ( a->isTame() && a->pastureID() == 0 && a->isAdult() && !a->toDestroy() && !a->isDead() && !a->inJob() )
			{
				if ( g->pf()->checkConnectedRegions( a->getPos(), m_properties.pos ) )
				{
					auto jobID = g->jm()->addJob( "ButcherAnimal", a->getPos(), 0, true );
					auto job = g->jm()->getJob( jobID );
					if( job )
					{
						job->setAnimal( a->id() );
						a->setInJob( job->id() );
						job->setPos( a->getPos() );
						job->addPossibleWorkPosition( m_properties.pos );
						job->setPosItemInput( m_properties.posIn );
						job->setPosItemOutput( m_properties.posOut );
						return job;
					}
				}
			}
		}
	}
	if ( m_properties.butcherCorpses )
	{
		QString itemID = "";
		if ( g->inv()->itemCount( "AnimalCorpse", "any" ) > 0 )
		{
			itemID = "AnimalCorpse";
		}
		else if ( g->inv()->itemCount( "GoblinCorpse", "any" ) > 0 )
		{
			itemID = "GoblinCorpse";
		}

		if ( !itemID.isEmpty() )
		{
			auto jobID = g->jm()->addJob( "ButcherCorpse", m_properties.pos, 0, true );
			auto job = g->jm()->getJob( jobID );
			if( job )
			{
				job->addPossibleWorkPosition( m_properties.pos );
				job->setPos( m_properties.pos );
				job->setPosItemInput( m_properties.posIn );
				job->setPosItemOutput( m_properties.posOut );

				int amount         = 1;
				QString materialID = "any";
				job->addRequiredItem( amount, itemID, materialID, QStringList(), false );

				return job;
			}
		}
	}
	return nullptr;
}

/**
 * @brief Creates a job to dye an animal from the workshop's to-dye list.
 * @return Shared pointer to the created job, or nullptr if no dyeable animal was found.
 */
QSharedPointer<Job> Workshop::createDyeSheepJob()
{
	for ( auto pair : m_toDye )
	{
		unsigned int animalID = pair.first;
		auto animal           = g->cm()->animal( animalID );
		if ( animal && !animal->inJob() )
		{
			if ( g->pf()->checkConnectedRegions( animal->getPos(), m_properties.pos ) )
			{
				QSharedPointer<Job> job( new Job() );
				job->setType( "DyeAnimal" );
				job->setRequiredSkill( "Dyeing" );
				job->setAnimal( animalID );
				job->addRequiredItem( 1, "Dye", pair.second, { "Dye" }, false );
				job->setMaterial( pair.second );
				animal->setInJob( job->id() );
				job->setPos( animal->getPos() );
				job->addPossibleWorkPosition( m_properties.pos );
				job->setPosItemInput( m_properties.posIn );
				job->setPosItemOutput( m_properties.posOut );

				return job;
			}
		}
	}
	return nullptr;
}

/**
 * @brief Creates a fishing job if there is sufficient water near the input tile.
 * @return Shared pointer to the created job, or nullptr if water level is too low.
 */
QSharedPointer<Job> Workshop::createFisherJob()
{
	Position pos   = m_properties.posIn;
	int waterLevel = 0;
	waterLevel += g->w()->fluidLevel( pos );
	waterLevel += g->w()->fluidLevel( pos + Position( 0, -1, -1 ) );
	waterLevel += g->w()->fluidLevel( pos + Position( 1, -1, -1 ) );
	waterLevel += g->w()->fluidLevel( pos + Position( -1, -1, -1 ) );
	waterLevel += g->w()->fluidLevel( pos + Position( 0, 0, -1 ) );
	waterLevel += g->w()->fluidLevel( pos + Position( 1, 0, -1 ) );
	waterLevel += g->w()->fluidLevel( pos + Position( -1, 0, -1 ) );
	waterLevel += g->w()->fluidLevel( pos + Position( 0, 1, -1 ) );
	waterLevel += g->w()->fluidLevel( pos + Position( 1, 1, -1 ) );
	waterLevel += g->w()->fluidLevel( pos + Position( -1, 1, -1 ) );

	if ( waterLevel > 40 )
	{
		auto jobID = g->jm()->addJob( "Fish", pos, 0, true );
		auto job = g->jm()->getJob( jobID );
		if( job )
		{
			job->setRequiredTool( "FishingRod", 0 );
			job->addPossibleWorkPosition( pos );
			job->setPosItemInput( pos );
			job->setPosItemOutput( m_properties.posOut );
			return job;
		}
	}
	return nullptr;
}

/**
 * @brief Creates a job to process (butcher) a caught fish into food items.
 * @return Shared pointer to the created job, or nullptr if no fish are available.
 */
QSharedPointer<Job> Workshop::createFishButcherJob()
{
	if ( g->inv()->itemCount( "Fish", "any" ) == 0 )
	{
		return nullptr;
	}

	auto jobID = g->jm()->addJob( "ButcherFish", m_properties.posOut, 0, true );
	auto job = g->jm()->getJob( jobID );
	if( job )
	{
		job->setRequiredSkill( "Fishing" );
		job->addPossibleWorkPosition( m_properties.pos );
		job->setPos( m_properties.posOut );
		job->setPosItemInput( m_properties.posOut );
		job->setPosItemOutput( m_properties.posOut );

		QString itemID     = "Fish";
		int amount         = 1;
		QString materialID = "any";
		job->addRequiredItem( amount, itemID, materialID, QStringList(), false );

		return job;
	}
	return nullptr;
}

/**
 * @brief Checks whether all required input items for a craft job are available in the inventory. Auto-generates craft orders for missing craftable items.
 * @param cj The craft job to check item availability for.
 * @return True if all required items are available in sufficient quantity.
 */
bool Workshop::checkItemsAvailable( CraftJob& cj )
{
	// restrictions are already determined at CraftJob creation
	//
	bool allFound = true;
	for ( int i = 0; i < cj.requiredItems.size(); ++i )
	{
		auto& ri                 = cj.requiredItems[i];

		int count               = ri.amount;
		QString itemID          = ri.itemSID;
		QString materialID      = ri.materialSID;
		bool requireSame        = ri.requireSame;

		QList<unsigned int> items;

		if( materialID == "any" && requireSame )
		{
			// will always return 0 if item count is less than needed
			ri.avail = 0;
			QList<QString> materials = g->inv()->materialsForItem( itemID, count );
			for ( auto mat : materials )
			{
				items = g->inv()->getClosestItems( m_properties.posIn, true, itemID, mat, count );
				if ( items.size() < count )
				{
					continue;
				}
				ri.avail = items.size();
				break;
			}
		}
		else
		{
			items = g->inv()->getClosestItems( m_properties.posIn, true, itemID, materialID, count );
			ri.avail = items.size();
		}

		if( ri.avail < ri.amount )
		{
			allFound = false;
			if( Global::craftable.contains( itemID ) )
			{
				g->wsm()->autoGenCraftJob( itemID, materialID, count - items.size() );
			}
		}
	}
	return allFound;
}

/**
 * @brief Handles completion of a craft job: updates crafted count, re-queues if repeating or not yet done, and triggers auto-generate for ingredients.
 * @param craftJobID Unique ID of the completed craft job.
 * @return True if the craft job was found and processed.
 */
bool Workshop::finishJob( unsigned int craftJobID )
{
	if ( !m_jobList.empty() )
	{
		// check job conditions
		for ( int i = 0; i < m_jobList.size(); ++i )
		{
			if ( craftJobID == m_jobList[i].id )
			{
				CraftJob cj = m_jobList.takeAt( i );
				cj.alreadyCrafted += cj.itemsPerCraft;

				if ( cj.mode == CraftMode::Repeat )
				{
					if ( cj.moveToBackWhenDone )
					{
						m_jobList.push_back( cj );
					}
					else
					{
						m_jobList.insert( i, cj );
					}
					if ( m_properties.craftIngredients )
					{
						checkAutoGenerate( cj );
					}
				}
				else if ( cj.mode == CraftMode::CraftNumber )
				{
					if ( cj.numItemsToCraft > cj.alreadyCrafted )
					{
						if ( cj.moveToBackWhenDone )
						{
							m_jobList.push_back( cj );
						}
						else
						{
							m_jobList.insert( i, cj );
						}
						if ( m_properties.craftIngredients )
						{
							checkAutoGenerate( cj );
						}
					}
				}
				else if ( cj.mode == CraftMode::CraftTo )
				{
					if ( cj.moveToBackWhenDone )
					{
						m_jobList.push_back( cj );
					}
					else
					{
						m_jobList.insert( i, cj );
					}
				}
				g->wsm()->emitJobListChanged( m_id );
				return true;
			}
		}
	}
	return false;
}

/**
 * @brief Marks the workshop for destruction, clearing the job list and canceling active jobs.
 */
void Workshop::destroy()
{
	m_jobList.clear();

	if ( m_job )
	{
		QSharedPointer<Job> spJob = m_job.toStrongRef();
		spJob->setCanceled();
	}
	if ( m_fishingJob )
	{
		QSharedPointer<Job> spJob = m_fishingJob.toStrongRef();
		spJob->setCanceled();
	}
	m_properties.toDestroy = true;
}

/**
 * @brief Returns whether the workshop can be safely deleted (all jobs finished after destroy was called).
 * @return True if the workshop is ready to be deleted.
 */
bool Workshop::canDelete()
{
	return m_properties.canDelete;
}

void Workshop::setLinkedStockpile( bool link )
{
	if ( link )
	{
		m_properties.linkedStockpile = getPossibleStockpile();
	}
	else
	{
		m_properties.linkedStockpile = 0;
	}
	qDebug() << "linked stockpile:" << m_properties.linkedStockpile;
}

unsigned int Workshop::getPossibleStockpile()
{
	const Position candidates[]  = {
		m_properties.posIn.northOf(),
		m_properties.posIn.westOf(),
		m_properties.posIn.southOf(),
		m_properties.posIn.eastOf(),
	};
	Position spPos;
	bool isStockpile = false;

	for ( const auto& candidate : candidates )
	{
		if ( g->w()->getTileFlag( candidate ) & TileFlag::TF_STOCKPILE )
		{
			spPos       = candidate;
			isStockpile = true;
			break;
		}
	}

	if ( isStockpile )
	{
		Stockpile* sp = g->spm()->getStockpileAtPos( spPos );
		if ( sp && sp->getFields().size() < 10 )
		{
			short xMin = Global::dimX;
			short yMin = Global::dimY;
			short xMax = 0;
			short yMax = 0;

			for ( auto spField : sp->getFields() )
			{
				Position pos = spField->pos;
				xMin         = qMin( xMin, pos.x );
				xMax         = qMax( xMax, pos.x );
				yMin         = qMin( yMin, pos.y );
				yMax         = qMax( yMax, pos.y );
			}
			if ( ( xMax - xMin ) < 3 && ( yMax - yMin ) < 3 )
			{
				return sp->id();
			}
		}
	}
	return 0;
}

unsigned int Workshop::linkedStockpile()
{
	return m_properties.linkedStockpile;
}

bool Workshop::outputTileFree()
{
	Position pos = outputPos();
	//TODO make number configurable

	bool isFree = g->inv()->countItemsAtPos( pos ) < 20;

	if ( !isFree )
	{
		bool alreadySet = g->w()->getTileFlag( m_tiles.first() ) & TileFlag::TF_BLOCKED;

		if ( !alreadySet )
		{
			for ( auto pos : m_tiles )
			{
				g->w()->setTileFlag( pos, TileFlag::TF_BLOCKED );
			}
		}
	}
	else
	{
		bool alreadySet = g->w()->getTileFlag( m_tiles.first() ) & TileFlag::TF_BLOCKED;

		if ( alreadySet )
		{
			for ( auto pos : m_tiles )
			{
				g->w()->clearTileFlag( pos, TileFlag::TF_BLOCKED );
			}
		}
	}

	return ( isFree );
}

void Workshop::setAcceptGenerated( bool accept )
{
	m_properties.acceptGenerated = accept;
}

bool Workshop::isAcceptingGenerated()
{
	return m_properties.acceptGenerated && !m_properties.noAutoGenerate;
}

void Workshop::setAutoCraftMissing( bool autoCraft )
{
	m_properties.craftIngredients = autoCraft;
}

bool Workshop::getAutoCraftMissing()
{
	return m_properties.craftIngredients;
}

void Workshop::setItemsToSell( QVariantList items )
{
	m_properties.itemsToSell = items;
}

QVariantList Workshop::getItemsToSell()
{
	return m_properties.itemsToSell;
}

void Workshop::assignGnome( unsigned int gnomeID )
{
	m_properties.owner = gnomeID;
	if ( gnomeID == 0 )
	{
		//m_properties.remove( "TraderInventory" );
	}
}

unsigned int Workshop::assignedGnome()
{
	return m_properties.owner;
}

int Workshop::rotation()
{
	return m_properties.rotation;
}

void Workshop::setSourceItems( QVariantList items )
{
	for ( const auto& item : items )
	{
		unsigned int uid = item.toUInt();
		m_properties.sourceMaterials.append( SourceMaterial(
			g->inv()->itemSID( uid ),
			g->inv()->materialSID( uid ),
			g->inv()->quality( uid )
		) );
		// Do NOT pickUpItem or setConstructed.
		// canwork.cpp post-loop will destroy these items automatically.
	}
}

const QList<SourceMaterial>& Workshop::getSourceMaterials() const
{
	return m_properties.sourceMaterials;
}

bool Workshop::noAutoGenerate()
{
	return m_properties.noAutoGenerate;
}

void Workshop::checkLinkedStockpile()
{
	if ( linkedStockpile() )
	{
		Stockpile* sp = g->spm()->getStockpile( linkedStockpile() );
		if ( sp )
		{
			if ( sp->countFields() > 9 )
			{
				setLinkedStockpile( 0 );
			}
		}
		else
		{
			setLinkedStockpile( 0 );
		}
	}
}