file(GLOB_RECURSE SOURCE_LIST
	"*.xml"
	"*.json"
	"*.glsl"
	"*.xaml"
	"*.otf"
	"*.png"
	"*.db"
)

target_sources(${PROJECT_NAME}
	PRIVATE
		${SOURCE_LIST}
)
