add_library(glad STATIC
    src/gl.c
)

target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

find_package(OpenGL REQUIRED)
target_link_libraries(glad PRIVATE OpenGL::GL)

set_target_properties(glad PROPERTIES FOLDER 3rdParty)
