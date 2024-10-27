#include "test_exec.h"

#include <omp.h>
#include <string.h>

#define GEKKIO_SUCCESS_SEQ "358132134"
#define GEKKIO_FAILURE_SEQ "666666666666"

static const char *test_rv[] = { "FAILURE", "SUCCESS", "TIMEOUT", "ERROR" };

#define MOONEYE_TEST_DIR TEST_BOY_DIR "/mooneye-gb-test-roms/"

#define TEST_AREA()                                             \
    _Pragma("omp parallel default(none) shared(test_rv)")       \
    _Pragma("omp single")

#define MOONEYE_TEST(test, timeout, skip) do {                                                  \
    if (skip) break;                                                                            \
    const size_t test_path_len = strlen(MOONEYE_TEST_DIR) + strlen(test) + 1;                   \
    char test_path[1024];                                                                       \
    test_path[test_path_len-1] = 0;                                                             \
    snprintf(test_path, test_path_len, "%s%s", MOONEYE_TEST_DIR, (test));                       \
    _Pragma("omp task")                                                                         \
    {                                                                                           \
        int rv = test_exec(test_path, GEKKIO_SUCCESS_SEQ, GEKKIO_FAILURE_SEQ, timeout);         \
        printf("%s ==> %s\n", (test), test_rv[rv]);                                             \
    }                                                                                           \
} while(0)

int main(int argc, char **argv) {
    TEST_AREA() {
        MOONEYE_TEST("emulator-only/mbc1/bits_bank1.gb",  20, 0);
        MOONEYE_TEST("emulator-only/mbc1/bits_bank2.gb",  20, 0);
        MOONEYE_TEST("emulator-only/mbc1/bits_mode.gb",   20, 0);
        MOONEYE_TEST("emulator-only/mbc1/bits_ramg.gb",   20, 0);
        MOONEYE_TEST("emulator-only/mbc1/rom_512kb.gb",   5, 0);
        MOONEYE_TEST("emulator-only/mbc1/rom_1Mb.gb",     5, 0);
        MOONEYE_TEST("emulator-only/mbc1/rom_2Mb.gb",     5, 0);
        MOONEYE_TEST("emulator-only/mbc1/rom_4Mb.gb",     5, 0);
        MOONEYE_TEST("emulator-only/mbc1/rom_8Mb.gb",     5, 0);
        MOONEYE_TEST("emulator-only/mbc1/rom_16Mb.gb",    5, 0);
        MOONEYE_TEST("emulator-only/mbc1/ram_64kb.gb",    5, 0);
        MOONEYE_TEST("emulator-only/mbc1/ram_256kb.gb",   5, 0);
        MOONEYE_TEST("emulator-only/mbc1/multicart_rom_8Mb.gb", 5, 1);
    }

    return 0;
}
