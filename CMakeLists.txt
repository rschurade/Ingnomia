cmake_minimum_required(VERSION 3.16.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Disable release-builds without debug info
set(CMAKE_CONFIGURATION_TYPES Debug RelWithDebInfo CACHE STRING INTERNAL FORCE)

# Project version, 4 numbers representation
set(DEFAULT_VERSION "0.9.0.0" CACHE STRING "Version" FORCE)
# Set from CI only
set(BUILD_ID "0" CACHE INTERNAL "")
set(GIT_REPO "" CACHE INTERNAL "")
set(GIT_REF "" CACHE INTERNAL "")
set(GIT_SHA "" CACHE INTERNAL "")

project(Ingnomia
	VERSION ${DEFAULT_VERSION}
	DESCRIPTION "Gnome city builder"
	HOMEPAGE_URL "https://github.com/rschurade/Ingnomia"
	LANGUAGES "CXX"
)

# TODO: Remove when removing sqlite3
enable_language(C)

if (APPLE)
	# TODO: Remove when removing sqlite3
	enable_language(OBJC)
	enable_language(OBJCXX)
endif()

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Gy /Gw /Oi /MP /GS- /W3 /permissive- /await:strict /Zc:__cplusplus /Zc:rvalueCast /Zc:strictStrings /Zc:referenceBinding /Zc:ternary /Zi /w24458 /w34456 /w24702")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /CGTHREADS:8 /DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Ob2")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")

find_package(Abseil REQUIRED)
find_package(Sigslot REQUIRED)
find_package(sqlite3 REQUIRED)
find_package(fmt REQUIRED)

find_package(Qt5 REQUIRED QUIET COMPONENTS Core Xml Sql OpenGL)

find_package(BGFX REQUIRED)
find_package(RangesV3 REQUIRED)

find_package(NlohmannJson REQUIRED)
find_package(spdlog REQUIRED)
find_package(pugixml REQUIRED)
find_package(googletest REQUIRED)
find_package(PlatformFolders REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_Image REQUIRED)
find_package(STB REQUIRED)

set(IMGUI_EXTRA_CONFIG "${CMAKE_SOURCE_DIR}/src/debug/imgui/ing_imconfig.h")
find_package(DearImGui REQUIRED)

find_package(Steam REQUIRED)
find_package(Noesis REQUIRED)
find_package(OpenALSoft REQUIRED)
# Noesis license
set(NOESIS_LICENSE_NAME "" CACHE STRING "Name")
set(NOESIS_LICENSE_KEY "" CACHE STRING "Key")

set(BUGSPLAT_DB "" CACHE STRING "BSDB")

# Without this, paths are not relative in the sources list
cmake_policy(SET CMP0076 NEW)

include(GNUInstallDirs)

# The program itself
add_executable(${PROJECT_NAME})

string(APPEND CMAKE_CXX_FLAGS_DEBUG " -D_DEBUG")

# C++ version
set_target_properties(${PROJECT_NAME}
	PROPERTIES
		CXX_STANDARD 20
		AUTOMOC ON
		INTERPROCEDURAL_OPTIMIZATION ON
		INSTALL_RPATH "$ORIGIN"
)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY AUTOGEN_SOURCE_GROUP "Generated")

add_subdirectory(content) # Deployable content
# Copy content to target directory
add_custom_command(TARGET ${PROJECT_NAME}
	POST_BUILD
	COMMAND
		${CMAKE_COMMAND} -E copy_directory
		${CMAKE_SOURCE_DIR}/content $<TARGET_FILE_DIR:${PROJECT_NAME}>/content
)

if(WIN32)
	set(RC_SOURCES
		${PROJECT_NAME}.ico
		windows/${PROJECT_NAME}.rc
		windows/${PROJECT_NAME}.manifest
		windows/resource.h
	)
	target_sources(${PROJECT_NAME}
		PRIVATE
			${RC_SOURCES}
	)

endif()

target_sources(${PROJECT_NAME}
	PRIVATE
		Changelog.md
		README.md
		LICENSE
		${sqlite3_SOURCE_DIR}/sqlite3.c
)

target_include_directories(${PROJECT_NAME}
	PRIVATE
		${sqlite3_SOURCE_DIR}
)

add_subdirectory(src) # Sources list
add_subdirectory(test) # Sources list
add_subdirectory(3rdparty) # 3rd party dependencies

target_include_directories(${PROJECT_NAME} PRIVATE ${stb_SOURCE_DIR})

# Linking
target_link_libraries(${PROJECT_NAME}
	PRIVATE
		Ingnomia::BehaviorTree

		Qt5::Core
		Qt5::Sql
		Qt5::OpenGL
		Steam
		NoesisApp
		fastnoise
		exprtk
		OpenAL::OpenAL
		absl::hash
		absl::flat_hash_map
		Pal::Sigslot
		fmt::fmt

		bgfx bx bimg
		range-v3::range-v3

		nlohmann_json::nlohmann_json
		pugixml::pugixml
		sago::platform_folders

		DearImGui::DearImGui

		SDL2::SDL2main SDL2::SDL2-static
		SDL2_image::SDL2_image-static
)

# Noesis license
configure_file(
	${CMAKE_SOURCE_DIR}/src/gui/license.h.in
	${CMAKE_SOURCE_DIR}/src/gui/license.h
	@ONLY
)

configure_file(
	${CMAKE_SOURCE_DIR}/src/version.h.in
	${CMAKE_SOURCE_DIR}/src/version.h
	@ONLY
)

target_sources(${PROJECT_NAME}
	PRIVATE
		${CMAKE_SOURCE_DIR}/src/gui/license.h.in
		${CMAKE_SOURCE_DIR}/src/version.h.in
)

if(UNIX)
	# Qt requires position independent code
	set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE TRUE)
endif()

get_target_property(SOURCE_LIST ${PROJECT_NAME} SOURCES)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCE_LIST})

if(UNIX AND NOT (DEFINED ENV{CXXFLAGS} OR CMAKE_CXX_FLAGS))
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -g3 -ggdb")
	# Kill some overly verbose warnings
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas -Wno-unused-parameter")
	# But treat likely errors as actual errors
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=missing-field-initializers -Werror=ignored-qualifiers -Werror=delete-non-virtual-dtor -Werror=unused-value")
endif()

if(MSVC)
	set_target_properties(${PROJECT_NAME} PROPERTIES
		LINK_FLAGS /SUBSYSTEM:WINDOWS
	)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()

if(MSVC AND NOT BUGSPLAT_DB STREQUAL "")
	find_package(BugSplat REQUIRED)

	target_link_libraries(${PROJECT_NAME}
		PRIVATE
			BugSplat
	)

	get_target_property(BUGSPLAT_DLL BugSplat LOCATION)
	get_target_property(BUGSPLAT_RC_DLL BugSplatRc LOCATION)
	get_target_property(BUGSPLAT_SEND_DLL BugSplatSendReport LOCATION)
	list(APPEND RUNTIME_DLLS ${BUGSPLAT_DLL} ${BUGSPLAT_RC_DLL} ${BUGSPLAT_SEND_DLL})
endif()

# Install target
install(
	TARGETS ${PROJECT_NAME}
	DESTINATION ${CMAKE_INSTALL_BINDIR}
)

get_target_property(STEAM_DLL Steam LOCATION)
get_target_property(NOESIS_DLL Noesis LOCATION)
list(APPEND RUNTIME_DLLS ${STEAM_DLL})
list(APPEND RUNTIME_DLLS ${NOESIS_DLL})

install(
	FILES
		${RUNTIME_DLLS}
		Changelog.md
		LICENSE
	DESTINATION
		${CMAKE_INSTALL_BINDIR}
)

if(WIN32)
	install(
		FILES $<TARGET_PDB_FILE:${PROJECT_NAME}>
		DESTINATION ${CMAKE_INSTALL_BINDIR}
		OPTIONAL
	)
endif()

# Deployment helpers for Windows
if(WIN32)
	include(windeployqt)
	windeployqt(${PROJECT_NAME})

	add_custom_command(TARGET ${PROJECT_NAME}
		POST_BUILD
		COMMAND
			${CMAKE_COMMAND} -E copy_if_different
			${RUNTIME_DLLS}
			$<TARGET_FILE_DIR:${PROJECT_NAME}>
	)

	INCLUDE_EXTERNAL_MSPROJECT("Blend" ${CMAKE_CURRENT_SOURCE_DIR}/gui/gui.csproj)
	set_target_properties("Blend" PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()
