set(NOESIS_ROOT "" CACHE PATH "Root to the NoesisGui sdk")

find_path(NOESIS_INCLUDE_DIR
	NAMES
		NoesisPCH.h
	HINTS
		${NOESIS_ROOT}/Include
)

find_library(NOESIS_LIBRARY
	NAMES
		libNoesis Noesis
	HINTS
		${NOESIS_ROOT}/Bin/linux_x86_64
		${NOESIS_ROOT}/Lib/windows_x86_64
)

if(WIN32)
	find_file(NOESIS_DLL
		NAMES
			Noesis.dll
		HINTS
			${NOESIS_ROOT}/Bin/windows_x86_64
	)
else()
	set(NOESIS_DLL ${NOESIS_LIBRARY})
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set NOESIS_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(Noesis
	DEFAULT_MSG
	NOESIS_LIBRARY NOESIS_INCLUDE_DIR NOESIS_DLL)

mark_as_advanced(NOESIS_INCLUDE_DIR NOESIS_LIBRARY NOESIS_DLL)

if(NOESIS_FOUND AND NOT TARGET Noesis)
	add_library(Noesis SHARED IMPORTED)
	set_target_properties(Noesis
		PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${NOESIS_INCLUDE_DIR}"
			IMPORTED_LOCATION "${NOESIS_DLL}"
			IMPORTED_IMPLIB "${NOESIS_LIBRARY}"
	)
endif()