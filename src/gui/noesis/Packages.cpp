#include "Packages.hpp"

#define PACKAGE_REGISTER(MODULE, PACKAGE) \
    void NsRegisterReflection##MODULE##PACKAGE(); \
    NsRegisterReflection##MODULE##PACKAGE()

#define PACKAGE_INIT(MODULE, PACKAGE) \
    void NsInitPackage##MODULE##PACKAGE(); \
    NsInitPackage##MODULE##PACKAGE()

#define PACKAGE_SHUTDOWN(MODULE, PACKAGE) \
    void NsShutdownPackage##MODULE##PACKAGE(); \
    NsShutdownPackage##MODULE##PACKAGE()

extern "C" auto InitPackages() -> void
{
    PACKAGE_REGISTER(App, Interactivity);
    PACKAGE_INIT(App, Interactivity);
}

extern "C" auto ShutdownPackages() -> void
{
    PACKAGE_SHUTDOWN(App, Interactivity);
}

namespace AppGUI::Packages
{
    auto Initialize() -> void
    {
        InitPackages();
    }

    auto Shutdown() -> void
    {
        ShutdownPackages();
    }
}

