//
// Created by Arcnor on 20/07/2022.
// Copyright (c) 2022 Reversed Games. All rights reserved.
//

#include "containersHelper.h"

#include <optional>
#include <string>

namespace vars
{
std::optional<std::string> opt( const std::string& str )
{
	if ( str.empty() )
	{
		return std::nullopt;
	}
	return str;
}
} // namespace vars