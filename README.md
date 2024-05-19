## xscreenshot

Small utility to take screenshots with X.

Recording doesn't work yet.

### Libraries needed to build
1. libX11
2. libpng
3. libav(format/util/codec)
4. libswscale

### make commands
`make` builds everything (the output file is a.out)

`make clean` removes anything produced by the previous command

`make lint` lints the code

### Using the binary
open `main.c` and look at the first comment
