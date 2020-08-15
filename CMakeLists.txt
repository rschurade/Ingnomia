cmake_minimum_required(VERSION 3.16.0)
# Disable release-builds without debug info
set(CMAKE_CONFIGURATION_TYPES Debug RelWithDebInfo CACHE STRING INTERNAL FORCE)
project(Ingnomia
	VERSION 0.0.0
	DESCRIPTION ""
	HOMEPAGE_URL "https://github.com/rschurade/Ingnomia"
	LANGUAGES "CXX"
)

find_package(Qt5 REQUIRED QUIET COMPONENTS Core Xml Sql OpenGL Network)

# Roots of libraries that don't use the standard directories
set(NOESIS_ROOT "" CACHE PATH "Root to the NoesisGui sdk")
set(STEAM_SDK_ROOT "" CACHE PATH "Root to the steam sdk")

# Noesis license
set(NOESIS_LICENSE_NAME "" CACHE STRING "Name")
set(NOESIS_LICENSE_KEY "" CACHE STRING "Key")

# Without this, paths are not relative in the sources list
cmake_policy(SET CMP0076 NEW)

include(GNUInstallDirs)

if (UNIX)
	include(FindPkgConfig)
endif()


# The program itself
add_executable(${PROJECT_NAME})

# C++ version
set_target_properties(${PROJECT_NAME}
	PROPERTIES
		CXX_STANDARD 17
		AUTOMOC ON
		INTERPROCEDURAL_OPTIMIZATION ON
		INSTALL_RPATH "$ORIGIN"
)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY AUTOGEN_SOURCE_GROUP "Generated")

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

add_subdirectory(content) # Deployable content
# Copy content to target directory
add_custom_command(TARGET ${PROJECT_NAME}
	POST_BUILD
	COMMAND
		${CMAKE_COMMAND} -E copy_directory
		${CMAKE_SOURCE_DIR}/content $<TARGET_FILE_DIR:${PROJECT_NAME}>/content
)

if(WIN32)
	target_sources(${PROJECT_NAME}
		PRIVATE
			${PROJECT_NAME}.rc
			${PROJECT_NAME}.ico
			${PROJECT_NAME}.manifest
	)
endif()

target_sources(${PROJECT_NAME}
	PRIVATE
		Changelog.md
		README.md
)

add_subdirectory(src) # Sources list
add_subdirectory(3rdparty) # 3rd party dependencies

if (UNIX)
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
endif()

if(UNIX)
	find_library(STEAM steam_api
		HINTS
			${STEAM_SDK_ROOT}/redistributable_bin/linux64
	)
else()
	find_library(STEAM steam_api64
		HINTS
			${STEAM_SDK_ROOT}/redistributable_bin/win64
	)
endif()

find_library(NOESIS Noesis
	HINTS
		${NOESIS_ROOT}/Bin/linux_x86_64
		${NOESIS_ROOT}/Lib/windows_x86_64
)

# TODO add a target to compile the library from the Makefile in the sdk
find_library(NOESIS_APP NoesisApp
	HINTS
		${NOESIS_ROOT}/Bin/linux_x86_64
		${NOESIS_ROOT}/Lib/windows_x86_64
)

# Linking
target_link_libraries(${PROJECT_NAME}
	PRIVATE
		Qt5::Core
		Qt5::Xml
		Qt5::Sql
		Qt5::OpenGL
		Qt5::Network
		${STEAM}
		${NOESIS}
		${NOESIS_APP}
		fastnoise
		exprtk
)

# Noesis license
configure_file(
	${CMAKE_SOURCE_DIR}/src/gui/license.h.in
	${CMAKE_SOURCE_DIR}/src/gui/license.h
	@ONLY
)

if(UNIX)
	# Qt requires position independent code
	target_compile_options(${PROJECT_NAME}
		PRIVATE
			-fPIC
	)
endif()

get_target_property(SOURCE_LIST ${PROJECT_NAME} SOURCES)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCE_LIST})

# Default flags
target_compile_definitions(${PROJECT_NAME}
	PRIVATE
		NS_DEBUG
		NS_PROFILE
)

if(UNIX AND NOT (DEFINED ENV{CXXFLAGS} OR CMAKE_CXX_FLAGS))
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -g")
endif()

if(MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Gy /Oi /MP /GS- /W3 /permissive-")
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /CGTHREADS:8 /PROFILE")
	SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Ob2")
	set_target_properties(${PROJECT_NAME} PROPERTIES
		LINK_FLAGS /SUBSYSTEM:WINDOWS
	)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()

# Install target
install(TARGETS ${PROJECT_NAME}
	DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
	DIRECTORY
	$<TARGET_FILE_DIR:${PROJECT_NAME}>/content
	DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(UNIX)
	install(FILES ${NOESIS}
		DESTINATION ${CMAKE_INSTALL_BINDIR}
	)

	install(FILES ${NOESIS_APP}
		DESTINATION ${CMAKE_INSTALL_BINDIR}
	)

	install(FILES ${STEAM}
		DESTINATION ${CMAKE_INSTALL_BINDIR}
	)
endif()

# Deployment helpers for Windows
if(WIN32)
	if(Qt5_FOUND AND TARGET Qt5::qmake AND NOT TARGET Qt5::windeployqt)
		get_target_property(_qt5_qmake_location Qt5::qmake IMPORTED_LOCATION)

		execute_process(
			COMMAND "${_qt5_qmake_location}" -query QT_INSTALL_PREFIX
			RESULT_VARIABLE return_code
			OUTPUT_VARIABLE qt5_install_prefix
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)

		set(imported_location "${qt5_install_prefix}/bin/windeployqt.exe")

		if(EXISTS ${imported_location})
			add_executable(Qt5::windeployqt IMPORTED)

			set_target_properties(Qt5::windeployqt PROPERTIES
				IMPORTED_LOCATION ${imported_location}
			)
		endif()
	endif()

	function(deployQt foo)
		if(TARGET Qt5::windeployqt)
			# execute windeployqt in a tmp directory after build
			add_custom_command(TARGET ${foo}
				POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/windeployqt_$<CONFIG>"
				COMMAND set PATH=%PATH%$<SEMICOLON>${qt5_install_prefix}/bin
				# for deployment
				COMMAND Qt5::windeployqt --pdb --no-translations --no-webkit2 --no-system-d3d-compiler --no-opengl-sw --dir "${CMAKE_CURRENT_BINARY_DIR}/windeployqt_$<CONFIG>" "$<TARGET_FILE_DIR:${foo}>/$<TARGET_FILE_NAME:${foo}>"
				COMMAND
					${CMAKE_COMMAND} -E copy_directory
					"${CMAKE_CURRENT_BINARY_DIR}/windeployqt_$<CONFIG>/"
					$<TARGET_FILE_DIR:${PROJECT_NAME}>
			)

			# copy deployment directory during installation
			install(
				DIRECTORY
				"${CMAKE_CURRENT_BINARY_DIR}/windeployqt_$<CONFIG>/"
				DESTINATION ${CMAKE_INSTALL_BINDIR}
			)
		endif()
	endfunction()

	deployQt(${PROJECT_NAME})

	set(RUNTIME_DLLS
		${STEAM_SDK_ROOT}/redistributable_bin/win64/steam_api64.dll
		${NOESIS_ROOT}/Bin/windows_x86_64/Noesis.dll
		${NOESIS_ROOT}/Bin/windows_x86_64/NoesisApp.dll
		${NOESIS_ROOT}/Bin/windows_x86_64/NoesisApp.pdb
	)

	add_custom_command(TARGET ${PROJECT_NAME}
		POST_BUILD
		COMMAND
			${CMAKE_COMMAND} -E copy_if_different
			${RUNTIME_DLLS}
			$<TARGET_FILE_DIR:${PROJECT_NAME}>
	)

	install(
		FILES
		${RUNTIME_DLLS}
		DESTINATION ${CMAKE_INSTALL_BINDIR}
	)

	INCLUDE_EXTERNAL_MSPROJECT("Blend" ${CMAKE_CURRENT_SOURCE_DIR}/gui/gui.csproj)

	target_link_libraries(${PROJECT_NAME}
		PRIVATE
			Version.lib
	)
endif()
