#pragma once

#include <NoesisPCH.h>

#include "../Shaders/VSListing.hpp"
#include "../Shaders/FSListing.hpp"

namespace AppGUI
{
    static_assert(sizeof(VertexShaders) / sizeof(*VertexShaders) == Noesis::Shader::Vertex::Count);
    static_assert(sizeof(FragmentShaders) / sizeof(*FragmentShaders) == Noesis::Shader::Count - 1); // -1 because we do not support custom effect!
}
