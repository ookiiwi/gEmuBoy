set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(testboy LANGUAGES C)

include(CTest)
enable_testing()

find_program(VALGRIND_FOUND "valgrind")
find_program(TIMEOUT_FOUND  "timeout")

if (VALGRIND_FOUND AND TIMEOUT_FOUND)
    message("Valgrind tests enabled")
else()
    message("Valgrind tests disabled")
endif()

set(GEMUBOY_PRGM_PATH "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>")

function (add_gb_test test_roms prefix test_suite_id)
    foreach (path ${test_roms})
        string(REGEX MATCH "[^\/]+\/[^\/]+\.gb$" RELATIVE_PATH ${path})
        string(REGEX REPLACE "\/" "_" ROM_NAME_GB ${RELATIVE_PATH})
        string(REGEX REPLACE "\.gb" "" ROM_NAME ${ROM_NAME_GB})

        set(TEST_NAME "${PROJECT_NAME}_${prefix}_${ROM_NAME}")
        set(TEST_ARGS ${path} ${test_suite_id})

        add_test(NAME ${TEST_NAME} COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/testboy.sh" ${TEST_ARGS} "${GEMUBOY_PRGM_PATH}") 
        set_tests_properties(${TEST_NAME} PROPERTIES TIMEOUT 40 TIMEOUT_SIGNAL_NAME SIGTERM)

        if (VALGRIND_FOUND AND TIMEOUT_FOUND)
            set(VALGRIND_TEST_NAME "${CMAKE_PROJECT_NAME}_${prefix}_${ROM_NAME}_valgrind")
            add_test(NAME ${VALGRIND_TEST_NAME} 
                        COMMAND timeout --preserve-status 10 valgrind
                            --error-exitcode=1
                            --track-origins=yes
                            --leak-check=full
                            "${GEMUBOY_PRGM_PATH}" ${path} "-l")

            set_tests_properties(${VALGRIND_TEST_NAME} PROPERTIES TIMEOUT 40 TIMEOUT_SIGNAL_NAME SIGTERM)
        endif()
    endforeach()
endfunction()

string(CONCAT EXLUDED_MOONEYE_TEST_ROMS 
        "/utils/|"
        "/misc/|"
        "/madness/|"
        "/manual-only/|"
        "/ppu/|"
        "if_ie_registers.gb|" # Note: VBLANK interrupt seems faulty. PPU's timing is surely to be revised
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
        "mbc1/multicart_rom_8Mb.gb"
)

file(GLOB_RECURSE MOONEYE_TEST_ROMS ${PROJECT_SOURCE_DIR}/**/mooneye-gb-test-roms/*.gb)
list(FILTER MOONEYE_TEST_ROMS EXCLUDE REGEX "${EXLUDED_MOONEYE_TEST_ROMS}") 

add_gb_test("${MOONEYE_TEST_ROMS}" mooneye 1)

