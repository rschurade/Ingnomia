#include "RenderDevice.hpp"

#include "../../../base/global.h"
#include "../../../bgfxUtils.h"
#include "Shaders.hpp"
#include "spdlog/spdlog.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include <bgfx/bgfx.h>
#include <bx/math.h>

namespace AppGUI
{
RenderDevice::RenderDevice( const SDL_MainWindow* window, const bgfx::ViewId viewID ) :
	ViewID_ { viewID }, m_activeRenderTarget( nullptr )
{
	bgfx::setViewName( viewID, "GUI-Frontend" );
	bgfx::setViewMode( viewID, bgfx::ViewMode::Sequential );

	for ( std::uint32_t i {}; i < Noesis::Shader::Vertex::Format::Count; ++i )
	{
		const std::uint32_t attributes { Noesis::AttributesForFormat[i] };
		this->CreateLayout( attributes, this->Layouts_[i] );
	}

	this->CreatePrograms();

	this->Width_  = window->getFBWidth();
	this->Height_ = window->getFBHeight();
}

auto RenderDevice::CreatePrograms() -> void
{
	spdlog::info( "Vertex shaders generation date: {}", ::VertexVersion );
	spdlog::info( "Fragment shaders generation date: {}", ::FragmentVersion );

	static const fs::path shaderDir { Global::exePath / "content" / "shaders" / "noesis" };

	LoadShaders<VertexShaderCount_>( ::VertexShaders, this->VertexShaders_, "VS", shaderDir );
	LoadShaders<FragmentShaderCount_>( ::FragmentShaders, this->FragmentShaders_, "FS", shaderDir );

	for ( std::size_t i {}; i < FragmentShaderCount_; ++i )
	{
		const std::size_t vsIdx { Noesis::VertexForShader[i] };
		this->Programs_[i] = bgfx::createProgram( this->VertexShaders_[vsIdx], this->FragmentShaders_[i], false );
		assert( bgfx::isValid( this->Programs_[i] ) && "Failed to create program!" );
	}
}

auto RenderDevice::DestroyPrograms() -> void
{
	for ( bgfx::ShaderHandle& handle : this->VertexShaders_ )
	{
		bgfx::destroy( handle );
	}
	for ( bgfx::ShaderHandle& handle : this->FragmentShaders_ )
	{
		bgfx::destroy( handle );
	}
	for ( bgfx::ProgramHandle& handle : this->Programs_ )
	{
		bgfx::destroy( handle );
	}
}

RenderDevice::~RenderDevice()
{
	this->DestroyPrograms();
}

static constexpr std::array<const bgfx::Attrib::Enum, Noesis::Shader::Vertex::Format::Attr::Count> SemanticAttributeLUT {
	bgfx::Attrib::Position,  // Pos
	bgfx::Attrib::Color0,    // Color
	bgfx::Attrib::TexCoord0, // Tex0
	bgfx::Attrib::TexCoord1, // Tex1
	bgfx::Attrib::TexCoord5, // Coverage
	bgfx::Attrib::TexCoord2, // Rect
	bgfx::Attrib::TexCoord3, // Tile
	bgfx::Attrib::TexCoord4, // ImagePos
};

static constexpr std::array<const std::tuple<const std::uint8_t, const bgfx::AttribType::Enum, const bool>, Noesis::Shader::Vertex::Format::Attr::Type::Count> SemanticTypeLUT {
	std::make_tuple( 1, bgfx::AttribType::Float, false ), // Float
	std::make_tuple( 2, bgfx::AttribType::Float, false ), // Float2
	std::make_tuple( 4, bgfx::AttribType::Float, false ), // Float4
	std::make_tuple( 4, bgfx::AttribType::Uint8, true ),  // UByte4Norm
	std::make_tuple( 4, bgfx::AttribType::Int16, true ),  // UShort4Norm
};

auto RenderDevice::CreateLayout( const std::uint32_t attributes, bgfx::VertexLayout& out ) const noexcept -> void
{
	out.begin();
	for ( std::size_t i {}; i < Noesis::Shader::Vertex::Format::Attr::Count; ++i )
	{
		if ( attributes & ( 1 << i ) )
		{
			const std::uint8_t attrib { Noesis::TypeForAttr[i] };
			const auto& [count, type, isNormalized] { SemanticTypeLUT[attrib] };
			out.add( SemanticAttributeLUT[i], count, type, isNormalized, false );
		}
	}
	out.end();
}

auto RenderDevice::Resize( const std::uint16_t width, const std::uint16_t height ) noexcept -> void
{
	this->Width_  = width;
	this->Height_ = height;
}

auto RenderDevice::GetCaps() const -> const Noesis::DeviceCaps&
{
	return this->Caps_;
}

auto RenderDevice::SetRenderTarget( Noesis::RenderTarget* const surface ) -> void
{
	m_activeRenderTarget = (AppGUI::RenderTarget*)surface;
}

auto RenderDevice::ResolveRenderTarget(
	Noesis::RenderTarget* const surface,
	const Noesis::Tile* const tiles,
	const std::uint32_t numTiles ) -> void
{
}

auto RenderDevice::CreateRenderTarget(
	const char* const label,
	const std::uint32_t width,
	const std::uint32_t height,
	const std::uint32_t sampleCount,
	const bool needsStencil ) -> Noesis::Ptr<Noesis::RenderTarget>
{
	Noesis::Ptr<RenderTarget> surface { *new RenderTarget { width, height } };
	bgfx::TextureHandle stencil = BGFX_INVALID_HANDLE;
	if (needsStencil)
	{
		stencil = bgfx::createTexture2D( width, height, false, 0, bgfx::TextureFormat::D24S8 );
	}

	const std::array<bgfx::TextureHandle, 1> attachments { dynamic_cast<Texture*>( surface->GetTexture() )->Handle };
	const bgfx::FrameBufferHandle frameBuffer { bgfx::createFrameBuffer( attachments.size(), attachments.data(), false ) };
	surface->StencilHandle = stencil;
	surface->FBOHandle     = frameBuffer;
	return surface;
}

auto RenderDevice::CloneRenderTarget(
	const char* const label,
	Noesis::RenderTarget* const surface ) -> Noesis::Ptr<Noesis::RenderTarget>
{
	RenderTarget* const shared { dynamic_cast<RenderTarget*>( surface ) };
	Noesis::Ptr<RenderTarget> clonedSurface { *new RenderTarget { shared->Width, shared->Height } };
	const std::array<bgfx::TextureHandle, 2> attachments { dynamic_cast<Texture*>( surface->GetTexture() )->Handle, shared->StencilHandle };
	clonedSurface->FBOHandle = bgfx::createFrameBuffer( attachments.size(), attachments.data(), false );
	return clonedSurface;
}

auto RenderDevice::CreateTexture(
	const char* const label,
	const std::uint32_t width,
	const std::uint32_t height,
	const std::uint32_t numLevels,
	const Noesis::TextureFormat::Enum format,
	const void** const data ) -> Noesis::Ptr<Noesis::Texture>
{
//	assert( numLevels == 1 && "Invalid levels for GUI texture" );

	bgfx::TextureFormat::Enum textureFormat;
	std::size_t stride;
	switch ( format )
	{
		case Noesis::TextureFormat::RGBA8:
			textureFormat = bgfx::TextureFormat::RGBA8;
			stride        = 4;
			break;
		case Noesis::TextureFormat::R8:
			textureFormat = bgfx::TextureFormat::R8;
			stride        = 1;
			break;
		default:
		{
			spdlog::critical( "Invalid texture format for GUI texture: {}. Allowed are B8, RGBA8", static_cast<std::uint8_t>( format ) );
			abort();
		}
	}

	// TODO: Mipmaps, they're part of the **data pointer
//	auto dataSize = 0;
//	auto w        = width;
//	auto h        = height;
//	int level     = static_cast<int>( numLevels );
//	for ( int j = 0; j < level; ++j )
//	{
//		dataSize += w * h * stride;
//		w >>= 1;
//		h >>= 1;
//	}
	const auto dataSize = width * height * stride;

	return *new Texture {
		bgfx::createTexture2D(
			width, height, false, 1, textureFormat, BGFX_TEXTURE_RT,
			data ? bgfx::copy( data[0], dataSize ) : nullptr ),
		width, height, stride, textureFormat == bgfx::TextureFormat::RGBA8
	};
}

auto RenderDevice::UpdateTexture(
	Noesis::Texture* const texture,
	const std::uint32_t level,
	const std::uint32_t x,
	const std::uint32_t y,
	const std::uint32_t width,
	const std::uint32_t height,
	const void* const data ) -> void
{
	assert( level == 0 && "Invalid levels for GUI texture" );
	Texture* const tex { dynamic_cast<Texture*>( texture ) };
	bgfx::updateTexture2D(
		tex->Handle,
		0,
		level,
		x,
		y,
		static_cast<std::uint16_t>( width ),
		static_cast<std::uint16_t>( height ),
		bgfx::copy( data, width * height * tex->Stride ) );
}

auto RenderDevice::MapVertices( const std::uint32_t bytes ) -> void*
{
	return this->VBData_.data();
}

auto RenderDevice::UnmapVertices() -> void
{
}

auto RenderDevice::MapIndices( const std::uint32_t bytes ) -> void*
{
	return this->IBData_.data();
}

auto RenderDevice::UnmapIndices() -> void
{
}

auto RenderDevice::DrawBatch( const Noesis::Batch& batch ) -> void
{
	float identity[16];
	bx::mtxIdentity(identity);
	bgfx::setViewTransform( this->ViewID_, &identity, batch.vertexUniforms[0].values );

	bgfx::VertexLayout& layout { this->Layouts_[Noesis::FormatForVertex[Noesis::VertexForShader[batch.shader.v]]] };

	const bool vsValid {
		batch.numVertices <= this->VBData_.size() &&
		bgfx::getAvailTransientVertexBuffer( batch.numVertices, layout ) == batch.numVertices
	};
	assert( vsValid && "Vertex count too large for transient buffer" );

	const bool fsValid {
		batch.numIndices <= this->IBData_.size() &&
		bgfx::getAvailTransientIndexBuffer( batch.numIndices ) == batch.numIndices
	};
	assert( fsValid && "Index count too large for transient buffer" );

	std::uint8_t *begin, *end, *target;

	bgfx::TransientVertexBuffer vb {};
	bgfx::allocTransientVertexBuffer( &vb, batch.numVertices, layout );
	target = vb.data;
	begin  = this->VBData_.data() + batch.vertexOffset;
	end    = begin + batch.numVertices * layout.getStride();
	std::copy( begin, end, target );

	bgfx::TransientIndexBuffer ib {};
	bgfx::allocTransientIndexBuffer( &ib, batch.numIndices );
	target = ib.data;
	begin  = this->IBData_.data() + batch.startIndex * sizeof( std::uint16_t );
	end    = begin + batch.numIndices * sizeof( std::uint16_t );
	std::copy( begin, end, target );

	bgfx::setVertexBuffer( 0, &vb, 0, batch.numVertices );
	bgfx::setIndexBuffer( &ib, 0, batch.numIndices );

	std::uint32_t state {};
	std::uint32_t stencil {};
	if ( batch.renderState.f.blendMode )
	{
		state |= BGFX_STATE_BLEND_FUNC( BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_SRC_ALPHA ) | BGFX_STATE_BLEND_EQUATION_ADD;
	}
	if ( batch.renderState.f.colorEnable )
	{
		state |= BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A;
	}
	if ( batch.renderState.f.stencilMode )
	{
		switch ( static_cast<Noesis::StencilMode::Enum>( batch.renderState.f.stencilMode ) )
		{
			default:
			case Noesis::StencilMode::Disabled:
				break;
			case Noesis::StencilMode::Equal_Keep:
				stencil |= BGFX_STENCIL_OP_FAIL_S_KEEP | BGFX_STENCIL_OP_FAIL_Z_KEEP | BGFX_STENCIL_OP_PASS_Z_KEEP;
				break;
			case Noesis::StencilMode::Equal_Incr:
				stencil |= BGFX_STENCIL_OP_FAIL_S_KEEP | BGFX_STENCIL_OP_FAIL_Z_KEEP | BGFX_STENCIL_OP_PASS_Z_INCR;
				break;
			case Noesis::StencilMode::Equal_Decr:
				stencil |= BGFX_STENCIL_OP_FAIL_S_KEEP | BGFX_STENCIL_OP_FAIL_Z_KEEP | BGFX_STENCIL_OP_PASS_Z_DECR;
				break;
		}
	}
	if ( batch.stencilRef )
	{
		stencil |= BGFX_STENCIL_TEST_EQUAL | BGFX_STENCIL_FUNC_REF( batch.stencilRef ) | BGFX_STENCIL_FUNC_RMASK( 0xFF );
	}
	else if ( stencil )
	{
		stencil |= BGFX_STENCIL_TEST_ALWAYS;
	}

	state |= BGFX_STATE_MSAA;

	const auto shaderIndex { static_cast<std::size_t>( batch.shader.v ) };
	assert( shaderIndex < FragmentShaderCount_ && "Invalid shader index! Must be between 0 and FragmentShaderCount" );

	this->SetTextures( batch );
	this->SetUniforms( batch );

	bgfx::setStencil( stencil );
	bgfx::setState( state );
	bgfx::submit( this->ViewID_, this->Programs_[shaderIndex] );
}

auto RenderDevice::BeginOffscreenRender() -> void
{
}

auto RenderDevice::EndOffscreenRender() -> void
{
}

auto RenderDevice::BeginOnscreenRender() -> void
{
	bgfx::setViewRect( this->ViewID_, 0, 0, this->Width_, this->Height_ );
}

auto RenderDevice::EndOnscreenRender() -> void
{
}

auto RenderDevice::SetTextures( const Noesis::Batch& batch ) noexcept -> void
{
	auto set {
		[i = -1, this]( ShaderUniform& sampler, Noesis::Texture* texture, const Noesis::SamplerState& samplerState ) mutable noexcept
		{
			++i;
			if ( texture )
			{
				auto* const instance { dynamic_cast<Texture*>( texture ) };
				bgfx::setTexture( i, sampler.Handle, instance->Handle, this->GetSamplerFlags( samplerState ) );
			}
		}
	};

	set( this->S_Pattern_, batch.pattern, batch.patternSampler );
	set( this->S_Ramps_, batch.ramps, batch.rampsSampler );
	set( this->S_Image_, batch.image, batch.imageSampler );
	set( this->S_Glyphs_, batch.glyphs, batch.glyphsSampler );
	set( this->S_Shadow_, batch.shadow, batch.shadowSampler );
}

auto RenderDevice::SetUniforms( const Noesis::Batch& batch ) noexcept -> void
{
	constexpr auto set {
		[]( ShaderUniform& target, const Noesis::UniformData& data ) noexcept
		{
			if ( data.values )
			{
				target.Set( data.values );
			}
		}
	};

	set( this->U_CBuffer1_VS_, batch.vertexUniforms[1] );

	set( this->U_CBuffer0_FS_, batch.pixelUniforms[0] );
	set( this->U_CBuffer1_FS_, batch.pixelUniforms[1] );
}

auto RenderDevice::GetSWrapMode( const Noesis::SamplerState sampler ) const noexcept -> std::uint32_t
{
	switch ( sampler.f.wrapMode )
	{
		case Noesis::WrapMode::Enum::ClampToEdge:
			return BGFX_SAMPLER_U_CLAMP;
		case Noesis::WrapMode::Enum::ClampToZero:
			return BGFX_SAMPLER_U_BORDER;
		case Noesis::WrapMode::Enum::Repeat:
			return 0;
		case Noesis::WrapMode::Enum::MirrorU:
			return BGFX_SAMPLER_U_MIRROR;
		case Noesis::WrapMode::Enum::MirrorV:
			return 0;
		case Noesis::WrapMode::Enum::Mirror:
			return BGFX_SAMPLER_U_MIRROR;
		default:
			return 0;
	}
}

auto RenderDevice::GetTWrapMode( const Noesis::SamplerState sampler ) const noexcept -> std::uint32_t
{
	switch ( sampler.f.wrapMode )
	{
		case Noesis::WrapMode::Enum::ClampToEdge:
			return BGFX_SAMPLER_V_CLAMP;
		case Noesis::WrapMode::Enum::ClampToZero:
			return BGFX_SAMPLER_V_BORDER;
		case Noesis::WrapMode::Enum::Repeat:
			return 0;
		case Noesis::WrapMode::Enum::MirrorU:
			return 0;
		case Noesis::WrapMode::Enum::MirrorV:
			return BGFX_SAMPLER_V_MIRROR;
		case Noesis::WrapMode::Enum::Mirror:
			return BGFX_SAMPLER_V_MIRROR;
		default:
			return 0;
	}
}

auto RenderDevice::GetMinMode( const Noesis::SamplerState sampler ) const noexcept -> std::uint32_t
{
	switch ( sampler.f.mipFilter )
	{
		case Noesis::MipFilter::Disabled:
			return 0;
		case Noesis::MipFilter::Nearest:
			return BGFX_SAMPLER_MIN_POINT;
		case Noesis::MipFilter::Linear:
			return BGFX_SAMPLER_MIN_ANISOTROPIC;
		default:
			return 0;
	}
}

auto RenderDevice::GetMagMode( const Noesis::SamplerState sampler ) const noexcept -> std::uint32_t
{
	switch ( sampler.f.minmagFilter )
	{
		case Noesis::MinMagFilter::Nearest:
			return BGFX_SAMPLER_MAG_POINT;
		case Noesis::MinMagFilter::Linear:
			return BGFX_SAMPLER_MAG_ANISOTROPIC;
		default:
			return 0;
	}
}

auto RenderDevice::GetSamplerFlags( const Noesis::SamplerState sampler ) const noexcept -> std::uint32_t
{
	return this->GetSWrapMode( sampler ) | this->GetTWrapMode( sampler ) | this->GetMinMode( sampler ) | this->GetMagMode( sampler );
}
} // namespace AppGUI
