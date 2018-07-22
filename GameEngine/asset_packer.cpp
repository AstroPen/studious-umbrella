
#include <common.h>
#include <push_allocator.h>
#include <unix_file_io.h>
#include "pixel.h"

// TODO I want to share these with the game through a header
struct PackedAssetHeader {
  uint32_t magic;
  uint32_t version;
  uint32_t size; // ??
  uint32_t packed_asset_count; // ??
  uint32_t bitmap_count;
  uint32_t bitmap_offset;
};

struct PackedAnimation {
  uint16_t animation_type;
  uint16_t frame_count;
  float duration;
  uint16_t animation_start_index[4]; // DIRECTION_COUNT
};

struct PackedTextureLayout {
  uint32_t layout_type;
  PackedAnimation animations[0];
};

struct PackedTextureGroup {
  uint64_t bitmap_offset; // In bytes from start of file
  uint32_t width; // In pixels
  uint32_t height; 
  uint16_t texture_group_id;
  int16_t sprite_count; // Negative value indicates it has a normal map
  uint16_t sprite_width; // In pixels
  uint16_t sprite_height;
  float offset_x;
  float offset_y;
  float offset_z;
  float sprite_depth; // TODO I'd like to be able to specify this more generally
};

// For each layout type :
// - Get TextureLayoutType
// assert(layout_type >= 0 && layout_type < LAYOUT_COUNT);
// TextureLayout *layout = assets->texture_layout + layout_type;
// 
// For each animation :
// - Get AnimationType with frame_count and duration
// layout->animation_frame_counts[animation] = frame_count;
// layout->animation_time[animation] = duration;
//
// For each facing direction :
// - Get start_index
// layout->animation_start_index[animation] = start_index;
//

// For each texture group :
// - Get TextureGroupID
// TextureGroup *group = assets->texture_groups + group_id;
// group->bitmap;
// group->layout;
// group->sprite_width;
// group->sprite_height;
// group->sprite_depth;
// group->render_id; // TODO init in OpenGL
// group->sprite_count;
// group->sprite_offset;
// group->has_normal_map;
//

void usage(char *program_name) {
  printf("Usage : %s <header filename>\n", program_name);
  exit(0);
}

int main(int argc, char *argv[]) {
  if (argc != 2) usage(argv[0]);
  char *header_filename = argv[1];

  printf("Packing assets from %s\n", header_filename);

  auto allocator_ = new_push_allocator(2048 * 4);
  auto allocator = &allocator_;

  int error;
  uint8_t *header_file = read_entire_file(header_filename, allocator, &error);
  print_io_error(error, header_filename);

  // TODO read header for info about what files to read and what to do with them

  //printf("Packing successful.\n");
  return 0;
}

