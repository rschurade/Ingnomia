////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_XAMLPROVIDER_H__
#define __GUI_XAMLPROVIDER_H__


#include <NsCore/Noesis.h>
#include <NsCore/BaseComponent.h>
#include <NsCore/Delegate.h>


namespace Noesis
{

class Stream;
template<class T> class Ptr;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Base class for implementing providers of XAMLs
////////////////////////////////////////////////////////////////////////////////////////////////////
class XamlProvider: public BaseComponent
{
public:
    /// Loads the specified XAML file. Returns null if no xaml found
    virtual Ptr<Stream> LoadXaml(const char* uri) = 0;

    /// Delegate to notify changes to the XAML file text
    typedef Delegate<void (const char*)> XamlChangedDelegate;
    XamlChangedDelegate& XamlChanged() { return mXamlChanged; }
    void RaiseXamlChanged(const char* uri) { mXamlChanged(uri); }

private:
    XamlChangedDelegate mXamlChanged;
};

}

#endif
