#pragma once

#include <cstdint>
#include <cstddef>
#include <NoesisPCH.h>

namespace AppGUI
{
    inline constinit bool IsHoveredThisFrame;

    [[nodiscard]] extern auto HoverTest(const Noesis::Visual* root) -> bool;
}
