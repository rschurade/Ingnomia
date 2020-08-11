add_library(fastnoise)

target_include_directories(exprtk
	INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(fastnoise
	PUBLIC
		FastNoise.h
	PRIVATE
		FastNoise.cpp
)
