cmake_minimum_required(VERSION 3.6)

project(gEmuBoy LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL2 REQUIRED)

include(cmake/ProjectExtra.cmake)

add_executable(${PROJECT_NAME}  src/main.c
                                src/cartridge/cartridge.c
                                src/cartridge/mbc.c
                                src/cpu/cpu.c
                                src/cpu/timer.c
                                src/cpu/interrupt.c
                                src/graphics/ppu.c
                                src/graphics/lcd.c
                                src/win_utils.c
                                src/gb.c
                                src/mmu.c
                                src/log.c )
target_include_directories(${PROJECT_NAME} PRIVATE include/)
target_compile_definitions(${PROJECT_NAME} PUBLIC DEBUG_LOG_DIR="${CMAKE_SOURCE_DIR}/logs/")
target_link_libraries(${PROJECT_NAME} PRIVATE ProjectExtra SDL2::SDL2)
