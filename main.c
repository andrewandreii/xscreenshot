#include "main.h"
#include "utils.h"

/*
 * Usage:
 *		./a.out [options] [-i|-g x1 y1 x2 y2|-w] [filename]
 *
 *		-i
 *			interactive mode, let's you choose the
 *			area you want to screenshot
 *
 *		-g x1 y1 x2 y2
 *			screenshots the rectangle define by the points
 *			(x1, y1) and (x2 y2)
 *
 *		-w
 *			let's you choose a window in interactive mode
 *			instead of an area
 *
 *		options:
 *			currently only -w has options
 *
 *			--with-border (not yet tested)
 *				captures a screenshot of a window along with
 *				its border
 *
 *		you can put an optional -- before the file if you want
 *		to name it -i or -g for some reason
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

	XUngrabPointer(dpy, CurrentTime);
	XUngrabKeyboard(dpy, CurrentTime);

	XFreeCursor(dpy, cur);

	*img = XGetImage(dpy, root, rect.x1, rect.y1, \
			abs(rect_width(rect)), abs(rect_height(rect)), \
			AllPlanes, ZPixmap \
		);
}

void
select_window (Display *dpy, Window root, XImage **img, Bool border) {
	printf("Window selection\n");

	Cursor cur = XCreateFontCursor(dpy, XC_tcross);

	XGrabPointer(dpy, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, cur, CurrentTime);

	XEvent ev;
	Window win;

	for (;;) {
		XNextEvent(dpy, &ev);
		if (ev.type == ButtonPress) {
			win = ev.xbutton.subwindow;
			break;
		}
	}

	Window root_window;
	int x, y;
	uint width, height, border_width, depth;
	XGetGeometry(dpy, win, &root_window, &x, &y, &width, &height, &border_width, &depth);

	if (border == True) {
		*img = XGetImage(dpy, root, \
				x - border_width, y - border_width, \
				width, height, \
				AllPlanes, ZPixmap \
			);
	} else {
		*img = XGetImage(dpy, win, 0, 0, width, height, AllPlanes, ZPixmap);
	}

	XUngrabPointer(dpy, CurrentTime);

	XFreeCursor(dpy, cur);
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

	XImage *img = NULL;
	Bool border = False;
	int i;
	for (i = 1; i < argc; ++ i) {
		printf("arg: %s\n", argv[i]);
		if (strcmp(argv[i], "-g") == 0) {
			if (i + 4 >= argc) {
				printe("Too little arguments for flag -g");
			} else if (img != NULL) {
				printe("Screenshot already took, -g shouldn't be used");
			}

			img = XGetImage(dpy, root, \
					atoi(argv[i + 1]), atoi(argv[i + 2]), \
					atoi(argv[i + 3]), atoi(argv[i + 4]), \
					AllPlanes, ZPixmap \
				);

			i += 4;
		} else if (strcmp(argv[i], "-i") == 0) {
			if (img != NULL) {
				printe("Screenshot already took, -i shouldn't be used");
			}

			interactive(dpy, root, screen, &img, rect);
		} else if (strcmp(argv[i], "-w") == 0) {
			if (img != NULL) {
				printe("Screenshot already took, -w shouldn't be used");
			}

			select_window(dpy, root, &img, border);
		} else if (strcmp(argv[i], "--with-border") == 0) {
			border = True;
		} else if (i + 1 >= argc) {
			filename = argv[i];
		} else if (strcmp(argv[i], "--") == 0) {
			if (i + 2 < argc) {
				printe("Too many arguments after '--'");
			}
			filename = argv[i + 1];
			break;
		} else {
			printe("Unknown argument");
		}
	}

	if (img == NULL) {
		img = XGetImage(dpy, root, \
				rect.x1, rect.y1, \
				rect_width(rect), rect_height(rect),
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
