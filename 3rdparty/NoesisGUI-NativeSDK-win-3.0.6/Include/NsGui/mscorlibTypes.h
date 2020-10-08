////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_MSCORESYSTEMTYPES_H__
#define __GUI_MSCORESYSTEMTYPES_H__


#include <NsCore/Noesis.h>
#include <NsCore/Ptr.h>
#include <NsGui/CoreApi.h>
#include <NsGui/MarkupExtension.h>


namespace Noesis
{

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Class representing bool values in xaml
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_CORE_API SystemBoolean: public MarkupExtension
{
public:
    /// From MarkupExtension
    //@{
    Ptr<BaseComponent> ProvideValue(const ValueTargetProvider* provider) override;
    //@}

private:
    Ptr<BaseComponent> mValue;

    NS_DECLARE_REFLECTION(SystemBoolean, MarkupExtension)
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Class representing int values in xaml
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_CORE_API SystemInt16: public MarkupExtension
{
public:
    /// From MarkupExtension
    //@{
    Ptr<BaseComponent> ProvideValue(const ValueTargetProvider* provider) override;
    //@}

private:
    Ptr<BaseComponent> mValue;

    NS_DECLARE_REFLECTION(SystemInt16, MarkupExtension)
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Class representing int values in xaml
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_CORE_API SystemInt32: public MarkupExtension
{
public:
    /// From MarkupExtension
    //@{
    Ptr<BaseComponent> ProvideValue(const ValueTargetProvider* provider) override;
    //@}

private:
    Ptr<BaseComponent> mValue;

    NS_DECLARE_REFLECTION(SystemInt32, MarkupExtension)
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Class representing int values in xaml
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_CORE_API SystemUInt16: public MarkupExtension
{
public:
    /// From MarkupExtension
    //@{
    Ptr<BaseComponent> ProvideValue(const ValueTargetProvider* provider) override;
    //@}

private:
    Ptr<BaseComponent> mValue;

    NS_DECLARE_REFLECTION(SystemUInt16, MarkupExtension)
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Class representing int values in xaml
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_CORE_API SystemUInt32: public MarkupExtension
{
public:
    /// From MarkupExtension
    //@{
    Ptr<BaseComponent> ProvideValue(const ValueTargetProvider* provider) override;
    //@}

private:
    Ptr<BaseComponent> mValue;

    NS_DECLARE_REFLECTION(SystemUInt32, MarkupExtension)
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Class representing float values in xaml
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_CORE_API SystemSingle: public MarkupExtension
{
public:
    /// From MarkupExtension
    //@{
    Ptr<BaseComponent> ProvideValue(const ValueTargetProvider* provider) override;
    //@}

private:
    Ptr<BaseComponent> mValue;

    NS_DECLARE_REFLECTION(SystemSingle, MarkupExtension)
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Class representing double values in xaml
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_CORE_API SystemDouble: public MarkupExtension
{
public:
    /// From MarkupExtension
    //@{
    Ptr<BaseComponent> ProvideValue(const ValueTargetProvider* provider) override;
    //@}

private:
    Ptr<BaseComponent> mValue;

    NS_DECLARE_REFLECTION(SystemDouble, MarkupExtension)
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Class representing a string object in xaml
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_CORE_API SystemString: public MarkupExtension
{
public:
    /// From MarkupExtension
    //@{
    Ptr<BaseComponent> ProvideValue(const ValueTargetProvider* provider) override;
    //@}

private:
    Ptr<BaseComponent> mValue;

    NS_DECLARE_REFLECTION(SystemString, MarkupExtension)
};

NS_WARNING_POP

}


#endif
