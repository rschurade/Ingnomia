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

#include "converters.h"

using namespace IngnomiaGUI;
using namespace Noesis;

bool ColorToBrushConverter::TryConvert( BaseComponent* value, const Type* targetType, BaseComponent*, Noesis::Ptr<BaseComponent>& result )
{
    if ( targetType == Noesis::TypeOf<Brush>() )
    {
        QString param = value->ToString().Str();
        auto cl = param.split( " " );
        if( cl.size() > 2 )
		{
		    auto col = Color( cl[0].toInt(), cl[1].toInt(), cl[2].toInt() );
            result = MakePtr<SolidColorBrush>(col);
        }
		else
		{
            auto col = Color( 255, 0, 255 );
            result = MakePtr<SolidColorBrush>(col);
		}
        return true;
    }

    return false;
}

bool ColorToBrushConverterDark::TryConvert( BaseComponent* value, const Type* targetType, BaseComponent*, Noesis::Ptr<BaseComponent>& result )
{
    if ( targetType == Noesis::TypeOf<Brush>() )
    {
        QString param = value->ToString().Str();
        auto cl = param.split( " " );
        if( cl.size() > 2 )
		{
		    auto col = Color( cl[0].toInt() / 2, cl[1].toInt() / 2, cl[2].toInt() / 2 );
            result = MakePtr<SolidColorBrush>(col);
        }
		else
		{
            auto col = Color( 155, 0, 155 );
            result = MakePtr<SolidColorBrush>(col);
		}
        return true;
    }

    return false;
}



NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION( ColorToBrushConverter, "IngnomiaGUI.ColorToBrushConverter")
{
}

NS_IMPLEMENT_REFLECTION( ColorToBrushConverterDark, "IngnomiaGUI.ColorToBrushConverterDark")
{
}
