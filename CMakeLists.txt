cmake_minimum_required(VERSION 3.13.0)

project(Ingnomia
	VERSION 0.0.0
	DESCRIPTION ""
	HOMEPAGE_URL "https://github.com/rschurade/Ingnomia"
	LANGUAGES "CXX"
)

# Noesis root
set(NOESIS_ROOT "" CACHE PATH "Root to the NoesisGui sdk")

# Without this, paths are not relative in the sources list
cmake_policy(SET CMP0076 NEW)

include(GNUInstallDirs)
include(FindPkgConfig)

# The program itself
add_executable(${PROJECT_NAME})

# C++ version
set_target_properties(${PROJECT_NAME}
	PROPERTIES
		CXX_STANDARD 17
)

# Headers
target_include_directories(${PROJECT_NAME}
	PRIVATE
		${NOESIS_ROOT}/Include
)

add_subdirectory(src) # Sources list

# Shared libraries
pkg_check_modules(shared
	REQUIRED IMPORTED_TARGET
		Qt5Core
		Qt5Xml
		Qt5Sql
		Qt5OpenGL
		Qt5Network
)

# Linking
target_link_libraries(${PROJECT_NAME}
	PRIVATE
		PkgConfig::shared
)

# Qt requieres position independent code
target_compile_options(${PROJECT_NAME}
	PRIVATE
		-fPIC
)

# Default flags
if(UNIX AND NOT DEFINED ENV{CXXFLAGS})
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -g")
endif()

# Install target
install(TARGETS ${PROJECT_NAME}
	DESTINATION ${CMAKE_INSTALL_BINDIR}
)
