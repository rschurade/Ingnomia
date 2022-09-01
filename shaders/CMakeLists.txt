file(
	GLOB_RECURSE FS_SOURCE_LIST
	RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	"*_FS.shader"
)

file(
	GLOB_RECURSE VS_SOURCE_LIST
	RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	"*_VS.shader"
)
file(
	GLOB_RECURSE CS_SOURCE_LIST
	RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	"*_CS.shader"
)
file(
	GLOB_RECURSE OTHER_SOURCE_LIST
	RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	"*.shader"
	"*.ishader"
)

target_sources(${PROJECT_NAME}
	PRIVATE
		${SOURCE_LIST}
		${CMAKE_CURRENT_LIST_FILE}
)

if(WIN32)
	set(BGFX_PLATFORM "windows")
	set(BGFX_PROFILES "glsl" "spirv" ) # "dx11"
elseif(APPLE)
	set(BGFX_PLATFORM "osx")
	set(BGFX_PROFILES "metal")
elseif(ANDROID)
	set(BGFX_PLATFORM "android")
	set(BGFX_PROFILES "essl" "spirv")
elseif(UNIX)
	set(BGFX_PLATFORM "windows")
	set(BGFX_PROFILES "glsl" "spirv")
endif()

get_target_property(BGFX_INCLUDE_DIRECTORIES bgfx::bgfx INTERFACE_INCLUDE_DIRECTORIES)

set(BGFX_INCLUDE)
foreach(BGFX_INCLUDE_DIRECTORY IN LISTS BGFX_INCLUDE_DIRECTORIES)
	list(APPEND BGFX_INCLUDE "-i" "${BGFX_INCLUDE_DIRECTORY}/bgfx")
endforeach()

set(GENERATED_LIST)

macro(compile_shader FILE TYPE)
	get_filename_component(FILE_NAME "${FILE}" NAME_WE)
	get_filename_component(FILE_BASE "${FILE}" DIRECTORY)
	set(VARYING_DEF "${CMAKE_CURRENT_SOURCE_DIR}/${FILE_BASE}/varying.def.shader")

	set(OUTPUTS)
	set(COMMANDS)
	foreach(PROFILE IN LISTS BGFX_PROFILES)
		set(FILE_DST "${CMAKE_CURRENT_BINARY_DIR}/${RUNTIME_OUTPUT_DIRECTORY}/${FILE_BASE}/${FILE_NAME}_${PROFILE}.bin")
		set(FILE_DEP "${FILE_DST}.d")

		if("${PROFILE}" STREQUAL "dx11")
			if("${TYPE}" STREQUAL "fragment")
				set(PROFILE "ps_5_0")
			elseif("${TYPE}" STREQUAL "vertex")
				set(PROFILE "vs_5_0")
			else()
				set(PROFILE "cs_5_0")
			endif()
		elseif("${PROFILE}" STREQUAL "glsl")
			set(PROFILE "430")
		elseif("${PROFILE}" STREQUAL "essl")
			set(PROFILE "310_es")
		endif()

		list(APPEND OUTPUTS "${FILE_DST}")

		list(APPEND COMMANDS
			COMMAND
				bgfx::shaderc
					-f "${CMAKE_CURRENT_SOURCE_DIR}/${FILE}"
					-o "${FILE_DST}"
					-i "${CMAKE_CURRENT_SOURCE_DIR}/${FILE_BASE}"
					${BGFX_INCLUDE}
					--type "${TYPE}"
					--platform "${BGFX_PLATFORM}"
					--profile "${PROFILE}"
					--varyingdef "${VARYING_DEF}"
					--depends
		)

		list(APPEND GENERATED_LIST "${FILE_DST}")
		install(
			FILES "${FILE_DST}"
			DESTINATION "${CMAKE_INSTALL_BINDIR}/content/shaders/${FILE_BASE}"
		)
	endforeach()
	# Only one add_custom_command per main dependency allowed
	add_custom_command(
		OUTPUT
			${OUTPUTS}
		DEPENDS
			"${CMAKE_CURRENT_SOURCE_DIR}/${FILE}"
			"${VARYING_DEF}"
		MAIN_DEPENDENCY
			"${CMAKE_CURRENT_SOURCE_DIR}/${FILE}"
		${COMMANDS}
		DEPFILE
			"${FILE_DEP}"
		VERBATIM
	)
endmacro()

add_custom_target(IngnomiaShaders
	SOURCES
		${FS_SOURCE_LIST}
		${VS_SOURCE_LIST}
		${CS_SOURCE_LIST}
		${OTHER_SOURCE_LIST}
		${GENERATED_LIST}
)

foreach ( FILE IN LISTS FS_SOURCE_LIST )
	compile_shader("${FILE}" "fragment")
endforeach()

foreach ( FILE IN LISTS VS_SOURCE_LIST )
	compile_shader("${FILE}" "vertex")
endforeach()

foreach ( FILE IN LISTS CS_SOURCE_LIST )
	compile_shader("${FILE}" "compute")
endforeach()

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${FS_SOURCE_LIST} ${VS_SOURCE_LIST} ${CS_SOURCE_LIST} ${OTHER_SOURCE_LIST})
