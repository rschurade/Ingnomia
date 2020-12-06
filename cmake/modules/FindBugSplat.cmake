set(BUGSPLAT_SDK_ROOT "" CACHE PATH "Root to the bugsplat sdk")

find_path(BUGSPLAT_INCLUDE_DIR
	NAMES
		BugSplat.h
	HINTS
		${BUGSPLAT_SDK_ROOT}/inc
)

find_library(BUGSPLAT_LIBRARY
	NAMES
		BugSplat64
	HINTS
		${BUGSPLAT_SDK_ROOT}/lib64
)

find_file(BUGSPLAT_DLL
	NAMES
		BugSplat64.dll
	HINTS
		${BUGSPLAT_SDK_ROOT}/bin64
)

find_file(BUGSPLAT_RC_DLL
	NAMES
		BugSplatRc64.dll
	HINTS
		${BUGSPLAT_SDK_ROOT}/bin64
)

find_file(BUGSPLAT_SEND_REPORT
	NAMES
		BsSndRpt64.exe
	HINTS
		${BUGSPLAT_SDK_ROOT}/bin64
)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set BUGSPLAT_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(BugSplat
	DEFAULT_MSG
	BUGSPLAT_LIBRARY BUGSPLAT_INCLUDE_DIR BUGSPLAT_DLL BUGSPLAT_RC_DLL BUGSPLAT_SEND_REPORT)

mark_as_advanced(BUGSPLAT_INCLUDE_DIR BUGSPLAT_LIBRARY BUGSPLAT_DLL BUGSPLAT_RC_DLL BUGSPLAT_SEND_REPORT)

if(BUGSPLAT_FOUND AND NOT TARGET BugSplat)
	add_executable(BugSplatSendReport IMPORTED)
	set_target_properties(BugSplatSendReport
		PROPERTIES
			IMPORTED_LOCATION
				"${BUGSPLAT_SEND_REPORT}"
	)
	add_library(BugSplatRc SHARED IMPORTED)
	set_target_properties(BugSplatRc
		PROPERTIES
			IMPORTED_LOCATION
				"${BUGSPLAT_RC_DLL}"
	)
	add_library(BugSplat SHARED IMPORTED)
	set_target_properties(BugSplat
		PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES
				"${BUGSPLAT_INCLUDE_DIR}"
			IMPORTED_LINK_DEPENDENT_LIBRARIES
				BugSplatRc
			IMPORTED_LOCATION
				"${BUGSPLAT_DLL}"
			IMPORTED_IMPLIB
				"${BUGSPLAT_LIBRARY}"
	)
	target_compile_definitions(
		BugSplat
		INTERFACE
			HAVE_BUGSPLAT=1
	)
endif()
