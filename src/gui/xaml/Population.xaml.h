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
/** @file Population.xaml.h
 *  @brief Code-behind for the Population window (PopulationWindow.xaml). Unlike most other
 *         XAML user controls, this one needs code-behind to keep the skill/schedule
 *         scrollviewers synchronised across the name and content columns.
 */
#ifndef __PopulationWindow_H__
#define __PopulastionPage_H__

#include <NoesisPCH.h>

namespace IngnomiaGUI
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Code-behind for the PopulationWindow Noesis user control. Forwards scroll events
///        between the three skill scrollviewers and the two schedule scrollviewers so the
///        header row and the per-gnome rows stay aligned.
class PopulationWindow final : public Noesis::UserControl
{
public:
	PopulationWindow();

private:
	void InitializeComponent();

	void OnLoaded( Noesis::BaseComponent*, const Noesis::RoutedEventArgs& );

	void OnScroll( Noesis::BaseComponent* sender, const Noesis::ScrollChangedEventArgs& );

	Noesis::ScrollViewer* m_skillNameScroll = nullptr;    ///< Skill tab — left (gnome names) scrollviewer.
	Noesis::ScrollViewer* m_skillContentScroll = nullptr; ///< Skill tab — right (skill grid) scrollviewer.
	Noesis::ScrollViewer* m_skillHeaderScroll = nullptr;  ///< Skill tab — top header row scrollviewer.
	Noesis::ScrollViewer* m_scheduleNameScroll  = nullptr;///< Schedule tab — left (gnome names) scrollviewer.
	Noesis::ScrollViewer* m_scheduleContentScroll = nullptr;///< Schedule tab — right (hours grid) scrollviewer.


	NS_DECLARE_REFLECTION( PopulationWindow, UserControl )



};

} // namespace IngnomiaGUI

#endif
