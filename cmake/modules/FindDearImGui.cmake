include(FetchContent)

FetchContent_Declare(
        DearImGui
        GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
        GIT_TAG "cb8ead1f7198924f97e52973d115e1d4eaeda2f3") # Docking support
FetchContent_MakeAvailable(DearImGui)

add_library(DearImGui
        STATIC EXCLUDE_FROM_ALL
        ${dearimgui_SOURCE_DIR}/imgui.h
        ${dearimgui_SOURCE_DIR}/imgui.cpp
        ${dearimgui_SOURCE_DIR}/imgui_demo.cpp
        ${dearimgui_SOURCE_DIR}/imgui_draw.cpp
        ${dearimgui_SOURCE_DIR}/imgui_tables.cpp
        ${dearimgui_SOURCE_DIR}/imgui_widgets.cpp
        )

if (NOT DEFINED IMGUI_EXTRA_CONFIG)
    message(FATAL_ERROR "Please define IMGUI_EXTRA_CONFIG with the path to the custom 'imconfig.h' file")
else()
    target_compile_definitions(DearImGui PUBLIC IMGUI_USER_CONFIG="${IMGUI_EXTRA_CONFIG}")
endif()

target_include_directories(DearImGui PUBLIC ${dearimgui_SOURCE_DIR})

add_library(DearImGui::DearImGui ALIAS DearImGui)