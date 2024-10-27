include(${CMAKE_SOURCE_DIR}/cmake/CompilerWarnings.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Sanitizers.cmake)

add_library(sanitizers INTERFACE)
enable_sanitizers(sanitizers)

add_library(compiler_warnings INTERFACE)
set_compiler_warnings(compiler_warnings)

add_library(ProjectExtra INTERFACE)
target_link_libraries(ProjectExtra INTERFACE sanitizers compiler_warnings)
