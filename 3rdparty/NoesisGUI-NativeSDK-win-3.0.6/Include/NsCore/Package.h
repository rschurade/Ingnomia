////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __CORE_PACKAGE_H__
#define __CORE_PACKAGE_H__


#include <NsCore/RegisterComponent.h>


NS_MSVC_WARNING_DISABLE(4100)


////////////////////////////////////////////////////////////////////////////////////////////////////
#define NS_REGISTER_REFLECTION(module, package) \
    extern "C" NS_COLD_FUNC void NsRegisterReflection##module##package()

////////////////////////////////////////////////////////////////////////////////////////////////////
#define NS_REGISTER_COMPONENT(componentClass) \
    { \
        const Noesis::TypeClass* type = Noesis::TypeOf<componentClass>(); \
        Noesis::RegisterComponent(type, [](Noesis::Symbol) -> Noesis::BaseComponent* \
        { \
            return new componentClass; \
        }); \
    }

////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef NS_TESTS_ENABLED
    #define NS_REGISTER_TEST(n) \
        extern void Noesis_Register##n##Test(); \
        Noesis_Register##n##Test();
#else
    #define NS_REGISTER_TEST(n)
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
#define NS_INIT_PACKAGE(module, package) \
    extern "C" NS_COLD_FUNC void NsInitPackage##module##package()

////////////////////////////////////////////////////////////////////////////////////////////////////
#define NS_SHUTDOWN_PACKAGE(module, package) \
    extern "C" NS_COLD_FUNC void NsShutdownPackage##module##package()

#endif
