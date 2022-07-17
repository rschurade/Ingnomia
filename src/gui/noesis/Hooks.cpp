#include "spdlog/spdlog.h"

#include <cstdint>
#include <string>

#include <absl/container/btree_set.h>

namespace AppGUI
{
	static absl::btree_set<std::string> m_noesisMessages;

    auto LogHook
    (
        [[maybe_unused]] const char*,
        [[maybe_unused]] std::uint32_t,
        std::uint32_t level,
        [[maybe_unused]] const char*,
        const char* message
    ) -> void
    {
		// [TRACE] [DEBUG] [INFO] [WARNING] [ERROR]
		const char* prefixes[] = { "T", "D", "I", "W", "E" };
		//printf("[NOESIS/%s] %s\n", prefixes[level], message);
		if ( m_noesisMessages.contains( message ) )
		{
			return;
		}
		m_noesisMessages.insert( message );

		spdlog::debug( "[NOESIS] {} {}", prefixes[level], message );
    }

    auto ErrorHook
    (
        const char* file,
        std::uint32_t line,
        const char* message,
        const bool fatal
    ) -> void
    {
		if ( fatal )
		{
			spdlog::critical( "[NOESIS] {} in {}:{}", message, file, line );
			abort();
		}
		else
		{
			spdlog::debug( "[NOESIS] {} in {}:{}", message, file, line );
		}
	}
}
