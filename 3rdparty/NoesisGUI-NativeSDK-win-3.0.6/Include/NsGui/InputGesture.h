////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_INPUTGESTURE_H__
#define __GUI_INPUTGESTURE_H__


#include <NsCore/Noesis.h>
#include <NsCore/BaseComponent.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsGui/CoreApi.h>


namespace Noesis
{

struct RoutedEventArgs;

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Abstract class that describes input device gestures.
///
/// http://msdn.microsoft.com/en-us/library/system.windows.input.inputgesture.aspx
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_CORE_API InputGesture: public BaseComponent
{
public:
    NS_DISABLE_COPY(InputGesture)

    InputGesture();
    virtual ~InputGesture() = 0;

    /// When overridden in a derived class, determines whether the specified InputGesture matches
    /// the input specified
    virtual bool Matches(BaseComponent* target, const RoutedEventArgs& args) = 0;

private:
    NS_DECLARE_REFLECTION(InputGesture, BaseComponent)
};

NS_WARNING_POP

}


#endif
