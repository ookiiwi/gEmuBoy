cmake_minimum_required(VERSION 3.6)

project(gEmuBoy LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL2 REQUIRED)

add_executable(${PROJECT_NAME}  src/main.c 
                                src/cpu.c 
                                src/ppu.c 
                                src/mmu.c 
                                src/gb.c 
                                src/lcd.c 
                                src/debugger.c 
                                src/log.c
                                src/interrupt.c
                                src/timer.c
                                src/win_utils.c 
								src/cartridge.c )
target_include_directories(${PROJECT_NAME} PRIVATE include/)
#target_compile_definitions(${PROJECT_NAME} PUBLIC ENABLE_GAMEBOY_DOCTOR_SETUP)
#target_compile_definitions(${PROJECT_NAME} PUBLIC NO_PPU)
target_compile_definitions(${PROJECT_NAME} PUBLIC DEBUG_LOG_DIR="${CMAKE_SOURCE_DIR}/logs/")
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2)
