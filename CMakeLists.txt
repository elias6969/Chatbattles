cmake_minimum_required(VERSION 3.10)
project(RaylibEngine LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --------------------------------------------------
# Sources
# --------------------------------------------------
file(GLOB_RECURSE ENGINE_SOURCES
    src/*.cpp
)

# --------------------------------------------------
# Raylib
# --------------------------------------------------
add_subdirectory(external/raylib)

# --------------------------------------------------
# rlImGui (ImGui for raylib)
# --------------------------------------------------
file(GLOB RLIMGUI_SOURCES
    external/rlImGui/*.cpp
)

add_library(rlimgui STATIC ${RLIMGUI_SOURCES})

target_include_directories(rlimgui PUBLIC
    external/rlImGui
    external/imgui-docking/imgui
)

target_link_libraries(rlimgui PUBLIC
    raylib
)

# --------------------------------------------------
# ImGui (Docking)
# --------------------------------------------------
file(GLOB IMGUI_SOURCES
    external/imgui-docking/imgui/*.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})

target_include_directories(imgui PUBLIC
    external/imgui-docking/imgui
)

# --------------------------------------------------
# Executable
# --------------------------------------------------
add_executable(RaylibEngine
    ${ENGINE_SOURCES}
)
set(USE_TLS TRUE)
set(IXWEBSOCKET_USE_TLS ON)
add_subdirectory(external/IXWebSocket)

add_subdirectory(external/json)
target_link_libraries(RaylibEngine PRIVATE nlohmann_json::nlohmann_json)   

find_package(CURL REQUIRED)
target_link_libraries(RaylibEngine PRIVATE CURL::libcurl)

target_link_libraries(RaylibEngine PRIVATE webp)

find_package(OpenSSL REQUIRED)

target_link_libraries(RaylibEngine
    PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
)

add_definitions(-DIXWEBSOCKET_USE_TLS)

target_include_directories(RaylibEngine PRIVATE
    include
    external
)

target_link_libraries(RaylibEngine PRIVATE
    raylib
    rlimgui
    imgui
    ixwebsocket
)

# --------------------------------------------------
# Platform specific (important for raylib)
# --------------------------------------------------
if (WIN32)
    target_link_libraries(RaylibEngine PRIVATE
        winmm
        gdi32
    )
elseif (UNIX)
    target_link_libraries(RaylibEngine PRIVATE
        m
        pthread
        dl
    )
endif()

# --------------------------------------------------
# Debug symbols
# --------------------------------------------------
target_compile_options(RaylibEngine PRIVATE
    $<$<CONFIG:Debug>:-g>
)
