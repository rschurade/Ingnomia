//
// Created by Arcnor on 16/07/2022.
// Copyright (c) 2022 Reversed Games. All rights reserved.
//

#ifndef INGNOMIA_SDL_UTIL_H
#define INGNOMIA_SDL_UTIL_H

#include <memory>
#include <SDL_surface.h>

using PixmapPtr = std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)>;

SDL_Color getPixelColor( const SDL_Surface* surface, uint32_t x, uint32_t y );
void setPixelColor( SDL_Surface* surface, uint32_t x, uint32_t y, SDL_Color col );
SDL_Surface* createPixmap( int w, int h );
void copyPixmap( SDL_Surface* dst, SDL_Surface* src, int x, int y );
void copyPixmap( SDL_Surface* dst, SDL_Surface* src, int x, int y, int w, int h );
void tintPixmap( SDL_Surface* surface, SDL_Color color );
SDL_Surface* clonePixmap( const SDL_Surface* src );
void flipPixmap( const SDL_Surface* surface, bool flipH, bool flipV );
void rotatePixmap90( const SDL_Surface* surface );
SDL_Surface* flipPixmapClone( const SDL_Surface* surface, bool flipH, bool flipV );
SDL_Surface* rotatePixmap90Clone( const SDL_Surface* surface );

SDL_Color string2SDLColor( const std::string& color );

#endif // INGNOMIA_SDL_UTIL_H
