////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __CORE_TYPEMETA_H__
#define __CORE_TYPEMETA_H__


#include <NsCore/Noesis.h>
#include <NsCore/KernelApi.h>
#include <NsCore/Type.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/MetaData.h>


namespace Noesis
{

class Symbol;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Type with associated metadata
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_CORE_KERNEL_API TypeMeta: public Type
{
public:
    /// Constructor
    TypeMeta(Symbol name);

    /// Destructor
    virtual ~TypeMeta() = 0;
    
    /// Returns the container of metadatas
    inline MetaData& GetMetaData();
    inline const MetaData& GetMetaData() const;

private:
    MetaData mMetaData;

    NS_DECLARE_REFLECTION(TypeMeta, Type)
};

}

#include <NsCore/TypeMeta.inl>

#endif
