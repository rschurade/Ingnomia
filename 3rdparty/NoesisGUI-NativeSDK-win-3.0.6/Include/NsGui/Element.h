
////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_ELEMENT_H__
#define __GUI_ELEMENT_H__


#include <NsCore/Noesis.h>
#include <NsGui/CoreApi.h>
#include <NsCore/ReflectionDeclare.h>


namespace Noesis
{

class DependencyProperty;
class RoutedEvent;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Extends UI elements with properties that are not standard in WPF.
////////////////////////////////////////////////////////////////////////////////////////////////////
struct NS_GUI_CORE_API Element
{
    /// Dependency properties
    //@{
    static const DependencyProperty* Transform3DProperty;
    static const DependencyProperty* IsFocusEngagedProperty;
    static const DependencyProperty* IsFocusEngagementEnabledProperty;
    static const DependencyProperty* PPAAModeProperty;
    static const DependencyProperty* PPAAInProperty;
    static const DependencyProperty* PPAAOutProperty;
    static const DependencyProperty* IsTapEnabledProperty;
    static const DependencyProperty* IsDoubleTapEnabledProperty;
    static const DependencyProperty* IsHoldingEnabledProperty;
    static const DependencyProperty* IsRightTapEnabledProperty;
    //@}


    /// Routed events
    //@{
    static const RoutedEvent* TappedEvent;
    static const RoutedEvent* DoubleTappedEvent;
    static const RoutedEvent* HoldingEvent;
    static const RoutedEvent* RightTappedEvent;
    //@}

    NS_DECLARE_REFLECTION(Element, NoParent)
};

}

#endif
