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
#ifndef __WorkshopModel_H__
#define __WorkshopModel_H__

#include "../aggregatorworkshop.h"

#include <NsApp/DelegateCommand.h>
#include <NsApp/NotifyPropertyChangedBase.h>
#include <NsCore/Noesis.h>
#include <NsCore/Nullable.h>
#include <NsCore/Ptr.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/ReflectionDeclareEnum.h>
#include <NsCore/String.h>
#include <NsGui/Collection.h>

#include <QString>

class WorkshopProxy;

namespace Noesis
{
template <class T>
class ObservableCollection;
}

namespace IngnomiaGUI
{

#pragma region WsPriority
////////////////////////////////////////////////////////////////////////////////////////////////////
class WsPriority final : public Noesis::BaseComponent
{
public:
	WsPriority( const char* name );

	const char* GetName() const;

private:
	Noesis::String _name;

	NS_DECLARE_REFLECTION( WsPriority, Noesis::BaseComponent )
};
#pragma endregion WsPriority

#pragma region RecipeTab
////////////////////////////////////////////////////////////////////////////////////////////////////
class WsAvailableMaterial final : public Noesis::BaseComponent
{
public:
	WsAvailableMaterial( QString sid, int amount, QString item );

	const char* GetName() const;
	const char* sid() const;
	const char* amount() const;

private:
	Noesis::String _name;
	Noesis::String _sid;
	Noesis::String _amount;

	NS_DECLARE_REFLECTION( WsAvailableMaterial, Noesis::BaseComponent )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class WsRequiredItem final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	WsRequiredItem( GuiWorkshopComponent gwc );

	const char* GetName() const;
	const char* amount() const;
	const QString sid();

	Noesis::ObservableCollection<WsAvailableMaterial>* availableMaterials() const;

	void SetSelectedMaterial( WsAvailableMaterial* mat );
	WsAvailableMaterial* GetSelectedMaterial() const;

private:
	Noesis::String _name;
	QString _sid;
	Noesis::String _amount;

	WsAvailableMaterial* _selectedMaterial;

	Noesis::Ptr<Noesis::ObservableCollection<WsAvailableMaterial>> _availableMaterials;

	NS_DECLARE_REFLECTION( WsRequiredItem, Noesis::BaseComponent )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class WsProduct final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	WsProduct( GuiWorkshopProduct gwp, WorkshopProxy* proxy );

	const char* GetName() const;

	int GetMode() const;
	void SetMode( int );
	const char* GetCraftNumber() const;
	void SetCraftNumber( const char* value );

	QString sid() const;
	Noesis::ObservableCollection<WsRequiredItem>* requiredItems() const;

private:
	Noesis::String m_name;
	QString m_sid;
	int m_mode = 0;
	Noesis::String m_craftNumber;

	WorkshopProxy* m_proxy = nullptr;

	Noesis::Ptr<Noesis::ObservableCollection<WsRequiredItem>> _requiredItems;

	void onCmdBuild( BaseComponent* param );

	const NoesisApp::DelegateCommand* GetCmdBuild() const;

	NoesisApp::DelegateCommand _cmdBuild;

	NS_DECLARE_REFLECTION( WsProduct, Noesis::BaseComponent )
};
#pragma endregion RecipeTab

#pragma region CraftingListTab

////////////////////////////////////////////////////////////////////////////////////////////////////
class WsCraftJob final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	WsCraftJob( CraftJob cj, WorkshopProxy* proxy );

	const char* GetName() const;

	int GetMode() const;
	void SetMode( int );
	const char* GetCraftNumber() const;
	void SetCraftNumber( const char* value );
	bool GetSuspended() const;
	void SetSuspended( bool value );
	bool GetMoveBack() const;
	void SetMoveBack( bool value );

	QString sid() const;

	const NoesisApp::DelegateCommand* GetButtonCmd() const
	{
		return &_buttonCmd;
	}
	void onButtonCmd( BaseComponent* param );

private:
	unsigned int m_craftJobID;
	Noesis::String m_name;
	QString m_sid;
	int m_mode = 0;
	Noesis::String m_craftNumber;

	bool m_suspended = false;
	bool m_moveBack  = false;

	WorkshopProxy* m_proxy = nullptr;

	NoesisApp::DelegateCommand _buttonCmd;

	NS_DECLARE_REFLECTION( WsCraftJob, Noesis::BaseComponent )
};

#pragma endregion CraftingListTab

#pragma region TradeItems

enum class CheckedAmount : unsigned char
{
	Amount1,
	Amount10,
	Amount100,
	AmountAll,
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class WsTradeItem final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	WsTradeItem( QString name, QString itemSID, QString materialSID, unsigned char quality, int count );

	const char* GetName() const;
	const char* GetCount() const;

	void SetCount( int count );

	QString m_itemSID;
	QString m_materialSID;

	unsigned char m_quality = 0;
	int m_count             = 0;

private:
	Noesis::String m_name;
	Noesis::String m_countString;

	NS_DECLARE_REFLECTION( WsTradeItem, NoesisApp::NotifyPropertyChangedBase )
};

#pragma endregion TradeItems

////////////////////////////////////////////////////////////////////////////////////////////////////
class WorkshopModel final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	WorkshopModel();

	void onUpdateInfo( const GuiWorkshopInfo& WorkshopInfo );
	void onUpdateCraftList( const GuiWorkshopInfo& WorkshopInfo );

	void updateTraderStock( const QList<GuiTradeItem>& items );
	void updatePlayerStock( const QList<GuiTradeItem>& items );

	void updateTraderStockItem( const GuiTradeItem& item );
	void updatePlayerStockItem( const GuiTradeItem& item );

	void updateTraderValue( int value );
	void updatePlayerValue( int value );

private:
	const char* GetName() const;
	void SetName( const char* value );
	bool GetSuspended() const;
	void SetSuspended( bool value );
	bool GetAutoCraftMissing() const;
	void SetAutoCraftMissing( bool value );
	bool GetAcceptGenerated() const;
	void SetAcceptGenerated( bool value );
	bool GetLinkStockpile() const;
	void SetLinkStockpile( bool value );
	bool GetButcherExcess() const;
	void SetButcherExcess( bool value );
	bool GetButcherCorpses() const;
	void SetButcherCorpses( bool value );
	bool GetCatchFish() const;
	void SetCatchFish( bool value );
	bool GetProcessFish() const;
	void SetProcessFish( bool value );

	const char* GetNormalVisible() const;
	const char* GetButcherVisible() const;
	const char* GetTraderVisible() const;
	const char* GetFisherVisible() const;

	const char* GetTraderValue() const;
	const char* GetPlayerValue() const;

	Noesis::ObservableCollection<WsPriority>* GetPrios() const;
	void SetSelectedPriority( WsPriority* prio );
	WsPriority* GetSelectedPriority() const;
	Noesis::Ptr<Noesis::ObservableCollection<WsPriority>> m_prios;
	WsPriority* m_selectedPrio;

	Noesis::ObservableCollection<WsProduct>* GetProducts() const
	{
		return m_products;
	}
	Noesis::Ptr<Noesis::ObservableCollection<WsProduct>> m_products;

	Noesis::ObservableCollection<WsCraftJob>* GetJobs() const
	{
		return m_jobs;
	}
	Noesis::Ptr<Noesis::ObservableCollection<WsCraftJob>> m_jobs;

#pragma region TradeItemLists
	Noesis::ObservableCollection<WsTradeItem>* GetTraderStock() const
	{
		return m_traderStock;
	}
	Noesis::Ptr<Noesis::ObservableCollection<WsTradeItem>> m_traderStock;
	WsTradeItem* m_selectedTraderStock = nullptr;
	void SetSelectedTraderStock( WsTradeItem* item );
	WsTradeItem* GetSelectedTraderStock() const;

	Noesis::ObservableCollection<WsTradeItem>* GetTraderOffer() const
	{
		return m_traderOffer;
	}
	Noesis::Ptr<Noesis::ObservableCollection<WsTradeItem>> m_traderOffer;
	WsTradeItem* m_selectedTraderOffer = nullptr;
	void SetSelectedTraderOffer( WsTradeItem* item );
	WsTradeItem* GetSelectedTraderOffer() const;

	Noesis::ObservableCollection<WsTradeItem>* GetPlayerStock() const
	{
		return m_playerStock;
	}
	Noesis::Ptr<Noesis::ObservableCollection<WsTradeItem>> m_playerStock;
	WsTradeItem* m_selectedPlayerStock = nullptr;
	void SetSelectedPlayerStock( WsTradeItem* item );
	WsTradeItem* GetSelectedPlayerStock() const;

	Noesis::ObservableCollection<WsTradeItem>* GetPlayerOffer() const
	{
		return m_playerOffer;
	}
	Noesis::Ptr<Noesis::ObservableCollection<WsTradeItem>> m_playerOffer;
	WsTradeItem* m_selectedPlayerOffer = nullptr;
	void SetSelectedPlayerOffer( WsTradeItem* item );
	WsTradeItem* GetSelectedPlayerOffer() const;

	CheckedAmount m_amountToTransfer = CheckedAmount::Amount1;

	bool GetAmount1Checked() const;
	bool GetAmount10Checked() const;
	bool GetAmount100Checked() const;
	bool GetAmountAllChecked() const;

	void SetAmount1Checked( bool checked );
	void SetAmount10Checked( bool checked );
	void SetAmount100Checked( bool checked );
	void SetAmountAllChecked( bool checked );

	void onTransferCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetTransferCmd() const
	{
		return &m_transferCmd;
	}
	NoesisApp::DelegateCommand m_transferCmd;

	Noesis::String m_traderValue = "0";
	Noesis::String m_playerValue = "0";

	void onTradeCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetTradeCmd() const
	{
		return &m_tradeCmd;
	}
	NoesisApp::DelegateCommand m_tradeCmd;

#pragma endregion TradeItemLists

	WorkshopProxy* m_proxy = nullptr;

	unsigned int m_workshopID = 0;
	Noesis::String m_name     = "-Workshop-";
	bool m_suspended          = false;
	bool m_acceptGenerated    = false;
	bool m_autoCraftMissing   = false;
	bool m_connectStockpile   = false;
	bool m_butcherCorpses     = false;
	bool m_butcherExcess      = false;
	bool m_catchFish          = false;
	bool m_processFish        = false;

	QString m_gui;

	NS_DECLARE_REFLECTION( WorkshopModel, NotifyPropertyChangedBase )
};

} // namespace IngnomiaGUI

#endif
