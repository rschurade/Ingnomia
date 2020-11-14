find_package(Qt5 REQUIRED QUIET COMPONENTS Core)
if(TARGET Qt5::qmake AND NOT TARGET Qt5::windeployqt)
	get_target_property(_qt5_qmake_location Qt5::qmake IMPORTED_LOCATION)

	execute_process(
		COMMAND "${_qt5_qmake_location}" -query QT_INSTALL_PREFIX
		RESULT_VARIABLE return_code
		OUTPUT_VARIABLE qt5_install_prefix
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	set(imported_location "${qt5_install_prefix}/bin/windeployqt.exe")

	if(EXISTS ${imported_location})
		add_executable(Qt5::windeployqt IMPORTED)

		set_target_properties(Qt5::windeployqt PROPERTIES
			IMPORTED_LOCATION ${imported_location}
		)
	endif()
endif()

function(windeployqt foo)
	if(TARGET Qt5::windeployqt)
		# execute windeployqt in a tmp directory after build
		add_custom_command(TARGET ${foo}
			POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/windeployqt_$<CONFIG>"
			COMMAND set PATH=%PATH%$<SEMICOLON>${qt5_install_prefix}/bin
			# for deployment
			COMMAND Qt5::windeployqt --pdb --no-translations --no-webkit2 --no-system-d3d-compiler --no-opengl-sw --dir "${CMAKE_CURRENT_BINARY_DIR}/windeployqt_$<CONFIG>" "$<TARGET_FILE_DIR:${foo}>/$<TARGET_FILE_NAME:${foo}>"
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