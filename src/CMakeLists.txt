add_library(minitool_core
    common/source_manager.cpp
    diagnostics/diagnostic.cpp
    diagnostics/diagnostic_engine.cpp
    isa/registers.cpp
    isa/opcode.cpp
    isa/encoding.cpp)
add_library(minitool::core ALIAS minitool_core)

target_link_libraries(minitool_core PUBLIC minitool_options)
target_include_directories(minitool_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>)

add_executable(minitool cli/main.cpp)
target_link_libraries(minitool PRIVATE minitool::core)
