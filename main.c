#include "main.h"
#include "definitions.h"

/* to do list:
 *
 *	TODO: write a man page and move the comment below
 *		  to a function which prints the usage
 *	TODO: maybe move to xcb and make the program
 *		  multithreaded
 *	TODO: add --with-audio option, which is used with
 *		  option -r (--record)
 *
 */

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
 *			--with-border
 *				captures a screenshot of a window along with
 *				its border
 *
 *			--record (-r)
 *				records the portion of the screen specified by
 *				-i, -g or -w
 *
 *				if used with -w (and without --with-border)
 *				make sure the window is visible the whole
 *				time you are recording, there is currently
 *				no check so the program tries to take the
 *				screenshot no matter what (ie. it crashes
 *				when the window is no longer visible)
 *
 *				the default keybinding to exit recording mode
 *				is Mod4 + Shift + x, it can be modified in the
 *				function `just_record` (lines 176-177)
 *
 *		you can put an optional -- before the file if you want
 *		to name it -i or -g for some reason
 *
 *		if the filename is specified it saves it there,
 *		else it saves it in the DEFAULT_FILENAME file
 */

void
interactive (x_conn_t *conn, rect_t *rect) {
	printf("Interactive mode\n");

	Cursor cur = XCreateFontCursor(conn->dpy, XC_tcross);

	XGrabKeyboard(conn->dpy, conn->win, True, GrabModeAsync, GrabModeAsync, CurrentTime);
	XGrabPointer(conn->dpy, conn->win, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, cur, CurrentTime);

	XEvent ev;

	int first = 1, x, y;
	for (;;) {
		XNextEvent(conn->dpy, &ev);
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

	XUngrabPointer(conn->dpy, CurrentTime);
	XUngrabKeyboard(conn->dpy, CurrentTime);

	XFreeCursor(conn->dpy, cur);

	int tmp;
	if (rect_width(*rect) < 0) {
		tmp = rect->x1;
		rect->x1 = rect->x2;
		rect->x2 = tmp;
	}
	if (rect_height(*rect) < 0) {
		tmp = rect->y1;
		rect->y1 = rect->y2;
		rect->y2 = tmp;
	}
}

void
select_window (x_conn_t *conn, rect_t *rect, Bool border) {
	printf("Window selection\n");

	Cursor cur = XCreateFontCursor(conn->dpy, XC_tcross);

	XGrabPointer(conn->dpy, conn->win, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, cur, CurrentTime);

	XEvent ev;
	Window win;

	for (;;) {
		XNextEvent(conn->dpy, &ev);
		if (ev.type == ButtonPress) {
			win = ev.xbutton.subwindow;
			break;
		}
	}

	Window window;
	int x, y;
	uint width, height, border_width, depth;
	XGetGeometry(conn->dpy, win, &window, &x, &y, &width, &height, &border_width, &depth);

	if (border == True) {
		rect->x1 = x;
		rect->y1 = y;
		rect->x2 = width + border_width * 2 + x;
		rect->y2 = height + border_width * 2 + y;
	} else {
		rect->x1 = rect->y1 = 0;
		rect->x2 = width;
		rect->y2 = height;
		conn->win = win;
	}

	XUngrabPointer(conn->dpy, CurrentTime);

	XFreeCursor(conn->dpy, cur);
}

void
just_screenshot (x_conn_t *conn, char *filename) {
	png_context_t *ctx = make_png_context();

	// write to file
	FILE *f = fopen(filename, "wb");
	checknull(f, "Cannot open file in wb mode");

	png_write_to_file(conn->img, f, ctx);

	// cleanup
	png_context_free(ctx);

	fclose(f);
}

#define TIME_TO_WAIT CLOCKS_PER_SEC * (26 / 1000)
void
just_record (x_conn_t *conn, char *filename) {
	// allocate an output stream
	ost_t *ost = make_ost(filename);
	ost_add_video_stream(ost, conn->img->width, conn->img->height);
	ost_init_io(ost);

	// color conversion
	struct SwsContext *sws_ctx = sws_getContext( \
			conn->img->width, conn->img->height, conn->pix_fmt, \
			conn->img->width, conn->img->height, ost->frame->format, \
			0, NULL, NULL, NULL \
		);

	int keycode = XKeysymToKeycode(conn->dpy, XStringToKeysym("x"));
	XGrabKey(conn->dpy, keycode, Mod4Mask | ShiftMask, conn->root, True, GrabModeAsync, GrabModeAsync);
	XSelectInput(conn->dpy, conn->root, KeyPressMask);

	XEvent ev;
	int time = clock();
	for (;;) {
		time = clock();
		capture_screenshot(conn);
		if (XPending(conn->dpy)) {
			XNextEvent(conn->dpy, &ev);
			if (ev.type == KeyPress) {
				break;
			}
		}
		ximage_to_frame(sws_ctx, conn->img, ost->frame);
		ost_encode_frame(ost);
		while ((clock() - time) < TIME_TO_WAIT);
	}

	sws_freeContext(sws_ctx);

	ost_free(ost);
}

int
main (int argc, char *argv[]) {
	x_conn_t *conn = make_x_conn(NULL, 0);

	// default values
	rect_t rect;
	rect.x1 = rect.y1 = 0;
	rect.x2 = XDisplayWidth(conn->dpy, conn->screen);
	rect.y2 = XDisplayHeight(conn->dpy, conn->screen);
	char *filename = DEFAULT_FILENAME;

	Bool border = False, record = False;
	int i;
	for (i = 1; i < argc; ++ i) {
		printf("arg: %s\n", argv[i]);
		if (strcmp(argv[i], "-g") == 0) {
			if (i + 4 >= argc) {
				printe("Too little arguments for flag -g");
			}

			rect.x1 = atoi(argv[i + 1]);
			rect.y1 = atoi(argv[i + 2]);
			rect.x2 = atoi(argv[i + 3]) + rect.x1;
			rect.y2 = atoi(argv[i + 4]) + rect.y1;

			i += 4;
		} else if (strcmp(argv[i], "-i") == 0) {
			interactive(conn, &rect);
		} else if (strcmp(argv[i], "-w") == 0) {
			select_window(conn, &rect, border);
		} else if (strcmp(argv[i], "--with-border") == 0) {
			border = True;
		} else if (strcmp(argv[i], "--record") == 0 || strcmp(argv[i], "-r") == 0) {
			record = True;
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

	// lazy way to make width and height even
	rect.x1 &= ~1;
	rect.x2 &= ~1;
	rect.y1 &= ~1;
	rect.y2 &= ~1;
	printf("Rectangle: (%d %d %d %d)\nWindow: %ld", rect.x1, rect.y1, rect.x2, rect.y2, conn->win);

	x_conn_init_ximage(conn, &rect);

	if (record == True) {
		just_record(conn, filename);
	}else {
		just_screenshot(conn, filename);
	}

	x_conn_free(conn);

	return SUCCESS;
}
