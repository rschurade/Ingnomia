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

/** @file converters.h
 *  @brief Custom Noesis IValueConverter implementations used by XAML bindings to translate
 *         a colour-key string from the view model into a SolidColorBrush for the GUI.
 */
#pragma once

#include <NsCore/Noesis.h>
#include <NsCore/ReflectionDeclare.h>

namespace IngnomiaGUI
{

/// @brief XAML value converter that maps a colour-key string (e.g. "Combat", "Nature") to
///        a bright SolidColorBrush, used for normal-weight text/foreground accents.
class ColorToBrushConverter : public Noesis::BaseValueConverter
{
public:
    bool TryConvert( BaseComponent* value, const Noesis::Type* targetType, BaseComponent*, Noesis::Ptr<BaseComponent>& result );

private:
    NS_DECLARE_REFLECTION( ColorToBrushConverter, Noesis::BaseValueConverter )
};

/// @brief Dark variant of ColorToBrushConverter, returning a darker SolidColorBrush for the
///        same colour-key strings (used for backgrounds and outlines).
class ColorToBrushConverterDark : public Noesis::BaseValueConverter
{
public:
    bool TryConvert( BaseComponent* value, const Noesis::Type* targetType, BaseComponent*, Noesis::Ptr<BaseComponent>& result );

private:
    NS_DECLARE_REFLECTION( ColorToBrushConverterDark, Noesis::BaseValueConverter )
};


}