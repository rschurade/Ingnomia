////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_INTEGRATIONAPI_H__
#define __GUI_INTEGRATIONAPI_H__


#include <NsCore/Noesis.h>
#include <NsCore/Ptr.h>
#include <NsCore/Init.h>
#include <NsCore/Error.h>
#include <NsCore/Log.h>
#include <NsGui/CoreApi.h>


namespace Noesis
{

NS_INTERFACE IView;
class Stream;
class UIElement;
class FontProvider;
class XamlProvider;
class BaseComponent;
class TextureProvider;
class FrameworkElement;
class ResourceDictionary;
enum Cursor: int32_t;
enum FontWeight: int32_t;
enum FontStyle: int32_t;
enum FontStretch: int32_t;
enum XamlDependencyType: int32_t;

namespace GUI
{

/// Installs custom assert handler. See 'NsCore/Error.h'
/// Must be invoked before Noesis::GUI::Init()
using Noesis::SetAssertHandler;

/// Installs custom error handler. See 'NsCore/Error.h'
/// Must be invoked before Noesis::GUI::Init()
using Noesis::SetErrorHandler;

/// Installs custom logging handler. See 'NsCore/Log.h'
/// Must be invoked before Noesis::GUI::Init()
using Noesis::SetLogHandler;

/// Installs custom memory handlers. See 'NsCore/Memory.h'
/// Must be invoked before Noesis::GUI::Init()
using Noesis::SetMemoryCallbacks;

/// Disables all connections from the remote Inspector tool
/// Must be invoked before Noesis::GUI::Init()
NS_GUI_CORE_API void DisableInspector();

/// Returns whether the remote Inspector is currently connected
NS_GUI_CORE_API bool IsInspectorConnected();

/// Keeps alive the Inspector connection. Only needed if Inspector is connected before any
/// view is created. Views call this function internally when updated
NS_GUI_CORE_API void UpdateInspector();

/// Noesis Initialization. See 'NsCore/Init.h'
using Noesis::Init;

/// Noesis Shutdown. See 'NsCore/Init.h'
using Noesis::Shutdown;

/// Sets the active license. Read 'NoesisLicense.h' for more information about licensing parameters
NS_GUI_CORE_API void SetLicense(const char* name, const char* key); 

/// Sets the provider in charge of loading XAML resources
NS_GUI_CORE_API void SetXamlProvider(XamlProvider* provider);

/// Sets the provider in charge of loading texture resources
NS_GUI_CORE_API void SetTextureProvider(TextureProvider* provider);

/// Sets the provider in charge of loading font resources
NS_GUI_CORE_API void SetFontProvider(FontProvider* provider);

/// Sets the family names to be used by the global font fallback mechanism. These fonts are used
/// whenever the FontFamily of an element does not contain needed glyphs. Fallback sequence can
/// specify font locations, for example { "./Fonts/#Pericles Light", "Tahoma", "Verdana" } 
NS_GUI_CORE_API void SetFontFallbacks(const char** familyNames, uint32_t numFamilies);

/// Sets default font properties to be used when not specified in an element
NS_GUI_CORE_API void SetFontDefaultProperties(float size, FontWeight weight, FontStretch stretch,
    FontStyle style);

/// Sets a collection of application-scope resources, such as styles and brushes
NS_GUI_CORE_API void LoadApplicationResources(const char* xaml);
NS_GUI_CORE_API void SetApplicationResources(ResourceDictionary* resources);
NS_GUI_CORE_API ResourceDictionary* GetApplicationResources();

/// Callback invoked each time an element requests opening or closing the on-screen keyboard
typedef void (*SoftwareKeyboardCallback)(void* user, UIElement* focused, bool open);
NS_GUI_CORE_API void SetSoftwareKeyboardCallback(void* user, SoftwareKeyboardCallback callback);

/// Callback invoked each time a view needs to update the mouse cursor icon
typedef void (*UpdateCursorCallback)(void* user, IView* view, Cursor cursor);
NS_GUI_CORE_API void SetCursorCallback(void* user, UpdateCursorCallback callback);

/// Callback for opening URL in a browser
typedef void (*OpenUrlCallback)(void* user, const char* url);
NS_GUI_CORE_API void SetOpenUrlCallback(void* user, OpenUrlCallback callback);
NS_GUI_CORE_API void OpenUrl(const char* url);

/// Callback for playing audio
typedef void (*PlayAudioCallback)(void* user, const char* filename, float volume);
NS_GUI_CORE_API void SetPlayAudioCallback(void* user, PlayAudioCallback callback);
NS_GUI_CORE_API void PlayAudio(const char* filename, float volume);

/// Finds dependencies to other XAMLS and resources (fonts, textures, sounds...)
typedef void (*XamlDependencyCallback)(void* user, const char* uri, XamlDependencyType type);
NS_GUI_CORE_API void GetXamlDependencies(Stream* xaml, const char* folder, void* user,
    XamlDependencyCallback callback);

/// Loads a XAML file that is located at the specified uniform resource identifier
NS_GUI_CORE_API Ptr<BaseComponent> LoadXaml(const char* filename);
template<class T> Ptr<T> LoadXaml(const char* filename);

/// Parses a well-formed XAML fragment and creates a corresponding object tree
NS_GUI_CORE_API Ptr<BaseComponent> ParseXaml(const char* xamlText);
template<class T> Ptr<T> ParseXaml(const char* xamlText);

/// Loads a XAML resource, like an audio, at the given uniform resource identifier
NS_GUI_CORE_API Ptr<Stream> LoadXamlResource(const char* filename);

/// Loads a XAML file passing an object of the same type as the root element
NS_GUI_CORE_API void LoadComponent(BaseComponent* component, const char* filename);

/// Creates a view with the given root element
NS_GUI_CORE_API Ptr<IView> CreateView(FrameworkElement* content);

}

}

#include <NsGui/IntegrationAPI.inl>


#endif
