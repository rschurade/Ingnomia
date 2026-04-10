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
/** @file LoadGamePage.xaml.cpp
 *  @brief LoadGamePage user-control implementation: loads LoadGamePage.xaml on construction
 *         and provides a stub Back click handler. Real load logic lives in LoadGameModel.
 */
#include "LoadGamePage.xaml.h"

#include <QDebug>

using namespace IngnomiaGUI;
using namespace Noesis;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Constructs the LoadGamePage and registers the Loaded delegate.
LoadGamePage::LoadGamePage()
{
	Loaded() += MakeDelegate( this, &LoadGamePage::OnLoaded );
	InitializeComponent();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Loads the LoadGamePage.xaml resource.
void LoadGamePage::InitializeComponent()
{
	GUI::LoadComponent( this, "LoadGamePage.xaml" );
}

/// @brief Noesis event-connection callback. The commented-out NS_CONNECT_EVENT line is a
///        placeholder for hooking up the Back button via code-behind (currently unused).
bool LoadGamePage::ConnectEvent( BaseComponent* source, const char* event, const char* handler )
{
	//NS_CONNECT_EVENT( Button, Click, onBack_Click );
	return false;
}

/// @brief Back button click handler — currently a no-op (navigation is handled by the page host).
void LoadGamePage::onBack_Click( BaseComponent* sender, const RoutedEventArgs& args )
{
}

/// @brief Loaded event handler — no-op; LoadGameModel handles state.
void LoadGamePage::OnLoaded( Noesis::BaseComponent*, const Noesis::RoutedEventArgs& )
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION_( IngnomiaGUI::LoadGamePage, "IngnomiaGUI.LoadGamePage" )
