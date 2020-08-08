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
#ifndef __NeighborsGui_H__
#define __NeighborsGui_H__

#include <NoesisPCH.h>

namespace IngnomiaGUI
{

////////////////////////////////////////////////////////////////////////////////////////////////////
class NeighborsGui final : public Noesis::UserControl
{
public:
	NeighborsGui();

private:
	void InitializeComponent();

	bool ConnectEvent( BaseComponent* source, const char* event, const char* handler ) override;

	void OnLoaded( Noesis::BaseComponent*, const Noesis::RoutedEventArgs& );

	NS_DECLARE_REFLECTION( NeighborsGui, UserControl )
};

} // namespace IngnomiaGUI

#endif
