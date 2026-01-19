cmake_minimum_required(VERSION 3.16)

project(test_wikipedia_parser VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core)

add_executable(test_app
    test_app.cpp
    src/sources/wikipediasource.cpp
)

target_compile_definitions(test_app PRIVATE)

target_link_libraries(test_app PRIVATE Qt6::Core)

include_directories(include)
target_include_directories(test_app PRIVATE src)
