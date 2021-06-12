file(
	GLOB_RECURSE SOURCE_LIST
	RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	"*.xml"
	"*.json"
	"*.glsl"
	"*.xaml"
	"*.ttf"
	"*.otf"
	"*.png"
	"*.sql"
	"*.wav"
	"*License*"
)

target_sources(${PROJECT_NAME}
	PRIVATE
		${SOURCE_LIST}
		${CMAKE_CURRENT_LIST_FILE}
)

foreach ( FILE ${SOURCE_LIST} )
    get_filename_component( DIR ${FILE} DIRECTORY )
    install(
		FILES ${FILE}
		DESTINATION "${CMAKE_INSTALL_BINDIR}/content/${DIR}"
	)
endforeach()
