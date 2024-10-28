include(${CMAKE_SOURCE_DIR}/cmake/CompilerWarnings.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Sanitizers.cmake)

option(ENABLE_SANITIZERS "Enable all available sanitizers" OFF)

message("ENABLE_SANITIZERS: ${ENABLE_SANITIZERS}")

add_library(sanitizers INTERFACE)

if (ENABLE_SANITIZERS)
    enable_sanitizers(sanitizers)
endif()

add_library(compiler_warnings INTERFACE)
set_compiler_warnings(compiler_warnings)

add_library(ProjectExtra INTERFACE)
target_link_libraries(ProjectExtra INTERFACE sanitizers compiler_warnings)
