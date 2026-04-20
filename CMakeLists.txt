cmake_minimum_required(VERSION 3.20)

project(PngMetadataReader VERSION 1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(pmr src/main.cpp)
find_package(ZLIB REQUIRED)
target_link_libraries(pmr PRIVATE ZLIB::ZLIB)

target_include_directories(pmr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_features(pmr PRIVATE cxx_std_23)

target_compile_options(pmr PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -O3
)
