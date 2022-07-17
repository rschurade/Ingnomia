#pragma once

#include <cstdint>

namespace AppGUI
{
    extern auto LogHook(const char*, std::uint32_t, std::uint32_t level, const char* , const char* msg) noexcept -> void;
    extern auto ErrorHook(const char*, std::uint32_t, const char* msg, bool fatal) -> void;
}
