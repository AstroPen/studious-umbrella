
#include <common.h>
#include <push_allocator.h>
#include <unix_file_io.h>
#include <vector_math.h>
#include "pixel.h"
#include "custom_stb.h"
#include "asset_interface.h"
#include "packed_assets.h"


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
  printf("Usage : %s\n", program_name);
  printf("Usage : %s <header filename>\n", program_name);
  exit(0);
}

int main(int argc, char *argv[]) {
  if (argc > 2) usage(argv[0]);
  // TODO I may actually want it to just read all the files ending in .spec
  char *build_filename = "assets/build.spec";
  if (argc == 2) build_filename = argv[1];

  printf("Packing assets from %s\n", build_filename);

#if 0
  auto allocator_ = new_push_allocator(2048 * 4);
  auto allocator = &allocator_;

  // TODO read build file for info about what files to read and what to do with them
  int error;
  uint8_t *build_file = read_entire_file(build_filename, allocator, &error);
  print_io_error(error, build_filename);
#endif

  // TODO - load_png on sprite sheet
  //      - hard code the dimensions and other stats to put in Packed structs
  //      - write structs to pack_file
  PixelBuffer buffer = load_image_file("assets/lttp_link.png");
  assert(is_initialized(&buffer));
  /*
  struct PackedTextureLayout {
    uint32_t layout_type;
    uint32_t animation_count;
    PackedAnimation animations[0];
  };
  struct PackedAnimation {
    uint16_t animation_type;
    uint16_t frame_count;
    float duration;
    uint16_t animation_start_index[4]; // DIRECTION_COUNT
  };
  struct PackedTextureGroup {
    uint64_t bitmap_offset; // In bytes from start of data
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
  struct PackedAssetHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t total_size; // ??
    uint32_t layout_count;
    uint32_t texture_group_count;
    uint32_t data_offset;
  };
  */
  uint32_t layout_count = 1;
  uint32_t animation_count = 1;
  uint32_t group_count = 1;
  uint64_t pre_data_size = 
    sizeof(PackedAssetHeader) + 
    sizeof(PackedTextureLayout) * layout_count + 
    sizeof(PackedAnimation) * animation_count +
    sizeof(PackedTextureGroup) * group_count;

  // TODO for convenience, I should make a dynamic push allocator
  PushAllocator dest_allocator_ = new_push_allocator(pre_data_size);
  auto dest_allocator = &dest_allocator_;

  PackedAssetHeader *asset_header = alloc_struct(dest_allocator, PackedAssetHeader);
  asset_header->magic = 'PACK';
  asset_header->version = 0;
  asset_header->layout_count = 1;
  asset_header->texture_group_count = 1;
  asset_header->data_offset = pre_data_size;

  PackedTextureLayout *character_layout = alloc_struct(dest_allocator, PackedTextureLayout);
  character_layout->layout_type = LAYOUT_CHARACTER;
  character_layout->animation_count = 1;

  PackedAnimation *anim0 = alloc_struct(dest_allocator, PackedAnimation);
  anim0->animation_type = ANIM_IDLE;
  anim0->frame_count = 1;
  anim0->duration = 1;
  //anim0->animation_start_index = {}; // All zeroes for now

  PackedTextureGroup *link_group = alloc_struct(dest_allocator, PackedTextureGroup);
  link_group->width = buffer.width;
  link_group->height = buffer.height;
  link_group->texture_group_id = TEXTURE_GROUP_LINK;
  link_group->layout_type = LAYOUT_CHARACTER;
  link_group->sprite_count = 1;
  link_group->sprite_width = buffer.width; // TODO needs to be per sprite
  link_group->sprite_height = buffer.height;
  link_group->flags = 0; // No normal map for now
  link_group->min_blend = LINEAR_BLEND;
  link_group->max_blend = NEAREST_BLEND;
  link_group->s_clamp = CLAMP_TO_EDGE;
  link_group->t_clamp = CLAMP_TO_EDGE;
  link_group->offset_x = 0; // Copied from game.cpp
  link_group->offset_y = 0.40;
  link_group->offset_z = 0.06;
  link_group->sprite_depth = 0.3;
  link_group->bitmap_offset = 0; //dest_allocator->bytes_allocated; // TODO maybe consider alignment
  
  uint64_t bitmap_size = uint64_t(buffer.width) * buffer.height * 4;
  asset_header->total_size = link_group->bitmap_offset + bitmap_size;


  // TODO I might want to make my own version of this, but this is fine for now
  FILE *pack_file = fopen("assets/packed_assets.pack", "wb");
  assert(pack_file);
  auto written = fwrite(dest_allocator->memory, 1, dest_allocator->bytes_allocated, pack_file);
  assert(written == dest_allocator->bytes_allocated);
  written = fwrite(buffer.buffer, 1, bitmap_size, pack_file);
  assert(written == bitmap_size);
  fclose(pack_file);


  printf("Packing successful.\n");
  return 0;
}

