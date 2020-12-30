#include "main.h"
#include "utils.h"

/*
 * Usage:
 *		prog xoffset yoffset width height [filename]
 *			- takes a screenshot of the specified area
 *		prog i [filename]
 *			- opens the screenshot before saving and lets you
 *			  select the area you want to save
 *			- in interactive mode you can exit by pressing any key,
 *			  it will keep updating the coordinates when you click
 *			  (ie. it swaps between updating x1 y1 to x2 y2 with every click)
 *		prog [filename]
 *			- save the full screenshot
 *
 *		if the filename is specified it saves it there,
 *		else it saves it in the DEFAULT_FILENAME file
 */

void
interactive (Display *dpy, Window root, XImage *img, rect_t *rect) {
	printf("Interactive mode\n");

	Window win = XCreateSimpleWindow(dpy, root, 0, 0, rect_width(*rect), rect_height(*rect), 0, 1, 1);

	GC gc = XCreateGC(dpy, win, 0, NULL);

	Cursor cur = XCreateFontCursor(dpy, XC_tcross);
	XDefineCursor(dpy, win, cur);
	XFreeCursor(dpy, cur);

	XSelectInput(dpy, win, KeyPressMask | ButtonPressMask | StructureNotifyMask);

	XEvent ev;
	XMapWindow(dpy, win);
	for (;;) {
		XNextEvent(dpy, &ev);
		if (ev.type == MapNotify) {
			break;
		}
	}

	// TODO: try to fullscreen the window and if it doesn't work
	// downscale the image
	XPutImage(dpy, win, gc, img, 0, 0, 0, 0, rect_width(*rect), rect_height(*rect));

	int first = 1, x, y;
	for (;;) {
		XNextEvent(dpy, &ev);
		if (ev.type == ButtonPress) {
			x = ev.xbutton.x;
			y = ev.xbutton.y;
			if (first) {
				rect->x1 = x;
				rect->y1 = y;
				first = 0;
			} else {
				rect->x2 = x;
				rect->y2 = y;
				first = 1;
			}
		} else if (ev.type == KeyPress) {
			break;
		}
	}

	XDestroyWindow(dpy, win);
	XFreeGC(dpy, gc);
}

int
main (int argc, char *argv[]) {
	Display *dpy = XOpenDisplay(NULL);
	Window root = DefaultRootWindow(dpy);
	int screen = DefaultScreen(dpy);

	// default values
	rect_t rect;
	rect.x1 = rect.y1 = 0;
	rect.x2 = XDisplayWidth(dpy, screen);
	rect.y2 = XDisplayHeight(dpy, screen);
	char *filename = DEFAULT_FILENAME;

	XImage *preimg = XGetImage(dpy, root, 0, 0, rect_width(rect), rect_height(rect), AllPlanes, ZPixmap);
	checknull(preimg, "Cannot get image from window");

	// TODO: rewrite the argument parsing
	XImage *img; // final image
	if (argc >= 5) {
		img = XSubImage(preimg, \
				atoi(argv[1]), atoi(argv[2]), \
				atoi(argv[3]), atoi(argv[4]) \
			);

		if (argc > 5) {
			filename = argv[5];
		}

	} else if (argc == 2 || argc == 3) {
		if (strcmp(argv[1], "i") == 0) {
			interactive(dpy, root, preimg, &rect);

			img = XSubImage(preimg, \
					rect.x1, rect.y1, \
					abs(rect_width(rect)), abs(rect_height(rect)) \
				);

			if (argc == 3) {
				filename = argv[2];
			}
		} else {
			filename = argv[1];
			img = XSubImage(preimg, 0, 0, preimg->width, preimg->height);
		}
	} else {
		img = XSubImage(preimg, 0, 0, preimg->width, preimg->height);
	}
	printf("Reactangle: (%d %d)\n", img->width, img->height);

	ximage_inherit_masks(img, preimg); // thanks xlib

	XDestroyImage(preimg);

	png_context_t *ctx = make_png_context();

	// write to file
	FILE *f = fopen(filename, "wb");
	checknull(f, "Cannot open file in wb mode");

	png_write_to_file(img, f, ctx);

	// cleanup
	png_context_free(ctx);

	fclose(f);

	XDestroyImage(img);
	XCloseDisplay(dpy);

	return SUCCESS;
}
