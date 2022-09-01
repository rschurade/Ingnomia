file(
	GLOB_RECURSE SOURCE_LIST
	RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	"*.xml"
	"*.json"
	"*.xaml"
	"*.ttf"
	"*.otf"
	"*.png"
	"*.sql"
	"*.textproto"
	"*.wav"
	"*License*"
)

foreach ( FILE ${SOURCE_LIST} )
    get_filename_component( DIR ${FILE} DIRECTORY )
    install(
		FILES ${FILE}
		DESTINATION "${CMAKE_INSTALL_BINDIR}/content/${DIR}"
	)
endforeach()

add_custom_target(IngnomiaContent
	SOURCES
		${SOURCE_LIST}
		${CMAKE_CURRENT_LIST_FILE}
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCE_LIST})
