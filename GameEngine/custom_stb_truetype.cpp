// NOTE : This is its own translation unit.

// Fonts are in /Library/Fonts/ on osx
#define STB_TRUETYPE_IMPLEMENTATION 
#include "stb_truetype.h"

#include "pixel.h"
#include "custom_stb.h"

int bake_font_bitmap(void *file_buf, int offset, 
                     float text_height, void *pixel_buf, int width, int height, 
                     int first_glyph, int num_glyphs, BakedChar *baked_chars) {

  int result = stbtt_BakeFontBitmap((uint8_t *)file_buf, offset, 
                                    text_height, (uint8_t *)pixel_buf, width, height, 
                                    first_glyph, num_glyphs, (stbtt_bakedchar *)baked_chars);
  return result;
}

