set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(testboy LANGUAGES C)

find_package(OpenMP COMPONENTS C REQUIRED)

add_executable(${PROJECT_NAME} src/main.c src/custom_io.c)
target_include_directories(${PROJECT_NAME} PRIVATE include/)
target_compile_definitions(${PROJECT_NAME} PUBLIC GB_MAIN_PRGM_DIR="${CMAKE_SOURCE_DIR}/build/")
target_compile_definitions(${PROJECT_NAME} PUBLIC TEST_BOY_DIR="${PROJECT_SOURCE_DIR}")
target_link_libraries(${PROJECT_NAME} PRIVATE ProjectExtra OpenMP::OpenMP_C)