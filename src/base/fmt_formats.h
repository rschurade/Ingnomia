//
// Created by Arcnor on 14/07/2022.
// Copyright (c) 2022 Reversed Games. All rights reserved.
//

#pragma once

#include <vector>
#include <fmt/format.h>

template<typename T, typename Allocator>
struct fmt::formatter<std::vector<T, Allocator>>
{
	template<typename ParseContext>
	constexpr auto parse(ParseContext& ctx) {
		return ctx.begin();
	}

	template<typename FormatContext>
	auto format(const std::vector<T, Allocator>& list, FormatContext &ctx) {
		if (list.empty()) {
			return fmt::format_to(ctx.out(), "");
		}

		auto it = fmt::format_to(ctx.out(), "{}", list[0]);
		for ( int j = 1; j < list.size(); ++j )
		{
			it = fmt::format_to(it, ", {}", list[j]);
		}
		return it;
	}
};
