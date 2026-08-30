include(FetchContent)

# Prefer an installed GoogleTest; fall back to a pinned source download so the
# build works on a clean machine. Marked SYSTEM so third-party headers cannot
# trip our -Werror settings.
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
        URL_HASH SHA256=7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        SYSTEM)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

include(GoogleTest)

function(minitool_add_test name)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE minitool::core GTest::gtest GTest::gtest_main)
    gtest_discover_tests(${name} DISCOVERY_TIMEOUT 60)
endfunction()

minitool_add_test(test_common
    unit/test_byte_order.cpp
    unit/test_source_manager.cpp)

minitool_add_test(test_diagnostics
    unit/test_diagnostics.cpp)

minitool_add_test(test_isa
    unit/test_registers.cpp
    unit/test_opcode_table.cpp
    unit/test_encoding.cpp)

minitool_add_test(test_properties
    property/test_encoding_properties.cpp)
