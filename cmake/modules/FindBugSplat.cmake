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

find_file(BUGSPLAT_DLL1
	NAMES
		BugSplat64.dll
	HINTS
		${BUGSPLAT_SDK_ROOT}/bin64
)

find_file(BUGSPLAT_DLL2
	NAMES
		BugSplatRc64.dll
	HINTS
		${BUGSPLAT_SDK_ROOT}/bin64
)

find_file(BUGSPLAT_DLL3
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
	BUGSPLAT_LIBRARY BUGSPLAT_INCLUDE_DIR BUGSPLAT_DLL1 BUGSPLAT_DLL2 BUGSPLAT_DLL3)

mark_as_advanced(BUGSPLAT_INCLUDE_DIR BUGSPLAT_LIBRARY BUGSPLAT_DLL1 BUGSPLAT_DLL2 BUGSPLAT_DLL3)

if(BUGSPLAT_FOUND AND NOT TARGET BugSplat)
	add_library(BugSplat SHARED IMPORTED)
	set_target_properties(BugSplat
		PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${BUGSPLAT_INCLUDE_DIR}"
			IMPORTED_LOCATIONS "${BUGSPLAT_DLL1} ${BUGSPLAT_DLL2} ${BUGSPLAT_DLL3}"
			IMPORTED_IMPLIB "${BUGSPLAT_LIBRARY}")
	add_definitions(-DHAVE_BUGSPLAT)
endif()