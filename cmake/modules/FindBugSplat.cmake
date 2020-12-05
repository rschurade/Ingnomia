set(BUGSPLAT_SDK_ROOT "" CACHE PATH "Root to the bugsplat sdk")

find_path(BUGSPLAT_INCLUDE_DIR
	NAMES
		BugSplat.h
	HINTS
		${BUGSPLAT_SDK_ROOT}/inc
	REQUIRED
)

find_library(BUGSPLAT_LIBRARY
	NAMES
		BugSplat64
	HINTS
		${BUGSPLAT_SDK_ROOT}/lib64
	REQUIRED
)

find_file(BUGSPLAT_DLL
	NAMES
		BugSplat64.dll
		BugSplat64Rc.dll
		BsSndRpt64.exe
	HINTS
		${BUGSPLAT_SDK_ROOT}/bin64
	REQUIRED
)


include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set BUGSPLAT_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(BugSplat
	DEFAULT_MSG
	BUGSPLAT_LIBRARY BUGSPLAT_INCLUDE_DIR BUGSPLAT_DLL)

mark_as_advanced(BUGSPLAT_INCLUDE_DIR BUGSPLAT_LIBRARY BUGSPLAT_DLL)

if(BUGSPLAT_FOUND AND NOT TARGET BugSplat)
	add_library(BugSplat SHARED IMPORTED)
	set_target_properties(BugSplat
		PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${BUGSPLAT_INCLUDE_DIR}"
			IMPORTED_LOCATION "${BUGSPLAT_DLL}"
			IMPORTED_IMPLIB "${BUGSPLAT_LIBRARY}")
	target_add_definitions(-DHAVE_BUGSPLAT)
endif()