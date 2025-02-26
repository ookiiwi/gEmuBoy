# gEmuBoy (ゲームボーイ)

As the name suggests, it is in fact another Game Boy emulator (written in C). 
Nothing fancy here, just a side project to get a bit more familiar with retro emulation stuff. 
The goal is also to get a more indepth understanding of the C language (CMake, ...). So please be aware, you may witness strange things ;)

Except for the PPU and some interrupt timing tests, most the common tests are successfully passed.
There is actually a CI executing Gekkio's tests (except for some). I also plan to automate PPU tests, once I got things right.

Right now, the PPU is the black beast. It kind of does the job and it passes dmg-acid2 but, there is too much misunderstanding. 
So I think I will just re-write it from scratch :)

## Dependencies

- [SDL2](https://www.libsdl.org/). 

## Build and Run

```sh
$ git clone --recurse-submodules -j8 https://github.com/ookiiwi/gEmuBoy.git
$ cd build
$ cmake ..
$ cmake --build .
$ ./gemuboy <PATH_TO_ROM>
```

## Acknowlegments

### Libraries

- [SDL2](https://www.libsdl.org/) is under the [Zlib license](https://github.com/libsdl-org/SDL/blob/main/LICENSE.txt)
- [argparse](https://github.com/cofyc/argparse) is under the [MIT license](https://github.com/cofyc/argparse/blob/master/LICENSE)

### Test suites

- [blargg-gb-tests](https://gbdev.gg8.se/files/roms/blargg-gb-tests/)
- [mooneye-test-suite](https://github.com/Gekkio/mooneye-test-suite/tree/main) is under the [MIT license](https://github.com/Gekkio/mooneye-test-suite/blob/main/LICENSE)
- [dmg-acid2](https://github.com/mattcurrie/dmg-acid2) is under the [MIT license](https://github.com/mattcurrie/dmg-acid2/blob/master/LICENSE)
- [mealy-bug-tearoom-tests](https://github.com/mattcurrie/mealybug-tearoom-tests) is under the [MIT license](https://github.com/mattcurrie/mealybug-tearoom-tests/blob/master/LICENSE)

### Documentations

- [Pandocs](https://gbdev.io/pandocs)
- [GBDEDG](https://hacktix.github.io/GBEDG)
- [gbctr](https://gekkio.fi/files/gb-docs/gbctr.pdf)
- [gbops](https://izik1.github.io/gbops/)
- [Nitty Gritty Gameboy Cycle Timing](http://blog.kevtris.org/blogfiles/Nitty%20Gritty%20Gameboy%20VRAM%20Timing.txt)

