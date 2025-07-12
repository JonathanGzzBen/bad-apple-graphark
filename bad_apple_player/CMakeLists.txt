cmake_minimum_required(VERSION 3.31)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)

FetchContent_Declare(
        graphark
        GIT_REPOSITORY https://github.com/JonathanGzzBen/graphark
        GIT_TAG fbf016649a2702324ad7247e2359d8d373a5b596
)

# ---- glfw ----
option(BAD_APPLE_PLAYER_FETCH_GLFW "Use FetchContent for glfw (overrides vcpkg)" OFF)
if (BAD_APPLE_PLAYER_FETCH_GLFW)
    message(STATUS "Using FetchContent for glfw")
    include(FetchContent)
    FetchContent_Declare(
            glfw
            GIT_REPOSITORY https://github.com/glfw/glfw
            GIT_TAG 7b6aead9fb88b3623e3b3725ebb42670cbe4c579 # 3.4.0
            FIND_PACKAGE_ARGS
    )
    FetchContent_MakeAvailable(glfw)
else ()
    message(STATUS "Using find_package(glfw3)")
    find_package(glfw3 REQUIRED)
endif ()

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# ---- glew ----
option(BAD_APPLE_PLAYER_FETCH_GLEW "Use FetchContent for glew (overrides vcpkg)" OFF)
if (BAD_APPLE_PLAYER_FETCH_GLEW)
    message(STATUS "Using FetchContent for glew")
    include(FetchContent)
    FetchContent_Declare(
            glew
            GIT_REPOSITORY https://github.com/Perlmint/glew-cmake
            GIT_TAG f456deace7b408655109aaeff71421ef2d3858c6 # 2.2.0
            FIND_PACKAGE_ARGS
    )
    FetchContent_MakeAvailable(glew)
else ()
    message(STATUS "Using find_package(glew)")
    find_package(glew REQUIRED)
endif ()

# ---- glm ----
option(BAD_APPLE_PLAYER_FETCH_GLM "Use FetchContent for glm (overrides vcpkg)" OFF)
if (BAD_APPLE_PLAYER_FETCH_GLM)
    message(STATUS "Using FetchContent for glm")
    include(FetchContent)
    FetchContent_Declare(
            glm
            GIT_REPOSITORY https://github.com/g-truc/glm
            GIT_TAG bf71a834948186f4097caa076cd2663c69a10e1e # refs/tags/1.0.1
    )
    FetchContent_MakeAvailable(glm)
else ()
    message(STATUS "Using find_package(glm)")
    find_package(glm REQUIRED)
endif ()

FetchContent_MakeAvailable(graphark)

#target_link_libraries(graphark graphark_lib glfw GLEW::GLEW glm::glm tl::expected CLI11::CLI11)
add_executable(bad_apple_player src/main.cpp)
target_link_libraries(bad_apple_player graphark_lib glfw GLEW::GLEW glm::glm)
set_target_properties(bad_apple_player PROPERTIES CMAKE_CXX_STANDARD 23)
