file(GLOB_RECURSE SOURCE_LIST
	"*.h"
	"*.hpp"
	"*.cpp"
)

list(FILTER SOURCE_LIST EXCLUDE REGEX "base/behaviortree/.*")
# Remove old QT + OpenGL renderers
list(FILTER SOURCE_LIST EXCLUDE REGEX "gui/mainwindow.*")
list(FILTER SOURCE_LIST EXCLUDE REGEX "gui/mainwindowrenderer.*")
list(FILTER SOURCE_LIST EXCLUDE REGEX "main.cpp")

target_sources(${PROJECT_NAME}
	PRIVATE
		${SOURCE_LIST}
		${CMAKE_CURRENT_LIST_FILE}
)

add_subdirectory(base/behaviortree)
add_subdirectory(protocol)

if (NOT APPLE)
	target_precompile_headers(${PROJECT_NAME}
		PRIVATE
			$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/pch.h>
	)
endif()
