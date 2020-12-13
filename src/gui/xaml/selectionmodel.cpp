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
#include "selectionmodel.h"
#include "selectionproxy.h"

#include <NsApp/Application.h>
#include <NsCore/Log.h>
#include <NsCore/ReflectionImplement.h>
#include <NsCore/ReflectionImplementEnum.h>
#include <NsGui/ObservableCollection.h>
#include <NsGui/UIElement.h>

#include <QDebug>

using namespace IngnomiaGUI;
using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
SelectionModel::SelectionModel()
{
	m_proxy = new SelectionProxy;
	m_proxy->setParent( this );
}

void SelectionModel::updateAction( QString action )
{
    m_action = action.toStdString().c_str();
    OnPropertyChanged( "Action" );
}

void SelectionModel::updateCursor( QString pos )
{
    m_cursor = pos.toStdString().c_str();
    OnPropertyChanged( "Cursor" );
}

void SelectionModel::updateFirstClick( QString pos )
{
    m_firstClick = pos.toStdString().c_str();
    OnPropertyChanged( "FirstClick" );
}

void SelectionModel::updateSize( QString size )
{
    m_size = size.toStdString().c_str();
    OnPropertyChanged( "Size" );
}


const char* SelectionModel::getAction() const
{
    return m_action.Str();
}

const char* SelectionModel::getCursor() const
{
    return m_cursor.Str();
}

const char* SelectionModel::getFirstClick() const
{
    return m_firstClick.Str();
}

const char* SelectionModel::getSize() const
{
    return m_size.Str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION( SelectionModel, "IngnomiaGUI.SelectionModel" )
{
    NsProp( "Action", &SelectionModel::getAction );
    NsProp( "Cursor", &SelectionModel::getCursor );
    NsProp( "FirstClick", &SelectionModel::getFirstClick );
    NsProp( "Size", &SelectionModel::getSize );
}
