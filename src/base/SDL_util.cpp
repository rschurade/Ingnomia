//
// Created by Arcnor on 16/07/2022.
// Copyright (c) 2022 Reversed Games. All rights reserved.
//

#include "SDL_util.h"

#include "range/v3/action/split.hpp"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <string>

double comp2float(uint8_t comp) {
	return comp / 255.0;
}

uint8_t float2comp(double comp) {
	return comp * 255.0;
}

SDL_Color getPixelColor( const SDL_Surface* surface, uint32_t x, uint32_t y )
{
	// FIXME: Expecting 32 bit RGBA surface
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	uint32_t col = *(Uint32*)( (Uint8*)surface->pixels + y * surface->pitch + x * 4 );
	return reinterpret_cast<SDL_Color const&>( col );
#else
	auto* col = ( (Uint8*)surface->pixels + y * surface->pitch + x * 4 );
	return SDL_Color
	{
		.r = col[3],
		.g = col[2],
		.b = col[1],
		.a = col[0],
	};
#endif
}

void setPixelColor( SDL_Surface* surface, uint32_t x, uint32_t y, SDL_Color col )
{
	// FIXME: Expecting 32 bit RGBA surface
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	auto *p = (Uint32*)( (Uint8*)surface->pixels + y * surface->pitch + x * 4 );
	p[0] = reinterpret_cast<Uint32 const&>( col );;
#else
	auto *p = ( (Uint8*)surface->pixels + y * surface->pitch + x * 4 );
	p[3] = col.r;
	p[2] = col.g;
	p[1] = col.b;
	p[0] = col.a;
#endif
}

SDL_Surface* createPixmap( int w, int h )
{
	return SDL_CreateRGBSurfaceWithFormat( 0, w, h, 32, SDL_PIXELFORMAT_RGBA8888 );
}

void copyPixmap( SDL_Surface* dst, SDL_Surface* src, int dstX, int dstY )
{
	SDL_Rect dstRect { dstX, dstY, src->w, src->h };
	assert(dstRect.x + dstRect.w <= dst->w && "Destination region W is bigger than destination width");
	assert(dstRect.y + dstRect.h <= dst->h && "Destination region H is bigger than destination height");

	const auto result = SDL_BlitSurface(src, nullptr, dst, &dstRect);
	if (result) {
		spdlog::critical("Cannot copy surface: {}", SDL_GetError());
	}
}

void copyPixmapFrom( SDL_Surface* dst, SDL_Surface* src, int srcX, int srcY, int srcW, int srcH )
{
	SDL_Rect srcRect { srcX, srcY, srcW, srcH };
	assert(srcW <= dst->w && "Pixmap region is bigger than destination width");
	assert(srcH <= dst->h && "Pixmap region is bigger than destination height");
	const auto result = SDL_BlitSurface(src, &srcRect, dst, nullptr);
	if (result) {
		spdlog::critical("Cannot copy surface: {}", SDL_GetError());
	}
}

void tintPixmap( SDL_Surface* surface, SDL_Color color ){ 
	
	SDL_LockSurface(surface);
	
	for ( int x = 0; x < surface->w; ++x )
	{
		for ( int y = 0; y < surface->h; ++y )
		{
			auto col = getPixelColor( surface, x, y );
			col.r = float2comp( std::min( 1.0, comp2float(col.r) * comp2float(color.r) ) );
			col.g = float2comp( std::min( 1.0, comp2float(col.g) * comp2float(color.g) ) );
			col.b = float2comp( std::min( 1.0, comp2float(col.b) * comp2float(color.b) ) );
			col.a = float2comp( std::min( 1.0, comp2float(col.a) * comp2float(color.a) ) );
			setPixelColor( surface, x, y, col );
		}
	}

	SDL_UnlockSurface(surface);
}

SDL_Surface* clonePixmap( const SDL_Surface* src ) {
	auto* result = createPixmap( src->w, src->h );
	SDL_BlitSurface( (SDL_Surface*)src, nullptr, result, nullptr );
	return result;
}

void flipPixmap( SDL_Surface* surface, bool flipH, bool flipV ) {
	if (flipH) {
		// FIXME: This only works for 32 bit images
		assert(surface->format->BitsPerPixel == 32);
		// FIXME: This only works for images where pitch == width * bpp
		assert(surface->pitch == surface->w * surface->format->BytesPerPixel);

		SDL_LockSurface(surface);
		const auto w2 = floor(surface->w / 2.0);
		auto* pixels = reinterpret_cast<uint32_t*>(surface->pixels);
		for ( int y = 0; y < surface->h; ++y )
		{
			const auto yOff = y * surface->w;
			for ( int x = 0; x < w2; ++x )
			{
				std::swap(pixels[yOff + x], pixels[yOff + surface->w - x - 1]);
			}
		}
		SDL_UnlockSurface(surface);
	}
	if (flipV) {
		throw std::runtime_error("TODO: Flip pixmap V");
	}
}

void rotatePixmap90( const SDL_Surface* surface ) {
	throw std::runtime_error("TODO: Rotate pixmap 90");
}

SDL_Surface* flipPixmapClone( const SDL_Surface* surface, bool flipH, bool flipV ) {
	throw std::runtime_error("TODO: Flip pixmap");
}

SDL_Surface* rotatePixmap90Clone( const SDL_Surface* surface ) {
	throw std::runtime_error("TODO: Rotate pixmap 90");
}

SDL_Color string2SDLColor( const std::string& color ) {
	const auto csl = ranges::actions::split( color, ' ' );
	if ( csl.size() == 4 )
	{
		return SDL_Color {
			.r = static_cast<Uint8>( std::stoi(csl[0]) ),
			.g = static_cast<Uint8>( std::stoi(csl[1]) ),
			.b = static_cast<Uint8>( std::stoi(csl[2]) ),
			.a = static_cast<Uint8>( std::stoi(csl[3]) )
		};
	}
	return SDL_Color { 0xFF, 0xFF, 0xFF, 0xFF };
}