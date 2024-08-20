cmake_minimum_required(VERSION 3.1)

project(gEmuBoy LANGUAGES C)

add_executable(${PROJECT_NAME} src/main.c src/load.c)
target_include_directories(${PROJECT_NAME} PRIVATE include/)
target_compile_definitions(${PROJECT_NAME} PUBLIC ENABLE_GAMEBOY_DOCTOR_SETUP)
target_compile_definitions(${PROJECT_NAME} PUBLIC DEBUG_LOG_DIR="${CMAKE_SOURCE_DIR}/logs/")