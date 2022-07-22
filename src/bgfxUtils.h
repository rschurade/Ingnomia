//
// Created by Arcnor on 15/07/2022.
// Copyright (c) 2022 Reversed Games. All rights reserved.
//

#ifndef INGNOMIA_BGFXUTILS_H
#define INGNOMIA_BGFXUTILS_H

#include <SDL.h>
#include <bgfx/bgfx.h>

#include <fmt/format.h>

#include <cassert>
#include <span>
#include <filesystem>

namespace fs = std::filesystem;


void* sdlNativeWindowHandle(SDL_Window* _window);

bgfx::PlatformData bgfxInitializePlatformData(SDL_Window* _window);

void bgfxUninitializePlatformData(SDL_Window* _window);

bgfx::ShaderHandle LoadCompiledShader( const fs::path& shaderPath );

template <const std::size_t Count>
static auto LoadShaders( const std::span<const std::string_view, Count> names, const std::span<bgfx::ShaderHandle, Count> out, const std::string& suffix, const fs::path& shaderDir ) -> void
{
	std::string rendererType;
	switch (bgfx::getRendererType() )
	{
		case bgfx::RendererType::Noop:
		case bgfx::RendererType::Direct3D9: rendererType = "dx9";   break;
		case bgfx::RendererType::Direct3D11:
		case bgfx::RendererType::Direct3D12: rendererType = "dx11";  break;
		case bgfx::RendererType::Agc:
		case bgfx::RendererType::Gnm: rendererType = "pssl";  break;
		case bgfx::RendererType::Metal: rendererType = "metal"; break;
		case bgfx::RendererType::Nvn: rendererType = "nvn";   break;
		case bgfx::RendererType::OpenGL: rendererType = "glsl";  break;
		case bgfx::RendererType::OpenGLES: rendererType = "essl";  break;
		case bgfx::RendererType::Vulkan:
		case bgfx::RendererType::WebGPU: rendererType = "spirv"; break;

		case bgfx::RendererType::Count:
			assert(false && "You should not be here!");
			break;
	}

	assert( names.size() == out.size() && "Size mismatch!" );
	for ( std::size_t i {}; i < names.size(); ++i )
	{
		const fs::path fileName { shaderDir / fmt::format("{}_{}_{}.bin", names[i], suffix, rendererType ) };
		const bgfx::ShaderHandle handle { LoadCompiledShader( fileName ) };
		out[i] = handle;
	}
}

#endif // INGNOMIA_BGFXUTILS_H
