cmake_minimum_required(VERSION 3.20)

project(oxidius CXX)

set(CMAKE_CXX_STANDARD 20)

add_compile_definitions(HOWLER_PREFIX="oxidius")

set(HOWLER_FMT_EXTERNAL TRUE)
add_subdirectory(thirdparty/fmt)
add_subdirectory(thirdparty/howler)

add_executable(oxidius source/exec/main.cpp)

target_link_libraries(oxidius fmt howler)

include_directories(thirdparty thirdparty/fmt/include)
