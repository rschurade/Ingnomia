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
#ifndef __MENU3D_VIEWMODEL_H__
#define __MENU3D_VIEWMODEL_H__

#include "NewGameModel.h"

#include <NsApp/DelegateCommand.h>
#include <NsApp/NotifyPropertyChangedBase.h>
#include <NsCore/Noesis.h>
#include <NsCore/Ptr.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/ReflectionDeclareEnum.h>
#include <NsCore/String.h>
#include <NsGui/Collection.h>

#include <QString>

class ProxyMainView;

namespace Noesis
{
template <class T>
class ObservableCollection;
}

namespace IngnomiaGUI
{

enum class State
{
	Main,
	Start,
	Settings,
	NewGame,
	LoadGame,
	Wait,
	GameRunning,
	Ingame
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class ViewModel final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	ViewModel();

	void setWindowSize( int w, int h );

	void OnBack( BaseComponent* param );

	void setUIScale( float value );
	void updateVersion( QString version );

	void OnResume( BaseComponent* param = nullptr );
	void OnContinueGameFinished( bool gameLoaded );

private:
	const NoesisApp::DelegateCommand* GetStart() const;
	const NoesisApp::DelegateCommand* GetSettings() const;
	const NoesisApp::DelegateCommand* GetNewGame() const;
	const NoesisApp::DelegateCommand* GetContinueGame() const;
	const NoesisApp::DelegateCommand* GetSetupGame() const;
	const NoesisApp::DelegateCommand* GetLoadGame() const;
	const NoesisApp::DelegateCommand* GetSaveGame() const;
	const NoesisApp::DelegateCommand* GetExit() const;
	const NoesisApp::DelegateCommand* GetBack() const;
	const NoesisApp::DelegateCommand* GetBackToMain() const;
	const NoesisApp::DelegateCommand* GetResume() const;
	const NoesisApp::DelegateCommand* GetFadeInCompleted() const;

	const char* GetPlatform() const;
	const char* GetShowMainMenu() const;
	const char* GetShowGameGUI() const;
	const char* GetWindowWidth() const;
	const char* GetWindowHeight() const;

	const char* GetVersion() const;

	void OnStart( BaseComponent* param );
	void OnSettings( BaseComponent* param );
	void OnNewGame( BaseComponent* param );
	void OnContinueGame( BaseComponent* param );
	void OnSetupGame( BaseComponent* param );
	void OnLoadGame( BaseComponent* param );
	void OnSaveGame( BaseComponent* param );
	void OnExit( BaseComponent* param );
	void OnBackToMain( BaseComponent* param );
	void OnFadeInCompleted( BaseComponent* params );

	State GetState() const;
	void SetState( State value );

	const NewGameModel* getNewGameModel() const;

private:
	NoesisApp::DelegateCommand _start;
	NoesisApp::DelegateCommand _settings;
	NoesisApp::DelegateCommand _newGame;
	NoesisApp::DelegateCommand _setupGame;
	NoesisApp::DelegateCommand _loadGame;
	NoesisApp::DelegateCommand _saveGame;
	NoesisApp::DelegateCommand _continueGame;
	NoesisApp::DelegateCommand _exit;
	NoesisApp::DelegateCommand _back;
	NoesisApp::DelegateCommand _backToMain;
	NoesisApp::DelegateCommand _resume;
	NoesisApp::DelegateCommand _fadeInCompleted;

	State _state;
	NewGameModel _newGameModel;

	bool _showMainMenu;
	bool _showGameGUI;
	bool _ingame;
	int _windowWidth;
	int _windowHeight;
	int _realWidth;
	int _realHeight;
	float m_scale = 1.0;
	Noesis::String _windowWidthString;
	Noesis::String _windowHeightString;
	Noesis::String m_version;

	ProxyMainView* m_proxy = nullptr;

	NS_DECLARE_REFLECTION( ViewModel, NotifyPropertyChangedBase )
};

} // namespace IngnomiaGUI

NS_DECLARE_REFLECTION_ENUM( IngnomiaGUI::State )

#endif
