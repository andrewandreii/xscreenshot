## xscreenshot

### Libraries
1. libX11
2. libpng

### make commands
`make` builds everything (the output file is a.out)

`make clean` removes anything produced by the previous command

`make lint` lints the code

### Using the binary
`prog xoffset yoffset width height [filename]`
- takes a screenshot of the specified area

`prog i [filename]`
- opens the screenshot before saving and lets you select the area you want to save
- in interactive mode you can exit by pressing any key, it will keep updating the coordinates when you click (ie. it swaps between updating x1 y1 to x2 y2 with every click)

`prog [filename]`
- save the full screenshot

if the filename is specified it saves it there,
else it saves it in the DEFAULT_FILENAME file

