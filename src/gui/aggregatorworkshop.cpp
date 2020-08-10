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
#include "aggregatorworkshop.h"

#include "../base/counter.h"
#include "../base/db.h"
#include "../base/dbhelper.h"
#include "../base/global.h"
#include "../base/util.h"
#include "../game/creaturemanager.h"
#include "../game/gnomemanager.h"
#include "../game/gnometrader.h"
#include "../game/inventory.h"
#include "../game/workshopmanager.h"
#include "../game/world.h"
#include "../gui/strings.h"

#include <QDebug>

void GuiWorkshopComponent::updateMaterials( QVariantMap row )
{
	materials.clear();

	auto mats = Global::inv().materialCountsForItem( sid );

	auto allowedMats = Util::possibleMaterials( row.value( "AllowedMaterial" ).toString(), row.value( "AllowedMaterialType" ).toString() );
	if ( !allowedMats.isEmpty() )
	{
		for ( auto mat : allowedMats )
		{
			materials.append( GuiWorkshopMaterial { mat, mats[mat] } );
		}
		return;
	}
	QString anyString = "any";
	if ( row.value( "RequireSame" ).toBool() )
	{
		anyString += " (same)";
	}
	materials.append( GuiWorkshopMaterial { anyString, mats["any"] } );

	mats.remove( "any" );
	for ( auto key : mats.keys() )
	{
		materials.append( GuiWorkshopMaterial { key, mats[key] } );
	}
}

void GuiWorkshopProduct::updateComponents()
{
	components.clear();

	for ( auto row : DB::selectRows( "Crafts_Components", sid ) )
	{
		GuiWorkshopComponent gwp { row.value( "ItemID" ).toString(), row.value( "Amount" ).toInt(), row.value( "RequireSame" ).toBool() };
		gwp.updateMaterials( row );
		components.append( gwp );
	}
}

AggregatorWorkshop::AggregatorWorkshop( QObject* parent )
{
	qRegisterMetaType<GuiWorkshopInfo>();

	connect( &Global::wsm(), &WorkshopManager::signalJobListChanged, this, &AggregatorWorkshop::onCraftListChanged, Qt::QueuedConnection );
}

AggregatorWorkshop::~AggregatorWorkshop()
{
}

void AggregatorWorkshop::onOpenWorkshopInfoOnTile( unsigned int tileID )
{
	Position pos( tileID );
	auto ws = Global::wsm().workshopAt( pos );
	if ( ws )
	{
		m_info.workshopID = ws->id();
		emit signalOpenWorkshopWindow( ws->id() );
		onUpdateWorkshopInfo( ws->id() );
	}
}

void AggregatorWorkshop::onOpenWorkshopInfo( unsigned int workshopID )
{
	m_info.workshopID = workshopID;
	emit signalOpenWorkshopWindow( workshopID );
	onUpdateWorkshopInfo( workshopID );
}

void AggregatorWorkshop::onUpdateWorkshopInfo( unsigned int workshopID )
{
	if ( aggregate( workshopID ) )
	{
		emit signalUpdateInfo( m_info );
	}
}

bool AggregatorWorkshop::aggregate( unsigned int workshopID )
{
	if ( m_info.workshopID == workshopID )
	{
		auto ws = Global::wsm().workshop( workshopID );
		if ( ws )
		{
			qDebug() << workshopID << ws->name();

			m_info.name             = ws->name();
			m_info.priority         = Global::wsm().priority( workshopID );
			m_info.maxPriority      = Global::wsm().maxPriority();
			m_info.suspended        = !ws->active();
			m_info.gui              = ws->gui();
			m_info.acceptGenerated  = ws->isAcceptingGenerated();
			m_info.autoCraftMissing = ws->getAutoCraftMissing();
			m_info.linkStockpile    = (bool)ws->linkedStockpile();

			m_info.products.clear();

			if ( m_info.gui.isEmpty() )
			{
				auto crafts = DB::select( "Crafts", "Workshops", ws->type() ).toString();

				for ( auto craft : crafts.split( "|" ) )
				{
					GuiWorkshopProduct gwp;
					gwp.sid = craft;
					gwp.updateComponents();
					m_info.products.append( gwp );
				}
			}
			else if ( m_info.gui == "Butcher" )
			{
				m_info.butcherExcess  = ws->butcherExcess();
				m_info.butcherCorpses = ws->butcherCorpses();
			}

			m_info.jobList = ws->jobList();
			return true;
		}
	}
	return false;
}

bool AggregatorWorkshop::updateCraftList( unsigned int workshopID )
{
	if ( m_info.workshopID == workshopID )
	{
		auto ws = Global::wsm().workshop( workshopID );
		if ( ws )
		{
			m_info.jobList = ws->jobList();
			emit signalUpdateCraftList( m_info );

			return true;
		}
	}
	return false;
}

void AggregatorWorkshop::onCraftListChanged( unsigned int workshopID )
{
	updateCraftList( workshopID );
}

void AggregatorWorkshop::onUpdateWorkshopContent( unsigned int WorkshopID )
{
	if ( m_info.workshopID == WorkshopID )
	{
		m_contentDirty = true;
	}
}

void AggregatorWorkshop::onUpdateAfterTick()
{
	if ( m_info.workshopID && m_contentDirty )
	{
		auto sp = Global::wsm().workshop( m_info.workshopID );
		emit signalUpdateContent( m_info );
		m_contentDirty = false;
	}
}

void AggregatorWorkshop::onSetBasicOptions( unsigned int workshopID, QString name, int priority, bool suspended, bool acceptGenerated, bool autoCraftMissing, bool connectStockpile )
{
	auto ws = Global::wsm().workshop( workshopID );
	if ( ws )
	{
		//qDebug() << WorkshopID << name << priority << suspended;
		ws->setName( name );
		Global::wsm().setPriority( workshopID, priority );
		ws->setActive( !suspended );
		ws->setAcceptGenerated( acceptGenerated );
		ws->setAutoCraftMissing( autoCraftMissing );
		ws->setLinkedStockpile( connectStockpile );
	}
}

void AggregatorWorkshop::onSetButcherOptions( unsigned int workshopID, bool butcherCorpses, bool butcherExcess )
{
	auto ws = Global::wsm().workshop( workshopID );
	if ( ws )
	{
		ws->setButcherCorpses( butcherCorpses );
		ws->setButcherExcess( butcherExcess );
	}
}

void AggregatorWorkshop::onCraftItem( unsigned int workshopID, QString craftID, int mode, int number, QStringList mats )
{
	qDebug() << "onCraftItem" << workshopID << craftID << mode << number << mats;
	auto ws = Global::wsm().workshop( workshopID );
	if ( ws )
	{
		ws->addJob( craftID, mode, number, mats );
		updateCraftList( workshopID );
	}
}

void AggregatorWorkshop::onCloseWindow()
{
	m_info.workshopID = 0;
}

void AggregatorWorkshop::onCraftJobCommand( unsigned int workshopID, unsigned int craftJobID, QString command )
{
	auto ws = Global::wsm().workshop( workshopID );
	if ( ws )
	{
		if ( command == "Cancel" )
		{
			ws->cancelJob( craftJobID );
			updateCraftList( workshopID );
		}
		else if ( command == "Up" || command == "Top" || command == "Down" || command == "Bottom" )
		{
			if ( ws->moveJob( craftJobID, command ) )
			{
				updateCraftList( workshopID );
			}
		}
	}
}

void AggregatorWorkshop::onCraftJobParams( unsigned int workshopID, unsigned int craftJobID, int mode, int numToCraft, bool suspended, bool moveBack )
{
	auto ws = Global::wsm().workshop( workshopID );
	if ( ws )
	{
		ws->setJobParams( craftJobID, mode, numToCraft, suspended, moveBack );
	}
}

void AggregatorWorkshop::onRequestAllTradeItems( unsigned int workshopID )
{
	updateTraderStock( workshopID );
	
	updatePlayerStock( workshopID );
}

void AggregatorWorkshop::updateTraderStock( unsigned int workshopID )
{
	auto workshop = Global::wsm().workshop( workshopID );

	if( workshop )
	{
		unsigned int traderID = workshop->assignedGnome();

		m_traderID = traderID;

		m_traderStock.clear();

		if( traderID )
		{
			GnomeTrader* gt = Global::gm().trader( traderID );
			if( gt )
			{
				auto& items = gt->inventory();
					
				updateTraderStock( items );

				emit signalTraderStock( m_traderStock );

				updateTraderValue();
			}
		}
	}
}

void AggregatorWorkshop::updateTraderStock( const QList<TraderItem>& source )
{
	m_traderStock.clear();

	for( auto item : source )
	{
		GuiTradeItem gti;
		gti.itemSID = item.itemSID;
		gti.value = item.value;
		gti.count = item.amount;
		gti.reserved = item.reserved;
		gti.quality = item.quality;

		if( item.type == "Animal" )
		{
			gti.materialSIDorGender = ( item.gender == Gender::MALE ) ? "Male" : "Female";
			gti.name = 	gti.materialSIDorGender + " " + S::s( "$CreatureName_" + gti.itemSID ) + " (" + QString::number( gti.value ) + ")";
		}
		else
		{
			gti.materialSIDorGender = item.materialSID;
			
			QString qName;
			QString qID;
			if( DB::select( "HasQuality", "Items", gti.itemSID ).toBool() )
			{
				qID = DBH::qualitySID( 3 );
				qName= S::s( "$QualityName_" + qID ) + " ";
			}

			gti.name = qName + S::s( "$MaterialName_" + gti.materialSIDorGender ) + " " + S::s( "$ItemName_" + gti.itemSID ) + " (" + QString::number( gti.value ) + ")";
		}
		m_traderStock.append( gti );
	}
}
	
void AggregatorWorkshop::updatePlayerStock( unsigned int workshopID )
{
	m_playerStock.clear();

	Inventory& inv = Global::inv();

	for( auto category : inv.categories() )
	{
		for( auto group : inv.groups( category ) )
		{
			for( auto item : inv.items( category, group ) )
			{
				QList<QString> mats = inv.materials( category, group, item );
				for( auto mat : mats )
				{
					int count = inv.itemCountInStockpile( item, mat );
					if( count > 0 )
					{
						QList<unsigned int> itemList = inv.tradeInventory( item, mat );

						Counter<unsigned int> counter;
						QMap<unsigned int, unsigned int>values;
						QMap<int, QList<unsigned int>> vlItems;
						for( auto itemUID : itemList )
						{
							int qual = inv.quality( itemUID );
							counter.add( qual );
							vlItems[qual].append( itemUID );
						}
						for( auto key : counter.keys() )
						{
							GuiTradeItem gti;

							QString qName;
							QString qID;
							if( DB::select( "HasQuality", "Items", item ).toBool() )
							{
								qID = DBH::qualitySID( (int)key );
								qName= S::s( "$QualityName_" + qID ) + " ";
							}

							gti.itemSID = item;
							gti.materialSIDorGender = mat;
							gti.value = inv.getTradeValue( item, mat, key );
							gti.quality = key; 
							gti.count = count;

							gti.name = qName + S::s( "$MaterialName_" + mat ) + " " + S::s( "$ItemName_" + item ) + " (" + QString::number( gti.value ) + ")";
							
							m_playerStock.append( gti );
						}
					}
				}
			}
		}
	}
	emit signalPlayerStock( m_playerStock );

}

void AggregatorWorkshop::onTraderStocktoOffer( unsigned int workshopID, QString itemSID, QString materialSID, unsigned char quality, int count )
{
	auto workshop = Global::wsm().workshop( workshopID );

	if( workshop )
	{
		unsigned int traderID = workshop->assignedGnome();
		
		m_traderID = traderID;

		if( traderID )
		{
			GnomeTrader* gt = Global::gm().trader( traderID );
			if( gt )
			{
				auto& items = gt->inventory();
					
				for( auto& item : items )
				{
					if( item.itemSID == itemSID && item.materialSID == materialSID && item.quality == quality )
					{
						item.reserved = qMin( item.amount, item.reserved + count );

						for( auto& gti : m_traderStock )
						{
							if( gti.itemSID == itemSID && gti.materialSIDorGender == materialSID && gti.quality == quality )
							{
								gti.reserved = item.reserved;
								emit signalUpdateTraderStockItem( gti );
								updateTraderValue();
								break;
							}
						}
						break;
					}
				}
			}
		}
	}
}
	
void AggregatorWorkshop::onTraderOffertoStock( unsigned int workshopID, QString itemSID, QString materialSID, unsigned char quality, int count )
{
	auto workshop = Global::wsm().workshop( workshopID );

	if( workshop )
	{
		unsigned int traderID = workshop->assignedGnome();
		
		m_traderID = traderID;

		if( traderID )
		{
			GnomeTrader* gt = Global::gm().trader( traderID );
			if( gt )
			{
				auto& items = gt->inventory();
					
				for( auto& item : items )
				{
					if( item.itemSID == itemSID && item.materialSID == materialSID && item.quality == quality )
					{
						item.reserved = qMax( 0, item.reserved - count );

						for( auto& gti : m_traderStock )
						{
							if( gti.itemSID == itemSID && gti.materialSIDorGender == materialSID && gti.quality == quality )
							{
								gti.reserved = item.reserved;
								emit signalUpdateTraderStockItem( gti );
								updateTraderValue();
								break;
							}
						}
						break;
					}
				}
			}
		}
	}
}

void AggregatorWorkshop::onPlayerStocktoOffer( unsigned int workshopID, QString itemSID, QString materialSID, unsigned char quality, int count )
{
	for( auto& item : m_playerStock )
	{
		if( item.itemSID == itemSID && item.materialSIDorGender == materialSID && item.quality == quality )
		{
			item.reserved = qMin( item.count, item.reserved + count );
		
			emit signalUpdatePlayerStockItem( item );
			updatePlayerValue();
			break;
		}
	}
}

void AggregatorWorkshop::onPlayerOffertoStock( unsigned int workshopID, QString itemSID, QString materialSID, unsigned char quality, int count )
{
	for( auto& item : m_playerStock )
	{
		if( item.itemSID == itemSID && item.materialSIDorGender == materialSID && item.quality == quality )
		{
			item.reserved = qMax( 0, item.reserved - count );
		
			emit signalUpdatePlayerStockItem( item );
			updatePlayerValue();
			break;
		}
	}
}

void AggregatorWorkshop::updateTraderValue()
{
	m_traderOfferValue = 0;
	for( const auto& item : m_traderStock )
	{
		m_traderOfferValue += item.reserved * item.value;
	}
	emit signalUpdateTraderValue( m_traderOfferValue );
}
	
void AggregatorWorkshop::updatePlayerValue()
{
	m_playerOfferValue = 0;
	for( const auto& item : m_playerStock )
	{
		m_playerOfferValue += item.reserved * item.value;
	}
	emit signalUpdatePlayerValue( m_playerOfferValue );
}

void AggregatorWorkshop::onTrade( unsigned int workshopID )
{
	auto workshop = Global::wsm().workshop( workshopID );

	if( workshop )
	{
		unsigned int traderID = workshop->assignedGnome();

		m_traderID = traderID;

		if( traderID )
		{
			GnomeTrader* gt = Global::gm().trader( traderID );
			if( gt )
			{
				if( m_playerOfferValue >= m_traderOfferValue )
				{
					QList<unsigned int> allItemsToSell;
					for( auto& item : m_playerStock )
					{
						auto sellItems = Global::inv().tradeInventory( item.itemSID, item.materialSIDorGender, item.quality );

						if( sellItems.size() >= item.reserved )
						{
							for( int i = 0; i < item.reserved; ++i )
							{
								allItemsToSell.append( sellItems[i] );
							}
				
						}
						else
						{
							item.reserved = sellItems.size();
							emit signalUpdatePlayerStockItem( item );
						}
					}
					int currentValue = 0;
					for( auto itemID : allItemsToSell )
					{
						currentValue += Global::inv().value( itemID );
					}
					if( currentValue >= m_traderOfferValue )
					{
						for( auto itemID : allItemsToSell )
						{
							Global::inv().pickUpItem( itemID );
							Global::inv().destroyObject( itemID );
						}
						updatePlayerStock( workshopID );

						auto& traderItems = gt->inventory();
						for( auto& item : traderItems )
						{
							if( item.type == "Animal" )
							{
								for( int i = 0; i < item.reserved; ++i )
								{
									Global::cm().addCreature( CreatureType::ANIMAL, item.itemSID, workshop->outputPos(), item.gender, true, true );
								}
							}
							else
							{
								for( int i = 0; i < item.reserved; ++i )
								{
									Global::inv().createItem( workshop->outputPos(), item.itemSID, item.materialSID );
								}
							}
							item.amount = qMax( 0, item.amount - item.reserved );
							item.reserved = 0;
						}

						updateTraderStock( traderItems );

						emit signalTraderStock( m_traderStock );

						updateTraderValue();
					}
				}
			}
		}
	}
}
