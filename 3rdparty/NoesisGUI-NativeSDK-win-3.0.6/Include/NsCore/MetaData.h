////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __CORE_METADATA_H__
#define __CORE_METADATA_H__


#include <NsCore/Noesis.h>
#include <NsCore/KernelApi.h>
#include <NsCore/Vector.h>


namespace Noesis
{

class TypeClass;
class TypeMetaData;

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Manages the metadata of reflection types
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_CORE_KERNEL_API MetaData
{
public:
    MetaData() = default;
    MetaData(const MetaData&) = delete;
    MetaData& operator=(const MetaData&) = delete;
    ~MetaData();

    /// Adds a new metadata
    void Add(TypeMetaData* metaData);

    /// Gets the number of metadatas
    uint32_t Count() const;

    /// Gets metadata by index
    TypeMetaData* Get(uint32_t index) const;

    /// Returns the first metadata found with the given type. May return null
    TypeMetaData* Find(const TypeClass* metaDataType) const;
    template<class T> T* Find() const;

private:
    typedef Vector<TypeMetaData*> MetaDatas;
    MetaDatas mMetaDatas;
};

NS_WARNING_POP

}

#include <NsCore/MetaData.inl>

#endif
