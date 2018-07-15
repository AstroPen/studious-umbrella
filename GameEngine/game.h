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



enum ButtonType : uint8_t {
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
  
  BUTTON_DEBUG_CAMERA_UP,
  BUTTON_DEBUG_CAMERA_DOWN,
  BUTTON_DEBUG_CAMERA_LEFT,
  BUTTON_DEBUG_CAMERA_RIGHT,

  BUTTON_DEBUG_CAMERA_IN,
  BUTTON_DEBUG_CAMERA_OUT,
  BUTTON_DEBUG_CAMERA_TILT_UP,
  BUTTON_DEBUG_CAMERA_TILT_DOWN,

  BUTTON_MAX,
  BUTTON_NONE  = 0xff
};

enum ButtonEventType : uint8_t {
  NO_EVENTS = 0,
  PRESS_EVENT = 1,
  RELEASE_EVENT = 2
};

struct ButtonState {
  // NOTE this is a little redundant, but it makes things less complicated
  ButtonEventType first_press;
  ButtonEventType last_press;
  uint8_t num_transitions;
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
  uint64_t permanent_size;
  void *temporary_store;
  uint64_t temporary_size;
};

struct RenderBuffer;

struct GameInput {
  ControllerState controller;
  RenderBuffer *render_buffer;
  WorkQueue *work_queue;
  float delta_t;
  uint8_t ticks;
};

enum BitmapID : uint32_t {
  BITMAP_INVALID,

  BITMAP_FACE,
  BITMAP_TEST_SPRITE,
  BITMAP_TEST_SPRITE_NORMAL_MAP,
  BITMAP_BACKGROUND,
  BITMAP_WALL,
  BITMAP_WALL_NORMAL_MAP,
  BITMAP_WHITE,
  BITMAP_CIRCLE,
  BITMAP_SPHERE_NORMAL_MAP,

  BITMAP_COUNT
};

struct VisualInfo {
  //PixelBuffer texture;
  V4 color;
  V3 offset;
  float sprite_height;
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
  uint64_t flags;
  // TODO make collsion more general, and 3D
  //Shape collision_box;
  AlignedBox collision_box;
  VisualInfo visual;
  V3 vel;
  V3 acc;

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

// TODO move asset stuff to assets.h file
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

struct GameAssets {
  WorkQueue *work_queue;
  HeapAllocator work_allocator;
  PixelBuffer bitmaps[BITMAP_COUNT];
  FontInfo fonts[FONT_COUNT];
  // TODO arrayed assets
  // TODO structured assets
};


static inline PixelBuffer *get_bitmap(GameAssets *assets, BitmapID id);
static inline PixelBuffer *get_bitmap_location(GameAssets *assets, BitmapID id);
static inline FontInfo *get_font(GameAssets *assets, uint32_t font_id);
static inline FontInfo *get_font_location(GameAssets *assets, uint32_t font_id);
static inline BakedChar *get_baked_char(FontInfo *font, char c);


// TODO use this for temporary storage management
struct TemporaryState {
};

struct GameCamera {
  V3 p;
  V3 target;
  V3 up;
  float aspect_ratio; // height/width
  float focal_length;
  float near_dist, far_dist;
};

struct GameState {
  GameAssets assets;
  GameCamera camera;
  Entity *entities;
  ControllerState *controller;
  PushAllocator temp_allocator;
  PushAllocator perm_allocator;
  V2 player_direction;
  V2 pointer_position;
  V2 collision_normal;
  float force_scale;
  float seconds_elapsed;
  float width;
  float height;
  float time_remaining;
  uint32_t game_ticks;
  uint32_t num_entities;
  uint32_t max_entities;
  V4 player_color;
  V4 cursor_color;
  uint32_t shooting;
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
