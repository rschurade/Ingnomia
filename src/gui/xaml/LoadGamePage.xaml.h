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

#ifndef __LoadGamePage_H__
#define __LoadGamePage_H__

#include <NoesisPCH.h>
#include <NsGui/RoutedEvent.h>

namespace IngnomiaGUI
{

////////////////////////////////////////////////////////////////////////////////////////////////////
class LoadGamePage final : public Noesis::UserControl
{
public:
	LoadGamePage();

private:
	void InitializeComponent();

	bool ConnectEvent( Noesis::BaseComponent* source, const char* event, const char* handler ) override;

	void onBack_Click( Noesis::BaseComponent* sender, const Noesis::RoutedEventArgs& args );

	void OnLoaded( Noesis::BaseComponent*, const Noesis::RoutedEventArgs& );

	NS_DECLARE_REFLECTION( LoadGamePage, UserControl )
};

} // namespace IngnomiaGUI

#endif
