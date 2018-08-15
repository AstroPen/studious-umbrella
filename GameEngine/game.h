#ifndef _NAR_GAME_H_
#define _NAR_GAME_H_

// NOTE : This file should be a true header and have no function definitions.

// TODO I don't love the current organization of this file. It should act as a simple, minimal interface between the
// various systems. This includes the renderer, asset manager, entity manager, debug system and platform layer.

#define VEL_MAX 15
// TODO this needs to be removed so that the screen can scale to different resolutions
#define PIXELS_PER_METER 50
#define METERS_PER_PIXEL (1.0f / PIXELS_PER_METER)
#define PHYSICS_FRAME_TIME (1.0f / 120.0)

#define MAX_ENTITY_COUNT 255

// TODO I'm thinking of dividing the arguments to the game into "Memory", "Input", and "Output" or something.
// TODO Or alternatively, maybe better, just make a global structure that has pointers to everything useful in the GameState.
// TODO At some point I need to add sound!

//
// Platform interface ---
//

enum ButtonType : u8 {
  BUTTON_LEFT,
  BUTTON_RIGHT,
  BUTTON_UP,
  BUTTON_DOWN,
  BUTTON_QUIT,
  BUTTON_MOUSE_LEFT,
  BUTTON_MOUSE_RIGHT,
  BUTTON_MOUSE_MIDDLE,

  BUTTON_DEBUG_DISPLAY_TOGGLE,
  BUTTON_DEBUG_CAMERA_TOGGLE,
  BUTTON_DEBUG_ANIMATION_TOGGLE,
  
  BUTTON_DEBUG_CAMERA_LEFT,
  BUTTON_DEBUG_CAMERA_RIGHT,
  BUTTON_DEBUG_CAMERA_UP,
  BUTTON_DEBUG_CAMERA_DOWN,

  BUTTON_DEBUG_CAMERA_IN,
  BUTTON_DEBUG_CAMERA_OUT,
  BUTTON_DEBUG_CAMERA_TILT_UP,
  BUTTON_DEBUG_CAMERA_TILT_DOWN,

  BUTTON_MAX,
  BUTTON_NONE  = 0xff
};

enum ButtonEventType : u8 {
  NO_EVENTS = 0,
  PRESS_EVENT = 1,
  RELEASE_EVENT = 2
};

struct ButtonState {
  // NOTE this is a little redundant, but it makes things less complicated
  ButtonEventType first_press;
  ButtonEventType last_press;
  u8 num_transitions;
  bool held;
};

// TODO Switch to using a union with the buttons instead of just the ButtonType enum.
// (though this would make it harder to add more buttons, so maybe do that after I've added them all?)
struct ControllerState {
  ButtonState buttons[BUTTON_MAX];
  V2 pointer;
  bool pointer_moved;
};

struct GameMemory {
  void *permanent_store;
  u64 permanent_size;
  void *temporary_store;
  u64 temporary_size;
};

struct RenderBuffer;

struct GameInput {
  ControllerState controller;
  RenderBuffer *render_buffer;
  WorkQueue *work_queue;
  float delta_t;
  u8 ticks;
};

//
// Asset interface ---
//

#include "asset_interface.h"

struct GameAssets {
  WorkQueue *work_queue;
  HeapAllocator work_allocator;
  PixelBuffer bitmaps[BITMAP_COUNT];
  FontInfo fonts[FONT_COUNT];

  TextureGroup texture_groups[TEXTURE_GROUP_COUNT];
  TextureLayout texture_layouts[LAYOUT_COUNT];
  TextureInfo texture_infos[TEXTURE_INDEX_MAX];
};

static inline PixelBuffer *get_bitmap(GameAssets *assets, BitmapID id);
static inline PixelBuffer *get_bitmap_location(GameAssets *assets, BitmapID id);
static inline u32 get_texture_id(GameAssets *assets, BitmapID id);
static inline TextureInfo get_texture_info(GameAssets *assets, BitmapID id);
static inline TextureInfo get_texture_info(GameAssets *assets, TextureGroupID id);
static inline FontInfo *get_font(GameAssets *assets, u32 font_id);
static inline FontInfo *get_font_location(GameAssets *assets, u32 font_id);
static inline BakedChar *get_baked_char(FontInfo *font, char c);

//static BitmapID get_keyframe(GameAssets *assets, TextureGroupID group_id, AnimationType animation, Direction dir, u32 frame);
static inline TextureGroup *get_texture_group(GameAssets * assets, TextureGroupID id);

//
// Game interface ---
//

struct VisualInfo {
  //PixelBuffer texture;
  V4 color;
  V3 offset;
  float sprite_depth;
  float scale;
  BitmapID texture_id;
  BitmapID normal_map_id;
};

enum EntityFlag {
  ENTITY_COLLIDES = 1,
  ENTITY_TEXTURE = (1 << 1),
  ENTITY_PLAYER_CONTROLLED = (1 << 2),
  ENTITY_SLIDING = (1 << 3),
  ENTITY_BOUNCING = (1 << 4),
  ENTITY_MOVING = (1 << 5),
  ENTITY_SOLID = (1 << 6),
  ENTITY_TEMPORARY = (1 << 7),
  ENTITY_CUBOID = (1 << 8),
  ENTITY_SPRITE = (1 << 9),
  ENTITY_TEXTURE_REPEAT = (1 << 10),
  ENTITY_NORMAL_MAP = (1 << 11),
};

// TODO massively rework this
// Option 1 : Keep entities large and store them in a compressed state.
//    - Simplest option, good if operations on each entity happen all at once
// Option 2 : Break entities into components, SOA accessed by hash
//    - Keeps entity size small (just a hash key) but can't easily iterate over arrays separately
//    - Good if iterating over entities multiple times, one for each component
// Option 3 : Break entities into components, use discriminated union to store in place
//    - Keeps entity size down without indirection, similar to Option 1 except always compressed
// Option 4 : Break entities into components, use indirection to component array
//    - Again allows for small entities, iterate over all entities quickly or process all components per entity
// Option 5 : Break entities into components, SOA with indirection, no hashing
//    - Can iterate over array of each component for separate proccessing, Entity size is a bit large though
struct Entity {
  u64 flags;
  // TODO make collsion more general, and 3D
  //Shape collision_box;
  AlignedBox collision_box;
  VisualInfo visual;
  V3 vel;
  V3 acc;

  TextureGroupID texture_group_id;
  Direction facing_direction;
  Direction pressed_direction;
  float animation_dt;
  float animation_duration;
  AnimationType current_animation;

  float mass;
  float accel_max;
  float time_to_max_accel;
  float time_to_zero_accel;
  float friction_multiplier;
  float bounce_factor;
  float slip_factor;

  float lifetime;

  ShapeType shape_type;
};

// TODO use this for temporary storage management
struct TemporaryState {
};

struct GameCamera {
  V3 p;
  V3 target;
  V3 right;
  V3 up;
  V3 forward;
  float aspect_ratio; // height/width
  float focal_length;
  float near_dist, far_dist;
  int screen_height;
  int screen_width;
};

// TODO maybe move these to another file
static inline void look_at(GameCamera *camera, V3 target, V3 up = {0,1,0}) {
  camera->target = target;
  auto eye = camera->p;
  // "Forward"
  V3 Z = normalize(eye - target);
  // "Left" (Right?)
  V3 X = normalize(cross(up, Z)); 
  // "Up" (recalculated so that the camera can tilt up and down)
  V3 Y = normalize(cross(Z,X));

  camera->right = X;
  camera->up = Y;
  camera->forward = Z;
}

inline void look_at_target(GameCamera *camera, V3 up = {0,1,0}) {
  look_at(camera, camera->target, up);
}


struct GameState {
  GameAssets assets;
  GameCamera camera;
  RenderBuffer *render_buffer;
  Entity *entities;
  ControllerState *controller;
  PushAllocator temp_allocator;
  PushAllocator perm_allocator;
  V2 player_direction;
  V2 pointer_screen_p;
  V3 pointer_world_p;
  V2 collision_normal;
  float force_scale;
  float seconds_elapsed;
  float width;
  float height;
  float time_remaining;
  u32 game_ticks;
  u32 num_entities;
  u32 max_entities;
  V4 player_color;
  V4 cursor_color;
  u32 shooting;
};


static inline V2 to_pixel_coordinate(V2 p, int buffer_height) {
  p *= PIXELS_PER_METER;
  p.y = buffer_height - p.y;
  p = round(p);
  return p;
}

static inline V2 to_game_coordinate(V2 p, int game_height) {
  p += vec2(0.5);
  p /= PIXELS_PER_METER;
  p.y = game_height - p.y;
  return p;
}

#endif // _NAR_GAME_H_
