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

AggregatorAgri::AggregatorAgri( QObject* parent ) :
	QObject(parent)
{
	qRegisterMetaType<GuiFarmInfo>();
	qRegisterMetaType<GuiPastureInfo>();
	qRegisterMetaType<GuiGroveInfo>();
	qRegisterMetaType<QList<GuiPlant>>();
	qRegisterMetaType<QList<GuiAnimal>>();
}

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

AggregatorAgri::~AggregatorAgri()
{
}

void AggregatorAgri::onCloseWindow()
{
	m_farmInfo.ID    = 0;
	m_pastureInfo.ID = 0;
	m_groveInfo.ID   = 0;
	m_currentTileID  = 0;
}

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

void AggregatorAgri::onRequestGlobalPlantInfo()
{
	emit signalGlobalPlantInfo( m_globalPlantInfo );
}

void AggregatorAgri::onRequestGlobalTreeInfo()
{
	emit signalGlobalTreeInfo( m_globalTreeInfo );
}

void AggregatorAgri::onRequestGlobalAnimalInfo()
{
	emit signalGlobalAnimalInfo( m_globalAnimalInfo );
}

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

void AggregatorAgri::onRequestPastureAnimalInfo( unsigned int pastureID )
{
	onRequestProductInfo( AgriType::Pasture, pastureID );
}

void AggregatorAgri::onRequestPastureFoodInfo( unsigned int pastureID )
{
	onRequestProductInfo( AgriType::Pasture, pastureID );
}

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