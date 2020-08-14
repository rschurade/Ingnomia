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
#ifndef __GameModel_H__
#define __GameModel_H__

#include "../../base/enums.h"
#include "NewGameModel.h"

#include <NsApp/DelegateCommand.h>
#include <NsApp/NotifyPropertyChangedBase.h>
#include <NsCore/Noesis.h>
#include <NsCore/Ptr.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/ReflectionDeclareEnum.h>
#include <NsCore/String.h>
#include <NsGui/BitmapImage.h>
#include <NsGui/BitmapSource.h>
#include <NsGui/Button.h>
#include <NsGui/Collection.h>
#include <NsGui/ImageSource.h>

#include <QString>

class ProxyGameView;

namespace Noesis
{
template <class T>
class ObservableCollection;
}

namespace IngnomiaGUI
{

////////////////////////////////////////////////////////////////////////////////////////////////////
class CommandButton final : public Noesis::BaseComponent
{
public:
	CommandButton( QString name, QString sid );

	const char* GetName() const;
	const char* GetID() const;

private:
	Noesis::String _name;
	Noesis::String _sid;

	NS_DECLARE_REFLECTION( CommandButton, Noesis::BaseComponent )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class BuildButton final : public Noesis::BaseComponent
{
public:
	BuildButton( QString name, QString sid, QString image );

	const char* GetName() const;
	const char* GetID() const;
	const char* GetImage() const;

private:
	Noesis::String _name;
	Noesis::String _sid;
	Noesis::String _image;

	NS_DECLARE_REFLECTION( BuildButton, Noesis::BaseComponent )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class AvailableMaterial final : public Noesis::BaseComponent
{
public:
	AvailableMaterial( QString sid, int amount, QString item );

	const char* GetName() const;
	const char* sid() const;
	const char* amount() const;

private:
	Noesis::String _name;
	Noesis::String _sid;
	Noesis::String _amount;

	NS_DECLARE_REFLECTION( AvailableMaterial, Noesis::BaseComponent )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class NRequiredItem final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	NRequiredItem( QString sid, int amount );

	const char* GetName() const;
	const char* amount() const;
	const QString sid();

	Noesis::ObservableCollection<AvailableMaterial>* availableMaterials() const;

	void SetSelectedMaterial( AvailableMaterial* mat );
	AvailableMaterial* GetSelectedMaterial() const;

private:
	Noesis::String _name;
	QString _sid;
	Noesis::String _amount;

	AvailableMaterial* _selectedMaterial;

	Noesis::Ptr<Noesis::ObservableCollection<AvailableMaterial>> _availableMaterials;

	NS_DECLARE_REFLECTION( NRequiredItem, Noesis::BaseComponent )
};

enum class BuildItemType
{
	Workshop,
	Item,
	Terrain
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class BuildItem final : public Noesis::BaseComponent
{
public:
	BuildItem( QString name, QString sid, BuildItemType type );

	const char* GetName() const;
	QString sid() const;
	Noesis::ObservableCollection<NRequiredItem>* requiredItems() const;

private:
	Noesis::String _name;
	QString _sid;
	BuildItemType _type;
	Noesis::Ptr<Noesis::BitmapSource> _bitmapSource;

	Noesis::Ptr<Noesis::ObservableCollection<NRequiredItem>> _requiredItems;

	void onCmdBuild( BaseComponent* param );

	const NoesisApp::DelegateCommand* GetCmdBuild() const;
	const Noesis::ImageSource* getBitmapSource() const;

	NoesisApp::DelegateCommand _cmdBuild;

	NS_DECLARE_REFLECTION( BuildItem, Noesis::BaseComponent )
};

////////////////////////////////////////////////////////////////////////////////////////////////////
enum class ButtonSelection
{
	None,
	Build,
	Mine,
	Agriculture,
	Designation,
	Job,
	Magic
};

enum class BuildSelection
{
	None,
	Workshop,
	Wall,
	Floor,
	Stairs,
	Ramps,
	Containers,
	Fence,
	Furniture,
	Utility
};

enum class ShownInfo
{
	None,
	TileInfo,
	Stockpile,
	Workshop,
	Agriculture,
	Population,
	CreatureInfo,
	Debug,
	Neighbors,
	Military
};


struct GuiMessageEvent {
	unsigned int id;
	QString title;
	QString msg;
	bool pause;
	bool yesno;
};




////////////////////////////////////////////////////////////////////////////////////////////////////
class GameModel final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	GameModel();

	void setTimeAndDate( int minute, int hour, int day, QString season, int year, QString sunStatus );
	void setViewLevel( int level );
	void updatePause();
	void updateGameSpeed();

	void eventMessage( unsigned int id, QString title, QString msg, bool pause, bool yesno );

	void onBuild();
	void onShowTileInfo( unsigned tileID );
	void onShowStockpileInfo( unsigned stockpileID );
	void onShowWorkshopInfo( unsigned workshopID );
	void onShowAgriculture( unsigned id );

	void setShownInfo( ShownInfo info );

private:
	void setGameSpeed( GameSpeed value );

	bool getPaused() const;
	bool getNormalSpeed() const;
	bool getFastSpeed() const;

	void setPaused( bool value );
	void setNormalSpeed( bool value );
	void setFastSpeed( bool value );

	const char* getYear() const;
	const char* getDay() const;
	const char* getTime() const;
	const char* getLevel() const;
	const char* getSun() const;
	const char* getTimeImagePath() const;

	const char* showCommandButtons() const;

	const char* showCategoryButtons() const;

	const char* getShowTileInfo() const;
	void setShowTileInfo( unsigned int tileID );

	const char* getShowStockpile() const;
	void setShowStockpile( unsigned int stockpileID );

	const char* getShowAgriculture() const;
	void setShowAgriculture( unsigned int id );

	const char* getShowWorkshop() const;
	void setShowWorkshop( unsigned int workshopID );

	const char* getShowPopulation() const;
	void setShowPopulation( bool value );

	const char* getShowDebug() const;
	void setShowDebug( bool value );

	const char* getShowNeighbors() const;
	void setShowNeighbors( bool value );

	const char* getShowMilitary() const;
	void setShowMilitary( bool value );


	const char* getShowCreatureInfo() const;

	Noesis::ObservableCollection<CommandButton>* GetCommandButtons() const;
	Noesis::ObservableCollection<BuildButton>* GetBuildButtons() const;
	Noesis::ObservableCollection<BuildItem>* GetBuildItems() const;

	void OnCmdButtonCommand( BaseComponent* param );
	void OnCmdCategory( BaseComponent* param );

	void OnCmdBack( BaseComponent* param );
	void OnCmdSimple( BaseComponent* param );

	void setCategory( const char* cat );

	void CmdLeftCommandButton( BaseComponent* param );
	void CmdRightCommandButton( BaseComponent* param );

	const NoesisApp::DelegateCommand* GetCmdButtonCommand() const;
	const NoesisApp::DelegateCommand* GetCmdCategory() const;
	const NoesisApp::DelegateCommand* GetCmdBack() const;
	const NoesisApp::DelegateCommand* GetSimpleCommand() const;

	const NoesisApp::DelegateCommand* GetCmdLeftCommandButton() const;
	const NoesisApp::DelegateCommand* GetCmdRightCommandButton() const;

	NoesisApp::DelegateCommand _cmdBack;

	NoesisApp::DelegateCommand _cmdButtonCommand;
	NoesisApp::DelegateCommand _cmdCategory;
	NoesisApp::DelegateCommand _cmdSimple;

	NoesisApp::DelegateCommand _cmdLeftCommandButton;
	NoesisApp::DelegateCommand _cmdRightCommandButton;

private:
	Noesis::String _year;
	Noesis::String _day;
	Noesis::String _time;
	Noesis::String _level;
	Noesis::String _sun;
	Noesis::String _timeImagePath;

	ProxyGameView* m_proxy = nullptr;

	ButtonSelection m_selectedButtons = ButtonSelection::None;
	BuildSelection m_buildSelection   = BuildSelection::None;

	ShownInfo m_shownInfo         = ShownInfo::None;
	unsigned int m_tileInfoID     = 0;
	unsigned int m_prevTileInfoID = 0;
	unsigned int m_stockpileID    = 0;
	unsigned int m_workshopID     = 0;
	unsigned int m_agricultureID  = 0;

	Noesis::Ptr<Noesis::ObservableCollection<CommandButton>> _commandButtons;
	Noesis::Ptr<Noesis::ObservableCollection<BuildButton>> _buildButtons;
	Noesis::Ptr<Noesis::ObservableCollection<BuildItem>> _buildItems;

	void onCloseWindowCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetCloseWindowCmd() const
	{
		return &m_closeWindowCmd;
	}
	NoesisApp::DelegateCommand m_closeWindowCmd;

	void onOpenGnomeDetailsCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetOpenGnomeDetailsCmd() const
	{
		return &m_openGnomeDetailsCmd;
	}
	NoesisApp::DelegateCommand m_openGnomeDetailsCmd;

	bool m_showMessageWindow = false;
	Noesis::String m_messageHeader;
	Noesis::String m_messageText;
	bool m_messageRequiresAnswer = false;

	const char* getShowMessage() const;
	const char* getShowMessageButtonOk() const;
	const char* getShowMessageButtonYesNo() const;

	bool m_showMessageButtonOk = false;
	bool m_showMessageButtonYesNo = false;
	unsigned int m_messageID = 0;

	const char* getMessageHeader() const;
	const char* getMessageText() const;

	void onMessageButtonCmd( BaseComponent* param );
	const NoesisApp::DelegateCommand* GetMessageButtonCmd() const
	{
		return &m_messageButtonCmd;
	}
	NoesisApp::DelegateCommand m_messageButtonCmd;

	QList<GuiMessageEvent> m_messageQueue;

	NS_DECLARE_REFLECTION( GameModel, NotifyPropertyChangedBase )
};

} // namespace IngnomiaGUI

#endif
