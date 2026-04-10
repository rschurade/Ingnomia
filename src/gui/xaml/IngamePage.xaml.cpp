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
/** @file IngamePage.xaml.cpp
 *  @brief IngamePage user-control implementation. Loads IngamePage.xaml on construction.
 */
#include "IngamePage.xaml.h"

using namespace IngnomiaGUI;
using namespace Noesis;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Constructs the IngamePage user control.
IngamePage::IngamePage()
{
	InitializeComponent();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Loads the IngamePage.xaml resource.
void IngamePage::InitializeComponent()
{
	GUI::LoadComponent( this, "IngamePage.xaml" );
}

/// @brief Noesis event-connection callback. IngamePage has no code-behind events; the
///        commented-out NS_CONNECT_EVENT lines are kept as placeholders for future menu actions.
bool IngamePage::ConnectEvent( BaseComponent* source, const char* event, const char* handler )
{
	//NS_CONNECT_EVENT( Button, Click, onMMExit_Click );
	//NS_CONNECT_EVENT( Button, Click, onMMSettings_Click );
	return false;
}

/*
void IngamePage::onMMExit_Click( BaseComponent* sender, const RoutedEventArgs& args )
{
	//emit signalExit();
}

*/

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION_( IngnomiaGUI::IngamePage, "IngnomiaGUI.IngamePage" );
