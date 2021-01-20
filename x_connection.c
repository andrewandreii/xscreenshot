#include "definitions.h"

x_conn_t *
make_x_conn (_Xconst char *dpy_name, Window win) {
	x_conn_t *conn = malloc(sizeof(x_conn_t));
	conn->dpy = XOpenDisplay(dpy_name);
	conn->root = DefaultRootWindow(conn->dpy);
	conn->win = win ? win : conn->root;
	conn->screen = DefaultScreen(conn->dpy);
	conn->shminfo = (XShmSegmentInfo){0, 0, 0, 0};

	conn->pix_fmt = AV_PIX_FMT_BGRA;
	return conn;
}

void
x_conn_init_ximage (x_conn_t *conn, rect_t *rect) {
	conn->area = *rect;

	if (!XShmQueryExtension(conn->dpy)) {
		printe("[x_conn] Shm not available");
	}

	conn->img = XShmCreateImage( \
			conn->dpy, DefaultVisual(conn->dpy, conn->screen), \
			DefaultDepth(conn->dpy, conn->screen), ZPixmap, NULL, \
			&conn->shminfo, rect_width(conn->area), rect_height(conn->area) \
		);
	checknull(conn->img, "[x_conn] XShmCreateImage failed");
	printf("\n%d %d\n", conn->img->bytes_per_line, conn->img->height);

	conn->shminfo.shmid = shmget(IPC_PRIVATE, \
			conn->img->bytes_per_line * conn->img->height, \
			IPC_CREAT | 0777 \
		);

	if (conn->shminfo.shmid < 0) {
		perror("");
		printe("[x_conn] shmget failed");
	}

	conn->shminfo.shmaddr = conn->img->data = shmat(conn->shminfo.shmid, 0, 0);

	if (conn->shminfo.shmaddr == (char *)-1) {
		printe("[x_conn] shmat failed");
	}

	conn->shminfo.readOnly = False;

	XShmAttach(conn->dpy, &conn->shminfo);
	XSync(conn->dpy, True);

	shmctl(conn->shminfo.shmid, IPC_RMID, 0);
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
	XShmDetach(conn->dpy, &conn->shminfo);
	shmdt(conn->shminfo.shmaddr);
	XDestroyImage(conn->img);
	XCloseDisplay(conn->dpy);
	free(conn);
}
