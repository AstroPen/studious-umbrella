#ifndef _PACKED_ASSETS_H_
#define _PACKED_ASSETS_H_

struct PackedAssetHeader {
  u32 magic;
  u32 version;
  u32 total_size; // TODO make this and data_offset u64s
  u32 layout_count;
  u32 texture_group_count;
  u32 data_offset;
};

static_assert(sizeof(PackedAssetHeader) % 8 == 0,"");

struct PackedAnimation {
  u16 animation_type;
  u16 frame_count;
  float duration;

  u16 start_index;
  u16 direction;
  u32 flags;
};

static_assert(sizeof(PackedAnimation) % 8 == 0,"");

struct PackedFaces {
  u16 sprite_index[8];
};

static_assert(sizeof(PackedFaces) % 8 == 0,"");

struct PackedTextureLayout {
  u32 layout_type;
  union {
    struct {
      u32 animation_count;
      PackedAnimation animations[0];
    };
    struct {
      u32 face_count;
      PackedFaces faces[0]; // NOTE : Should only contain 1 element
    };
  };
};

static_assert(sizeof(PackedTextureLayout) % 8 == 0,"");

enum PackedGroupFlags {
  GROUP_HAS_NORMAL_MAP = 1,
};

struct PackedTextureGroup {
  u64 bitmap_offset; // In bytes from start of data

  u32 width; // In pixels
  u32 height; 

  u16 texture_group_id;
  u16 layout_type;
  u16 sprite_count;
  u16 sprite_width; // In pixels
  u16 sprite_height;

  u16 flags;
  u8 min_blend;
  u8 max_blend;
  u8 s_clamp;
  u8 t_clamp;

  float offset_x;
  float offset_y;
  float offset_z;
  float sprite_depth; // TODO I'd like to be able to specify this more generally
};

static_assert(sizeof(PackedTextureGroup) % 8 == 0,"");

struct PackedFont {
  u32 font_id;
  u32 size_count;
};

static_assert(sizeof(PackedFont) % 8 == 0,"");


struct PackedFontSize {
  u64 bitmap_offset;

  u32 font_size;
  u32 width;
  u32 height;
  char start;
  char end;
  u16 char_count;
};

static_assert(sizeof(PackedFontSize) % 8 == 0,"");

#endif
