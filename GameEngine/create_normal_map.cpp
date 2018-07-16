
#include <common.h>
#include "pixel.h"
#include "custom_stb.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void usage() {
  printf("Usage : create_normal_map <lighting filename> <normal map filename>\n");
  exit(0);
}

int main(int argc, char *argv[]) {
  if (argc != 3) usage();
  char *lighting_filename = argv[1];
  char *normal_map_filename = argv[2];

  printf("Making %s from %s\n", normal_map_filename, lighting_filename);

  PixelBuffer lighting_buffer = load_image_file(lighting_filename);
  if (!is_initialized(&lighting_buffer)) return 1;
  
  int total_width = lighting_buffer.width;
  int width = total_width / 4;
  assert(width % 4 == 0);
  int height = lighting_buffer.height;

  // TODO combine the lighting images into a normal map and write out the png
  PixelBuffer output = alloc_texture(width, height);
  assert(is_initialized(&output));

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      Color x_light = *get_pixel(&lighting_buffer, width + x, y);
      Color y_light = *get_pixel(&lighting_buffer, width * 2 + x, y);
      Color z_light = *get_pixel(&lighting_buffer, width * 3 + x, y);

      if (x_light.a) {
        assert(x_light.r == x_light.g && x_light.r == x_light.b);
        assert(y_light.r == y_light.g && y_light.r == y_light.b);
        assert(z_light.r == z_light.g && z_light.r == z_light.b);
      }

      auto out = get_pixel(&output, x, y);
      Color mix;
      mix.rgba.r = x_light.r;
      mix.rgba.g = y_light.r;
      mix.rgba.b = z_light.r;
      mix.rgba.a = x_light.a;

      *out = mix;
    }
  }

  int stride = 4 * width;
  int success = stbi_write_png(normal_map_filename, width, height, 4, output.buffer, stride);
  if (success) printf("Successfully wrote %s.\n", normal_map_filename);
  else printf("Write failed.\n");

  return 0;
}



