#include "main.h"

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
make_png (png_structp *png_ptr, png_infop *info_ptr) {
	*png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	check(*png_ptr, "Cannot create the png struct");

	*info_ptr = png_create_info_struct(*png_ptr);
	check(*info_ptr, "Cannot create the info struct");
}

void
write_ximage_to_file (XImage *img, rect_t *rect, FILE *f, png_structp png_ptr, png_infop info_ptr) {
	png_init_io(png_ptr, f);

	png_set_IHDR(png_ptr, info_ptr, rect->width, rect->height, 8, \
			PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, \
			PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_write_info(png_ptr, info_ptr);

	png_bytep row = malloc(3 * rect->width * 8);
	int i, j;
	for (i = 0; i < rect->height; ++ i) {
		for (j = 0; j < rect->width; ++ j) {
			ulong pixel = XGetPixel(img, j + rect->x, i + rect->y);
			row[3 * j] = (pixel & img->red_mask) >> 16;
			row[3 * j + 1] = (pixel & img->green_mask) >> 8;
			row[3 * j + 2] = (pixel & img->blue_mask);
		}
		png_write_row(png_ptr, row);
	}
	free(row);

	png_write_end(png_ptr, info_ptr);
}

int
main (int argc, char *argv[]) {
	Display *dpy = XOpenDisplay(NULL);
	Window root = DefaultRootWindow(dpy);
	int screen = DefaultScreen(dpy);

	// default values
	rect_t rect;
	rect.x = rect.y = 0;
	rect.width = XDisplayWidth(dpy, screen);
	rect.height = XDisplayHeight(dpy, screen);
	char *filename = DEFAULT_FILENAME;

	XImage *img = XGetImage(dpy, root, 0, 0, rect.width, rect.height, AllPlanes, ZPixmap);
	check(img, "Cannot get image from window");

	if (argc >= 5) {
		rect.x = atoi(argv[1]);
		rect.y = atoi(argv[2]);
		rect.width = atoi(argv[3]);
		rect.height = atoi(argv[4]);
		if (argc > 5) {
			filename = argv[5];
		}
	} else if (argc == 2 || argc == 3) {
		if (strcmp(argv[1], "i") == 0) { // create a window to select the points
			printf("Interactive mode\n");

			Window win = XCreateSimpleWindow(dpy, root, 0, 0, rect.width, rect.height, 0, 1, 1);

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
			XPutImage(dpy, win, gc, img, 0, 0, 0, 0, rect.width, rect.height);

			int first = 1, x, y;
			for (;;) {
				XNextEvent(dpy, &ev);
				if (ev.type == ButtonPress) {
					x = ev.xbutton.x;
					y = ev.xbutton.y;
					if (first) {
						rect.x = x;
						rect.y = y;
						first = 0;
					} else {
						rect.width = x - rect.x;
						rect.height = y - rect.y;
						// swap x and y with width and height
						if (rect.width < 0 || rect.height < 0) {
							rect.width = rect.x - x;
							rect.height = rect.y - y;
							rect.x = x;
							rect.y = y;
						}
						first = 1;
					}
				} else if (ev.type == KeyPress) {
					break;
				}
			}

			XDestroyWindow(dpy, win);
			XFreeGC(dpy, gc);

			if (argc == 3) {
				filename = argv[2];
			}
		} else {
			filename = argv[1];
		}
	}
	printf("Reactangle: (%d %d %d %d)\n", rect.x, rect.y, rect.width, rect.height);

	png_structp png_ptr;
	png_infop info_ptr;

	make_png(&png_ptr, &info_ptr);

	// write to file
	FILE *f = fopen(filename, "wb");
	check(f, "Cannot open file in wb mode");

	write_ximage_to_file(img, &rect, f, png_ptr, info_ptr);

	// cleanup
	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(f);

	XDestroyImage(img);
	XCloseDisplay(dpy);

	return SUCCESS;
}
