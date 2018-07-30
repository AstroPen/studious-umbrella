#ifndef _PACKED_ASSETS_H_
#define _PACKED_ASSETS_H_

struct PackedAssetHeader {
  uint32_t magic;
  uint32_t version;
  uint32_t total_size; // TODO make this and data_offset u64s
  uint32_t layout_count;
  uint32_t texture_group_count;
  uint32_t data_offset;
};

struct PackedAnimation {
  uint16_t animation_type;
  uint16_t frame_count;
  float duration;
  uint16_t animation_start_index[4]; // DIRECTION_COUNT
};

struct PackedTextureLayout {
  uint32_t layout_type;
  uint32_t animation_count;
  PackedAnimation animations[0];
};

enum PackedGroupFlags {
  GROUP_HAS_NORMAL_MAP = 1,
};

struct PackedTextureGroup {
  uint64_t bitmap_offset; // In bytes from start of data

  uint32_t width; // In pixels
  uint32_t height; 

  uint16_t texture_group_id;
  uint16_t layout_type;
  uint16_t sprite_count;
  uint16_t sprite_width; // In pixels
  uint16_t sprite_height;

  uint16_t flags;
  uint8_t min_blend;
  uint8_t max_blend;
  uint8_t s_clamp;
  uint8_t t_clamp;

  float offset_x;
  float offset_y;
  float offset_z;
  float sprite_depth; // TODO I'd like to be able to specify this more generally
};

#endif
