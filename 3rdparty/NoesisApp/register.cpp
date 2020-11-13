////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsCore/CompilerSettings.h>


#define PACKAGE_REGISTER(MODULE, PACKAGE) \
    void NsRegisterReflection##MODULE##PACKAGE(); \
    NsRegisterReflection##MODULE##PACKAGE()

#define PACKAGE_INIT(MODULE, PACKAGE) \
    void NsInitPackage##MODULE##PACKAGE(); \
    NsInitPackage##MODULE##PACKAGE()

#define PACKAGE_SHUTDOWN(MODULE, PACKAGE) \
    void NsShutdownPackage##MODULE##PACKAGE(); \
    NsShutdownPackage##MODULE##PACKAGE()


////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" void NsRegisterReflection_NoesisApp()
{
    PACKAGE_REGISTER(Render, RenderContext);
    PACKAGE_REGISTER(Render, GLRenderDevice);
    PACKAGE_REGISTER(App, Display);
    PACKAGE_REGISTER(Render, GLRenderContext);
    PACKAGE_REGISTER(App, Providers);
    PACKAGE_REGISTER(App, Launcher);
    PACKAGE_REGISTER(App, Theme);
    PACKAGE_REGISTER(App, DisplayLauncher);
    PACKAGE_REGISTER(App, ApplicationLauncher);
	PACKAGE_REGISTER(App, Interactivity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" void NsInitPackages_NoesisApp()
{
    PACKAGE_INIT(Render, RenderContext);
    PACKAGE_INIT(Render, GLRenderDevice);
    PACKAGE_INIT(App, Display);
    PACKAGE_INIT(Render, GLRenderContext);
    PACKAGE_INIT(App, Providers);
    PACKAGE_INIT(App, Launcher);
    PACKAGE_INIT(App, Theme);
    PACKAGE_INIT(App, DisplayLauncher);
    PACKAGE_INIT(App, ApplicationLauncher);
	PACKAGE_INIT(App, Interactivity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" void NsShutdownPackages_NoesisApp()
{
	PACKAGE_SHUTDOWN(App, Interactivity);
    PACKAGE_SHUTDOWN(App, ApplicationLauncher);
    PACKAGE_SHUTDOWN(App, DisplayLauncher);
    PACKAGE_SHUTDOWN(App, Theme);
    PACKAGE_SHUTDOWN(App, Launcher);
    PACKAGE_SHUTDOWN(App, Providers);
    PACKAGE_SHUTDOWN(Render, GLRenderContext);
    PACKAGE_SHUTDOWN(App, Display);
    PACKAGE_SHUTDOWN(Render, GLRenderDevice);
    PACKAGE_SHUTDOWN(Render, RenderContext);
}
