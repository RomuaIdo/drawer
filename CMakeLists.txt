cmake_minimum_required(VERSION 3.21)
project(drawer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Test)

# Everything except main() lives in a library so tests can link it.
file(GLOB_RECURSE DRAWER_SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.h)
list(FILTER DRAWER_SOURCES EXCLUDE REGEX "src/main\\.cpp$")

add_library(drawer_core STATIC ${DRAWER_SOURCES})
target_include_directories(drawer_core PUBLIC src)
target_link_libraries(drawer_core PUBLIC Qt6::Widgets Qt6::Sql)
target_compile_options(drawer_core PUBLIC -Wall -Wextra)

add_executable(drawer src/main.cpp)
target_link_libraries(drawer PRIVATE drawer_core)

enable_testing()
add_subdirectory(tests)
