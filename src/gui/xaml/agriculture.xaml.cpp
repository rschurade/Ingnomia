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
/** @file agriculture.xaml.cpp
 *  @brief Agriculture user-control implementation: loads Agriculture.xaml on construction
 *         and registers the Loaded event delegate. Most behaviour lives in AgricultureModel.
 */
#include "agriculture.xaml.h"

using namespace IngnomiaGUI;
using namespace Noesis;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Constructs the Agriculture user control and binds OnLoaded.
Agriculture::Agriculture()
{
	Loaded() += MakeDelegate( this, &Agriculture::OnLoaded );
	InitializeComponent();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Loads the Agriculture.xaml resource into this user control.
void Agriculture::InitializeComponent()
{
	GUI::LoadComponent( this, "Agriculture.xaml" );
}

/// @brief Noesis event-connection callback. The Agriculture window has no code-behind events,
///        so this returns false (everything is bound via the view model in XAML).
bool Agriculture::ConnectEvent( BaseComponent* source, const char* event, const char* handler )
{
	return false;
}

/// @brief Loaded event handler — currently a no-op; the AgricultureModel handles GUI state.
void Agriculture::OnLoaded( Noesis::BaseComponent*, const Noesis::RoutedEventArgs& )
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION_( IngnomiaGUI::Agriculture, "IngnomiaGUI.Agriculture" )
