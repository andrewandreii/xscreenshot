#include "utils.h"

png_context_t *
make_png_context (void) {
	png_context_t *ctx = malloc(sizeof(png_context_t));
	ctx->png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	checknull(ctx->png_ptr, "[libpng] Cannot create the png struct");

	ctx->info_ptr = png_create_info_struct(ctx->png_ptr);
	checknull(ctx->info_ptr, "[libpng] Cannot create the info struct");

	return ctx;
}

void
png_write_to_file (XImage *img, FILE *f, png_context_t *ctx) {
	png_init_io(ctx->png_ptr, f);

	png_set_IHDR(ctx->png_ptr, ctx->info_ptr, img->width, img->height, 8, \
			PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, \
			PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_write_info(ctx->png_ptr, ctx->info_ptr);

	png_bytep row = malloc(3 * img->width * 8);
	int i, j;
	for (i = 0; i < img->height; ++ i) {
		for (j = 0; j < img->width; ++ j) {
			ulong pixel = XGetPixel(img, j, i);
			row[3 * j] = (pixel & img->red_mask) >> 16;
			row[3 * j + 1] = (pixel & img->green_mask) >> 8;
			row[3 * j + 2] = (pixel & img->blue_mask);
		}
		png_write_row(ctx->png_ptr, row);
	}
	free(row);

	png_write_end(ctx->png_ptr, ctx->info_ptr);
}

void
png_context_free (png_context_t *ctx) {
	png_destroy_write_struct(&(ctx->png_ptr), &(ctx->info_ptr));
}

int
abs (int a) {
	if (a < 0) {
		return -a;
	}
	return a;
}

x_conn_t *
make_x_conn (_Xconst char *dpy_name, Window win) {
	x_conn_t *conn = malloc(sizeof(x_conn_t));
	conn->dpy = XOpenDisplay(dpy_name);
	conn->win = win ? win : DefaultRootWindow(conn->dpy);
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
