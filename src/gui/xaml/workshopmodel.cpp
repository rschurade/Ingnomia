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

#include "workshopmodel.h"

#include "../strings.h"
#include "workshopproxy.h"

#include <NsApp/Application.h>
#include <NsCore/Log.h>
#include <NsCore/ReflectionImplement.h>
#include <NsCore/ReflectionImplementEnum.h>
#include <NsGui/ObservableCollection.h>
#include <NsGui/UIElement.h>

#include <QDebug>

using namespace IngnomiaGUI;
using namespace Noesis;
using namespace NoesisApp;

#pragma region WsPriority
////////////////////////////////////////////////////////////////////////////////////////////////////
WsPriority::WsPriority( const char* name ) :
	_name( name )
{
}

const char* WsPriority::GetName() const
{
	return _name.Str();
}
#pragma endregion WsPriority

#pragma region RecipeTab
////////////////////////////////////////////////////////////////////////////////////////////////////
WsAvailableMaterial::WsAvailableMaterial( QString sid, int amount, QString item )
{
	_name   = ( S::s( "$MaterialName_" + sid ) + " " + S::s( "$ItemName_" + item ) ).toStdString().c_str();
	_sid    = sid.toStdString().c_str();
	_amount = QString::number( amount ).toStdString().c_str();
}

const char* WsAvailableMaterial::GetName() const
{
	return _name.Str();
}

const char* WsAvailableMaterial::sid() const
{
	return _sid.Str();
}

const char* WsAvailableMaterial::amount() const
{
	return _amount.Str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
WsRequiredItem::WsRequiredItem( GuiWorkshopComponent gwc )
{
	_name   = S::s( "$ItemName_" + gwc.sid ).toStdString().c_str();
	_sid    = gwc.sid;
	_amount = QString::number( gwc.amount ).toStdString().c_str();

	_availableMaterials = *new Noesis::ObservableCollection<WsAvailableMaterial>();

	for ( auto mat : gwc.materials )
	{
		_availableMaterials->Add( MakePtr<WsAvailableMaterial>( mat.sid, mat.amount, _sid ) );
	}

	SetSelectedMaterial( _availableMaterials->Get( 0 ) );
}

const char* WsRequiredItem::GetName() const
{
	return _name.Str();
}

const QString WsRequiredItem::sid()
{
	return _sid;
}

const char* WsRequiredItem::amount() const
{
	return _amount.Str();
}

Noesis::ObservableCollection<WsAvailableMaterial>* WsRequiredItem::availableMaterials() const
{
	return _availableMaterials;
}

void WsRequiredItem::SetSelectedMaterial( WsAvailableMaterial* mat )
{
	if ( _selectedMaterial != mat )
	{
		_selectedMaterial = mat;
		OnPropertyChanged( "SelectedMaterial" );
	}
}

WsAvailableMaterial* WsRequiredItem::GetSelectedMaterial() const
{
	return _selectedMaterial;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
WsProduct::WsProduct( GuiWorkshopProduct gwp, WorkshopProxy* proxy ) :
	m_proxy( proxy )
{
	m_name        = S::s( "$CraftName_" + gwp.sid ).toStdString().c_str();
	m_sid         = gwp.sid;
	m_craftNumber = "1";
	_cmdBuild.SetExecuteFunc( MakeDelegate( this, &WsProduct::onCmdBuild ) );

	_requiredItems = *new Noesis::ObservableCollection<WsRequiredItem>();

	for ( auto gwc : gwp.components )
	{
		_requiredItems->Add( MakePtr<WsRequiredItem>( gwc ) );
	}
}

const char* WsProduct::GetName() const
{
	return m_name.Str();
}

QString WsProduct::sid() const
{
	return m_sid;
}

Noesis::ObservableCollection<WsRequiredItem>* WsProduct::requiredItems() const
{
	return _requiredItems;
}

const NoesisApp::DelegateCommand* WsProduct::GetCmdBuild() const
{
	return &_cmdBuild;
}

void WsProduct::onCmdBuild( BaseComponent* param )
{
	QStringList mats;
	for ( int i = 0; i < _requiredItems->Count(); ++i )
	{
		auto item = _requiredItems->Get( i );
		auto mat  = item->GetSelectedMaterial();
		mats.append( mat->sid() );
	}
	QString num( m_craftNumber.Str() );

	bool ok;
	int number = num.toInt( &ok );
	if ( !ok )
		number = 1;
	m_proxy->craftItem( m_sid, m_mode, number, mats );
}

int WsProduct::GetMode() const
{
	return m_mode;
}

void WsProduct::SetMode( int mode )
{
	if ( m_mode != mode )
	{
		m_mode = mode;
		OnPropertyChanged( "Mode" );
	}
}

const char* WsProduct::GetCraftNumber() const
{
	return m_craftNumber.Str();
}

void WsProduct::SetCraftNumber( const char* value )
{
	if ( m_craftNumber != value )
	{
		m_craftNumber = value;
		OnPropertyChanged( "CraftNumber" );
	}
}
#pragma endregion RecipeTab

#pragma region CraftingListTab

////////////////////////////////////////////////////////////////////////////////////////////////////
WsCraftJob::WsCraftJob( CraftJob cj, WorkshopProxy* proxy ) :
	m_proxy( proxy )
{
	//
	if ( cj.requiredItems.size() )
	{
		m_name = ( S::s( "$MaterialName_" + cj.requiredItems.first().materialSID ) + " " + S::s( "$ItemName_" + cj.itemSID ) ).toStdString().c_str();
	}
	else
	{
		m_name = S::s( "$CraftName_" + cj.craftID ).toStdString().c_str();
	}

	m_craftJobID  = cj.id;
	m_sid         = cj.craftID;
	m_craftNumber = QString::number( cj.numItemsToCraft ).toStdString().c_str();
	m_mode        = (int)cj.mode;
	m_suspended   = cj.paused;
	m_moveBack    = cj.moveToBackWhenDone;

	_buttonCmd.SetExecuteFunc( MakeDelegate( this, &WsCraftJob::onButtonCmd ) );
}

const char* WsCraftJob::GetName() const
{
	return m_name.Str();
}

QString WsCraftJob::sid() const
{
	return m_sid;
}

int WsCraftJob::GetMode() const
{
	return m_mode;
}

void WsCraftJob::SetMode( int mode )
{
	if ( m_mode != mode )
	{
		m_mode = mode;
		m_proxy->craftJobParams( m_craftJobID, m_mode, m_craftNumber.Str(), m_suspended, m_moveBack );
		OnPropertyChanged( "Mode" );
	}
}

const char* WsCraftJob::GetCraftNumber() const
{
	return m_craftNumber.Str();
}

void WsCraftJob::SetCraftNumber( const char* value )
{
	if ( m_craftNumber != value )
	{
		m_craftNumber = value;
		m_proxy->craftJobParams( m_craftJobID, m_mode, m_craftNumber.Str(), m_suspended, m_moveBack );
		OnPropertyChanged( "CraftNumber" );
	}
}

bool WsCraftJob::GetSuspended() const
{
	return m_suspended;
}

void WsCraftJob::SetSuspended( bool value )
{
	if ( m_suspended != value )
	{
		m_suspended = value;
		m_proxy->craftJobParams( m_craftJobID, m_mode, m_craftNumber.Str(), m_suspended, m_moveBack );
		OnPropertyChanged( "Suspended" );
	}
}

bool WsCraftJob::GetMoveBack() const
{
	return m_moveBack;
}

void WsCraftJob::SetMoveBack( bool value )
{
	if ( m_moveBack != value )
	{
		m_moveBack = value;
		m_proxy->craftJobParams( m_craftJobID, m_mode, m_craftNumber.Str(), m_suspended, m_moveBack );
		OnPropertyChanged( "MoveBack" );
	}
}

void WsCraftJob::onButtonCmd( BaseComponent* param )
{
	if ( param )
	{
		m_proxy->craftJobCommand( m_craftJobID, param->ToString().Str() );
	}
}

#pragma endregion CraftingListTab

#pragma region TradeItems

WsTradeItem::WsTradeItem( QString name, QString itemSID, QString materialSID, unsigned char quality, int count ) :
	m_name( name.toStdString().c_str() ),
	m_itemSID( itemSID ),
	m_materialSID( materialSID ),
	m_quality( quality ),
	m_count( count ),
	m_countString( QString::number( count ).toStdString().c_str() )
{
}

const char* WsTradeItem::GetName() const
{
	return m_name.Str();
}

const char* WsTradeItem::GetCount() const
{
	return m_countString.Str();
}

void WsTradeItem::SetCount( int count )
{
	m_count       = count;
	m_countString = QString::number( count ).toStdString().c_str();
	OnPropertyChanged( "Count" );
}

#pragma endregion TradeItems

////////////////////////////////////////////////////////////////////////////////////////////////////
WorkshopModel::WorkshopModel()
{
	m_proxy = new WorkshopProxy;
	m_proxy->setParent( this );

	m_prios = *new ObservableCollection<WsPriority>();
	for ( int i = 0; i < 2; ++i )
	{
		m_prios->Add( MakePtr<WsPriority>( ( QString( "Priority " ) + QString::number( i + 1 ) ).toStdString().c_str() ) );
	}
	SetSelectedPriority( m_prios->Get( 0 ) );

	m_products = *new ObservableCollection<WsProduct>();
	m_jobs     = *new ObservableCollection<WsCraftJob>();

	m_traderStock = *new ObservableCollection<WsTradeItem>();
	m_traderOffer = *new ObservableCollection<WsTradeItem>();
	m_playerStock = *new ObservableCollection<WsTradeItem>();
	m_playerOffer = *new ObservableCollection<WsTradeItem>();

	m_transferCmd.SetExecuteFunc( MakeDelegate( this, &WorkshopModel::onTransferCmd ) );
	m_tradeCmd.SetExecuteFunc( MakeDelegate( this, &WorkshopModel::onTradeCmd ) );
}

void WorkshopModel::onUpdateInfo( const GuiWorkshopInfo& info )
{
	bool isSameWorkshop = ( m_workshopID == info.workshopID );
	m_workshopID        = info.workshopID;
	m_gui               = info.gui;
	
	m_proxy->blockWriteBack();
	
	SetName( info.name.toStdString().c_str() );
	SetSuspended( info.suspended );
	SetLinkStockpile( info.linkStockpile );

	m_prios->Clear();
	for ( int i = 0; i < info.maxPriority; ++i )
	{
		m_prios->Add( MakePtr<WsPriority>( ( QString( "Priority " ) + QString::number( i + 1 ) ).toStdString().c_str() ) );
	}
	SetSelectedPriority( m_prios->Get( qMin( qMax( 0, info.priority ), m_prios->Count() ) ) );

	if ( m_gui.isEmpty() )
	{
		SetAcceptGenerated( info.acceptGenerated );
		SetAutoCraftMissing( info.autoCraftMissing );

		m_products->Clear();
		for ( auto gwp : info.products )
		{
			m_products->Add( MakePtr<WsProduct>( gwp, m_proxy ) );
		}
		OnPropertyChanged( "Products" );

		m_jobs->Clear();
		for ( auto cj : info.jobList )
		{
			m_jobs->Add( MakePtr<WsCraftJob>( cj, m_proxy ) );
		}

		OnPropertyChanged( "Jobs" );
	}
	else if ( m_gui == "Butcher" )
	{
		m_butcherCorpses = info.butcherCorpses;
		m_butcherExcess  = info.butcherExcess;
	}
	else if ( m_gui == "Trader" )
	{
		m_selectedTraderStock = nullptr;
		m_selectedTraderOffer = nullptr;
		m_selectedPlayerStock = nullptr;
		m_selectedPlayerOffer = nullptr;
		m_proxy->requestAllTradeItems( m_workshopID );
	}
	else if ( m_gui == "Fishery" )
	{
		m_catchFish   = info.catchFish;
		m_processFish = info.processFish;
	}
	qDebug() << m_gui;

	m_proxy->unblockWriteBack();

	OnPropertyChanged( "Priorities" );
	OnPropertyChanged( "SelectedPrio" );

	OnPropertyChanged( "NormalGui" );
	OnPropertyChanged( "ButcherGui" );
	OnPropertyChanged( "TraderGui" );
	OnPropertyChanged( "FisherGui" );

	OnPropertyChanged( "ButcherExcess" );
	OnPropertyChanged( "ButcherCorpses" );
	OnPropertyChanged( "CatchFish" );
	OnPropertyChanged( "ProcessFish" );

	m_proxy->unblockWriteBack();
}

void WorkshopModel::onUpdateCraftList( const GuiWorkshopInfo& info )
{
	bool isSameWorkshop = ( m_workshopID == info.workshopID );
	if ( isSameWorkshop )
	{
		m_jobs->Clear();
		for ( auto cj : info.jobList )
		{
			m_jobs->Add( MakePtr<WsCraftJob>( cj, m_proxy ) );
		}
		OnPropertyChanged( "Jobs" );
	}
}

bool WorkshopModel::GetSuspended() const
{
	return m_suspended;
}

void WorkshopModel::SetSuspended( bool value )
{
	if ( m_suspended != value )
	{
		m_suspended = value;
		m_proxy->setBasicOptions( m_workshopID, m_name.Str(), m_prios->IndexOf( m_selectedPrio ), m_suspended, m_acceptGenerated, m_autoCraftMissing, m_connectStockpile );
		OnPropertyChanged( "Suspended" );
	}
}

Noesis::ObservableCollection<WsPriority>* WorkshopModel::GetPrios() const
{
	return m_prios;
}

void WorkshopModel::SetSelectedPriority( WsPriority* prio )
{
	if ( m_selectedPrio != prio )
	{
		m_selectedPrio = prio;
		m_proxy->setBasicOptions( m_workshopID, m_name.Str(), m_prios->IndexOf( m_selectedPrio ), m_suspended, m_acceptGenerated, m_autoCraftMissing, m_connectStockpile );
		OnPropertyChanged( "SelectedPrio" );
	}
}

WsPriority* WorkshopModel::GetSelectedPriority() const
{
	return m_selectedPrio;
}

const char* WorkshopModel::GetNormalVisible() const
{
	if ( m_gui.isEmpty() )
	{
		return "Visible";
	}
	return "Collapsed";
}

const char* WorkshopModel::GetButcherVisible() const
{
	if ( m_gui == "Butcher" )
	{
		return "Visible";
	}
	return "Collapsed";
}

const char* WorkshopModel::GetTraderVisible() const
{
	if ( m_gui == "Trader" )
	{
		return "Visible";
	}
	return "Collapsed";
}

const char* WorkshopModel::GetFisherVisible() const
{
	if ( m_gui == "Fishery" )
	{
		return "Visible";
	}
	return "Collapsed";
}

bool WorkshopModel::GetAutoCraftMissing() const
{
	return m_autoCraftMissing;
}

void WorkshopModel::SetAutoCraftMissing( bool value )
{
	if ( m_autoCraftMissing != value )
	{
		m_autoCraftMissing = value;
		m_proxy->setBasicOptions( m_workshopID, m_name.Str(), m_prios->IndexOf( m_selectedPrio ), m_suspended, m_acceptGenerated, m_autoCraftMissing, m_connectStockpile );
		OnPropertyChanged( "AutoCraft" );
	}
}

bool WorkshopModel::GetAcceptGenerated() const
{
	return m_acceptGenerated;
}

void WorkshopModel::SetAcceptGenerated( bool value )
{
	if ( m_acceptGenerated != value )
	{
		m_acceptGenerated = value;
		m_proxy->setBasicOptions( m_workshopID, m_name.Str(), m_prios->IndexOf( m_selectedPrio ), m_suspended, m_acceptGenerated, m_autoCraftMissing, m_connectStockpile );
		OnPropertyChanged( "AcceptGenerated" );
	}
}

bool WorkshopModel::GetLinkStockpile() const
{
	return m_connectStockpile;
}

void WorkshopModel::SetLinkStockpile( bool value )
{
	if ( m_connectStockpile != value )
	{
		m_connectStockpile = value;
		m_proxy->setBasicOptions( m_workshopID, m_name.Str(), m_prios->IndexOf( m_selectedPrio ), m_suspended, m_acceptGenerated, m_autoCraftMissing, m_connectStockpile );
		OnPropertyChanged( "LinkStockpile" );
	}
}

const char* WorkshopModel::GetName() const
{
	return m_name.Str();
}

void WorkshopModel::SetName( const char* value )
{
	m_name = value;

	m_proxy->setBasicOptions( m_workshopID, m_name.Str(), m_prios->IndexOf( m_selectedPrio ), m_suspended, m_acceptGenerated, m_autoCraftMissing, m_connectStockpile );
	OnPropertyChanged( "Name" );
}

#pragma region ButcherSpecific

bool WorkshopModel::GetButcherExcess() const
{
	return m_butcherExcess;
}

void WorkshopModel::SetButcherExcess( bool value )
{
	if ( m_butcherExcess != value )
	{
		m_butcherExcess = value;
		m_proxy->setButcherOptions( m_workshopID, m_butcherCorpses, m_butcherExcess );
	}
}

bool WorkshopModel::GetButcherCorpses() const
{
	return m_butcherCorpses;
}

void WorkshopModel::SetButcherCorpses( bool value )
{
	if ( m_butcherCorpses != value )
	{
		m_butcherCorpses = value;
		m_proxy->setButcherOptions( m_workshopID, m_butcherCorpses, m_butcherExcess );
	}
}
#pragma endregion ButcherSpecific

bool WorkshopModel::GetCatchFish() const
{
	return m_catchFish;
}

void WorkshopModel::SetCatchFish( bool value )
{
	if ( m_catchFish != value )
	{
		m_catchFish = value;
		m_proxy->setFisherOptions( m_workshopID, m_catchFish, m_processFish );
	}
}

bool WorkshopModel::GetProcessFish() const
{
	return m_processFish;
}

void WorkshopModel::SetProcessFish( bool value )
{
	if ( m_processFish != value )
	{
		m_processFish = value;
		m_proxy->setFisherOptions( m_workshopID, m_catchFish, m_processFish );
	}
}

#pragma region TraderSpecific
void WorkshopModel::updateTraderStock( const QList<GuiTradeItem>& items )
{
	m_traderStock->Clear();

	for ( const auto& item : items )
	{
		if ( item.count - item.reserved > 0 )
		{
			m_traderStock->Add( MakePtr<WsTradeItem>( item.name, item.itemSID, item.materialSIDorGender, item.quality, item.count - item.reserved ) );
		}
	}
	OnPropertyChanged( "TraderStock" );

	m_traderOffer->Clear();

	for ( const auto& item : items )
	{
		if ( item.reserved > 0 )
		{
			m_traderOffer->Add( MakePtr<WsTradeItem>( item.name, item.itemSID, item.materialSIDorGender, item.quality, item.reserved ) );
		}
	}
	OnPropertyChanged( "TraderOffer" );
}

void WorkshopModel::updatePlayerStock( const QList<GuiTradeItem>& items )
{
	m_playerStock->Clear();

	for ( const auto& item : items )
	{
		m_playerStock->Add( MakePtr<WsTradeItem>( item.name, item.itemSID, item.materialSIDorGender, item.quality, item.count ) );
	}
	OnPropertyChanged( "PlayerStock" );

	m_playerOffer->Clear();

	for ( const auto& item : items )
	{
		if ( item.reserved > 0 )
		{
			m_playerOffer->Add( MakePtr<WsTradeItem>( item.name, item.itemSID, item.materialSIDorGender, item.quality, item.reserved ) );
		}
	}
	OnPropertyChanged( "PlayerOffer" );
}

void WorkshopModel::updateTraderStockItem( const GuiTradeItem& gti )
{
	for ( int i = 0; i < m_traderStock->Count(); ++i )
	{
		auto item = m_traderStock->Get( i );
		if ( item->m_itemSID == gti.itemSID && item->m_materialSID == gti.materialSIDorGender && item->m_quality == gti.quality )
		{
			if ( gti.count - gti.reserved > 0 )
			{
				item->SetCount( gti.count - gti.reserved );
				break;
			}
			else
			{
				m_traderStock->RemoveAt( i );
				OnPropertyChanged( "TraderStock" );
				break;
			}
		}
	}
	bool found = false;
	for ( int i = 0; i < m_traderOffer->Count(); ++i )
	{
		auto item = m_traderOffer->Get( i );

		if ( item->m_itemSID == gti.itemSID && item->m_materialSID == gti.materialSIDorGender && item->m_quality == gti.quality )
		{
			found = true;
			if ( gti.reserved > 0 )
			{
				item->SetCount( gti.reserved );
				break;
			}
			else
			{
				m_traderOffer->RemoveAt( i );
				OnPropertyChanged( "TraderOffer" );
				break;
			}
		}
	}
	if ( !found )
	{
		m_traderOffer->Add( MakePtr<WsTradeItem>( gti.name, gti.itemSID, gti.materialSIDorGender, gti.quality, gti.reserved ) );
		OnPropertyChanged( "TraderOffer" );
	}
}

void WorkshopModel::updatePlayerStockItem( const GuiTradeItem& gti )
{
	for ( int i = 0; i < m_playerStock->Count(); ++i )
	{
		auto item = m_playerStock->Get( i );
		if ( item->m_itemSID == gti.itemSID && item->m_materialSID == gti.materialSIDorGender && item->m_quality == gti.quality )
		{
			if ( gti.count - gti.reserved > 0 )
			{
				item->SetCount( gti.count - gti.reserved );
				break;
			}
			else
			{
				m_playerStock->RemoveAt( i );
				OnPropertyChanged( "PlayerStock" );
				break;
			}
		}
	}
	bool found = false;
	for ( int i = 0; i < m_playerOffer->Count(); ++i )
	{
		auto item = m_playerOffer->Get( i );

		if ( item->m_itemSID == gti.itemSID && item->m_materialSID == gti.materialSIDorGender && item->m_quality == gti.quality )
		{
			found = true;
			if ( gti.reserved > 0 )
			{
				item->SetCount( gti.reserved );
				break;
			}
			else
			{
				m_playerOffer->RemoveAt( i );
				OnPropertyChanged( "PlayerOffer" );
				break;
			}
		}
	}
	if ( !found )
	{
		m_playerOffer->Add( MakePtr<WsTradeItem>( gti.name, gti.itemSID, gti.materialSIDorGender, gti.quality, gti.reserved ) );
		OnPropertyChanged( "PlayerOffer" );
	}
}

void WorkshopModel::SetSelectedTraderStock( WsTradeItem* item )
{
	if ( m_selectedTraderStock != item )
	{
		m_selectedTraderStock = item;
		OnPropertyChanged( "SelectedTraderStock" );
	}
}

WsTradeItem* WorkshopModel::GetSelectedTraderStock() const
{
	return m_selectedTraderStock;
}

void WorkshopModel::SetSelectedTraderOffer( WsTradeItem* item )
{
	if ( m_selectedTraderOffer != item )
	{
		m_selectedTraderOffer = item;
		OnPropertyChanged( "SelectedTraderOffer" );
	}
}

WsTradeItem* WorkshopModel::GetSelectedTraderOffer() const
{
	return m_selectedTraderOffer;
}

void WorkshopModel::SetSelectedPlayerStock( WsTradeItem* item )
{
	if ( m_selectedPlayerStock != item )
	{
		m_selectedPlayerStock = item;
		OnPropertyChanged( "SelectedPlayerStock" );
	}
}

WsTradeItem* WorkshopModel::GetSelectedPlayerStock() const
{
	return m_selectedPlayerStock;
}

void WorkshopModel::SetSelectedPlayerOffer( WsTradeItem* item )
{
	if ( m_selectedPlayerOffer != item )
	{
		m_selectedPlayerOffer = item;
		OnPropertyChanged( "SelectedPlayerOffer" );
	}
}

WsTradeItem* WorkshopModel::GetSelectedPlayerOffer() const
{
	return m_selectedPlayerOffer;
}

bool WorkshopModel::GetAmount1Checked() const
{
	if ( m_amountToTransfer == CheckedAmount::Amount1 )
	{
		return true;
	}
	return false;
}

bool WorkshopModel::GetAmount10Checked() const
{
	if ( m_amountToTransfer == CheckedAmount::Amount10 )
	{
		return true;
	}
	return false;
}

bool WorkshopModel::GetAmount100Checked() const
{
	if ( m_amountToTransfer == CheckedAmount::Amount100 )
	{
		return true;
	}
	return false;
}

bool WorkshopModel::GetAmountAllChecked() const
{
	if ( m_amountToTransfer == CheckedAmount::AmountAll )
	{
		return true;
	}
	return false;
}

void IngnomiaGUI::WorkshopModel::SetAmount1Checked( bool checked )
{
	m_amountToTransfer = CheckedAmount::Amount1;
	OnPropertyChanged( "Amount1" );
	OnPropertyChanged( "Amount10" );
	OnPropertyChanged( "Amount100" );
	OnPropertyChanged( "AmountAll" );
}

void IngnomiaGUI::WorkshopModel::SetAmount10Checked( bool checked )
{
	m_amountToTransfer = CheckedAmount::Amount10;
	OnPropertyChanged( "Amount1" );
	OnPropertyChanged( "Amount10" );
	OnPropertyChanged( "Amount100" );
	OnPropertyChanged( "AmountAll" );
}

void IngnomiaGUI::WorkshopModel::SetAmount100Checked( bool checked )
{
	m_amountToTransfer = CheckedAmount::Amount100;
	OnPropertyChanged( "Amount1" );
	OnPropertyChanged( "Amount10" );
	OnPropertyChanged( "Amount100" );
	OnPropertyChanged( "AmountAll" );
}

void IngnomiaGUI::WorkshopModel::SetAmountAllChecked( bool checked )
{
	m_amountToTransfer = CheckedAmount::AmountAll;
	OnPropertyChanged( "Amount1" );
	OnPropertyChanged( "Amount10" );
	OnPropertyChanged( "Amount100" );
	OnPropertyChanged( "AmountAll" );
}

void WorkshopModel::onTransferCmd( BaseComponent* param )
{
	QString qParam = param->ToString().Str();
	int amount     = 0;
	switch ( m_amountToTransfer )
	{
		case CheckedAmount::Amount1:
			amount = 1;
			break;
		case CheckedAmount::Amount10:
			amount = 10;
			break;
		case CheckedAmount::Amount100:
			amount = 100;
			break;
		case CheckedAmount::AmountAll:
			amount = -1;
			break;
	}

	if ( qParam == "TraderLeft" )
	{
		if ( m_selectedTraderOffer )
		{
			m_proxy->traderOffertoStock( m_workshopID, m_selectedTraderOffer->m_itemSID, m_selectedTraderOffer->m_materialSID, m_selectedTraderOffer->m_quality, amount );
		}
	}
	else if ( qParam == "TraderRight" )
	{
		if ( m_selectedTraderStock )
		{
			m_proxy->traderStocktoOffer( m_workshopID, m_selectedTraderStock->m_itemSID, m_selectedTraderStock->m_materialSID, m_selectedTraderStock->m_quality, amount );
		}
	}
	else if ( qParam == "PlayerLeft" )
	{
		if ( m_selectedPlayerOffer )
		{
			m_proxy->playerOffertoStock( m_workshopID, m_selectedPlayerOffer->m_itemSID, m_selectedPlayerOffer->m_materialSID, m_selectedPlayerOffer->m_quality, amount );
		}
	}
	else if ( qParam == "PlayerRight" )
	{
		if ( m_selectedPlayerStock )
		{
			m_proxy->playerStocktoOffer( m_workshopID, m_selectedPlayerStock->m_itemSID, m_selectedPlayerStock->m_materialSID, m_selectedPlayerStock->m_quality, amount );
		}
	}
}

const char* WorkshopModel::GetTraderValue() const
{
	return m_traderValue.Str();
}

const char* WorkshopModel::GetPlayerValue() const
{
	return m_playerValue.Str();
}

void WorkshopModel::updateTraderValue( int value )
{
	m_traderValue = QString::number( value ).toStdString().c_str();
	OnPropertyChanged( "TraderValue" );
}

void WorkshopModel::updatePlayerValue( int value )
{
	m_playerValue = QString::number( value ).toStdString().c_str();
	OnPropertyChanged( "PlayerValue" );
}

void WorkshopModel::onTradeCmd( BaseComponent* param )
{
	m_proxy->trade( m_workshopID );
}

#pragma endregion TraderSpecific

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION( WorkshopModel, "IngnomiaGUI.WorkshopModel" )
{
	NsProp( "Name", &WorkshopModel::GetName, &WorkshopModel::SetName );
	NsProp( "Suspended", &WorkshopModel::GetSuspended, &WorkshopModel::SetSuspended );
	NsProp( "AutoCraft", &WorkshopModel::GetAutoCraftMissing, &WorkshopModel::SetAutoCraftMissing );
	NsProp( "AcceptGenerated", &WorkshopModel::GetAcceptGenerated, &WorkshopModel::SetAcceptGenerated );
	NsProp( "LinkStockpile", &WorkshopModel::GetLinkStockpile, &WorkshopModel::SetLinkStockpile );
	NsProp( "ButcherCorpses", &WorkshopModel::GetButcherCorpses, &WorkshopModel::SetButcherCorpses );
	NsProp( "ButcherExcess", &WorkshopModel::GetButcherExcess, &WorkshopModel::SetButcherExcess );

	NsProp( "CatchFish", &WorkshopModel::GetCatchFish, &WorkshopModel::SetCatchFish );
	NsProp( "ProcessFish", &WorkshopModel::GetProcessFish, &WorkshopModel::SetProcessFish );

	NsProp( "Priorities", &WorkshopModel::GetPrios );
	NsProp( "SelectedPrio", &WorkshopModel::GetSelectedPriority, &WorkshopModel::SetSelectedPriority );

	NsProp( "Products", &WorkshopModel::GetProducts );
	NsProp( "Jobs", &WorkshopModel::GetJobs );

	NsProp( "NormalGui", &WorkshopModel::GetNormalVisible );
	NsProp( "ButcherGui", &WorkshopModel::GetButcherVisible );
	NsProp( "TraderGui", &WorkshopModel::GetTraderVisible );
	NsProp( "FisherGui", &WorkshopModel::GetFisherVisible );

	NsProp( "TraderStock", &WorkshopModel::GetTraderStock );
	NsProp( "TraderOffer", &WorkshopModel::GetTraderOffer );
	NsProp( "PlayerStock", &WorkshopModel::GetPlayerStock );
	NsProp( "PlayerOffer", &WorkshopModel::GetPlayerOffer );

	NsProp( "SelectedTraderStock", &WorkshopModel::GetSelectedTraderStock, &WorkshopModel::SetSelectedTraderStock );
	NsProp( "SelectedTraderOffer", &WorkshopModel::GetSelectedTraderOffer, &WorkshopModel::SetSelectedTraderOffer );
	NsProp( "SelectedPlayerStock", &WorkshopModel::GetSelectedPlayerStock, &WorkshopModel::SetSelectedPlayerStock );
	NsProp( "SelectedPlayerOffer", &WorkshopModel::GetSelectedPlayerOffer, &WorkshopModel::SetSelectedPlayerOffer );

	NsProp( "CmdTransfer", &WorkshopModel::GetTransferCmd );
	NsProp( "CmdTrade", &WorkshopModel::GetTradeCmd );

	NsProp( "Amount1", &WorkshopModel::GetAmount1Checked, &WorkshopModel::SetAmount1Checked );
	NsProp( "Amount10", &WorkshopModel::GetAmount10Checked, &WorkshopModel::SetAmount10Checked );
	NsProp( "Amount100", &WorkshopModel::GetAmount100Checked, &WorkshopModel::SetAmount100Checked );
	NsProp( "AmountAll", &WorkshopModel::GetAmountAllChecked, &WorkshopModel::SetAmountAllChecked );

	NsProp( "TraderValue", &WorkshopModel::GetTraderValue );
	NsProp( "PlayerValue", &WorkshopModel::GetPlayerValue );
}

NS_IMPLEMENT_REFLECTION( WsPriority )
{
	NsProp( "Name", &WsPriority::GetName );
}

NS_IMPLEMENT_REFLECTION( WsProduct )
{
	NsProp( "Name", &WsProduct::GetName );
	NsProp( "RequiredItems", &WsProduct::requiredItems );
	NsProp( "Craft", &WsProduct::GetCmdBuild );
	NsProp( "Mode", &WsProduct::GetMode, &WsProduct::SetMode );
	NsProp( "CraftNumber", &WsProduct::GetCraftNumber, &WsProduct::SetCraftNumber );
}

NS_IMPLEMENT_REFLECTION( WsRequiredItem )
{
	NsProp( "Name", &WsRequiredItem::GetName );
	NsProp( "Amount", &WsRequiredItem::amount );
	NsProp( "Materials", &WsRequiredItem::availableMaterials );
	NsProp( "SelectedMaterial", &WsRequiredItem::GetSelectedMaterial, &WsRequiredItem::SetSelectedMaterial );
}

NS_IMPLEMENT_REFLECTION( WsAvailableMaterial )
{
	NsProp( "Name", &WsAvailableMaterial::GetName );
	NsProp( "Amount", &WsAvailableMaterial::amount );
}

NS_IMPLEMENT_REFLECTION( WsCraftJob )
{
	NsProp( "Name", &WsCraftJob::GetName );
	NsProp( "Mode", &WsCraftJob::GetMode, &WsCraftJob::SetMode );
	NsProp( "CraftNumber", &WsCraftJob::GetCraftNumber, &WsCraftJob::SetCraftNumber );
	NsProp( "Suspended", &WsCraftJob::GetSuspended, &WsCraftJob::SetSuspended );
	NsProp( "MoveBack", &WsCraftJob::GetMoveBack, &WsCraftJob::SetMoveBack );
	NsProp( "ButtonCmd", &WsCraftJob::GetButtonCmd );
}

NS_IMPLEMENT_REFLECTION( WsTradeItem )
{
	NsProp( "Name", &WsTradeItem::GetName );
	NsProp( "Count", &WsTradeItem::GetCount );
}
