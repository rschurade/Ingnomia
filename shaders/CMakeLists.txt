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
	"*_c.shader"
)
file(
	GLOB_RECURSE CS_SOURCE_LIST
	RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	"*_c.shader"
)

target_sources(${PROJECT_NAME}
	PRIVATE
		${SOURCE_LIST}
		${CMAKE_CURRENT_LIST_FILE}
)

if(WIN32)
	set(BGFX_PLATFORM "windows")
	set(BGFX_PROFILES "glsl" "spirv" "dx11")
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
	foreach(PROFILE IN LISTS BGFX_PROFILES)
		set(FILE_DST "${CMAKE_CURRENT_BINARY_DIR}/${FILE_BASE}/${FILE_NAME}_${PROFILE}.bin")

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

		add_custom_command(
			OUTPUT
				"${FILE_DST}"
			DEPENDS
				"${CMAKE_CURRENT_SOURCE_DIR}/${FILE}"
				"${VARYING_DEF}"
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
			VERBATIM
		)
		target_sources(shaders
			PRIVATE
				"${FILE_DST}"
		)
		list(APPEND GENERATED_LIST "${FILE_DST}")
	endforeach()

	target_sources(shaders
		PRIVATE
			"${FILE}"
	)
endmacro()

add_custom_target(shaders
	SOURCES
		${FS_SOURCE_LIST}
		${VS_SOURCE_LIST}
		${CS_SOURCE_LIST}
	DEPENDS
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

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${FS_SOURCE_LIST} ${VS_SOURCE_LIST} ${CS_SOURCE_LIST})
list(REMOVE_DUPLICATES GENERATED_LIST)
source_group(TREE "${CMAKE_CURRENT_BINARY_DIR}" PREFIX "Generated" FILES ${GENERATED_LIST})
