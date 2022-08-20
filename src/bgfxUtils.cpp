//
// Created by Arcnor on 15/07/2022.
// Copyright (c) 2022 Reversed Games. All rights reserved.
//

#include "bgfxUtils.h"

#include <bx/bx.h>
#include <SDL_syswm.h>

#include <fstream>

#define ENTRY_CONFIG_USE_WAYLAND 1

void* sdlNativeWindowHandle(SDL_Window* _window)
{
	SDL_SysWMinfo wmi;
	SDL_VERSION(&wmi.version);
	if (!SDL_GetWindowWMInfo(_window, &wmi) )
	{
		return nullptr;
	}

#	if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
#		if ENTRY_CONFIG_USE_WAYLAND
	wl_egl_window *win_impl = (wl_egl_window*)SDL_GetWindowData(_window, "wl_egl_window");
	if(!win_impl)
	{
		int width, height;
		SDL_GetWindowSize(_window, &width, &height);
		struct wl_surface* surface = wmi.info.wl.surface;
		if(!surface)
			return nullptr;
		win_impl = wl_egl_window_create(surface, width, height);
		SDL_SetWindowData(_window, "wl_egl_window", win_impl);
	}
	return (void*)(uintptr_t)win_impl;
#		else
	return (void*)wmi.info.x11.window;
#		endif
#	elif BX_PLATFORM_OSX || BX_PLATFORM_IOS
	return wmi.info.cocoa.window;
#	elif BX_PLATFORM_WINDOWS
	return wmi.info.win.window;
#   elif BX_PLATFORM_ANDROID
	return wmi.info.android.window;
#	endif // BX_PLATFORM_
}

bgfx::PlatformData bgfxInitializePlatformData(SDL_Window* _window)
{
	SDL_SysWMinfo wmi;
	SDL_VERSION(&wmi.version);
	if (!SDL_GetWindowWMInfo(_window, &wmi) )
	{
		return {};
	}

	bgfx::PlatformData pd;
#	if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
#		if ENTRY_CONFIG_USE_WAYLAND
	pd.ndt          = wmi.info.wl.display;
#		else
	pd.ndt          = wmi.info.x11.display;
#		endif
#	else
	pd.ndt          = nullptr;
#	endif // BX_PLATFORM_
	pd.nwh          = sdlNativeWindowHandle(_window);

	pd.context      = nullptr;
	pd.backBuffer   = nullptr;
	pd.backBufferDS = nullptr;
	return pd;
}

void bgfxUninitializePlatformData(SDL_Window* _window)
{
	if(!_window)
		return;
#	if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
#		if ENTRY_CONFIG_USE_WAYLAND
	wl_egl_window *win_impl = (wl_egl_window*)SDL_GetWindowData(_window, "wl_egl_window");
	if(win_impl)
	{
		SDL_SetWindowData(_window, "wl_egl_window", nullptr);
		wl_egl_window_destroy(win_impl);
	}
#		endif
#	endif
}

const bgfx::Memory* loadFileContent(const fs::path& _file)
{
	std::ifstream fileStream(_file, std::ios::ate | std::ios::binary);
	assert( fileStream.good() );
	const auto fLength = (size_t)fileStream.tellg();
	fileStream.seekg(0);
	const auto mem = bgfx::alloc(fLength + 1);
	mem->data[fLength] = 0;
	fileStream.read(reinterpret_cast<char *>(mem->data), fLength);
	assert( fileStream.good() );
	return mem;
}

bgfx::ShaderHandle LoadCompiledShader( const fs::path& shaderPath )
{
	auto mem = loadFileContent(shaderPath);
	return bgfx::createShader(mem);
}
