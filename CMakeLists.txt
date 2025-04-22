cmake_minimum_required(VERSION 3.29)
project(ncs LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# configurations
option(NCS_BUILD_TESTS "NCS Build Tests" ON)
option(NCS_BUILD_BENCHES "NCS Build Benches" ON)
option(NCS_ASAN_ENABLE "Enable Address Sanitizer" ON)
option(NCS_TSAN_ENABLE "Enable Thread Sanitizer" OFF)
option(NCS_UBSAN_ENABLE "Enable Undefined Behavior Sanitizer" ON)

# some status messages
message(STATUS "NCS Build Tests: ${NCS_BUILD_TESTS}")
message(STATUS "NCS Build Benches: ${NCS_BUILD_BENCHES}")
message(STATUS "Enable Address Sanitizer: ${NCS_ASAN_ENABLE}")
message(STATUS "Enable Thread Sanitizer: ${NCS_TSAN_ENABLE}")
message(STATUS "Enable Undefined Behavior Sanitizer: ${NCS_UBSAN_ENABLE}")

set(SANITIZER_FLAGS "")

if(NCS_ASAN_ENABLE)
    list(APPEND SANITIZER_FLAGS "-fsanitize=address")
endif()

if(NCS_TSAN_ENABLE)
    list(APPEND SANITIZER_FLAGS "-fsanitize=thread")
endif()

if(NCS_UBSAN_ENABLE)
    list(APPEND SANITIZER_FLAGS "-fsanitize=undefined")
endif()

if(SANITIZER_FLAGS)
    list(APPEND SANITIZER_FLAGS
            "-fno-omit-frame-pointer"
            "-fno-sanitize-recover=all"
            "-g"
    )
endif()

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

    add_executable(${NCS_TEST}
            tests/crud.cpp
            tests/lifecycle.cpp
            tests/query.cpp
    )

    target_include_directories(${NCS_TEST} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    if(SANITIZER_FLAGS)
        target_compile_options(${NCS_TEST} PRIVATE ${SANITIZER_FLAGS})
        target_link_options(${NCS_TEST} PRIVATE ${SANITIZER_FLAGS})
    endif()

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
