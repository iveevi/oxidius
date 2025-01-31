cmake_minimum_required(VERSION 3.20)

project(oxidius CXX)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

# FMT for printing and string manipulation
FetchContent_Declare(fmt
	GIT_REPOSITORY https://github.com/fmtlib/fmt.git
	GIT_TAG master)
FetchContent_MakeAvailable(fmt)

# Howler for consistent logging
FetchContent_Declare(howler
	GIT_REPOSITORY https://github.com/iveevi/howler.git
	GIT_TAG main)
FetchContent_MakeAvailable(howler)

add_executable(oxidius source/exec/main.cpp)

target_link_libraries(oxidius fmt howler)

include_directories(oxidius
	source
	thirdparty)