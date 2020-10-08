////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_DEPENDENCYDATA_H__
#define __GUI_DEPENDENCYDATA_H__


#include <NsCore/Noesis.h>
#include <NsGui/DependencySystemApi.h>
#include <NsGui/DependencyProperty.h>
#include <NsCore/TypeMetaData.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/Ptr.h>
#include <NsCore/HashMap.h>


namespace Noesis
{

class PropertyMetadata;
class DependencyDataTest;
class UIElementDataTest;

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Metadata used for registering dependency properties
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_DEPENDENCYSYSTEM_API DependencyData: public TypeMetaData
{
public:
    /// Constructor
    DependencyData(const TypeClass* ownerType);

    /// Registers a dependency property
    template<class T>
    void RegisterProperty(const DependencyProperty*& dp, const char* name,
        PropertyMetadata* metadata);

    /// Registers a dependency property with a given validation callback
    template<class T>
    void RegisterProperty(const DependencyProperty*& dp, const char* name,
        PropertyMetadata* metadata, ValidateValueCallback validate);

    /// Registers a read-only dependency property
    template<class T>
    void RegisterPropertyRO(const DependencyProperty*& dp, const char* name,
        PropertyMetadata* metadata);

    /// Registers a read-only dependency property with a given validation callback
    template<class T>
    void RegisterPropertyRO(const DependencyProperty*& dp, const char* name,
        PropertyMetadata* metadata, ValidateValueCallback validate);

    /// Adds another type as an owner of a dependency property that has already been registered,
    /// providing dependency property metadata for the dependency property as it will exist on
    /// the provided owner type
    template<class T>
    void AddOwner(const DependencyProperty*& dp, const char* name,
        const DependencyProperty*& source, PropertyMetadata* metadata = 0);

    /// Specifies alternate metadata for this dependency property when it is present on instances
    /// of a specified type, overriding the metadata that existed for the dependency property as
    /// it was inherited from base types
    template<class T>
    void OverrideMetadata(const DependencyProperty*& dp, const char* name,
        PropertyMetadata* metadata);

    /// Finds a property for the given owner type name
    const DependencyProperty* FindProperty(Symbol name) const;

    /// Removes specified type metadatas from all registered properties
    void ClearMetadata(const TypeClass* forType);

    const DependencyProperty* InsertProperty(const DependencyProperty* dp);

protected:
    const TypeClass* mOwnerType;

private:
    void CheckMetadata(const char* name, PropertyMetadata* metadata, const Type* type) const;

    template<class T>
    void RegisterProperty(const DependencyProperty*& dp, const char* name,
        PropertyMetadata* metadata, ValidateValueCallback validate, PropertyAccess access);
    void RegisterExistingProperty(const DependencyProperty* dp, const char* name,
        PropertyMetadata* metadata, ValidateValueCallback validate, PropertyAccess access);

    typedef Int2Type<0> IsNotBaseComponent;
    typedef Int2Type<1> IsBaseComponent;

    template<class T>
    Ptr<DependencyProperty> CreateProperty(const char* name, PropertyMetadata* metadata,
        ValidateValueCallback validate, PropertyAccess access, IsNotBaseComponent);
    template<class T>
    Ptr<DependencyProperty> CreateProperty(const char* name, PropertyMetadata* metadata,
        ValidateValueCallback validate, PropertyAccess access, IsBaseComponent);

    template<class T>
    Ptr<DependencyProperty> CreateProperty(const char* name, IsNotBaseComponent);
    template<class T>
    Ptr<DependencyProperty> CreateProperty(const char* name, IsBaseComponent);

private:
    friend class DependencyDataTest;
    friend class UIElementDataTest;

    typedef HashMap<Symbol, Ptr<const DependencyProperty>> PropertyMap;
    PropertyMap mProperties;

    friend class VisualTreeInspectorHelper;

    NS_DECLARE_REFLECTION(DependencyData, TypeMetaData)
};

NS_WARNING_POP

}

#include <NsGui/DependencyData.inl>

#endif
