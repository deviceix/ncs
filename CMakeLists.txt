cmake_minimum_required(VERSION 3.29)
project(ncs LANGUAGES C CXX)

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

if (NCS_BUILD_TESTS)
    enable_testing()

    find_package(GTest REQUIRED)

    set(NCS_TEST ${PROJECT_NAME}-test)
    set(SANITIZER_FLAGS
        "-fsanitize=address,undefined"
        "-fno-omit-frame-pointer"
        "-fno-sanitize-recover=all"
    )

    add_executable(${NCS_TEST}
        tests/crud.cpp
        tests/lifecycle.cpp
        tests/query.cpp
        tests/column.cpp
    )

    target_include_directories(${NCS_TEST} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_compile_options(${NCS_TEST} PRIVATE 
        ${SANITIZER_FLAGS}
    )

    target_link_options(${NCS_TEST} PRIVATE
        ${SANITIZER_FLAGS}
    )
    
    target_link_libraries(${NCS_TEST} PRIVATE
        ${PROJECT_NAME}
        GTest::GTest
        GTest::Main
    )

    include(GoogleTest)
    gtest_discover_tests(${NCS_TEST})
endif()

if (NCS_BUILD_BENCHES)
    find_package(Benchmark REQUIRED)
endif()
