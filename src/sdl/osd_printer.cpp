/*
	SDL OSD - printer bitmap rendering (basic; no host font rasterizer)
*/

#include "osd.h"

#ifdef USE_PRINTER

static inline scrntype_t pen_color(uint8_t r, uint8_t g, uint8_t b)
{
	return RGB_COLOR(r, g, b);
}

void OSD::create_bitmap(bitmap_t *bitmap, int width, int height)
{
	initialize_screen_buffer(bitmap, width, height, 0);
}

void OSD::release_bitmap(bitmap_t *bitmap)
{
	release_screen_buffer(bitmap);
}

void OSD::create_font(font_t *font, const _TCHAR *family, int width, int height, int rotate, bool bold, bool italic)
{
	if(family != NULL) {
		my_tcscpy_s(font->family, 64, family);
	}
	font->width = width;
	font->height = height;
	font->rotate = rotate;
	font->bold = bold;
	font->italic = italic;
	font->loaded = true;
}

void OSD::release_font(font_t *font)
{
	font->loaded = false;
}

void OSD::create_pen(pen_t *pen, int width, uint8_t r, uint8_t g, uint8_t b)
{
	pen->width = width;
	pen->r = r;
	pen->g = g;
	pen->b = b;
	pen->loaded = true;
}

void OSD::release_pen(pen_t *pen)
{
	pen->loaded = false;
}

void OSD::clear_bitmap(bitmap_t *bitmap, uint8_t r, uint8_t g, uint8_t b)
{
	if(bitmap->lpBmp == NULL) {
		return;
	}
	scrntype_t c = pen_color(r, g, b);
	int n = bitmap->width * bitmap->height;
	for(int i = 0; i < n; i++) {
		bitmap->lpBmp[i] = c;
	}
}

int OSD::get_text_width(bitmap_t *bitmap, font_t *font, const char *text)
{
	// no rasterizer: approximate by half the font height per glyph
	int gw = (font != NULL && font->height > 0) ? (font->height / 2) : 8;
	return (int)strlen(text) * gw;
}

void OSD::draw_text_to_bitmap(bitmap_t *bitmap, font_t *font, int x, int y, const char *text, uint8_t r, uint8_t g, uint8_t b)
{
	// text rendering not implemented (no host font rasterizer)
}

void OSD::draw_point_to_bitmap(bitmap_t *bitmap, int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
	if(bitmap->lpBmp == NULL) {
		return;
	}
	if(x < 0 || y < 0 || x >= bitmap->width || y >= bitmap->height) {
		return;
	}
	bitmap->lpBmp[bitmap->width * y + x] = pen_color(r, g, b);
}

void OSD::draw_line_to_bitmap(bitmap_t *bitmap, pen_t *pen, int sx, int sy, int ex, int ey)
{
	if(bitmap->lpBmp == NULL) {
		return;
	}
	// Bresenham
	int dx = abs(ex - sx), dy = abs(ey - sy);
	int stepx = (sx < ex) ? 1 : -1;
	int stepy = (sy < ey) ? 1 : -1;
	int err = dx - dy;
	uint8_t r = pen->r, g = pen->g, b = pen->b;
	int x = sx, y = sy;
	while(true) {
		draw_point_to_bitmap(bitmap, x, y, r, g, b);
		if(x == ex && y == ey) {
			break;
		}
		int e2 = err * 2;
		if(e2 > -dy) { err -= dy; x += stepx; }
		if(e2 <  dx) { err += dx; y += stepy; }
	}
}

void OSD::draw_rectangle_to_bitmap(bitmap_t *bitmap, int x, int y, int width, int height, uint8_t r, uint8_t g, uint8_t b)
{
	for(int yy = 0; yy < height; yy++) {
		for(int xx = 0; xx < width; xx++) {
			draw_point_to_bitmap(bitmap, x + xx, y + yy, r, g, b);
		}
	}
}

void OSD::stretch_bitmap(bitmap_t *dest, int dest_x, int dest_y, int dest_width, int dest_height, bitmap_t *source, int source_x, int source_y, int source_width, int source_height)
{
	if(dest->lpBmp == NULL || source->lpBmp == NULL || dest_width <= 0 || dest_height <= 0 || source_width <= 0 || source_height <= 0) {
		return;
	}
	for(int dy = 0; dy < dest_height; dy++) {
		int sy = source_y + dy * source_height / dest_height;
		if(sy < 0 || sy >= source->height) continue;
		for(int dx = 0; dx < dest_width; dx++) {
			int sx = source_x + dx * source_width / dest_width;
			if(sx < 0 || sx >= source->width) continue;
			int tx = dest_x + dx, ty = dest_y + dy;
			if(tx < 0 || ty < 0 || tx >= dest->width || ty >= dest->height) continue;
			dest->lpBmp[dest->width * ty + tx] = source->lpBmp[source->width * sy + sx];
		}
	}
}

#endif // USE_PRINTER
