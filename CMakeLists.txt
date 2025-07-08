cmake_minimum_required(VERSION 3.31)
project(bad_apple_graphark)

set(CMAKE_CXX_STANDARD 23)

add_executable(bad_apple_graphark main.cpp)

find_package( OpenCV REQUIRED )

if( OpenCV_FOUND )
    # Additional Include Directories
    target_include_directories(bad_apple_graphark PUBLIC ${OpenCV_INCLUDE_DIRS} )

    # Additional Dependencies
    target_link_libraries(bad_apple_graphark PUBLIC ${OpenCV_LIBS} )
endif()