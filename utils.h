#ifndef XSCRSHOT_UTILS_H
#define XSCRSHOT_UTILS_H
#include "main.h"

typedef struct {
	png_structp png_ptr;
	png_infop info_ptr;
} png_context_t;

// creates png_ptr and info_ptr
// also checks if the values are NULL
png_context_t *make_png_context(void);
void png_write_to_file(XImage *img, FILE *f, png_context_t *ctx);
void png_context_free(png_context_t *ctx);

void ximage_inherit_masks(XImage *i1, XImage *i2);

#endif
