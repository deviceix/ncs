cmake_minimum_required(VERSION 3.29)
project(ncs LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# configurations
option(NCS_BUILD_TESTS "NCS Build Tests" ON)
option(NCS_BUILD_BENCHES "NCS Build Benches" ON)

# some status messages
message(STATUS "NCS Build Tests: ${NCS_BUILD_TESTS}")
message(STATUS "NCS Build Benches: ${NCS_BUILD_BENCHES}")

add_library(${PROJECT_NAME}
    lib/containers/archetypes.cpp
    lib/containers/column.cpp
    lib/world/world.cpp
    lib/utils.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
