#include "definitions.h"

x_conn_t *
make_x_conn (_Xconst char *dpy_name, Window win) {
	x_conn_t *conn = malloc(sizeof(x_conn_t));
	conn->dpy = XOpenDisplay(dpy_name);
	conn->root = DefaultRootWindow(conn->dpy);
	conn->win = win ? win : conn->root;
	conn->screen = DefaultScreen(conn->dpy);

	conn->pix_fmt = AV_PIX_FMT_BGRA;
	return conn;
}

void
x_conn_init_ximage (x_conn_t *conn, rect_t *rect) {
	// lazy way to initialize ximage
	conn->img = XGetImage( \
			conn->dpy, conn->win, rect->x1, \
			rect->y1, rect_width(*rect), rect_height(*rect), \
			AllPlanes, ZPixmap \
		);
	checknull(conn->img, "[xlib] Cannot capture an image from the provided window (%ld)", conn->win);

	conn->area = (rect_t){ rect->x1, rect->y1, rect->x2, rect->y2 };
}

// converted to a macro (utils.h)
/* void */
/* capture_screenshot (x_conn_t *conn) { */
/* 	XGetSubImage( \ */
/* 			conn->dpy, conn->win, conn->area.x1, \ */
/* 			conn->area.y1, rect_width(conn->area), rect_height(conn->area), \ */
/* 			AllPlanes, ZPixmap, conn->img, 0, 0 \ */
/* 		); */
/* } */

void
x_conn_free (x_conn_t *conn) {
	XDestroyImage(conn->img);
	XCloseDisplay(conn->dpy);
	free(conn);
}
