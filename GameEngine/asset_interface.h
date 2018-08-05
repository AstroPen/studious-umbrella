#ifndef _ASSET_INTERFACE_H_
#define _ASSET_INTERFACE_H_

enum BitmapID : u32 {
  BITMAP_INVALID,

  BITMAP_FACE,
  BITMAP_TEST_SPRITE,
  BITMAP_TEST_SPRITE_NORMAL_MAP,
  BITMAP_LINK,
  BITMAP_LINK_NORMAL_MAP,
  BITMAP_BACKGROUND,
  BITMAP_WALL,
  BITMAP_WALL_NORMAL_MAP,
  BITMAP_WHITE,
  BITMAP_CIRCLE,
  BITMAP_SPHERE_NORMAL_MAP,

  BITMAP_COUNT
};

//
// NOTE : TextureGroups represent individual bitmaps while TextureLayouts are shared between
// skins. In other words, a reskin has the same sprite layout, but a different bitmap. 
//
// Different layouts will represent different ingame uses when necessary, but it may be 
// beneficial to try and use reskins when possible since they can be easily swapped out by
// only changing the TextureGroupID.
//
// I may also want to define a way to make 'subtypes' of layouts so that simpler layouts
// can be used in place of complex onces by just reusing animations or sprites. This can
// already be done to some extent by just mapping to the same animation/sprite multiple times.
//


// TODO 
// Replace the BitmapID in entity/visual info with a TextureGroupID
// Lookup should happen through an accessor that passes the entity in. 
// Based on the entity it chooses what animation to play, what direction to use,
// and what frame its on (as well as color, scale, etc). Then in creates a 
// VisualInfo (or something) that gets passed to the renderer. 
//
// Also, for sprites that are in multiple pieces (head, body, shadow, etc) maybe
// we call push_sprite once for each?
//
// I should add a "HAS_TEXTURE" to the shader so that I can get rid of BITMAP_WHITE.
//
//
// UPDATE
// In addition to a TextureGroupID, each entity should have a TextureType that describes
// whether it is an animated character, cube object, etc. The TextureType is also used to 
// look up the layout or something. The group may also need to contribute layout information.
// For example, it could have an array of TEXTURE_TYPE_COUNT length of layout pointers that
// are null if the group doesn't have sprites of that type. It might also store counts of each,
// and then each entity has an index along with its TextureType.
//
// 
enum TextureGroupID {
  TEXTURE_GROUP_INVALID,

  TEXTURE_GROUP_LINK,
  TEXTURE_GROUP_WALL,

  TEXTURE_GROUP_COUNT
};

// TODO This might replace VisualInfo
// TODO Rename this 'RenderInfo' and rename the thing called RenderInfo to 'PlatformWindow' or something
struct RenderingInfo {
  // NOTE : {0,0,0,0} means no texture or normal_map
  V4 texture_uv; 
  V4 normal_map_uv;
  V4 color;
  V3 offset;
  float sprite_depth; // TODO This should maybe become a V4 or something
  float scale;
  u32 width;
  u32 height;
  float texture_width;
  float texture_height;

  u32 bitmap_id; // NOTE : This is an OpenGL texture id
};

// Specifies how to enumerate a texture into different sprites
enum TextureLayoutType : u32 {
  LAYOUT_SINGLETON,
  LAYOUT_CHARACTER,
  LAYOUT_TERRAIN,

  LAYOUT_COUNT,
  LAYOUT_INVALID
};

struct TextureGroup {
  PixelBuffer bitmap; // TODO remove the texture_id from PixelBuffer
  TextureLayoutType layout;
  u32 sprite_width; // TODO Should maybe be column count, row count
  u32 sprite_height;
  union {
    float sprite_depth;
    u32 sprite_depth_index; // used to index an array of sprite_depths
  };
  u32 render_id;
  u32 sprite_count; // TODO I'd like to get rid of this
  V3 sprite_offset;
  bool has_normal_map;
};

enum Direction {
  LEFT,
  RIGHT,
  UP,
  DOWN,

  DIRECTION_COUNT,
  DIRECTION_INVALID,
};

static Direction flip_direction(Direction dir) {
  switch (dir) {
    case LEFT : return RIGHT;
    case RIGHT : return LEFT;
    case UP : return DOWN;
    case DOWN : return UP;
    default : assert(!"Invalid direction.");
  }
  
  return DIRECTION_INVALID;
}

enum AnimationType {
  ANIM_IDLE,
  ANIM_MOVE,
  ANIM_SLIDE,

  ANIM_COUNT,
  ANIM_INVALID
};

union TextureLayout {
  struct { // For LAYOUT_CHARACTER
    u16 animation_frame_counts[ANIM_COUNT][DIRECTION_COUNT];
    u16 animation_start_index[ANIM_COUNT][DIRECTION_COUNT];
    float animation_times[ANIM_COUNT][DIRECTION_COUNT];
  };
  struct { // For LAYOUT_TERRAIN
    u16 cube_face_index[6];
  };
};

enum FontID {
  FONT_INVALID,
  FONT_ARIAL,
  FONT_COURIER_NEW_BOLD,
  FONT_DEBUG,
  FONT_COUNT,
};

struct BakedChar;

struct FontInfo {
  PixelBuffer bitmap;
  BakedChar *baked_chars;
};

// NOTE : WARNING : Clamping and blending values must be contiguous for the packer to work.
enum TextureFormatSpecifier : u32 {
  RGBA, BGRA,

  CLAMP_FIRST,
  CLAMP_TO_EDGE = CLAMP_FIRST,
  REPEAT_CLAMPING,
  CLAMP_LAST = REPEAT_CLAMPING,

  BLEND_FIRST,
  LINEAR_BLEND = BLEND_FIRST,
  NEAREST_BLEND,
  BLEND_LAST = NEAREST_BLEND,

  CLAMP_COUNT = CLAMP_LAST - CLAMP_FIRST + 1,
  CLAMP_INVALID,
  BLEND_COUNT = BLEND_LAST - BLEND_FIRST + 1,
  BLEND_INVALID,
};

#endif

