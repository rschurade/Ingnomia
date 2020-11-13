add_library(exprtk)

target_include_directories(exprtk
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(exprtk
	PRIVATE
		exprtk.hpp
		exprtk.cpp
)