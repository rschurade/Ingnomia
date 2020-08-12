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
#include "ViewModel.h"

#include "../../base/config.h"
#include "../../base/db.h"
#include "../../game/gamemanager.h"
#include "../../game/newgamesettings.h"
#include "../eventconnector.h"
#include "../strings.h"
#include "ProxyMainView.h"

#include <NsApp/Application.h>
#include <NsCore/Log.h>
#include <NsCore/ReflectionImplement.h>
#include <NsCore/ReflectionImplementEnum.h>
#include <NsGui/ObservableCollection.h>
#include <NsGui/UIElement.h>

#include <QDebug>

#include <functional>

using namespace IngnomiaGUI;
using namespace Noesis;
using namespace NoesisApp;

////////////////////////////////////////////////////////////////////////////////////////////////////
ViewModel::ViewModel()
{
	m_proxy = new ProxyMainView;
	m_proxy->setParent( this );

	_start.SetExecuteFunc( MakeDelegate( this, &ViewModel::OnStart ) );
	_settings.SetExecuteFunc( MakeDelegate( this, &ViewModel::OnSettings ) );
	_newGame.SetExecuteFunc( MakeDelegate( this, &ViewModel::OnNewGame ) );
	_setupGame.SetExecuteFunc( MakeDelegate( this, &ViewModel::OnSetupGame ) );
	_loadGame.SetExecuteFunc( MakeDelegate( this, &ViewModel::OnLoadGame ) );
	_saveGame.SetExecuteFunc( MakeDelegate( this, &ViewModel::OnSaveGame ) );
	_continueGame.SetExecuteFunc( MakeDelegate( this, &ViewModel::OnContinueGame ) );
	_exit.SetExecuteFunc( MakeDelegate( this, &ViewModel::OnExit ) );
	_back.SetExecuteFunc( MakeDelegate( this, &ViewModel::OnBack ) );
	_backToMain.SetExecuteFunc( MakeDelegate( this, &ViewModel::OnBackToMain ) );
	_resume.SetExecuteFunc( MakeDelegate( this, &ViewModel::OnResume ) );
	_fadeInCompleted.SetExecuteFunc( MakeDelegate( this, &ViewModel::OnFadeInCompleted ) );
	_pause.SetExecuteFunc( MakeDelegate( this, &ViewModel::OnPause ) );
	_guiZoom.SetExecuteFunc( MakeDelegate( this, &ViewModel::OnGuiZoom ) );

	_state        = State::Main;
	_showMainMenu = true;
	_showGameGUI  = false;
	_ingame       = false;
	setWindowSize( 1920, 1080 );
}

void ViewModel::setWindowSize( int w, int h )
{
	_realWidth          = w;
	_realHeight         = h;
	_windowWidth        = _realWidth / Config::getInstance().get( "GUIScale" ).toFloat();
	_windowHeight       = _realHeight / Config::getInstance().get( "GUIScale" ).toFloat();
	_windowWidthString  = QString::number( _windowWidth ).toStdString().c_str();
	_windowHeightString = QString::number( _windowHeight ).toStdString().c_str();
	OnPropertyChanged( "WindowWidth" );
	OnPropertyChanged( "WindowHeight" );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const DelegateCommand* ViewModel::GetStart() const
{
	return &_start;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const DelegateCommand* ViewModel::GetSettings() const
{
	return &_settings;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const DelegateCommand* ViewModel::GetNewGame() const
{
	return &_newGame;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const DelegateCommand* ViewModel::GetContinueGame() const
{
	return &_continueGame;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const DelegateCommand* ViewModel::GetSetupGame() const
{
	return &_setupGame;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const DelegateCommand* ViewModel::GetLoadGame() const
{
	return &_loadGame;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const DelegateCommand* ViewModel::GetSaveGame() const
{
	return &_saveGame;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const DelegateCommand* ViewModel::GetExit() const
{
	return &_exit;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const DelegateCommand* ViewModel::GetBack() const
{
	return &_back;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const DelegateCommand* ViewModel::GetBackToMain() const
{
	return &_backToMain;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const DelegateCommand* ViewModel::GetResume() const
{
	return &_resume;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const DelegateCommand* ViewModel::GetFadeInCompleted() const
{
	return &_fadeInCompleted;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const NewGameModel* ViewModel::getNewGameModel() const
{
	return &_newGameModel;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const char* ViewModel::GetPlatform() const
{
#if defined( NS_PLATFORM_XBOX_ONE ) || defined( NS_PLATFORM_NX )
	return "XBOX";
#elif defined( NS_PLATFORM_PS4 )
	return "PS4";
#else
	return "PC";
#endif
}

const char* ViewModel::GetShowMainMenu() const
{
	if ( _showMainMenu )
	{
		return "Visible";
	}
	return "Hidden";
}

const char* ViewModel::GetShowGameGUI() const
{
	if ( _showGameGUI )
	{
		return "Visible";
	}
	return "Hidden";
}

const char* ViewModel::GetWindowWidth() const
{
	return _windowWidthString.Str();
}

const char* ViewModel::GetWindowHeight() const
{
	return _windowHeightString.Str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ViewModel::OnStart( BaseComponent* )
{
	SetState( State::Start );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ViewModel::OnSettings( BaseComponent* )
{
	qDebug() << "ViewModel OnSettings";
	NS_LOG_INFO( "ViewModel OnSettings" );
	SetState( State::Settings );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ViewModel::OnNewGame( BaseComponent* )
{
	qDebug() << "ViewModel OnNewGame";
	NS_LOG_INFO( "ViewModel OnNewGame" );
	SetState( State::Wait );
	GameManager::getInstance().startNewGame( std::bind( &ViewModel::OnNewGameFinished, this ) );
}

void ViewModel::OnNewGameFinished()
{
	qDebug() << "new game setup finished";
	SetState( State::GameRunning );
	_ingame       = true;
	_showMainMenu = false;
	OnPropertyChanged( "ShowMainMenu" );
	_showGameGUI = true;
	OnPropertyChanged( "ShowGameGui" );
}

void ViewModel::OnContinueGame( BaseComponent* param )
{
	using namespace std::placeholders;

	if ( !param )
	{
		qDebug() << "ViewModel OnContinueGame last game";
		SetState( State::Wait );
		GameManager::getInstance().continueLastGame( std::bind( &ViewModel::OnContinueGameFinished, this, _1 ) );
	}
	else
	{
		qDebug() << "ViewModel OnContinueGame" << param->ToString().Str();
		SetState( State::Wait );
		GameManager::getInstance().loadGame( param->ToString().Str(), std::bind( &ViewModel::OnContinueGameFinished, this, _1 ) );
	}
	NS_LOG_INFO( "ViewModel OnContinueGame" );
	//SetState( State::Wait );
	//GameManager::getInstance().startNewGame( std::bind( &ViewModel::OnContinueGameFinished, this ) );
}

void ViewModel::OnContinueGameFinished( bool gameLoaded )
{
	if ( gameLoaded )
	{
		qDebug() << "load game finished";
		SetState( State::GameRunning );
		_showMainMenu = false;
		_ingame       = true;
		OnPropertyChanged( "ShowMainMenu" );
		_showGameGUI = true;
		OnPropertyChanged( "ShowGameGui" );
	}
	else
	{
		SetState( State::Main );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ViewModel::OnSetupGame( BaseComponent* )
{
	qDebug() << "ViewModel OnSetupGame";
	NS_LOG_INFO( "ViewModel OnNewGame" );
	SetState( State::NewGame );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ViewModel::OnLoadGame( BaseComponent* )
{
	qDebug() << "ViewModel OnLoadGame";
	NS_LOG_INFO( "ViewModel OnLoadGame" );
	SetState( State::LoadGame );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ViewModel::OnSaveGame( BaseComponent* )
{
	qDebug() << "ViewModel OnSaveGame";
	NS_LOG_INFO( "ViewModel OnSaveGame" );

	GameManager::getInstance().saveGame();

	SetState( State::GameRunning );
	_showMainMenu = false;
	_ingame       = true;
	OnPropertyChanged( "ShowMainMenu" );
	_showGameGUI = true;
	OnPropertyChanged( "ShowGameGui" );
	GameManager::getInstance().setShowMainMenu( false );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ViewModel::OnExit( BaseComponent* )
{
	qDebug() << "ViewModel OnExit";
	NS_LOG_INFO( "Exiting game" );
	//NoesisApp::Application::Current()->Shutdown();
	EventConnector::getInstance().onExit();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ViewModel::OnBackToMain( BaseComponent* )
{
	_showMainMenu = true;
	_ingame       = false;
	OnPropertyChanged( "ShowMainMenu" );
	_showGameGUI = false;
	OnPropertyChanged( "ShowGameGui" );
	GameManager::getInstance().setShowMainMenu( true );
	SetState( State::Main );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ViewModel::OnBack( BaseComponent* )
{
	qDebug() << "ViewModel OnBack";
	switch ( _state )
	{
		case State::Main:
			break;
		case State::GameRunning:
			_showMainMenu = true;
			OnPropertyChanged( "ShowMainMenu" );
			_showGameGUI = false;
			OnPropertyChanged( "ShowGameGui" );
			SetState( State::Ingame );
			GameManager::getInstance().setShowMainMenu( true );
			break;
		case State::Settings:
		case State::LoadGame:
		case State::NewGame:
			if ( _ingame )
			{
				SetState( State::Ingame );
			}
			else
			{
				SetState( State::Main );
			}
			_showMainMenu = true;
			_showGameGUI = false;
			OnPropertyChanged( "ShowMainMenu" );
			OnPropertyChanged( "ShowGameGui" );
			GameManager::getInstance().setShowMainMenu( true );
			break;
		case State::Start:
		case State::Ingame:
		{
			_showMainMenu = false;
			_ingame       = false;
			OnPropertyChanged( "ShowMainMenu" );
			_showGameGUI = true;
			OnPropertyChanged( "ShowGameGui" );
			GameManager::getInstance().setShowMainMenu( false );
			SetState( State::GameRunning );
			break;
		}

		default:
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ViewModel::OnResume( BaseComponent* )
{
	SetState( State::GameRunning );
	_showMainMenu = false;
	_showGameGUI = true;
	OnPropertyChanged( "ShowGameGui" );
	GameManager::getInstance().setShowMainMenu( false );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ViewModel::OnFadeInCompleted( BaseComponent* param )
{
	UIElement* target = (UIElement*)param;
	target->Focus();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
State ViewModel::GetState() const
{
	return _state;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ViewModel::SetState( State value )
{
	if ( _state != value )
	{
		_state = value;
		OnPropertyChanged( "State" );
	}
}

void ViewModel::OnPause( BaseComponent* params )
{
	qDebug() << "viewmodel on pause";
	GameManager::getInstance().setPaused( !GameManager::getInstance().paused() );
}

const NoesisApp::DelegateCommand* ViewModel::GetPause() const
{
	return &_pause;
}

void ViewModel::OnGuiZoom( BaseComponent* params )
{

	QString param( params->ToString().Str() );
	float scale = Config::getInstance().get( "GUIScale" ).toFloat();
	if ( param == "In" )
	{
		scale += 0.1f;
		scale = qMin( 3.f, scale );
	}
	else
	{
		scale -= 0.1f;
		scale = qMax( 0.5f, scale );
	}
	Config::getInstance().set( "GUIScale", scale );

	// Trigger layout update
	setWindowSize( _realWidth, _realHeight );
}

const NoesisApp::DelegateCommand* ViewModel::GetGuiZoom() const
{
	return &_guiZoom;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION( IngnomiaGUI::ViewModel, "IngnomiaGUI.ViewModel" )
{
	// menu properties
	NsProp( "Start", &ViewModel::GetStart );
	NsProp( "Settings", &ViewModel::GetSettings );
	NsProp( "NewGame", &ViewModel::GetNewGame );
	NsProp( "ContinueGame", &ViewModel::GetContinueGame );
	NsProp( "SetupGame", &ViewModel::GetSetupGame );
	NsProp( "LoadGame", &ViewModel::GetLoadGame );
	NsProp( "SaveGame", &ViewModel::GetSaveGame );
	NsProp( "Exit", &ViewModel::GetExit );
	NsProp( "Back", &ViewModel::GetBack );
	NsProp( "BackToMain", &ViewModel::GetBackToMain );
	NsProp( "CmdPause", &ViewModel::GetPause );
	NsProp( "Resume", &ViewModel::GetResume );
	NsProp( "FadeInCompleted", &ViewModel::GetFadeInCompleted );
	NsProp( "State", &ViewModel::GetState, &ViewModel::SetState );
	NsProp( "Platform", &ViewModel::GetPlatform );
	NsProp( "ShowMainMenu", &ViewModel::GetShowMainMenu );
	NsProp( "ShowGameGUI", &ViewModel::GetShowGameGUI );
	NsProp( "WindowWidth", &ViewModel::GetWindowWidth );
	NsProp( "WindowHeight", &ViewModel::GetWindowHeight );
	NsProp( "CmdGuiZoom", &ViewModel::GetGuiZoom );

	NsProp( "NewGameModel", &ViewModel::getNewGameModel );

	// New Game properties
}

NS_IMPLEMENT_REFLECTION_ENUM( IngnomiaGUI::State, "IngnomiaGUI.State" )
{
	NsVal( "Main", State::Main );
	NsVal( "Start", State::Start );
	NsVal( "Settings", State::Settings );
	NsVal( "NewGame", State::NewGame );
	NsVal( "LoadGame", State::LoadGame );
	NsVal( "Wait", State::Wait );
	NsVal( "GameRunning", State::GameRunning );
	NsVal( "Ingame", State::Ingame );
}
