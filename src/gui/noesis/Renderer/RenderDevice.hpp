#pragma once

#include "../../SDL_mainwindow.h"
#include "RenderTarget.hpp"
#include "ShaderUniform.h"

#include <NoesisPCH.h>

#include <array>

#include <bgfx/bgfx.h>

namespace AppGUI
{
struct RenderDevice final : Noesis::RenderDevice
{
	RenderDevice( const SDL_MainWindow* window, bgfx::ViewId viewID );
	RenderDevice( const RenderDevice& )                    = delete;
	RenderDevice( RenderDevice&& )                         = delete;
	auto operator=( const RenderDevice& ) -> RenderDevice& = delete;
	auto operator=( RenderDevice&& ) -> RenderDevice&      = delete;
	~RenderDevice() override;

	auto Resize( std::uint16_t width, std::uint16_t height ) noexcept -> void;

	virtual auto GetCaps() const -> const Noesis::DeviceCaps& override;

	virtual auto CreateTexture( const char* label, std::uint32_t width, std::uint32_t height, std::uint32_t numLevels, Noesis::TextureFormat::Enum format, const void** data ) -> Noesis::Ptr<Noesis::Texture> override;
	virtual auto UpdateTexture( Noesis::Texture* texture, std::uint32_t level, std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height, const void* data ) -> void override;

	virtual auto SetRenderTarget( Noesis::RenderTarget* surface ) -> void override;
	virtual auto ResolveRenderTarget( Noesis::RenderTarget* surface, const Noesis::Tile* tiles, std::uint32_t numTiles ) -> void override;
	virtual auto CreateRenderTarget( const char* label, std::uint32_t width, std::uint32_t height, std::uint32_t sampleCount, bool needsStencil ) -> Noesis::Ptr<Noesis::RenderTarget> override;
	virtual auto CloneRenderTarget( const char* label, Noesis::RenderTarget* surface ) -> Noesis::Ptr<Noesis::RenderTarget> override;

	virtual auto MapVertices( std::uint32_t bytes ) -> void* override;
	virtual auto UnmapVertices() -> void override;
	virtual auto MapIndices( std::uint32_t bytes ) -> void* override;
	virtual auto UnmapIndices() -> void override;
	virtual auto DrawBatch( const Noesis::Batch& batch ) -> void override;

	virtual auto BeginOffscreenRender() -> void override;
	virtual auto EndOffscreenRender() -> void override;

	virtual auto BeginOnscreenRender() -> void override;
	virtual auto EndOnscreenRender() -> void override;

private:
	auto CreateLayout( std::uint32_t type, bgfx::VertexLayout& out ) const noexcept -> void;
	[[nodiscard]] auto GetSWrapMode( Noesis::SamplerState sampler ) const noexcept -> std::uint32_t;
	[[nodiscard]] auto GetTWrapMode( Noesis::SamplerState sampler ) const noexcept -> std::uint32_t;
	[[nodiscard]] auto GetMinMode( Noesis::SamplerState sampler ) const noexcept -> std::uint32_t;
	[[nodiscard]] auto GetMagMode( Noesis::SamplerState sampler ) const noexcept -> std::uint32_t;
	[[nodiscard]] auto GetSamplerFlags( Noesis::SamplerState sampler ) const noexcept -> std::uint32_t;

	auto CreatePrograms() -> void;
	auto DestroyPrograms() -> void;

	auto SetTextures( const Noesis::Batch& batch ) noexcept -> void;
	auto SetUniforms( const Noesis::Batch& batch ) noexcept -> void;

	static constexpr std::size_t Vec4Size_ { 4 };
	static constexpr std::size_t VertexShaderCount_ { Noesis::Shader::Vertex::Count };
	static constexpr std::size_t FragmentShaderCount_ { Noesis::Shader::Count - 1 };

	// general
	const bgfx::ViewId ViewID_;
	std::uint16_t Width_ {}, Height_ {};
	Noesis::DeviceCaps Caps_ {};

	// transient mesh buffers
	std::array<std::uint8_t, DYNAMIC_VB_SIZE> VBData_ {};
	std::array<std::uint8_t, DYNAMIC_IB_SIZE> IBData_ {};

	// texture sampler uniforms
	ShaderUniform S_Pattern_ { "s_pattern", UniformType::Sampler };
	ShaderUniform S_Ramps_ { "s_ramps", UniformType::Sampler };
	ShaderUniform S_Image_ { "s_image", UniformType::Sampler };
	ShaderUniform S_Glyphs_ { "s_glyphs", UniformType::Sampler };
	ShaderUniform S_Shadow_ { "s_shadow", UniformType::Sampler };

	// vertex shader uniforms
	ShaderUniform U_CBuffer1_VS_ { "u_cbuffer1_vs", UniformType::Vec4, 16 / Vec4Size_ };

	// fragment shader uniforms
	ShaderUniform U_CBuffer0_FS_ { "u_cbuffer0_fs", UniformType::Vec4, 8 / Vec4Size_ };
	ShaderUniform U_CBuffer1_FS_ { "u_cbuffer1_fs", UniformType::Vec4, 128 / Vec4Size_ };

	// shader programs
	std::array<bgfx::VertexLayout, Noesis::Shader::Vertex::Format::Count> Layouts_ {};
	std::array<bgfx::ShaderHandle, VertexShaderCount_> VertexShaders_ {};
	std::array<bgfx::ShaderHandle, FragmentShaderCount_> FragmentShaders_ {}; // -1 because we do not support custom effect
	std::array<bgfx::ProgramHandle, FragmentShaderCount_> Programs_ {};
};
} // namespace AppGUI
