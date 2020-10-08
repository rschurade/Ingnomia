////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_URI_H__
#define __GUI_URI_H__


#include <NsCore/Noesis.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/String.h>
#include <NsGui/CoreApi.h>


namespace Noesis
{

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// A URI is a compact representation of a resource available to your application.
////////////////////////////////////////////////////////////////////////////////////////////////////
struct NS_GUI_CORE_API Uri
{
    Uri(const char* uri = "");

    /// Gets stored path
    const char* Get() const;

    /// Comparison operators
    //@{
    bool operator==(const Uri& uri) const;
    bool operator!=(const Uri& uri) const;
    //@}

    /// Generates a string representation of the Uri
    String ToString() const;

private:
    FixedString<512> mUri;

    NS_DECLARE_REFLECTION(Uri, NoParent)
};

NS_WARNING_POP

}


#endif
