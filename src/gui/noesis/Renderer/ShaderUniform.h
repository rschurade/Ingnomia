//
// Created by Arcnor on 17/07/2022.
// Copyright (c) 2022 Reversed Games. All rights reserved.
//

#ifndef INGNOMIA_SHADERUNIFORM_H
#define INGNOMIA_SHADERUNIFORM_H

#include <string>
#include <bgfx/bgfx.h>

enum class UniformType {
	Sampler, Vec4
};

class ShaderUniform
{
public:
	ShaderUniform( const std::string& name, const UniformType& type, const uint32_t count = 0 );
	void Set(const void* values);

public:
	const bgfx::UniformHandle Handle;
private:
	const uint32_t m_count;
};

#endif // INGNOMIA_SHADERUNIFORM_H
