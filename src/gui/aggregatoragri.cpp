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
/** @file aggregatoragri.cpp
 *  @brief AggregatorAgri implementation: builds the global plant/tree/animal lists at init,
 *         handles open/update/close for farm, pasture, and grove designations, and routes
 *         user-triggered setting changes back to the game-side managers.
 */
#include "aggregatoragri.h"
#include "../game/game.h"

#include "../base/counter.h"
#include "../base/db.h"
#include "../base/dbhelper.h"
#include "../base/global.h"
#include "../base/gamestate.h"
#include "../base/util.h"

#include "../game/creaturemanager.h"
#include "../game/farmingmanager.h"
#include "../game/inventory.h"
#include "../game/plant.h"
#include "../game/world.h"

#include "../gfx/spritefactory.h"

#include "../gui/strings.h"



#include <QDebug>

/// @brief Constructs the AggregatorAgri and registers all Gui*Info structs with Qt's metatype
///        system so they can cross thread boundaries via queued signal/slot connections.
/// @param parent Qt parent object.
AggregatorAgri::AggregatorAgri( QObject* parent ) :
	QObject(parent)
{
	qRegisterMetaType<GuiFarmInfo>();
	qRegisterMetaType<GuiPastureInfo>();
	qRegisterMetaType<GuiGroveInfo>();
	qRegisterMetaType<QList<GuiPlant>>();
	qRegisterMetaType<QList<GuiAnimal>>();
}

/// @brief Binds the aggregator to a specific Game instance and pre-builds the global plant,
///        tree, and pasture-animal catalogues by querying the Plants and Animals DB tables.
///        Each entry gets a small preview sprite for the GUI.
/// @param game Game instance to bind to.
void AggregatorAgri::init( Game* game )
{
	g = game;
	m_globalPlantInfo.clear();
	QStringList keys = DB::ids( "Plants" );
	keys.sort();
	for ( auto key : keys )
	{
		auto plantRow = DB::selectRow( "Plants", key );
		if ( plantRow.value( "Type" ).toString() == "Plant" )
		{
			GuiPlant gp;
			gp.plantID       = key;
			gp.seedID        = plantRow.value( "SeedItemID" ).toString();
			gp.materialID    = plantRow.value( "Material" ).toString();
			gp.seedCount     = 0;
			gp.harvestedItem = DB::select( "ItemID", "Plants_OnHarvest_HarvestedItem", key ).toString();
			gp.name          = S::s( "$MaterialName_" + gp.materialID );
			gp.sprite		 = Global::util->smallPixmap( g->sf()->createSprite( gp.harvestedItem, { gp.materialID } ), GameState::seasonString, 0 );
			m_globalPlantInfo.append( gp );

			//QIcon icon = Global::util->smallPixmap( g->sf()->createSprite( harvestedItemID, { material } ), season, 0 );
		}
	}

	m_globalTreeInfo.clear();
	keys = DB::ids( "Plants" );
	keys.sort();
	for ( auto key : keys )
	{
		auto plantRow = DB::selectRow( "Plants", key );
		if ( plantRow.value( "Type" ).toString() == "Tree" )
		{
			GuiPlant gp;
			gp.plantID       = key;
			gp.seedID        = plantRow.value( "SeedItemID" ).toString();
			gp.materialID    = plantRow.value( "Material" ).toString();
			gp.sprite        = Global::util->smallPixmap( g->sf()->createSprite( plantRow.value( "ToolButtonSprite" ).toString(), { gp.materialID } ), GameState::seasonString, 0 );
			gp.seedCount     = 0;
			gp.harvestedItem = DB::select( "ItemID", "Plants_OnHarvest_HarvestedItem", key ).toString();
			gp.name          = S::s( "$MaterialName_" + gp.materialID );
			

			m_globalTreeInfo.append( gp );

			//QIcon icon = Global::util->smallPixmap( g->sf()->createSprite( harvestedItemID, { material } ), GameState::seasonString, 0 );
		}
	}

	m_globalAnimalInfo.clear();
	keys = DB::ids( "Animals" );
	keys.sort();
	for ( auto key : keys )
	{
		auto animalRow = DB::selectRow( "Animals", key );
		if ( animalRow.value( "Pasture" ).toBool() )
		{
			GuiAnimal ga;
			ga.animalID   = key;
			ga.materialID = key;
			ga.name       = S::s( "$CreatureName_" + key );

			for ( auto sm : DB::selectRows( "Animals_States", key ) )
			{
				if ( sm.value( "ID2" ).toString() == "Adult" )
				{
					ga.sprite = Global::util->smallPixmap( g->sf()->createAnimalSprite( sm.value( "SpriteID" ).toString() ), GameState::seasonString, 0 );
					break;
				}
			}
			m_globalAnimalInfo.append( ga );
		}
	}
}

/// @brief Destructor.
AggregatorAgri::~AggregatorAgri()
{
}

/// @brief Clears the cached farm/pasture/grove IDs when the Agriculture window closes.
void AggregatorAgri::onCloseWindow()
{
	m_farmInfo.ID    = 0;
	m_pastureInfo.ID = 0;
	m_groveInfo.ID   = 0;
	m_currentTileID  = 0;
}

/// @brief Opens the Agriculture GUI for the designation at @p tileID. Detects whether the tile
///        belongs to a farm, pasture, or grove and triggers the matching update. Emits
///        signalShowAgri so the GUI shell can raise the window.
/// @param designation Tile-flag indicating the designation kind.
/// @param tileID      Integer position key of the clicked tile.
void AggregatorAgri::onOpen( TileFlag designation, unsigned int tileID )
{
	if( !g ) return;
	if ( m_currentTileID != tileID )
	{
		m_currentTileID = tileID;

		switch ( designation )
		{
			case TileFlag::TF_FARM:
			{
				auto farm = g->fm()->getFarmAtPos( Position( tileID ) );
				if ( farm )
				{
					m_farmInfo.ID    = farm->id();
					m_pastureInfo.ID = 0;
					m_groveInfo.ID   = 0;
					onUpdateFarm( m_farmInfo.ID );
				}
				break;
			}
			case TileFlag::TF_PASTURE:
			{
				auto pasture = g->fm()->getPastureAtPos( Position( tileID ) );
				if ( pasture )
				{
					m_pastureInfo.ID = pasture->id();
					m_farmInfo.ID    = 0;
					m_groveInfo.ID   = 0;
					onUpdatePasture( m_pastureInfo.ID );
				}
				break;
			}
			case TileFlag::TF_GROVE:
			{
				auto grove = g->fm()->getGroveAtPos( Position( tileID ) );
				if ( grove )
				{
					m_groveInfo.ID   = grove->id();
					m_farmInfo.ID    = 0;
					m_pastureInfo.ID = 0;
					onUpdateGrove( m_groveInfo.ID );
				}
				break;
			}
		}
	}
	emit signalShowAgri( tileID );
}

/// @brief Emits the cached global plant list for the GUI's farm-product dropdown.
void AggregatorAgri::onRequestGlobalPlantInfo()
{
	emit signalGlobalPlantInfo( m_globalPlantInfo );
}

/// @brief Emits the cached global tree list for the GUI's grove-product dropdown.
void AggregatorAgri::onRequestGlobalTreeInfo()
{
	emit signalGlobalTreeInfo( m_globalTreeInfo );
}

/// @brief Emits the cached global pasture-animal list for the GUI's pasture-product dropdown.
void AggregatorAgri::onRequestGlobalAnimalInfo()
{
	emit signalGlobalAnimalInfo( m_globalAnimalInfo );
}

/// @brief Dispatches a generic update notification to the correct per-type update handler.
/// @param designationID UID of the designation whose state changed.
void AggregatorAgri::onUpdate( unsigned int designationID )
{
	//qDebug() << "AggregatorAgri::onUpdate";
	if ( m_farmInfo.ID == designationID )
	{
		onUpdateFarm( designationID );
	}
	else if ( m_pastureInfo.ID == designationID )
	{
		onUpdatePasture( designationID );
	}
	else if ( m_groveInfo.ID == designationID )
	{
		onUpdateGrove( designationID );
	}
}

/// @brief Refreshes cached farm state for the currently open farm and emits signalUpdateFarm.
///        Also re-requests product info if the crop type changed since last refresh.
/// @param id Farm UID. Ignored if it doesn't match m_farmInfo.ID.
void AggregatorAgri::onUpdateFarm( unsigned int id )
{
	if( !g ) return;
	//qDebug() << "AggregatorAgri::onUpdateFarm";
	if ( m_farmInfo.ID == id )
	{
		auto farm = g->fm()->getFarm( id );
		if ( farm )
		{
			farm->getInfo( m_farmInfo.numPlots, m_farmInfo.tilled, m_farmInfo.planted, m_farmInfo.cropReady );
			/*
			for( auto& gp : m_globalPlantInfo )
			{
				gp.seedCount = m_inv->itemCount( gp.seedID, gp.materialID );
			}
			*/
			m_farmInfo.suspended   = farm->suspended();
			m_farmInfo.name        = farm->name();
			m_farmInfo.priority    = g->fm()->farmPriority( id );
			m_farmInfo.maxPriority = g->fm()->countFarms();
			m_farmInfo.harvest     = farm->harvest();
			m_farmInfo.plantType   = farm->plantType();

			if ( m_farmInfo.product.plantID != m_farmInfo.plantType )
			{
				onRequestProductInfo( AgriType::Farm, m_farmInfo.ID );
			}
			else
			{
				emit signalUpdateFarm( m_farmInfo );
			}
		}
	}
}

/// @brief Refreshes cached pasture state for the currently open pasture: counts, capacities,
///        food/hay levels, and the food allow-list. Always triggers a product-info request at
///        the end so the GUI receives a fully populated GuiPastureInfo with species details.
/// @param id Pasture UID. Ignored if it doesn't match m_pastureInfo.ID.
void AggregatorAgri::onUpdatePasture( unsigned int id )
{
	if( !g ) return;
	//qDebug() << "AggregatorAgri::onUpdatePasture";
	if ( m_pastureInfo.ID == id )
	{
		auto past = g->fm()->getPasture( id );
		if ( past )
		{
			m_pastureInfo.suspended   = past->suspended();
			m_pastureInfo.name        = past->name();
			m_pastureInfo.priority    = g->fm()->pasturePriority( id );
			m_pastureInfo.maxPriority = g->fm()->countPastures();
			m_pastureInfo.animalType  = past->animalType();
			m_pastureInfo.harvest     = past->harvest();
			m_pastureInfo.harvestHay  = past->harvestHay();
			m_pastureInfo.hayMax      = past->maxHay();
			m_pastureInfo.hayCurrent  = g->inv()->itemCount( "Hay", "Grass" );
			m_pastureInfo.foodMax     = past->maxFoodLevel();
			m_pastureInfo.foodCurrent = past->foodLevel();

			m_pastureInfo.food.clear();
			if ( !past->animalType().isEmpty() )
			{
				past->getInfo( m_pastureInfo.numPlots, m_pastureInfo.numMale, m_pastureInfo.numFemale );
				m_pastureInfo.animalSize = past->animalSize();
				m_pastureInfo.maxNumber  = past->maxNumber();
				m_pastureInfo.maxMale    = past->m_properties.maxMale;
				m_pastureInfo.maxFemale  = past->m_properties.maxFemale;
				m_pastureInfo.total      = m_pastureInfo.numMale + m_pastureInfo.numFemale;

				auto foodSettings = past->foodSettings();

				auto foods = DB::select( "Food", "Animals", past->animalType() ).toString();

				for( auto food : foods.split( "|" ) )
				{
					auto mats = g->inv()->materialsForItem( food, 0 );

					for( auto mat : mats )
					{
						QString name = S::s( "$MaterialName_" + mat ) + " " + S::s( "$ItemName_" + food );
						GuiPastureFoodItem pfi;
						pfi.itemSID = food;
						pfi.materialSID = mat;
						pfi.name = name;
						pfi.checked = foodSettings.contains( food + "_" + mat );
						pfi.sprite = Global::util->smallPixmap( g->sf()->createSprite( food, { mat } ), GameState::seasonString, 0 );
						m_pastureInfo.food.append( pfi );
					}
				}
			}
			else
			{
				m_pastureInfo.numPlots   = 0;
				m_pastureInfo.numMale    = 0;
				m_pastureInfo.numFemale  = 0;
				m_pastureInfo.maxMale    = 0;
				m_pastureInfo.maxFemale  = 0;
				m_pastureInfo.total      = 0;
				m_pastureInfo.maxNumber  = 0;
				m_pastureInfo.animalSize = 0;
				m_pastureInfo.hayMax     = 0;
				m_pastureInfo.hayCurrent = 0;
				m_pastureInfo.foodMax    = 0;
				m_pastureInfo.foodCurrent= 0;
			}

			onRequestProductInfo( AgriType::Pasture, m_pastureInfo.ID );
		}
	}
	else
	{
		qDebug() << "but no update";
	}
}

/// @brief Refreshes cached grove state (plot count, tree count, pick/plant/fell flags, species)
///        and emits signalUpdateGrove. Re-requests product info if the tree type changed.
/// @param id Grove UID. Ignored if it doesn't match m_groveInfo.ID.
void AggregatorAgri::onUpdateGrove( unsigned int id )
{
	if( !g ) return;
	//qDebug() << "AggregatorAgri::onUpdateGrove";
	if ( m_groveInfo.ID == id )
	{
		auto grove = g->fm()->getGrove( id );
		if ( grove )
		{
			//grove->getInfo( m_farmInfo.numPlots, m_farmInfo.tilled, m_farmInfo.planted, m_farmInfo.cropReady );
			/*
			for( auto& gp : m_globalPlantInfo )
			{
				gp.seedCount = m_inv->itemCount( gp.seedID, gp.materialID );
			}
			*/
			m_groveInfo.suspended   = grove->suspended();
			m_groveInfo.name        = grove->name();
			m_groveInfo.priority    = g->fm()->farmPriority( id );
			m_groveInfo.maxPriority = g->fm()->countFarms();

			auto props             = grove->properties();
			m_groveInfo.pickFruits = props.pickFruit;
			m_groveInfo.plantTrees = props.plant;
			m_groveInfo.fellTrees  = props.fell;
			m_groveInfo.treeType   = props.treeType;
			m_groveInfo.planted	   = grove->numTrees();
			m_groveInfo.numPlots   = grove->numPlots();

			if ( m_groveInfo.product.plantID != m_groveInfo.treeType )
			{
				onRequestProductInfo( AgriType::Grove, m_groveInfo.ID );
			}
			else
			{
				emit signalUpdateGrove( m_groveInfo );
			}
		}
	}
}

/// @brief Applies name, priority, and suspended state edits from the GUI to the appropriate
///        farm/pasture/grove object.
/// @param type          Kind of designation being edited.
/// @param designationID UID of the designation.
/// @param name          New display name.
/// @param priority      New priority index.
/// @param suspended     New suspended state.
void AggregatorAgri::onSetBasicOptions( AgriType type, unsigned int designationID, QString name, int priority, bool suspended )
{
	if( !g ) return;
	//qDebug() << "AggregatorAgri::onSetBasicOptions";
	if ( m_farmInfo.ID == designationID || m_pastureInfo.ID == designationID || m_groveInfo.ID == designationID )
	{
		switch ( type )
		{
			case AgriType::Farm:
			{
				auto farm = g->fm()->getFarm( designationID );
				if ( farm )
				{
					farm->setName( name );
					farm->setSuspended( suspended );
					g->fm()->setFarmPriority( designationID, priority );
				}
			}
			break;
			case AgriType::Pasture:
			{
				auto pasture = g->fm()->getPasture( designationID );
				if ( pasture )
				{
					pasture->setName( name );
					pasture->setSuspended( suspended );
					g->fm()->setPasturePriority( designationID, priority );
				}
			}
			break;
			case AgriType::Grove:
			{
				auto grove = g->fm()->getGrove( designationID );
				if ( grove )
				{
					grove->setName( name );
					grove->setSuspended( suspended );
					g->fm()->setGrovePriority( designationID, priority );
				}
				break;
			}
		}
	}
}

/// @brief Applies a product change (crop, animal species, or tree species) from the GUI to the
///        target designation and requests a fresh product-info payload to refresh the view.
/// @param type          Kind of designation.
/// @param designationID UID of the designation.
/// @param productSID    New product string ID (plant / animal / tree).
void AggregatorAgri::onSelectProduct( AgriType type, unsigned designationID, QString productSID )
{
	if( !g ) return;
	//collect info
	//qDebug() << "AggregatorAgri::onSelectProduct";
	if ( m_farmInfo.ID == designationID || m_pastureInfo.ID == designationID || m_groveInfo.ID == designationID )
	{
		switch ( type )
		{
			case AgriType::Farm:
			{
				auto farm = g->fm()->getFarm( designationID );
				if ( farm )
				{
					farm->setPlantType( productSID );
					m_farmInfo.plantType = productSID;
				}
			}
			break;
			case AgriType::Pasture:
			{
				auto pasture = g->fm()->getPasture( designationID );
				if ( pasture )
				{
					pasture->setAnimalType( productSID );
					m_pastureInfo.animalType = productSID;
					onUpdatePasture( designationID );
				}
			}
			break;
			case AgriType::Grove:
			{
				auto grove = g->fm()->getGrove( designationID );
				if ( grove )
				{
					grove->properties().treeType = productSID;
					m_groveInfo.treeType         = productSID;
				}
			}
			break;
		}
		onRequestProductInfo( type, designationID );
	}
}

/// @brief Applies harvest/hay/tame checkbox changes from the GUI. Farm supports harvest only;
///        pasture additionally supports harvestHay and tame wild; grove is handled separately.
/// @param type          Kind of designation.
/// @param designationID UID of the designation.
/// @param harvest       Harvest-enabled flag.
/// @param harvestHay    Hay-harvest flag (pasture only).
/// @param tame          Tame-wild flag (pasture only).
void AggregatorAgri::onSetHarvestOptions( AgriType type, unsigned int designationID, bool harvest, bool harvestHay, bool tame )
{
	if( !g ) return;
	//qDebug() << "AggregatorAgri::onSetHarvestOptions";
	if ( m_farmInfo.ID == designationID || m_pastureInfo.ID == designationID || m_groveInfo.ID == designationID )
	{
		switch ( type )
		{
			case AgriType::Farm:
			{
				auto farm = g->fm()->getFarm( designationID );
				if ( farm )
				{
					farm->setHarvest( harvest );
				}
			}
			break;
			case AgriType::Pasture:
			{
				auto past = g->fm()->getPasture( designationID );
				if ( past )
				{
					past->setHarvest( harvest );
					past->setHarvestHay( harvestHay );
					past->setTameWild( tame );
				}
			}
			break;
		}
	}
}

/// @brief Applies grove pick/plant/fell checkbox changes from the GUI to the target grove.
/// @param groveID Grove UID.
/// @param pick    True to pick fruit.
/// @param plant   True to plant new saplings.
/// @param fell    True to fell mature trees.
void AggregatorAgri::onSetGroveOptions( unsigned int groveID, bool pick, bool plant, bool fell )
{
	if( !g ) return;
	//qDebug() << "AggregatorAgri::onSetGroveOptions";
	if ( m_groveInfo.ID == groveID )
	{
		auto grove = g->fm()->getGrove( groveID );
		if ( grove )
		{
			auto& props     = grove->properties();
			props.pickFruit = pick;
			props.plant     = plant;
			props.fell      = fell;
		}
	}
}

/// @brief Collects the full GuiPlant / GuiAnimal product details (sprite, seeds, stock counts,
///        assigned animal roster) for the given designation and emits the matching
///        signalUpdateFarm / signalUpdatePasture / signalUpdateGrove.
/// @param type          Kind of designation.
/// @param designationID UID of the designation.
void AggregatorAgri::onRequestProductInfo( AgriType type, unsigned int designationID )
{
	if( !g ) return;
	//qDebug() << "AggregatorAgri::onRequestProductInfo";
	if ( m_farmInfo.ID == designationID || m_pastureInfo.ID == designationID || m_groveInfo.ID == designationID )
	{
		switch ( type )
		{
			case AgriType::Farm:
			{
				auto farm = g->fm()->getFarm( designationID );
				if ( farm )
				{
					auto plantRow = DB::selectRow( "Plants", m_farmInfo.plantType );
					GuiPlant gp;
					if ( plantRow.value( "Type" ).toString() == "Plant" )
					{
						gp.plantID    = m_farmInfo.plantType;
						gp.seedID     = plantRow.value( "SeedItemID" ).toString();
						gp.materialID = plantRow.value( "Material" ).toString();
						gp.seedCount  = g->inv()->itemCount( gp.seedID, gp.materialID );

						gp.harvestedItem = DB::select( "ItemID", "Plants_OnHarvest_HarvestedItem", m_farmInfo.plantType ).toString();
						gp.itemCount     = g->inv()->itemCount( gp.plantID, gp.materialID );

						gp.name = S::s( "$MaterialName_" + gp.materialID );

						gp.sprite = Global::util->smallPixmap( g->sf()->createSprite( gp.harvestedItem, { gp.materialID } ), GameState::seasonString, 0 );
					}
					m_farmInfo.product = gp;
					emit signalUpdateFarm( m_farmInfo );
				}
			}
			break;
			case AgriType::Pasture:
			{
				auto pasture = g->fm()->getPasture( designationID );
				if ( pasture )
				{
					auto animalRow = DB::selectRow( "Animals", m_pastureInfo.animalType );
					GuiAnimal ga;
					if ( animalRow.value( "Pasture" ).toBool() )
					{
						ga.animalID   = m_pastureInfo.animalType;
						ga.materialID = m_pastureInfo.animalType;
						ga.name       = S::s( "$CreatureName_" + m_pastureInfo.animalType );

						if ( !pasture->animalType().isEmpty() )
						{
							pasture->getInfo( m_pastureInfo.numPlots, m_pastureInfo.numMale, m_pastureInfo.numFemale );
							m_pastureInfo.animalSize = pasture->animalSize();
							m_pastureInfo.maxNumber  = pasture->maxNumber();
							m_pastureInfo.maxMale    = pasture->m_properties.maxMale;
							m_pastureInfo.maxFemale  = pasture->m_properties.maxFemale;
							m_pastureInfo.total      = m_pastureInfo.numMale + m_pastureInfo.numFemale;
						}

						for ( auto sm : DB::selectRows( "Animals_States", m_pastureInfo.animalType ) )
						{
							if ( sm.value( "ID2" ).toString() == "Adult" )
							{
								ga.sprite = Global::util->smallPixmap( g->sf()->createAnimalSprite( sm.value( "SpriteID" ).toString() ), GameState::seasonString, 0 );
								break;
							}
						}

						m_pastureInfo.animals.clear();
						for( const auto& id : pasture->animals() )
						{
							auto animal = g->cm()->animal( id );
							if( animal )
							{
								GuiPastureAnimal pa;
								pa.id = id;
								pa.name = animal->name();
								pa.isYoung = animal->isYoung();
								pa.gender = animal->gender();
								pa.toButcher = animal->toButcher();

								m_pastureInfo.animals.append( pa );
							}
						}

					}
					m_pastureInfo.product = ga;
					emit signalUpdatePasture( m_pastureInfo );
				}
			}
			break;
			case AgriType::Grove:
			{
				auto grove = g->fm()->getGrove( designationID );
				if ( grove )
				{
					auto plantRow = DB::selectRow( "Plants", m_groveInfo.treeType );
					GuiPlant gp;
					if ( plantRow.value( "Type" ).toString() == "Tree" )
					{
						gp.plantID    = m_groveInfo.treeType;
						gp.seedID     = plantRow.value( "SeedItemID" ).toString();
						gp.materialID = plantRow.value( "Material" ).toString();
						gp.seedCount  = g->inv()->itemCount( gp.seedID, gp.materialID );

						gp.harvestedItem = DB::select( "ItemID", "Plants_OnHarvest_HarvestedItem", m_groveInfo.treeType ).toString();
						gp.itemCount     = g->inv()->itemCount( gp.harvestedItem, gp.materialID );

						gp.name     = S::s( "$MaterialName_" + gp.materialID );

						auto pm = Global::util->smallPixmap( g->sf()->createSprite( plantRow.value( "ToolButtonSprite" ).toString(), { gp.materialID } ), GameState::seasonString, 0 );
						gp.sprite = pm;
					}
					m_groveInfo.product = gp;
					emit signalUpdateGrove( m_groveInfo );
				}
			}
			break;
		}
	}
}

/// @brief Applies the max-male-animals cap edit from the GUI to the target pasture.
/// @param pastureID Pasture UID.
/// @param max       New maximum male animal count.
void AggregatorAgri::onSetMaxMale( unsigned int pastureID, int max )
{
	if( !g ) return;
	if ( m_pastureInfo.ID == pastureID )
	{
		auto pasture = g->fm()->getPasture( pastureID );
		if ( pasture )
		{
			pasture->setMaxMale( max );
		}
	}
}
	
/// @brief Applies the max-female-animals cap edit from the GUI to the target pasture.
/// @param pastureID Pasture UID.
/// @param max       New maximum female animal count.
void AggregatorAgri::onSetMaxFemale( unsigned int pastureID, int max )
{
	if( !g ) return;
	if ( m_pastureInfo.ID == pastureID )
	{
		auto pasture = g->fm()->getPasture( pastureID );
		if ( pasture )
		{
			pasture->setMaxFemale( max );
		}
	}
}

/// @brief Flags or clears the butcher mark on a specific animal.
/// @param animalID Creature UID.
/// @param value    True to mark for butcher, false to clear.
void AggregatorAgri::onSetButchering( unsigned int animalID, bool value )
{
	if( !g ) return;
	//qDebug() << "AggregatorAgri::onSetButchering" << animalID << value;
	auto animal = g->cm()->animal( animalID );
	if( animal )
	{
		animal->setToButcher( value );
	}
}

/// @brief Refreshes the pasture animal roster by delegating to onRequestProductInfo().
/// @param pastureID Pasture UID.
void AggregatorAgri::onRequestPastureAnimalInfo( unsigned int pastureID )
{
	onRequestProductInfo( AgriType::Pasture, pastureID );
}

/// @brief Refreshes the pasture food allow-list by delegating to onRequestProductInfo().
/// @param pastureID Pasture UID.
void AggregatorAgri::onRequestPastureFoodInfo( unsigned int pastureID )
{
	onRequestProductInfo( AgriType::Pasture, pastureID );
}

/// @brief Adds or removes a (item, material) pair from the pasture's food allow-list.
/// @param pastureID   Pasture UID.
/// @param itemSID     Food item string ID.
/// @param materialSID Food material string ID.
/// @param checked     True to allow, false to disallow.
void AggregatorAgri::onSetFoodItemChecked( unsigned int pastureID, QString itemSID, QString materialSID, bool checked )
{
	if( !g ) return;
	if ( m_pastureInfo.ID == pastureID )
	{
		auto pasture = g->fm()->getPasture( pastureID );
		if ( pasture )
		{
			if( checked )
			{
				pasture->addFoodSetting( itemSID, materialSID );
			}
			else
			{
				pasture->removeFoodSetting( itemSID, materialSID );
			}
		}
	}
}