
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG

// TODO #define STBI_MALLOC, STBI_REALLOC, and STBI_FREE to avoid using malloc,realloc,free

#include "stb_image.h"


// NOTE : Copy pasted from stb_image.h ::

// Basic usage (see HDR discussion below for HDR usage):
//    int x,y,n;
//    unsigned char *data = stbi_load(filename, &x, &y, &n, 0);
//    // ... process data if not NULL ...
//    // ... x = width, y = height, n = # 8-bit components per pixel ...
//    // ... replace '0' with '1'..'4' to force that many components per pixel
//    // ... but 'n' will always be the number that it would have been if you said 0
//    stbi_image_free(data)
//
// Standard parameters:
//    int *x                 -- outputs image width in pixels
//    int *y                 -- outputs image height in pixels
//    int *channels_in_file  -- outputs # of image components in image file
//    int desired_channels   -- if non-zero, # of image components requested in result
//
// The return value from an image loader is an 'unsigned char *' which points
// to the pixel data, or NULL on an allocation failure or if the image is
// corrupt or invalid. The pixel data consists of *y scanlines of *x pixels,
// with each pixel consisting of N interleaved 8-bit components; the first
// pixel pointed to is top-left-most in the image. There is no padding between
// image scanlines or between pixels, regardless of format. The number of
// components N is 'desired_channels' if desired_channels is non-zero, or
// *channels_in_file otherwise. If desired_channels is non-zero,
// *channels_in_file has the number of components that _would_ have been
// output otherwise. E.g. if you set desired_channels to 4, you will always
// get RGBA output, but you can check *channels_in_file to see if it's trivially
// opaque because e.g. there were only 3 channels in the source image.
//
// An output image with N components has the following components interleaved
// in this order in each pixel:
//
//     N=#comp     components
//       1           grey
//       2           grey, alpha
//       3           red, green, blue
//       4           red, green, blue, alpha
//
// If image loading fails for any reason, the return value will be NULL,
// and *x, *y, *channels_in_file will be unchanged. The function
// stbi_failure_reason() can be queried for an extremely brief, end-user
// unfriendly explanation of why the load failed. Define STBI_NO_FAILURE_STRINGS
// to avoid compiling these strings at all, and STBI_FAILURE_USERMSG to get slightly
// more user-friendly ones.

static inline void pre_multiply_alpha(PixelBuffer image) {
  int num_pixels = image.width * image.height;
  auto pixels = (uint32_t *) image.buffer;
  for (int i = 0; i < num_pixels; i++) {
    uint32_t *pixel = pixels + i;
    // NOTE since a is still last, we don't have to reverse the bytes
    Color c0 = *pixel;
    auto c1 = to_vector4(c0);
    c1.rgb *= c1.a;
    c0 = c1;
    *pixel = c0.value;
  }
}

static PixelBuffer load_image_file(const char* filename) {
  assert(filename);
  int width;
  int height;
  int pixel_bytes = 4;
  int original_pixel_bytes;
  uint8_t *data = stbi_load(filename, &width, &height, &original_pixel_bytes, pixel_bytes);

  if (!data) {
    printf("ERROR : Failed to load image file %s : %s\n", filename, stbi_failure_reason());
    //assert(false);
  }

  if (!data) return {};

  PixelBuffer buffer;
  buffer.width = width;
  buffer.height = height;
  buffer.buffer = data;

  if (original_pixel_bytes == 4) {
    pre_multiply_alpha(buffer);
  }

  return buffer;
}

