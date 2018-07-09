
#ifndef _CUSTOM_STB_H_
#define _CUSTOM_STB_H_

struct BakedChar {
  uint16_t x0, y0, x1, y1;
  float xoff, yoff, xadvance;
};

int bake_font_bitmap(void *file_buf, int offset, 
                     float text_height, void *pixel_buf, int width, int height, 
                     int first_glyph, int num_glyphs, BakedChar *baked_chars);

PixelBuffer load_image_file(const char* filename);
#endif

