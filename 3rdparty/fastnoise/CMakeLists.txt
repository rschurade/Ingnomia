add_library(fastnoise)

target_include_directories(fastnoise
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(fastnoise
	PRIVATE
		FastNoise.h
		FastNoise.cpp
)
