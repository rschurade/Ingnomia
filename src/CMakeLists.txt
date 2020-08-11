file(GLOB_RECURSE SOURCE_LIST
	"*.h"
	"*.hpp"
	"*.cpp"
)

target_sources(${PROJECT_NAME}
	PRIVATE
		${SOURCE_LIST}
)

target_precompile_headers(${PROJECT_NAME}
	PRIVATE
		pch.h
)
