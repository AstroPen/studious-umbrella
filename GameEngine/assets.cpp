
static inline PixelBuffer *get_bitmap(GameAssets *assets, BitmapID id) {
  assert(id);
  assert(id < BITMAP_COUNT);
  auto result = assets->bitmaps + id;
  if (!result->buffer) return NULL;
  return result;
}

static inline u32 get_texture_id(GameAssets *assets, BitmapID id) {
  assert(id);
  assert(id < BITMAP_COUNT);
  auto texture = assets->bitmaps + id;
  if (!texture->buffer) return 0;
  return texture->texture_id;
}

static inline PixelBuffer *get_bitmap_location(GameAssets *assets, BitmapID id) {
  assert(id);
  assert(id < BITMAP_COUNT);
  auto result = assets->bitmaps + id;
  return result;
}

// TODO some of these getters should be made safe in case the group/layout does not exist
static inline TextureGroup *get_texture_group(GameAssets * assets, TextureGroupID id) {
  assert(id);
  assert(id < TEXTURE_GROUP_COUNT);
  auto result = assets->texture_groups + id;
  if (result->bitmap.buffer) return result;
  return NULL;
}

static inline TextureLayout *get_layout(GameAssets *assets, TextureLayoutType id) {
  assert(id);
  assert(id < LAYOUT_COUNT);
  auto result = assets->texture_layouts + id;
  return result;
}

static V4 get_sprite_uv(TextureGroup *group, int sprite_index, bool reversed) {
  if (sprite_index < 0) return {};

  u32 h_count = group->bitmap.width / group->sprite_width;
  u32 v_count = group->bitmap.height / group->sprite_height;
  u32 row_idx = sprite_index / h_count;
  assert(row_idx < v_count);
  // TODO consider using modf or remainder from the c lib
  u32 col_idx = sprite_index % h_count;

  float umin = float(col_idx) / float(h_count);
  assert(umin >= 0 && umin <= 1);
  float umax = float(col_idx + 1) / float(h_count);
  assert(umax >= 0 && umax <= 1);

  float vmin = float(row_idx) / float(v_count);
  assert(vmin >= 0 && vmin <= 1);
  float vmax = float(row_idx + 1) / float(v_count);
  assert(vmax >= 0 && vmax <= 1);

  if (reversed) return vec4(umax, vmin, umin, vmax);
  return vec4(umin, vmin, umax, vmax);
}

static AnimationType get_current_animation(Entity *e) {
  if (non_zero(e->vel)) {
    return ANIM_MOVE;
  }
  return ANIM_IDLE;
}

enum AnimationFlags {
  ANIMATION_LOOPING = 1,
  ANIMATION_REVERSABLE = (1 << 1),
};

struct AnimationState {
  AnimationType type;
  Direction direction;
  float t;
  u32 flags;
};

static AnimationState get_current_animation_state(Entity *e) {
  AnimationState result = {};
  result.type = e->current_animation;
  result.flags |= ANIMATION_LOOPING;
  result.direction = e->facing_direction;
  result.t = e->animation_dt;

  return result;
}

#if 0
// TODO Figure out how I want the animation state machine to work.
// Right now, I'm thinking that there should be many state machines : one for 
// each property I care about. In other words, one for the pressed direction, 
// one for the moving direction, and more for others. On top of that, I want 
// to record a speed value as a float. 

static AnimationState get_current_animation_state(Entity *e) {
  AnimationState result = {};
  result.type = ANIM_IDLE;
  result.direction = e->facing_direction;
  if (result.direction == RIGHT || result.direction == LEFT) result.flags |= ANIMATION_REVERSABLE;

  if (length_sq(e->vel) > 1.0) {
    result.type = ANIM_MOVE;
    result.flags |= ANIMATION_LOOPING;
  }
  result.t = e->animation_dt;

  return result;
}

namespace Animator {
  enum StateTransition {
    NO_TRANSITION,
    STOP_MOVING,
    START_MOVING,
    FULL_SPEED,
    TURN_AROUND,
    HALF_TURN,
    COLLIDED,
    PUSH_WALL,
  };

  enum PressState {
    IDLE,
    ,
    RUNNING,
    SLOWING,
  };

}

// TODO replace facing direction with this under the name "pressed"
enum class DirectionState : u8 {
  NONE,
  LEFT, RIGHT,
  UP, DOWN
};

struct AnimationState {
};


struct StateTransition {
  V3 da, dv;
  Collision collision;
  float dt;
  u32 flags; // ???
};

static DirectionState to_direction_state(V2 velocity) {
  if (length_sq(velocity) < 0.5) return DirectionState::NONE;

  float dirs[4];
  dirs[DirectionState::LEFT - 1] = -velocity.x;
  dirs[DirectionState::RIGHT - 1] = velocity.x;
  dirs[DirectionState::UP - 1] = velocity.y;
  dirs[DirectionState::DOWN - 1] = -velocity.y;

  u8 result = 0;
  if (dirs[1] > dirs[result]) result = 1;
  if (dirs[2] > dirs[result]) result = 2;
  if (dirs[3] > dirs[result]) result = 3;

  return DirectionState(result);
}

static void update_state(Entity *e, StateTransition *transition) {
  auto old_pressed = e->pressed;
  auto old_moving = to_direction_state(e->vel);
  float speed = length(e->vel);
}

static void update_animation(Entity *e, V3 da, V3 dv, float dt) {
  //e->animation_state; // Old state
  //update_animation_state(graph, animation, transition);

  auto transition = animator::NO_TRANSITION;
  auto a = e->acc;
  auto v = e->vel;
  bool was_moving;
  bool is_moving;
  bool was_accelerating;
  bool is_accelerating;
  bool collided;
  if (length_sq(a)) 
  if ()
  e->animation_dt += dt;
}
#endif

static float get_animation_duration(GameAssets *assets, Entity *e) {
  auto group_id = e->texture_group_id;
  auto group = get_texture_group(assets, group_id);
  auto layout_type = group->layout;
  auto layout = get_layout(assets, layout_type);
  auto animation = e->current_animation;
  auto dir = e->facing_direction;
  float duration = layout->animation_times[animation][dir];
  if (!duration) duration = layout->animation_times[animation][flip_direction(dir)];
  return duration;
}

static int get_sprite_index(TextureLayout *layout, AnimationState anim) {
  // TODO handle left/right reflection somehow
 
  auto frame_count = layout->animation_frame_counts[anim.type][anim.direction];
  if (!frame_count) return -1;

  float duration = layout->animation_times[anim.type][anim.direction];
  assert(duration >= 0);
  if (anim.flags & ANIMATION_LOOPING && duration < anim.t) anim.t -= duration;

  float normalized_dt = anim.t / duration;
  u16 frame_index = lroundf(normalized_dt * frame_count);
  if (frame_index >= frame_count) frame_index = frame_count - 1;

  auto start_index = layout->animation_start_index[anim.type][anim.direction];
  
  return start_index + frame_index;
}

static bool has_normal_map(TextureGroup *group) {
  return group->has_normal_map;
}

static V4 get_color(Entity *e) {
  return e->visual.color;
}
static float get_sprite_scale(Entity *e) {
  return e->visual.scale;
}
static V3 get_sprite_offset(TextureGroup *group) {
  return group->sprite_offset;
}
static float get_sprite_depth(TextureGroup *group, int sprite_index) {
  return group->sprite_depth;
}

#define LINK_RUN_SPEED 19.0f

static RenderingInfo get_render_info(GameAssets *assets, Entity *e) {
  TextureGroup *group = get_texture_group(assets, e->texture_group_id);
  if (!group) return {};

  switch (group->layout) {
    case LAYOUT_CHARACTER : {
      TextureLayout *layout = get_layout(assets, group->layout);

      auto anim_state = get_current_animation_state(e);

      bool reversed = false;
      int sprite_index = get_sprite_index(layout, anim_state);
      if (sprite_index < 0 && anim_state.flags | ANIMATION_REVERSABLE) {
        anim_state.direction = flip_direction(anim_state.direction);
        sprite_index = get_sprite_index(layout, anim_state);
        if (sprite_index >= 0) reversed = true;
      }
      if (sprite_index < 0) sprite_index = 0; // TODO I'm not sure what I should do for this...

      RenderingInfo result = {};
      result.bitmap_id = group->render_id;
      result.texture_uv = get_sprite_uv(group, sprite_index, reversed);
      if (has_normal_map(group)) 
        result.normal_map_uv = get_sprite_uv(group, sprite_index + group->sprite_count / 2, reversed);
      result.color = get_color(e);
      result.offset = get_sprite_offset(group);
      result.sprite_depth = get_sprite_depth(group, sprite_index);
      result.scale = get_sprite_scale(e);
      result.width = group->sprite_width;
      result.height = group->sprite_height;
      result.texture_width = group->bitmap.width;
      result.texture_height = group->bitmap.height;

      return result;
    } break;

    default : assert(!"Invalid texture layout.");
  }

  return {};
}
#if 0
// TODO Delete this when it is no longer needed for reference
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
  DOWN
};

enum AnimationType {
  ANIM_IDLE,
  ANIM_MOVE,

  ANIM_COUNT
};

struct TextureLayout {
  u16 animation_frame_counts[ANIM_COUNT];
  u16 animation_start_index[ANIM_COUNT][4]; // 4 is for direction count
  float animation_times[ANIM_COUNT];
};
#endif

#include "packed_assets.h"

static void unpack_assets(GameAssets *assets) {

  // TODO pass in allocator probably
  PushAllocator temporary_ = new_push_allocator(2048*128);
  auto temporary = &temporary_;
  assert(is_initialized(temporary));
  // TODO once files get large, this needs to be a stream
  auto file_buffer = read_entire_file("assets/packed_assets.pack", temporary, 64);
  assert(file_buffer);

  auto header = (PackedAssetHeader *)file_buffer;

  assert(header->magic == 'PACK');
  assert(header->version == 0);
  assert(header->total_size <= temporary->bytes_allocated);

  u32 layout_count = header->layout_count;
  u32 texture_group_count = header->texture_group_count;
  uint8_t *data = file_buffer + header->data_offset;

  assert(file_buffer == temporary->memory); // TODO delete these :
  assert(header->total_size == temporary->bytes_allocated);
  assert(layout_count == 2);
  assert(texture_group_count == 2);

  // NOTE : This only works if the structs are tightly packed and aligned properly.
  file_buffer += sizeof(PackedAssetHeader);

  // For each layout type :
  for (u32 lt = 0; lt < layout_count; lt++) {
    auto packed_layout = (PackedTextureLayout *) file_buffer;
    u32 layout_type = packed_layout->layout_type;

    ASSERT(layout_type >= 0 && layout_type < LAYOUT_COUNT);
    TextureLayout *layout = assets->texture_layouts + layout_type;
    file_buffer += sizeof(PackedTextureLayout);

    if (layout_type == LAYOUT_CHARACTER) {
      u32 animation_count = packed_layout->animation_count;

      // For each animation :
      for (u32 an = 0; an < animation_count; an++) {
        auto packed_animation = packed_layout->animations + an;
        auto animation_type = packed_animation->animation_type;
        auto frame_count = packed_animation->frame_count;
        auto duration = packed_animation->duration;
        auto direction = packed_animation->direction;
        auto start_index = packed_animation->start_index;

        layout->animation_frame_counts[animation_type][direction] = frame_count;
        layout->animation_times[animation_type][direction] = duration;
        layout->animation_start_index[animation_type][direction] = start_index;

        file_buffer += sizeof(PackedAnimation);
      }
    } else {
      ASSERT_EQUAL(layout_type, LAYOUT_TERRAIN);
      //u32 face_count = packed_layout->face_count;
      // TODO set the unspecified ones to some sort of default?
      array_copy(packed_layout->faces->sprite_index, layout->cube_face_index, FACE_COUNT);

      file_buffer += sizeof(PackedFaces);
    }
  }

  // For each texture group :
  for (u32 tg = 0; tg < texture_group_count; tg++) {
    auto packed_group = (PackedTextureGroup *) file_buffer;
    TextureGroupID group_id = (TextureGroupID) packed_group->texture_group_id;
    ASSERT(group_id > TEXTURE_GROUP_INVALID && group_id < TEXTURE_GROUP_COUNT);

    // TODO delete these:
    if (tg == 0) ASSERT_EQUAL(packed_group->s_clamp, CLAMP_TO_EDGE);
    if (tg == 1) ASSERT_EQUAL(packed_group->s_clamp, REPEAT_CLAMPING);


    TextureGroup *group = assets->texture_groups + group_id;
    // TODO FIXME We cant free the buffer because we point into it here
    group->bitmap.buffer = data + packed_group->bitmap_offset;
    group->bitmap.width = packed_group->width;
    group->bitmap.height = packed_group->height;
    // TODO 0 is actually legal, just not implemented yet
    ASSERT(packed_group->layout_type > 0 && packed_group->layout_type < LAYOUT_COUNT);
    group->layout = (TextureLayoutType) packed_group->layout_type;
    group->sprite_width = packed_group->sprite_width;
    group->sprite_height = packed_group->sprite_height;
    group->sprite_depth = packed_group->sprite_depth;
    group->render_id = 0; // init in OpenGL with init_texture
    group->sprite_count = packed_group->sprite_count;
    group->sprite_offset.x = packed_group->offset_x;
    group->sprite_offset.y = packed_group->offset_y;
    group->sprite_offset.z = packed_group->offset_z;
    group->has_normal_map = (packed_group->flags & GROUP_HAS_NORMAL_MAP) != 0;

    TextureParameters param = default_texture_parameters;
    param.pixel_format = RGBA;
    param.min_blend = (TextureFormatSpecifier) packed_group->min_blend;
    param.max_blend = (TextureFormatSpecifier) packed_group->max_blend;
    param.s_clamp = (TextureFormatSpecifier) packed_group->s_clamp;
    param.t_clamp = (TextureFormatSpecifier) packed_group->t_clamp;
    init_texture(assets, group_id, param);

    file_buffer += sizeof(PackedTextureGroup);
  }
}

struct LoadBitmapWork {
  GameAssets *assets;
  char const *filename;
  PixelBuffer *pixel_buffer;
};

void do_load_bitmap_work(LoadBitmapWork *work) {

  *work->pixel_buffer = load_image_file(work->filename);
  free_element(&work->assets->work_allocator, work);
}

// TODO make some decisions about the api here...
static inline void load_bitmap(GameAssets *assets, BitmapID id) {
  LoadBitmapWork *work = alloc_element(&assets->work_allocator, LoadBitmapWork);
  assert(work);
  if (!work) return;
  assert(id);
  assert(id < BITMAP_COUNT);

  switch (id) {
    case BITMAP_INVALID : {
      assert(!"Invalid BitmapID load.");
      return;
    } break;
    case BITMAP_FACE : {
      work->filename = "assets/face.png";
    } break;
    case BITMAP_TEST_SPRITE : {
      work->filename = "assets/test_sprite.png";
    } break;
    case BITMAP_LINK : {
      work->filename = "assets/lttp_link.png";
    } break;
    case BITMAP_LINK_NORMAL_MAP : {
      work->filename = "assets/lttp_normal_map.png";
    } break;
    case BITMAP_TEST_SPRITE_NORMAL_MAP : {
      work->filename = "assets/test_sprite_normal_map.png";
    } break;
    case BITMAP_BACKGROUND : {
      assert(!"Invalid BitmapID load.");
      return;
    } break;
    case BITMAP_WALL : {
      work->filename = "assets/wall_texture.png";
    } break;
    case BITMAP_WALL_NORMAL_MAP : {
      work->filename = "assets/wall_normal_map.png";
    } break;
    case BITMAP_WHITE : {
      assert(!"Invalid BitmapID load.");
      return;
    } break;

    default : {
      assert(!"Invalid BitmapID load.");
      return;
    } break;
  }

  work->pixel_buffer = get_bitmap_location(assets, id);
  work->assets = assets;
  //*pixel_buffer = load_image_file(work.filename);
  push_work(assets->work_queue, work, (work_queue_callback *) do_load_bitmap_work);
}

static inline FontInfo *get_font(GameAssets *assets, u32 font_id) {
  auto font = assets->fonts + font_id;
  if (font->baked_chars) return font;
  //assert(!"Font not loaded.");
  return NULL;
}

static inline FontInfo *get_font_location(GameAssets *assets, u32 font_id) {
  auto font = assets->fonts + font_id;
  return font;
}

static inline BakedChar *get_baked_char(FontInfo *font, char c) {
  char first_char = ' ';
  int index = c - first_char;
  assert(font->baked_chars);
  auto result = font->baked_chars + index;
  return result;
}

// TODO Baking text should happen offline, or before the first frame during developement.
// Additionally, the temporary storeage needed on the cpu should use a SwapAllocator 
// to handle all asset loading concurrently.
static FontInfo load_font_file(const char* filename, u32 text_height, 
                               PushAllocator *perm_allocator, PushAllocator *temp_allocator) {

  // TODO test for different alignments and speed gains/losses
  int alignment = 64;
  auto file_buffer = read_entire_file(filename, temp_allocator, alignment);
  if (!file_buffer) return {};

  // NOTE : We want space to ~, which is 32 to 126
  // 126 - 32 = 95 glyphs
  // sqrt(94) ~ 10
  u32 bitmap_dim = next_power_2(10 * text_height);
  int first_glyph = ' ';
  int num_glyphs = 95;
  // TODO test for different alignments and speed gains/losses

  int pixel_bytes = 1;
  auto bitmap = alloc_texture(perm_allocator, bitmap_dim, bitmap_dim, 1);
  if (!bitmap.buffer) {
    assert(!"Failed to allocate font bitmap.");
    return {};
  }

  auto baked_chars = ALLOC_ARRAY(perm_allocator, BakedChar, num_glyphs + 1);
  if (!baked_chars) {
    assert(!"Failed to allocate baked_chars.");
    return {};
  }

  // if return is positive, the first unused row of the bitmap
  // if return is negative, returns the negative of the number of characters that fit
  // if return is 0, no characters fit and no rows were used
  int result = bake_font_bitmap(file_buffer, 0, text_height, bitmap.buffer, bitmap_dim, bitmap_dim, first_glyph, num_glyphs, baked_chars);
  if (result <= 0) {
    assert(!"Failed to bake font bitmap.");
    return {};
  } else {
    int first_unused_row = result;
    if (first_unused_row < bitmap_dim / 2) {
      u32 new_height = next_power_2(first_unused_row);
      assert(new_height <= bitmap_dim);
      if (new_height < bitmap_dim) {
        bitmap.height = new_height;
        pop_size(perm_allocator, (bitmap_dim - new_height) * bitmap_dim * pixel_bytes);
      }
    }
  }

  // TODO separate this out to a "init font" call
  u32 font_id;
  glGenTextures(1, &font_id);
  assert(font_id);
  glBindTexture(GL_TEXTURE_2D, font_id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, bitmap.width, bitmap.height, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap.buffer);
  GLint swizzle_mask[] = {GL_RED, GL_RED, GL_RED, GL_RED};
  glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle_mask);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  FontInfo info;
  info.bitmap = bitmap;
  info.bitmap.texture_id = font_id;
  info.baked_chars = baked_chars;

  return info;
}

#if 0
struct LoadFontWork {
  GameAssets *assets;
  char const *filename;
  FontInfo *font_info;
};

void do_load_font_work(LoadFontWork *work) {

  *work->font_info = load_font_file(work->filename);
  free_element(&work->assets->work_allocator, work);
}

static inline void load_font(GameAssets *assets, FontID id) {
  LoadFontWork *work = alloc_element(&assets->work_allocator, LoadFontWork);
  assert(work);
  if (!work) return;
  assert(id);
  assert(id < FONT_COUNT);

  switch (id) {
    case FONT_INVALID : {
      assert(!"Invalid FontID load.");
      return;
    } break;
    case FONT_ARIAL : {
      work->filename = "/Library/Fonts/Arial.ttf";
    } break;
    case FONT_COURIER_NEW_BOLD : {
      work->filename = "/Library/Fonts/Courier New Bold.ttf";
    } break;

    default : {
      assert(!"Invalid FontID load.");
      return;
    } break;
  }

  work->font_info = get_font_location(assets, id);
  push_work(assets->work_queue, work, (work_queue_callback *) do_load_font_work);
}

#else
// NOTE : single threaded version
static inline void load_font(GameAssets *assets, PushAllocator *perm_allocator, PushAllocator *temp_allocator, FontID id, int pixel_height = 20) {
  assert(id);
  assert(id < FONT_COUNT);

  char const *filename = NULL;

  switch (id) {
    case FONT_INVALID : {
      assert(!"Invalid FontID load.");
      return;
    } break;
    case FONT_ARIAL : {
      filename = "/Library/Fonts/Arial.ttf";
    } break;
    case FONT_COURIER_NEW_BOLD : {
      filename = "/Library/Fonts/Courier New Bold.ttf";
    } break;
    case FONT_DEBUG : {
      filename = "/Library/Fonts/Courier New Bold.ttf";
    } break;

    default : {
      assert(!"Invalid FontID load.");
      return;
    } break;
  }

  auto font = get_font_location(assets, id);
  *font = load_font_file(filename, pixel_height, perm_allocator, temp_allocator);
}
#endif

// TODO move this somewhere reasonable
static void draw_circle_asset(uint8_t *texture_buf, uint8_t *normal_buf, u32 diameter) {
  float radius = diameter / 2.0f;
  V2 center = vec2(radius);
  float radius_sq = radius * radius;

  u32 pixel_bytes = 4;
  u32 pitch = pixel_bytes * diameter;

  for (u32 y = 0; y < diameter; y++) {
    u32 row = pitch * y;
    for (u32 x = 0; x < diameter; x++) {
      Color color = 0;
      Color ncolor = 0;

      V2 p = vec2(x,y) + vec2(0.5);

      V2 diff = p - center;
      float len_sq = length_sq(diff);
      if (len_sq <= radius_sq) {
        color = 0xffffffff;
        V3 normal;
        // c^2 = a^2 + b^2
        // radius_sq = len_sq + b^2
        float depth_sq = radius_sq - len_sq;
        normal.x = diff.x;
        normal.y = diff.y;
        normal.z = -sqrt(depth_sq);
        normal += vec3(radius);
        normal /= diameter;
        // TODO include depth information
        ncolor = to_color(vec4(normal, 1));
      }

      u32 index = row + x * pixel_bytes;
      u32 *pixel = (u32 *) (texture_buf + index);
      u32 *normal_pixel = (u32 *) (normal_buf + index);
      *pixel = color;
      *normal_pixel = ncolor;
    }
  }
}

static inline void init_assets(GameState *g, WorkQueue *queue, RenderBuffer *render_buffer) {
  auto assets = &g->assets;
  render_buffer->assets = assets;
  assets->work_queue = queue;
  assets->work_allocator = create_heap(&g->perm_allocator, 16, LoadBitmapWork);
  assert(assets->work_allocator.free_list);
  assert(assets->work_allocator.count == 16);
  assert(assets->work_allocator.element_size == sizeof(LoadBitmapWork));


  load_bitmap(assets, BITMAP_FACE);
  load_bitmap(assets, BITMAP_TEST_SPRITE);
  load_bitmap(assets, BITMAP_TEST_SPRITE_NORMAL_MAP);
  load_bitmap(assets, BITMAP_LINK);
  load_bitmap(assets, BITMAP_LINK_NORMAL_MAP);
  load_bitmap(assets, BITMAP_WALL);
  load_bitmap(assets, BITMAP_WALL_NORMAL_MAP);
  

  TextureParameters background_param = default_texture_parameters;
  background_param.max_blend = LINEAR_BLEND;
  background_param.min_blend = LINEAR_BLEND;

  auto background_texture = get_bitmap_location(assets, BITMAP_BACKGROUND);
  *background_texture = alloc_texture(&g->perm_allocator, render_buffer->screen_width, render_buffer->screen_height);
  init_texture(assets, BITMAP_BACKGROUND, background_param);


  auto white_texture = get_bitmap_location(assets, BITMAP_WHITE);
  *white_texture = alloc_texture(&g->perm_allocator, 1, 1);
  auto white_pixel = (u32 *) white_texture->buffer;
  *white_pixel = 0xffffffff;
  init_texture(assets, BITMAP_WHITE);

  u32 circle_diameter = 128;
  auto circle_texture = get_bitmap_location(assets, BITMAP_CIRCLE);
  *circle_texture = alloc_texture(&g->perm_allocator, circle_diameter, circle_diameter);
  auto sphere_normal_map = get_bitmap_location(assets, BITMAP_SPHERE_NORMAL_MAP);
  *sphere_normal_map = alloc_texture(&g->perm_allocator, circle_diameter, circle_diameter);
  draw_circle_asset(circle_texture->buffer, sphere_normal_map->buffer, circle_diameter);
  TextureParameters circle_param = default_texture_parameters;
  circle_param.max_blend = LINEAR_BLEND;
  circle_param.min_blend = LINEAR_BLEND;
  init_texture(assets, BITMAP_CIRCLE, circle_param);
  init_texture(assets, BITMAP_SPHERE_NORMAL_MAP, circle_param);

  int pixel_height = 30;

  auto temp_allocator = push_temporary(&g->temp_allocator);
  load_font(assets, &g->perm_allocator, &temp_allocator, FONT_ARIAL, pixel_height);
  clear(&temp_allocator);
  load_font(assets, &g->perm_allocator, &temp_allocator, FONT_COURIER_NEW_BOLD, pixel_height);
  clear(&temp_allocator);
  load_font(assets, &g->perm_allocator, &temp_allocator, FONT_DEBUG, 19);
  clear(&temp_allocator);
  pop_temporary(&g->temp_allocator, &temp_allocator);

  complete_all_work(assets->work_queue);

  TextureParameters param;
  param.pixel_format = RGBA;
  init_texture(assets, BITMAP_FACE, param);
  init_texture(assets, BITMAP_TEST_SPRITE, param);
  init_texture(assets, BITMAP_TEST_SPRITE_NORMAL_MAP, param);
  init_texture(assets, BITMAP_LINK, param);
  init_texture(assets, BITMAP_LINK_NORMAL_MAP, param);

  param.s_clamp = REPEAT_CLAMPING;
  param.t_clamp = REPEAT_CLAMPING;
  param.max_blend = LINEAR_BLEND;
  param.min_blend = LINEAR_BLEND;
  init_texture(assets, BITMAP_WALL, param);
  init_texture(assets, BITMAP_WALL_NORMAL_MAP, param);

  unpack_assets(assets);
}


