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

#include "../base/counter.h"
#include "../base/db.h"
#include "../base/dbhelper.h"
#include "../base/global.h"
#include "../game/creaturemanager.h"
#include "../game/farmingmanager.h"
#include "../game/inventory.h"
#include "../game/plant.h"
#include "../game/world.h"
#include "../gui/strings.h"

#include <QDebug>

AggregatorAgri::AggregatorAgri( QObject* parent )
{
	qRegisterMetaType<GuiFarmInfo>();
	qRegisterMetaType<GuiPastureInfo>();
	qRegisterMetaType<GuiGroveInfo>();
	qRegisterMetaType<QList<GuiPlant>>();
	qRegisterMetaType<QList<GuiAnimal>>();

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
			m_globalPlantInfo.append( gp );

			//QIcon icon = Util::smallPixmap( Global::sf().createSprite( harvestedItemID, { material } ), season, 0 );
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
			gp.spriteID      = plantRow.value( "ToolButtonSprite" ).toString();
			gp.seedCount     = 0;
			gp.harvestedItem = DB::select( "ItemID", "Plants_OnHarvest_HarvestedItem", key ).toString();
			gp.name          = S::s( "$MaterialName_" + gp.materialID );
			m_globalTreeInfo.append( gp );

			//QIcon icon = Util::smallPixmap( Global::sf().createSprite( harvestedItemID, { material } ), season, 0 );
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
					ga.spriteSID = sm.value( "SpriteID" ).toString();
					break;
				}
			}
			m_globalAnimalInfo.append( ga );
		}
	}

	connect( &Global::fm(), &FarmingManager::signalFarmChanged, this, &AggregatorAgri::onUpdateFarm, Qt::QueuedConnection );
	connect( &Global::fm(), &FarmingManager::signalPastureChanged, this, &AggregatorAgri::onUpdatePasture, Qt::QueuedConnection );
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
	qDebug() << "AggregatorAgri::onOpen";
	if ( m_currentTileID != tileID )
	{
		m_currentTileID = tileID;

		switch ( designation )
		{
			case TileFlag::TF_FARM:
			{
				auto farm = Global::fm().getFarmAtPos( Position( tileID ) );
				if ( farm && m_farmInfo.ID != farm->id() )
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
				auto pasture = Global::fm().getPastureAtPos( Position( tileID ) );
				if ( pasture && m_pastureInfo.ID != pasture->id() )
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
				auto grove = Global::fm().getGroveAtPos( Position( tileID ) );
				if ( grove && m_groveInfo.ID != grove->id() )
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
	qDebug() << "AggregatorAgri::onUpdate";
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
	qDebug() << "AggregatorAgri::onUpdateFarm";
	if ( m_farmInfo.ID == id )
	{
		auto farm = Global::fm().getFarm( id );
		if ( farm )
		{
			farm->getInfo( m_farmInfo.numPlots, m_farmInfo.tilled, m_farmInfo.planted, m_farmInfo.cropReady );
			/*
			for( auto& gp : m_globalPlantInfo )
			{
				gp.seedCount = Global::inv().itemCount( gp.seedID, gp.materialID );
			}
			*/
			m_farmInfo.suspended   = farm->suspended();
			m_farmInfo.name        = farm->name();
			m_farmInfo.priority    = Global::fm().farmPriority( id );
			m_farmInfo.maxPriority = Global::fm().countFarms();
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
	qDebug() << "AggregatorAgri::onUpdatePasture";
	if ( m_pastureInfo.ID == id )
	{
		auto past = Global::fm().getPasture( id );
		if ( past )
		{
			m_pastureInfo.suspended   = past->suspended();
			m_pastureInfo.name        = past->name();
			m_pastureInfo.priority    = Global::fm().pasturePriority( id );
			m_pastureInfo.maxPriority = Global::fm().countPastures();
			m_pastureInfo.animalType  = past->animalType();
			m_pastureInfo.harvest     = past->harvest();
			m_pastureInfo.harvestHay  = past->harvestHay();

			if ( !past->animalType().isEmpty() )
			{
				past->getInfo( m_pastureInfo.numPlots, m_pastureInfo.numMale, m_pastureInfo.numFemale );
				m_pastureInfo.animalSize = past->animalSize();
				m_pastureInfo.maxNumber  = past->maxNumber();
				m_pastureInfo.maxMale    = past->m_properties.maxMale;
				m_pastureInfo.maxFemale  = past->m_properties.maxFemale;
				m_pastureInfo.total      = m_pastureInfo.numMale + m_pastureInfo.numFemale;
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
			}

			onRequestProductInfo( AgriType::Pasture, m_pastureInfo.ID );
		}
	}
}

void AggregatorAgri::onUpdateGrove( unsigned int id )
{
	qDebug() << "AggregatorAgri::onUpdateGrove";
	if ( m_groveInfo.ID == id )
	{
		auto grove = Global::fm().getGrove( id );
		if ( grove )
		{
			//grove->getInfo( m_farmInfo.numPlots, m_farmInfo.tilled, m_farmInfo.planted, m_farmInfo.cropReady );
			/*
			for( auto& gp : m_globalPlantInfo )
			{
				gp.seedCount = Global::inv().itemCount( gp.seedID, gp.materialID );
			}
			*/
			m_groveInfo.suspended   = grove->suspended();
			m_groveInfo.name        = grove->name();
			m_groveInfo.priority    = Global::fm().farmPriority( id );
			m_groveInfo.maxPriority = Global::fm().countFarms();

			auto props             = grove->properties();
			m_groveInfo.pickFruits = props.pickFruit;
			m_groveInfo.plantTrees = props.plant;
			m_groveInfo.fellTrees  = props.fell;
			m_groveInfo.treeType   = props.treeType;

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
	qDebug() << "AggregatorAgri::onSetBasicOptions";
	if ( m_farmInfo.ID == designationID || m_pastureInfo.ID == designationID || m_groveInfo.ID == designationID )
	{
		switch ( type )
		{
			case AgriType::Farm:
			{
				auto farm = Global::fm().getFarm( designationID );
				if ( farm )
				{
					farm->setName( name );
					farm->setSuspended( suspended );
					Global::fm().setFarmPriority( designationID, priority );
				}
			}
			break;
			case AgriType::Pasture:
			{
				auto pasture = Global::fm().getPasture( designationID );
				if ( pasture )
				{
					pasture->setName( name );
					pasture->setSuspended( suspended );
					Global::fm().setPasturePriority( designationID, priority );
				}
			}
			break;
			case AgriType::Grove:

				break;
		}
	}
}

void AggregatorAgri::onSelectProduct( AgriType type, unsigned designationID, QString productSID )
{
	//collect info
	qDebug() << "AggregatorAgri::onSelectProduct";
	if ( m_farmInfo.ID == designationID || m_pastureInfo.ID == designationID || m_groveInfo.ID == designationID )
	{
		switch ( type )
		{
			case AgriType::Farm:
			{
				auto farm = Global::fm().getFarm( designationID );
				if ( farm )
				{
					farm->setPlantType( productSID );
					m_farmInfo.plantType = productSID;
				}
			}
			break;
			case AgriType::Pasture:
			{
				auto pasture = Global::fm().getPasture( designationID );
				if ( pasture )
				{
					pasture->setAnimalType( productSID );
					m_pastureInfo.animalType = productSID;
				}
			}
			break;
			case AgriType::Grove:
			{
				auto grove = Global::fm().getGrove( designationID );
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
	qDebug() << "AggregatorAgri::onSetHarvestOptions";
	if ( m_farmInfo.ID == designationID || m_pastureInfo.ID == designationID || m_groveInfo.ID == designationID )
	{
		switch ( type )
		{
			case AgriType::Farm:
			{
				auto farm = Global::fm().getFarm( designationID );
				if ( farm )
				{
					farm->setHarvest( harvest );
				}
			}
			break;
			case AgriType::Pasture:
			{
				auto past = Global::fm().getPasture( designationID );
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
	qDebug() << "AggregatorAgri::onSetGroveOptions";
	if ( m_groveInfo.ID == groveID )
	{
		auto grove = Global::fm().getGrove( groveID );
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
	qDebug() << "AggregatorAgri::onRequestProductInfo";
	if ( m_farmInfo.ID == designationID || m_pastureInfo.ID == designationID || m_groveInfo.ID == designationID )
	{
		switch ( type )
		{
			case AgriType::Farm:
			{
				auto farm = Global::fm().getFarm( designationID );
				if ( farm )
				{
					auto plantRow = DB::selectRow( "Plants", m_farmInfo.plantType );
					GuiPlant gp;
					if ( plantRow.value( "Type" ).toString() == "Plant" )
					{
						gp.plantID    = m_farmInfo.plantType;
						gp.seedID     = plantRow.value( "SeedItemID" ).toString();
						gp.materialID = plantRow.value( "Material" ).toString();
						gp.seedCount  = Global::inv().itemCount( gp.seedID, gp.materialID );

						gp.harvestedItem = DB::select( "ItemID", "Plants_OnHarvest_HarvestedItem", m_farmInfo.plantType ).toString();
						gp.itemCount     = Global::inv().itemCount( gp.plantID, gp.materialID );

						gp.name = S::s( "$MaterialName_" + gp.materialID );
					}
					m_farmInfo.product = gp;
					emit signalUpdateFarm( m_farmInfo );
				}
			}
			break;
			case AgriType::Pasture:
			{
				auto pasture = Global::fm().getPasture( designationID );
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
								ga.spriteSID = sm.value( "SpriteID" ).toString();
								break;
							}
						}

						m_pastureInfo.animals.clear();
						for( const auto& id : pasture->animals() )
						{
							auto animal = Global::cm().animal( id );
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
				auto grove = Global::fm().getGrove( designationID );
				if ( grove )
				{
					auto plantRow = DB::selectRow( "Plants", m_groveInfo.treeType );
					GuiPlant gp;
					if ( plantRow.value( "Type" ).toString() == "Tree" )
					{
						gp.plantID    = m_groveInfo.treeType;
						gp.seedID     = plantRow.value( "SeedItemID" ).toString();
						gp.materialID = plantRow.value( "Material" ).toString();
						gp.seedCount  = Global::inv().itemCount( gp.seedID, gp.materialID );

						gp.harvestedItem = DB::select( "ItemID", "Plants_OnHarvest_HarvestedItem", m_groveInfo.treeType ).toString();
						gp.itemCount     = Global::inv().itemCount( gp.harvestedItem, gp.materialID );

						gp.name     = S::s( "$MaterialName_" + gp.materialID );
						gp.spriteID = plantRow.value( "ToolButtonSprite" ).toString();
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
	if ( m_pastureInfo.ID == pastureID )
	{
		auto pasture = Global::fm().getPasture( pastureID );
		if ( pasture )
		{
			pasture->setMaxMale( max );
		}
	}
}
	
void AggregatorAgri::onSetMaxFemale( unsigned int pastureID, int max )
{
	if ( m_pastureInfo.ID == pastureID )
	{
		auto pasture = Global::fm().getPasture( pastureID );
		if ( pasture )
		{
			pasture->setMaxFemale( max );
		}
	}
}

void AggregatorAgri::onSetButchering( unsigned int animalID, bool value )
{
	qDebug() << "AggregatorAgri::onSetButchering" << animalID << value;
	auto animal = Global::cm().animal( animalID );
	if( animal )
	{
		animal->setToButcher( value );
	}
}

void AggregatorAgri::onRequestPastureAnimalInfo( unsigned int pastureID )
{
	onRequestProductInfo( AgriType::Pasture, pastureID );
}
