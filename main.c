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
interactive (Display *dpy, Window root, int screen, XImage **img, rect_t rect) {
	printf("Interactive mode\n");

	Cursor cur = XCreateFontCursor(dpy, XC_tcross);

	XGrabKeyboard(dpy, root, True, GrabModeAsync, GrabModeAsync, CurrentTime);
	XGrabPointer(dpy, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, cur, CurrentTime);

	XEvent ev;

	int first = 1, x, y;
	for (;;) {
		XNextEvent(dpy, &ev);
		if (ev.type == ButtonPress) {
			x = ev.xbutton.x;
			y = ev.xbutton.y;
			if (first) {
				rect.x1 = x;
				rect.y1 = y;
				first = 0;
			} else {
				rect.x2 = x;
				rect.y2 = y;
				first = 1;
			}
		} else if (ev.type == KeyPress) {
			break;
		}
	}

	XFreeCursor(dpy, cur);

	*img = XGetImage(dpy, root, rect.x1, rect.y1, \
			abs(rect_width(rect)), abs(rect_height(rect)), \
			AllPlanes, ZPixmap \
		);
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

	// TODO: rewrite the argument parsing
	XImage *img; // final image
	if (argc >= 5) {
		img = XGetImage(dpy, root, \
				atoi(argv[1]), atoi(argv[2]), \
				atoi(argv[3]), atoi(argv[4]), \
				AllPlanes, ZPixmap \
			);

		if (argc > 5) {
			filename = argv[5];
		}

	} else if (argc == 2 || argc == 3) {
		if (strcmp(argv[1], "i") == 0) {
			interactive(dpy, root, screen, &img, rect);

			if (argc == 3) {
				filename = argv[2];
			}
		} else {
			filename = argv[1];

			img = XGetImage(dpy, root, 0, 0, \
					rect_width(rect), rect_height(rect), \
					AllPlanes, ZPixmap \
				);
		}
	} else {
		img = XGetImage(dpy, root, 0, 0, \
				rect_width(rect), rect_height(rect), \
				AllPlanes, ZPixmap \
			);
	}
	checknull(img, "Unable to take screenshot");
	printf("Reactangle: (%d %d)\n", img->width, img->height);

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
