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
/** @file Population.xaml.cpp
 *  @brief PopulationWindow implementation: loads the XAML and synchronises the skill /
 *         schedule scrollviewers so the name/content/header columns stay aligned.
 */
#include "Population.xaml.h"

using namespace IngnomiaGUI;
using namespace Noesis;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Constructs the PopulationWindow and registers the Loaded delegate.
PopulationWindow::PopulationWindow()
{
	Loaded() += MakeDelegate( this, &PopulationWindow::OnLoaded );
	InitializeComponent();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Loads the PopulationWindow.xaml resource.
void PopulationWindow::InitializeComponent()
{
	GUI::LoadComponent( this, "PopulationWindow.xaml" );
}

/// @brief Helper: returns the ScrollViewer associated with a virtualised list, either the
///        given element itself or its PART_Scroll template child.
/// @param root Framework element expected to host or be a ScrollViewer.
/// @return Pointer to the inner ScrollViewer, or nullptr (with an NS_CHECK failure) on type mismatch.
static Noesis::ScrollViewer* getVirtualListScroller( Noesis::FrameworkElement* root )
{
	NS_CHECK( root != 0, "Missing input" );
	if ( auto scrollViewer = Noesis::DynamicCast<Noesis::ScrollViewer*>(root) )
	{
		return scrollViewer;
	}
	if(auto child = root->GetTemplateChild<BaseComponent>( "PART_Scroll" ))
	{
		if ( auto scrollViewer = Noesis::DynamicCast<Noesis::ScrollViewer*>( child ) )
		{
			return scrollViewer;
		}
		else
		{
			NS_CHECK( root != 0, "Type mismatch" );
		}
	}
	NS_CHECK( false, "No ScrollViewer found" );
	return nullptr;
}

/// @brief Loaded handler: looks up the five named ScrollViewer elements by XAML x:Name and
///        wires their ScrollChanged events to OnScroll so the columns scroll in sync.
void PopulationWindow::OnLoaded( Noesis::BaseComponent*, const Noesis::RoutedEventArgs& )
{

	m_skillNameScroll       = getVirtualListScroller( this->FindName<FrameworkElement>( "SVNames" ) );
	m_skillContentScroll    = getVirtualListScroller( this->FindName<FrameworkElement>( "SVSkills" ) );
	m_skillHeaderScroll     = getVirtualListScroller( this->FindName<FrameworkElement>( "SVSkillHeaders" ) );
	m_scheduleNameScroll    = getVirtualListScroller( this->FindName<FrameworkElement>( "SchedNames" ) );
	m_scheduleContentScroll = getVirtualListScroller( this->FindName<FrameworkElement>( "SVActivities" ) );

	auto scrollHandler = Noesis::MakeDelegate( this, &SelfClass::OnScroll );
	m_skillNameScroll->ScrollChanged() += scrollHandler;
	m_skillContentScroll->ScrollChanged() += scrollHandler;
	m_skillHeaderScroll->ScrollChanged() += scrollHandler;
	m_scheduleNameScroll->ScrollChanged() += scrollHandler;
	m_scheduleContentScroll->ScrollChanged() += scrollHandler;
}

/// @brief ScrollChanged handler: propagates the sender's vertical/horizontal offset to the
///        sibling scrollviewers so the name, header, and content columns stay aligned.
/// @param sender The ScrollViewer whose offset changed.
/// @param args   Scroll-changed event args (unused — offsets are read directly from the senders).
void PopulationWindow::OnScroll( Noesis::BaseComponent* sender, const Noesis::ScrollChangedEventArgs& args )
{
	if ( sender == m_skillNameScroll )
	{
		m_skillContentScroll->ScrollToVerticalOffset( m_skillNameScroll->GetVerticalOffset() );
	}
	else if ( sender == m_skillContentScroll )
	{
		m_skillNameScroll->ScrollToVerticalOffset( m_skillContentScroll->GetVerticalOffset() );
		m_skillHeaderScroll->ScrollToHorizontalOffset( m_skillContentScroll->GetHorizontalOffset() );
	}
	else if ( sender == m_skillHeaderScroll )
	{
		m_skillContentScroll->ScrollToHorizontalOffset( m_skillHeaderScroll->GetHorizontalOffset() );
	}
	else if ( sender == m_scheduleNameScroll )
	{
		m_scheduleContentScroll->ScrollToVerticalOffset( m_scheduleNameScroll->GetVerticalOffset() );
	}
	else if ( sender == m_scheduleContentScroll )
	{
		m_scheduleNameScroll->ScrollToVerticalOffset( m_scheduleContentScroll->GetVerticalOffset() );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION_( IngnomiaGUI::PopulationWindow, "IngnomiaGUI.PopulationWindow" )
