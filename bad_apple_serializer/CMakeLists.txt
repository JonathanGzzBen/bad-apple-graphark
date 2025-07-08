cmake_minimum_required(VERSION 3.31)

set(CMAKE_CXX_STANDARD 23)

add_executable(bad_apple_serializer main.cpp)

find_package( OpenCV REQUIRED )

if( OpenCV_FOUND )
    target_include_directories(bad_apple_serializer PRIVATE ${OpenCV_INCLUDE_DIRS} )
    target_link_libraries(bad_apple_serializer PRIVATE ${OpenCV_LIBS} )
endif()