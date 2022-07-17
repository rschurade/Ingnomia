//
// Created by Arcnor on 15/07/2022.
// Copyright (c) 2022 Reversed Games. All rights reserved.
//

#ifndef INGNOMIA_BGFXUTILS_H
#define INGNOMIA_BGFXUTILS_H

#include <SDL.h>
#include <bgfx/bgfx.h>

void* sdlNativeWindowHandle(SDL_Window* _window);

bgfx::PlatformData bgfxInitializePlatformData(SDL_Window* _window);

void bgfxUninitializePlatformData(SDL_Window* _window);

#endif // INGNOMIA_BGFXUTILS_H
