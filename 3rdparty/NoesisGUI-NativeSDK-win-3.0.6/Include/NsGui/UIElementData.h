////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_UIELEMENTDATA_H__
#define __GUI_UIELEMENTDATA_H__


#include <NsCore/Noesis.h>
#include <NsCore/HashMap.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/Ptr.h>
#include <NsGui/CoreApi.h>
#include <NsGui/RoutedEvent.h>
#include <NsGui/DependencyData.h>


namespace Noesis
{

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
struct EventHandlerInfo
{
    EventHandler handler;
    bool invokeHandledEvents;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to register routed events for a UIElement type.
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_CORE_API UIElementData: public DependencyData
{
public:
    UIElementData(const TypeClass* ownerType);

    /// Registers a routed event for a type
    /// \param event Element that receives the new routed event handler
    /// \param name The name of routed event. The name must be unique for the type
    /// \param strategy Type of routing strategy followed by this event
    void RegisterEvent(const RoutedEvent*& event, const char* name, RoutingStrategy strategy);

    /// Register an event owner by another class
    /// \param event Element that receives the routed event handler
    /// \param name The name of routed event. The name must be unique for the type
    /// \param source Source routed element
    void AddEventOwner(const RoutedEvent*& event, const char* name, const RoutedEvent*& source);

    /// Finds a routed event for the owner type given its name
    const RoutedEvent* FindEvent(Symbol name) const;

    /// Registers an event with a delegate to be executed in first place when the event is raised
    void RegisterEventHandler(const RoutedEvent* event, const EventHandler& handler,
        bool invokeHandledEvents = false);

    /// Find the static method associated with an event
    const EventHandlerInfo* FindEventHandler(const RoutedEvent* event) const;

private:
    const RoutedEvent* InsertEvent(const RoutedEvent* ev);

private:
    friend class UIElementDataTest;

    typedef HashMap<Symbol, Ptr<const RoutedEvent>> EventMap;
    EventMap mEvents;

    typedef HashMap<const RoutedEvent*, EventHandlerInfo> EventHandlerMap;
    EventHandlerMap mEventHandlers;

    NS_DECLARE_REFLECTION(UIElementData, DependencyData)
};

NS_WARNING_POP

/// Search for the static handler associated with an event
NS_GUI_CORE_API const EventHandlerInfo* FindRoutedEventHandler(const TypeClass* type,
    const RoutedEvent* routed);

}


#endif
