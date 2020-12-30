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

// its better if this file doesn't include utils.h, since utils.h includes main.h
/* void interactive(Display *dpy, Window root, XImage *img, rect_t *rect); */

#endif
