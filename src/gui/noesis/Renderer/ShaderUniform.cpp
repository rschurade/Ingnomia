//
// Created by Arcnor on 17/07/2022.
// Copyright (c) 2022 Reversed Games. All rights reserved.
//

#include "ShaderUniform.h"

#include <bgfx/bgfx.h>

bgfx::UniformType::Enum type2bgfx(UniformType type) {
	switch(type) {
		case UniformType::Sampler: return bgfx::UniformType::Sampler;
		case UniformType::Vec4: return bgfx::UniformType::Vec4;
	}
	throw std::runtime_error("Invalid uniform type");
}

ShaderUniform::ShaderUniform( const std::string& name, const UniformType& type, const uint32_t count ) :
	Handle( bgfx::createUniform( name.c_str(), type2bgfx( type ), count ) ), m_count( count )
{
}

void ShaderUniform::Set( const void* values )
{
	bgfx::setUniform(Handle, values, m_count);
}
