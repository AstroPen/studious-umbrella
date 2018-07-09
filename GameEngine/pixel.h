
#ifndef _PIXEL_H_
#define _PIXEL_H_

union Color {
  uint32_t value;
  struct {
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t a;
  };

  inline Color() { value = 0; }
  inline Color(uint32_t v) { value = v; }
  inline operator uint32_t() { return value; }
};

struct PixelBuffer {
  uint8_t *buffer;
  int width;
  int height;
  uint32_t texture_id;
};

// TODO change this to work better with repeated includes
#ifdef _PUSH_ALLOCATOR_H_
static PixelBuffer alloc_texture(PushAllocator *allocator, int width, int height, int pixel_bytes = 4) {
  PixelBuffer result = {};
  result.width = width;
  result.height = height;

  int buffer_size = pixel_bytes * width * height;
  // TODO test different alignments
  result.buffer = (uint8_t *) alloc_size(allocator, buffer_size, 64);

  return result;
}
#endif // _PUSH_ALLOCATOR_H_

#ifdef _VECTOR_MATH_H_
static inline Color to_color(V3 c) {
  Color result;
  result.a = 0xff;
  result.r = 255 * c.r;
  result.g = 255 * c.g;
  result.b = 255 * c.b;
  return result;
}

static inline Color to_color(V4 c) {
  Color result;
  result.a = 255 * c.a;
  result.r = 255 * c.r;
  result.g = 255 * c.g;
  result.b = 255 * c.b;
  return result;
}

static inline V3 to_vector3(Color c) {
  V3 r;
  r.r = c.r / 255.0f;
  r.g = c.g / 255.0f;
  r.b = c.b / 255.0f;
  return r;
}

static inline V4 to_vector4(Color c) {
  V4 r;
  r.r = c.r / 255.0f;
  r.g = c.g / 255.0f;
  r.b = c.b / 255.0f;
  r.a = c.a / 255.0f;
  return r;
}
#endif // _VECTOR_MATH_H_


#if 0
static void draw_texture(PixelBuffer buf, V2 p, PixelBuffer texture) {

  int pixel_bytes = 4;
  V2 tex_lower_left = to_pixel_coordinate(p, buf.height);

  int tex_width = texture.width;
  int tex_height = texture.height;

  int tex_origin_x = tex_lower_left.x;
  int tex_origin_y = tex_lower_left.y - texture.height;

  int min_x = max(tex_origin_x, 0);
  int min_y = max(tex_origin_y, 0);

  int max_x = min(tex_origin_x + tex_width,  buf.width);
  int max_y = min(tex_origin_y + tex_height, buf.height);

  int screen_pitch = buf.width * pixel_bytes;
  int tex_pitch = tex_width * pixel_bytes;

  for (int screen_y = min_y; screen_y < max_y; screen_y++) {

    uint8_t *screen_row = buf.buffer + screen_y * screen_pitch;

    int texture_y = screen_y - tex_origin_y;
    uint8_t *texture_row = texture.buffer + texture_y * tex_pitch; 

    for (int screen_x = min_x; screen_x < max_x; screen_x++) {

      uint32_t *screen_pixel = (uint32_t *) (screen_row + screen_x * pixel_bytes);

      int texture_x = screen_x - tex_origin_x;
      uint32_t tex_pixel = * (uint32_t *) (texture_row + texture_x * pixel_bytes);

      *screen_pixel = tex_pixel;
    }
  }
}
#endif

#endif // _PIXEL_H_

