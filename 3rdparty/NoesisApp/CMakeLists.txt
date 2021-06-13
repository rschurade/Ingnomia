project(NoesisApp)

add_library(${PROJECT_NAME})

# All used packages. Same list of packages must be added to register.cpp
set(NOESIS_APP_PACKAGES
	App/ApplicationLauncher
	App/DisplayLauncher
	App/Display
	App/Launcher
	App/Providers
	App/Theme
	App/Interactivity
	App/MediaElement
	Render/GLRenderContext
	Render/GLRenderDevice
	Render/RenderContext
)

find_package(Noesis REQUIRED)

target_link_libraries(${PROJECT_NAME}
	PUBLIC
		Noesis
)

target_compile_definitions(
	${PROJECT_NAME}
	PUBLIC
		NS_DEBUG
		NS_PROFILE
		NS_MINIMUM_LOG_LEVEL=0
	PRIVATE
		NS_APP_FRAMEWORK
)

target_precompile_headers(${PROJECT_NAME}
	PRIVATE
		pch.h
)


# Iterate over packages, and collect include + source directories
foreach(PACKAGE ${NOESIS_APP_PACKAGES})
	set(PACKAGE_ROOT "${NOESIS_ROOT}/Src/Packages/${PACKAGE}")
	set(INCLUDE_DIR "${PACKAGE_ROOT}/Include")

	file(GLOB_RECURSE PKG_SRCS "${PACKAGE_ROOT}/*.cpp"  "${PACKAGE_ROOT}/*.h")
	file(GLOB_RECURSE PKG_RES "${PACKAGE_ROOT}/*.xaml" "${PACKAGE_ROOT}/*.ttf" "${PACKAGE_ROOT}/*.ttf" "${PACKAGE_ROOT}/*.otf")

	list(APPEND NOESIS_APP_SRC ${PKG_SRCS})
	list(APPEND NOESIS_APP_BIN2H ${PKG_RES})

	if(EXISTS ${INCLUDE_DIR})
		target_include_directories(${PROJECT_NAME}
			PUBLIC
				${INCLUDE_DIR}
		)
		string(REPLACE "/" ";" PACKAGE_SPLIT ${PACKAGE})
		list(GET PACKAGE_SPLIT 0 PACKAGE_NAME)
		list(GET PACKAGE_SPLIT 1 COMPONENT_NAME)
		set(MAIN_INCLUDE ${INCLUDE_DIR}/Ns${PACKAGE_NAME}/${COMPONENT_NAME}.h)
		if(EXISTS ${MAIN_INCLUDE})
			target_precompile_headers(${PROJECT_NAME}
				PRIVATE
					${MAIN_INCLUDE}
			)
		endif()
	endif()

	# Switch package to static linkage
	string(REPLACE "/" "_" PACKAGENAME ${PACKAGE})
	string(TOUPPER ${PACKAGENAME} PACKAGENAME)
	target_compile_definitions(${PROJECT_NAME}
		PUBLIC
			"NS_${PACKAGENAME}_API="
	)
endforeach()

# Generated sources
if(UNIX)
	set(BIN2H_EXE "${NOESIS_ROOT}/Src/Tools/Bin2h/bin2h.py")
elseif(WIN32)
	set(BIN2H_EXE "${NOESIS_ROOT}/Src/Tools/Bin2h/bin2h.exe")
endif()
foreach(BIN2H_FILE ${NOESIS_APP_BIN2H})
	get_filename_component(BASENAME ${BIN2H_FILE} NAME)
	set(OUTPUTNAME "${BASENAME}.bin.h")
	set(OUTPUTPATH "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUTNAME}")
	add_custom_command(
		OUTPUT
			${OUTPUTNAME}
		COMMAND
			${BIN2H_EXE} ${BIN2H_FILE} > ${OUTPUTPATH}
		DEPENDS
			${BIN2H_FILE}
	)
	list(APPEND NOESIS_APP_GENERATED_PATHS ${OUTPUTPATH})
endforeach()

target_include_directories(${PROJECT_NAME}
	PRIVATE
		${CMAKE_CURRENT_BINARY_DIR}
)

file(GLOB_RECURSE NOESIS_CORE_INCLUDES "${NOESIS_ROOT}/Include/*.h")

target_sources(${PROJECT_NAME}
	PRIVATE
		${NOESIS_APP_GENERATED_PATHS}
		${NOESIS_APP_SRC}
		${NOESIS_APP_BIN2H}
		${NOESIS_CORE_INCLUDES}
		pch.h
		register.cpp
		README.md
)

source_group(TREE "${CMAKE_CURRENT_BINARY_DIR}" PREFIX "Generated" FILES ${NOESIS_APP_GENERATED_PATHS})
source_group(TREE "${NOESIS_ROOT}/Src/Packages" PREFIX "NoesisApp" FILES ${NOESIS_APP_SRC})
source_group(TREE "${NOESIS_ROOT}/Src/Packages" PREFIX "Resources" FILES ${NOESIS_APP_BIN2H})
source_group(TREE "${NOESIS_ROOT}/Include" PREFIX "Core" FILES ${NOESIS_CORE_INCLUDES})

if(UNIX)
	# NoesisApp requires several additional libraries on Linux
	find_package(Threads REQUIRED)
	find_package(OpenGL REQUIRED)
	find_package(X11 REQUIRED)
	target_link_libraries(${PROJECT_NAME}
		PRIVATE
			OpenGL::GL
			Threads::Threads
			${X11_LIBRARIES}
			${CMAKE_DL_LIBS}
	)
endif()
