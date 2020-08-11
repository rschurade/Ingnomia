cmake_minimum_required(VERSION 3.13.0)

project(Ingnomia
	VERSION 0.0.0
	DESCRIPTION ""
	HOMEPAGE_URL "https://github.com/rschurade/Ingnomia"
	LANGUAGES "CXX"
)

find_package(Qt5 REQUIRED QUIET COMPONENTS Core)

# Roots of libraries that don't use the standard directories
set(NOESIS_ROOT "" CACHE PATH "Root to the NoesisGui sdk")
set(STEAM_SDK_ROOT "" CACHE PATH "Root to the steam sdk")

# Noesis license
set(NOESIS_LICENSE_NAME "" CACHE STRING "Name")
set(NOESIS_LICENSE_KEY "" CACHE STRING "Key")

# Without this, paths are not relative in the sources list
cmake_policy(SET CMP0076 NEW)

include(GNUInstallDirs)
include(FindPkgConfig)

add_subdirectory(3rdparty)

# The program itself
add_executable(${PROJECT_NAME})

add_dependencies(${PROJECT_NAME} fastnoise)

# C++ version
set_target_properties(${PROJECT_NAME}
	PROPERTIES
		CXX_STANDARD 17
		AUTOMOC ON
)

# Headers
target_include_directories(${PROJECT_NAME}
	PRIVATE
		${NOESIS_ROOT}/Include
		${NOESIS_ROOT}/Src/Packages/App/ApplicationLauncher/Include
		${NOESIS_ROOT}/Src/Packages/App/Launcher/Include
		${NOESIS_ROOT}/Src/Packages/App/Providers/Include
		${NOESIS_ROOT}/Src/Packages/Render/GLRenderDevice/Include
		${STEAM_SDK_ROOT}/public/steam
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
		gl
)

find_library(STEAM steam_api
	HINTS
		${STEAM_SDK_ROOT}/redistributable_bin/linux64
)

find_library(NOESIS Noesis
	HINTS
		${NOESIS_ROOT}/Bin/linux_x86_64
)

# TODO add a target to compile the library from the Makefile in the sdk
find_library(NOESIS_APP NoesisApp
	HINTS
		${NOESIS_ROOT}/Bin/linux_x86_64
)

# Linking
target_link_libraries(${PROJECT_NAME}
	PRIVATE
		PkgConfig::shared
		${STEAM}
		${NOESIS}
		${NOESIS_APP}
		fastnoise
)

# Noesis license
configure_file(
	${CMAKE_SOURCE_DIR}/src/gui/_license.h.in
	${CMAKE_SOURCE_DIR}/src/gui/_license.h
	@ONLY
)

# Qt requieres position independent code
target_compile_options(${PROJECT_NAME}
	PRIVATE
		-fPIC
)

# Default flags
if(UNIX AND NOT (DEFINED ENV{CXXFLAGS} OR DEFINED CMAKE_CXX_FLAGS))
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -g")
endif()

# Install target
install(TARGETS ${PROJECT_NAME}
	DESTINATION ${CMAKE_INSTALL_BINDIR}
)
