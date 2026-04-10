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
/** @file selectionmodel.h
 *  @brief View model for the placement cursor info panel. Exposes action/cursor/first-click/
 *         size strings as Noesis bindings.
 */
#ifndef __SelectionModel_H__
#define __SelectionModel_H__

#include "../aggregatorselection.h"

#include <QString>

#include <NsApp/DelegateCommand.h>
#include <NsApp/NotifyPropertyChangedBase.h>
#include <NsCore/Noesis.h>
#include <NsCore/Nullable.h>
#include <NsCore/Ptr.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/ReflectionDeclareEnum.h>
#include <NsCore/String.h>
#include <NsGui/Collection.h>

class SelectionProxy;

namespace Noesis
{
template <class T>
class ObservableCollection;
}

namespace IngnomiaGUI
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Selection info panel view model. Pushes the four info strings (action, cursor,
///        first-click, size) from SelectionProxy into XAML bindings.
class SelectionModel final : public NoesisApp::NotifyPropertyChangedBase
{
public:
	SelectionModel();

	/// @brief Updates the displayed action name.
    void updateAction( QString action );
	/// @brief Updates the displayed cursor position string.
    void updateCursor( QString pos );
	/// @brief Updates the displayed first-click anchor position string.
    void updateFirstClick( QString pos );
	/// @brief Updates the displayed drag rectangle size string.
    void updateSize( QString size );

private:
	SelectionProxy* m_proxy = nullptr;

    Noesis::String m_action;
    Noesis::String m_cursor;
    Noesis::String m_firstClick;
    Noesis::String m_size;

    const char* getAction() const;
    const char* getCursor() const;
    const char* getFirstClick() const;
    const char* getSize() const;

	NS_DECLARE_REFLECTION( SelectionModel, NotifyPropertyChangedBase )
};

} // namespace IngnomiaGUI

#endif
