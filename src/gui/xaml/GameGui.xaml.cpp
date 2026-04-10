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

/** @file GameGui.xaml.cpp
 *  @brief GameGui user-control implementation: loads GameGui.xaml on construction. State is
 *         held by GameModel, bound via the view model in XAML.
 */
#include "GameGui.xaml.h"

using namespace IngnomiaGUI;
using namespace Noesis;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Constructs the GameGui user control and registers the Loaded delegate.
GameGui::GameGui()
{
	Loaded() += MakeDelegate( this, &GameGui::OnLoaded );
	InitializeComponent();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Loads the GameGui.xaml resource into this user control.
void GameGui::InitializeComponent()
{
	GUI::LoadComponent( this, "GameGui.xaml" );
}

/// @brief Noesis event-connection callback. GameGui has no code-behind events.
bool GameGui::ConnectEvent( BaseComponent* source, const char* event, const char* handler )
{
	return false;
}

/// @brief Loaded event handler — no-op; GameModel handles state.
void GameGui::OnLoaded( Noesis::BaseComponent*, const Noesis::RoutedEventArgs& )
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION_( IngnomiaGUI::GameGui, "IngnomiaGUI.GameGui" )
