file(GLOB_RECURSE SOURCE_LIST
	"*.h"
	"*.hpp"
	"*.cpp"
)

target_sources(${PROJECT_NAME}
	PRIVATE
		${SOURCE_LIST}
		${CMAKE_CURRENT_LIST_FILE}
)

if (NOT APPLE)
	target_precompile_headers(${PROJECT_NAME}
		PRIVATE
			$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/pch.h>
	)
endif()
