set(STEAM_SDK_ROOT "" CACHE PATH "Root to the steam sdk")

find_path(STEAM_INCLUDE_DIR
	NAMES
		steam/steam_api.h
	HINTS
		${STEAM_SDK_ROOT}/public
)

if(UNIX)
	find_library(STEAM_LIBRARY
		NAMES
			steam_api
		HINTS
			${STEAM_SDK_ROOT}/redistributable_bin/linux64
	)
	set(STEAM_DLL ${STEAM_LIBRARY})
else()
	find_library(STEAM_LIBRARY
		NAMES
			steam_api64
		HINTS
			${STEAM_SDK_ROOT}/redistributable_bin/win64
	)
	find_file(STEAM_DLL
		NAMES
			steam_api64.dll
		HINTS
			${STEAM_SDK_ROOT}/redistributable_bin/win64
	)
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set STEAM_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(Steam
	DEFAULT_MSG
	STEAM_LIBRARY STEAM_INCLUDE_DIR STEAM_DLL)

mark_as_advanced(STEAM_INCLUDE_DIR STEAM_LIBRARY STEAM_DLL)

if(STEAM_FOUND AND NOT TARGET Steam)
	add_library(Steam SHARED IMPORTED)
	set_target_properties(Steam
		PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${STEAM_INCLUDE_DIR}"
			IMPORTED_LOCATION "${STEAM_DLL}"
			IMPORTED_IMPLIB "${STEAM_LIBRARY}")
endif()