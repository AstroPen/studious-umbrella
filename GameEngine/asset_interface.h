// TODO guards

enum BitmapID : uint32_t {
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

  uint32_t bitmap_id; // NOTE : This is an OpenGL texture id
};

// Specifies how to enumerate a texture into different sprites
enum TextureLayoutType : uint32_t {
  LAYOUT_SINGLETON,
  LAYOUT_CHARACTER,
  LAYOUT_TERRAIN,

  LAYOUT_COUNT
};

struct TextureGroup {
  PixelBuffer bitmap; // TODO remove the texture_id from PixelBuffer
  TextureLayoutType layout;
  uint32_t sprite_width; // TODO Should maybe be column count, row count
  uint32_t sprite_height;
  union {
    float sprite_depth;
    uint32_t sprite_depth_index; // used to index an array of sprite_depths
  };
  uint32_t render_id;
  uint32_t sprite_count; // TODO I'd like to get rid of this
  V3 sprite_offset;
  bool has_normal_map;
};

enum Direction {
  LEFT,
  RIGHT,
  UP,
  DOWN
};

enum AnimationType {
  ANIM_IDLE,
  ANIM_MOVE,

  ANIM_COUNT
};

struct TextureLayout {
  uint16_t animation_frame_counts[ANIM_COUNT];
  uint16_t animation_start_index[ANIM_COUNT][4]; // 4 is for direction count
  float animation_times[ANIM_COUNT];
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


