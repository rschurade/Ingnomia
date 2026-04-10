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
/** @file military.xaml.cpp
 *  @brief MilitaryGui user-control implementation: loads MilitaryGui.xaml on construction.
 */
#include "military.xaml.h"

using namespace IngnomiaGUI;
using namespace Noesis;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Constructs the MilitaryGui user control and registers the Loaded delegate.
MilitaryGui::MilitaryGui()
{
	Loaded() += MakeDelegate( this, &MilitaryGui::OnLoaded );
	InitializeComponent();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Loads the MilitaryGui.xaml resource.
void MilitaryGui::InitializeComponent()
{
	GUI::LoadComponent( this, "MilitaryGui.xaml" );
}

/// @brief Noesis event-connection callback. MilitaryGui has no code-behind events.
bool MilitaryGui::ConnectEvent( BaseComponent* source, const char* event, const char* handler )
{
	return false;
}

/// @brief Loaded event handler — no-op; MilitaryModel handles state.
void MilitaryGui::OnLoaded( Noesis::BaseComponent*, const Noesis::RoutedEventArgs& )
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION_( IngnomiaGUI::MilitaryGui, "IngnomiaGUI.MilitaryGui" )
