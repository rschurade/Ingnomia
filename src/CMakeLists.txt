target_sources(${PROJECT_NAME}
	PRIVATE
		main.cpp
		pch.cpp
)

add_subdirectory(base)
add_subdirectory(game)
add_subdirectory(gfx)
add_subdirectory(gui)
