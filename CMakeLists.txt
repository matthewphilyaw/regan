cmake_minimum_required(VERSION 4.0)
project(regan)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Raylib
FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG        6.0
        GIT_SHALLOW    TRUE
)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(raylib)

# Dear ImGui
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.92.8
        GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(imgui)

# rlImGui
FetchContent_Declare(
        rlimgui
        GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
        GIT_TAG        main
        GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(rlimgui)

FetchContent_Declare(
        imguizmo
        GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
        GIT_TAG        master
        GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imguizmo)

add_executable(regan
        src/main.cpp

        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${rlimgui_SOURCE_DIR}/rlImGui.cpp
        ${imguizmo_SOURCE_DIR}/src/ImGuizmo.cpp
        src/editor/editor.hpp
        src/editor/editor.cpp
        src/editor/editor.hpp
        src/engine/engine.cpp
        src/engine/engine.hpp
        src/menu/menu.cpp
        src/menu/menu.hpp
        src/editor/registry.hpp
        src/editor/entity.hpp
        src/common/managed_model.hpp
        src/common/managed_texture_2d.hpp
)

target_include_directories(regan PRIVATE
        src
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${rlimgui_SOURCE_DIR}
        ${imguizmo_SOURCE_DIR}/src
)

target_link_libraries(regan PRIVATE raylib)

if(APPLE)
    target_link_libraries(regan PRIVATE
            "-framework OpenGL"
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo"
    )
endif()