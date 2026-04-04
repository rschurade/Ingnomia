find_package(Qt6 REQUIRED QUIET COMPONENTS Core)
if(TARGET Qt6::qmake AND NOT TARGET Qt6::windeployqt)
	get_target_property(_qt6_qmake_location Qt6::qmake IMPORTED_LOCATION)

	execute_process(
		COMMAND "${_qt6_qmake_location}" -query QT_INSTALL_PREFIX
		RESULT_VARIABLE return_code
		OUTPUT_VARIABLE qt6_install_prefix
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	set(imported_location "${qt6_install_prefix}/bin/windeployqt6.exe")

	if(EXISTS ${imported_location})
		add_executable(Qt6::windeployqt IMPORTED)

		set_target_properties(Qt6::windeployqt PROPERTIES
			IMPORTED_LOCATION ${imported_location}
		)
	endif()
endif()

function(windeployqt foo)
	if(TARGET Qt6::windeployqt)
		# execute windeployqt in a tmp directory after build
		add_custom_command(TARGET ${foo}
			POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/windeployqt_$<CONFIG>"
			COMMAND set PATH=%PATH%$<SEMICOLON>${qt6_install_prefix}/bin
			# for deployment
			COMMAND Qt6::windeployqt --no-translations --no-system-d3d-compiler --no-opengl-sw --dir "${CMAKE_CURRENT_BINARY_DIR}/windeployqt_$<CONFIG>" "$<TARGET_FILE_DIR:${foo}>/$<TARGET_FILE_NAME:${foo}>"
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
