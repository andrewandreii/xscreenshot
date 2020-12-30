#ifndef XSCRSHOT_H
#define XSCRSHOT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* #include <X11/Xlib.h> */ // already incldued by Xutil.h
#include <X11/cursorfont.h>
#include <X11/Xutil.h>

#include <png.h>

#define DEFAULT_FILENAME "test.png"
#define SUCCESS 0
#define ERROR 1
#define printe(message) fprintf(stderr, message); exit(ERROR)
#define checknull(var, message) if (var == NULL) { printe(message); }

#define rect_width(r) ((r).x2 - (r).x1)
#define rect_height(r) ((r).y2 - (r).y1)

typedef struct {
	int x1, y1, x2, y2;
} rect_t;



// save the portion defined by rect from the image img in the file f
void write_ximage_to_file( \
		XImage *img, rect_t *recy, FILE *f, \
		png_structp png_ptr, png_infop info_ptr);

#endif
