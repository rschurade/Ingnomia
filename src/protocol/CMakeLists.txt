file(
	GLOB_RECURSE PROTO_SOURCE_LIST
	RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	"*.proto"
)

set(SOURCE_LIST)
set(DST_DIR "${CMAKE_CURRENT_BINARY_DIR}/${RUNTIME_OUTPUT_DIRECTORY}")

foreach ( FILE IN LISTS PROTO_SOURCE_LIST )
	get_filename_component(FILE_NAME "${FILE}" NAME_WE)
	get_filename_component(FILE_BASE "${FILE}" DIRECTORY)

	set(DST_DEP "${DST_DIR}/${FILE_NAME}.depends")
	set(OUTPUTS "${DST_DIR}/${FILE_NAME}.pb.h" "${DST_DIR}/${FILE_NAME}.pb.cc")
	list(APPEND SOURCE_LIST ${OUTPUTS})

	add_custom_command(
		OUTPUT
			${OUTPUTS}
		DEPENDS
			"${CMAKE_CURRENT_SOURCE_DIR}/${FILE}"
		MAIN_DEPENDENCY
			"${CMAKE_CURRENT_SOURCE_DIR}/${FILE}"
		COMMAND
			protobuf::protoc
				--proto_path "${CMAKE_CURRENT_SOURCE_DIR}"
				--cpp_out "${DST_DIR}"
				--dependency_out "${DST_DEP}"
				"${FILE}"
		DEPFILE
			"${DST_DEP}"
		VERBATIM
	)
endforeach()

add_library(IngnomiaProtocol
	STATIC
		${SOURCE_LIST}
)

target_link_libraries(IngnomiaProtocol
	PUBLIC
		protobuf::libprotobuf-lite
)

target_include_directories(
	IngnomiaProtocol
	PUBLIC
		"${DST_DIR}"
)

add_library(Ingnomia::Protocol ALIAS IngnomiaProtocol)
