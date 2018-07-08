#ifndef _NAR_GAME_H_
#define _NAR_GAME_H_


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
  BUTTON_LEFT  = 0,
  BUTTON_RIGHT = 1,
  BUTTON_UP    = 2,
  BUTTON_DOWN  = 3,
  BUTTON_QUIT  = 4,
  BUTTON_MOUSE_LEFT = 5,
  BUTTON_MOUSE_RIGHT = 6,
  BUTTON_MOUSE_MIDDLE = 7,
  BUTTON_DEBUG_TOGGLE = 8,
  BUTTON_MAX   = 8,
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
struct ControllerState {
  ButtonState buttons[BUTTON_MAX];
  float pointer_x; // TODO switch to vector probably
  float pointer_y;
  float delta_t;
  bool pointer_moved;
  uint8_t ticks;
};

struct GameMemory {
  void *permanent_store;
  uint64_t permanent_size;
  void *temporary_store;
  uint64_t temporary_size;
  bool initialized;
};

enum BitmapID : uint32_t {
  BITMAP_INVALID,

  BITMAP_FACE,
  BITMAP_TEST_SPRITE,
  BITMAP_BACKGROUND,
  BITMAP_WALL,
  BITMAP_WHITE,

  BITMAP_COUNT
};

struct VisualInfo {
  //PixelBuffer texture;
  V4 color;
  V3 offset;
  float sprite_height;
  float scale;
  BitmapID texture_id;
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
  ENTITY_TEXTURE_REPEAT = (1 << 10)
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

struct FontInfo {
  PixelBuffer bitmap;
  stbtt_bakedchar *baked_chars;
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
// TODO either typedef my own bakechar or move this to the custom_stbtt file
static inline stbtt_bakedchar *get_baked_char(FontInfo *font, char c);


// TODO use this for temporary storage management
struct TemporaryState {
};

struct GameState {
  GameAssets assets;
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
  p += V2{0.5,0.5};
  p /= PIXELS_PER_METER;
  p.y = game_height - p.y;
  return p;
}

#endif // _NAR_GAME_H_
