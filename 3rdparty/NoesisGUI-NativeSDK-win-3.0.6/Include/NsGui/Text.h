
////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_TEXT_H__
#define __GUI_TEXT_H__


#include <NsCore/Noesis.h>
#include <NsGui/CoreApi.h>
#include <NsCore/ReflectionDeclare.h>


namespace Noesis
{

class BaseComponent;
class PasswordBox;
class DependencyObject;
class DependencyProperty;
struct DependencyPropertyChangedEventArgs;
struct RoutedEventArgs;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Adds stroke capabilities to text controls.
////////////////////////////////////////////////////////////////////////////////////////////////////
struct NS_GUI_CORE_API Text
{
    /// Gets Placeholder value from the specified element
    static const char* GetPlaceholder(const DependencyObject* element);

    /// Sets Placeholder values in the specified element
    static void SetPlaceholder(DependencyObject* element, const char* value);

    /// Gets password length from the specified element
    static uint32_t GetPasswordLength(const PasswordBox* element);

    /// Dependency properties
    //@{
    static const DependencyProperty* CharacterSpacingProperty;
    static const DependencyProperty* StrokeProperty;
    static const DependencyProperty* StrokeThicknessProperty;
    static const DependencyProperty* PlaceholderProperty;
    static const DependencyProperty* PasswordLengthProperty;
    //@}

    NS_DECLARE_REFLECTION(Text, NoParent)
};

}


#endif
