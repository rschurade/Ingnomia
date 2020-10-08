
////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_PATHEXTENSIONS_H__
#define __GUI_PATHEXTENSIONS_H__


#include <NsCore/Noesis.h>
#include <NsGui/CoreApi.h>
#include <NsCore/ReflectionDeclare.h>


namespace Noesis
{

class DependencyProperty;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Adds path trimming to Shape elements.
////////////////////////////////////////////////////////////////////////////////////////////////////
struct NS_GUI_CORE_API PathExtensions
{
    /// Dependency properties
    //@{
    static const DependencyProperty* TrimStartProperty;
    static const DependencyProperty* TrimEndProperty;
    static const DependencyProperty* TrimOffsetProperty;
    //@}

    NS_DECLARE_REFLECTION(PathExtensions, NoParent)
};

}


#endif
