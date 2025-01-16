set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(testboy LANGUAGES C)

include(CTest)
enable_testing()

find_package(OpenMP COMPONENTS C REQUIRED)

add_executable(${PROJECT_NAME} src/testboy.c src/custom_io.c)
target_include_directories(${PROJECT_NAME} PRIVATE include/)
target_compile_definitions(${PROJECT_NAME} PUBLIC GB_MAIN_PRGM_DIR="${CMAKE_SOURCE_DIR}/build/")
target_compile_definitions(${PROJECT_NAME} PUBLIC TEST_BOY_DIR="${PROJECT_SOURCE_DIR}")
target_link_libraries(${PROJECT_NAME} PRIVATE ProjectExtra OpenMP::OpenMP_C)


string(CONCAT EXLUDED_TEST_ROMS 
        "/utils/|"
        "/misc/|"
        "/madness/|"
        "/manual-only/|"
        "/mbc2/|"
        "/mbc5/|"
        "/ppu/|"
        "call_timing2.gb|"
        "call_cc_timing2.gb|"
        "halt_ime0_nointr_timing.gb|"
        "if_ie_registers.gb|"
        "intr_timing.gb|"
        "push_timing.gb|"
        "rapid_di_ei.gb|"
        "ret_cc_timing.gb|"
        "rst_timing.gb|"
        "boot_div-S.gb|"
        "boot_div-dmg0.gb|"
        "boot_div-dmgABCmgb.gb|"
        "boot_div2-S.gb|"
        "boot_hwio-S.gb|"
        "boot_hwio-dmg0.gb|"
        "boot_hwio-dmgABCmgb.gb|"
        "boot_regs-dmg0.gb|"
        "boot_regs-dmgABC.gb|"
        "boot_regs-mgb.gb|"
        "boot_regs-sgb.gb|"
        "boot_regs-sgb2.gb|"
        "di_timing-GS.gb|"
        "halt_ime1_timing2-GS.gb|"
        "bits/unused_hwio-GS.gb|"
        "interrupts/ie_push.gb|"
        "oam_dma/sources-GS.gb|"
        "/serial/|"
        "timer/tima_reload.gb|"
        "timer/tima_write_reloading.gb|"
        "timer/tma_write_reloading.gb|"
        "mbc1/multicart_rom_8Mb.gb"
)

file(GLOB_RECURSE TEST_ROMS ${PROJECT_SOURCE_DIR}/**/*.gb)
list(FILTER TEST_ROMS EXCLUDE REGEX "${EXLUDED_TEST_ROMS}") 


foreach (path ${TEST_ROMS})
    string(REGEX MATCH "[^\/]+\.gb$" TMP ${path})
    string(REGEX REPLACE "\.gb" "" TEST_NAME ${TMP})
    add_test(NAME "${PROJECT_NAME}_${TEST_NAME}" COMMAND ${PROJECT_NAME} ${path} 1)
    set_tests_properties("${PROJECT_NAME}_${TEST_NAME}" PROPERTIES TIMEOUT 20)
endforeach()

#add_test(NAME "${PROJECT_NAME}_valgrind" 
#            COMMAND valgrind
#                --error-exitcode=1
#                --track-origins=yes
#                --leak-check=full
#                --show-leak-kinds=all
#                $<TARGET_FILE:${PROJECT_NAME}>)

