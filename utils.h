#ifndef XSCRSHOT_UTILS_H
#define XSCRSHOT_UTILS_H
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include "main.h"

#define rect_width(r) ((r).x2 - (r).x1)
#define rect_height(r) ((r).y2 - (r).y1)

typedef struct {
	int x1, y1, x2, y2;
} rect_t;

typedef struct {
	Display* dpy;
	Window win;
	int screen;

	XImage *img;
	rect_t area;

	// not yet used
	int pix_fmt;
} x_conn_t;

x_conn_t *make_x_conn(_Xconst char *dpy_name, Window win);
void x_conn_init_ximage(x_conn_t *conn, rect_t *rect);
#define capture_screenshot(conn) \
	XGetSubImage( \
			conn->dpy, conn->win, conn->area.x1, \
			conn->area.y1, rect_width(conn->area), rect_height(conn->area), \
			AllPlanes, ZPixmap, conn->img, 0, 0 \
		);
/* void capture_screenshot(x_conn_t *conn); // x_conn_init_ximage must be called first */
void x_conn_free(x_conn_t *conn);

// connection between Xlib and libav
void ximage_to_frame(struct SwsContext *sws_ctx, XImage *image, AVFrame *frane);

typedef struct {
	png_structp png_ptr;
	png_infop info_ptr;
} png_context_t;

// creates png_ptr and info_ptr
// also checks if the values are NULL
png_context_t *make_png_context(void);
void png_write_to_file(XImage *img, FILE *f, png_context_t *ctx);
void png_context_free(png_context_t *ctx);

#endif
