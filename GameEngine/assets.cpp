
static inline PixelBuffer *get_bitmap(GameAssets *assets, BitmapID id) {
  assert(id);
  assert(id < BITMAP_COUNT);
  auto result = assets->bitmaps + id;
  if (!result->buffer) return NULL;
  return result;
}

static inline uint32_t get_texture_id(GameAssets *assets, BitmapID id) {
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
static inline TextureGroup *get_texure_group(GameAssets * assets, TextureGroupID id) {
  assert(id);
  assert(id < TEXTURE_GROUP_COUNT);
  auto result = assets->texture_groups + id;
  return result;
}

static inline TextureLayout *get_layout(GameAssets *assets, TextureLayoutType id) {
  assert(id);
  assert(id < LAYOUT_COUNT);
  auto result = assets->texture_layouts + id;
  return result;
}

static V4 get_sprite_uv(TextureGroup *group, int sprite_index) {
  if (sprite_index < 0) return {};

  uint32_t h_count = group->bitmap.width / group->sprite_width;
  uint32_t v_count = group->bitmap.height / group->sprite_height;
  uint32_t row_idx = sprite_index / h_count;
  assert(row_idx < v_count);
  uint32_t col_idx = sprite_index % h_count;

  float umin = float(col_idx) / float(h_count);
  assert(umin >= 0 && umin <= 1);
  float umax = float(col_idx + 1) / float(h_count);
  assert(umax >= 0 && umax <= 1);

  float vmin = float(row_idx) / float(v_count);
  assert(vmin >= 0 && vmin <= 1);
  float vmax = float(row_idx + 1) / float(v_count);
  assert(vmax >= 0 && vmax <= 1);

  return vec4(umin, vmin, umax, vmax);
}

static AnimationType get_current_animation(Entity *e) {
  if (non_zero(e->vel)) {
    return ANIM_MOVE;
  }
  return ANIM_IDLE;
}


static int get_sprite_index(TextureLayout *layout, AnimationType animation, Direction dir, float dt) {
  // TODO handle left/right reflection somehow
  
  auto frame_count = layout->animation_frame_counts[animation];
  if (!frame_count) return -1;

  float animation_time = layout->animation_times[animation];

  float normalized_dt = dt / animation_time;
  uint16_t frame_index = lroundf(normalized_dt * frame_count);
  if (frame_index >= frame_count) frame_index = frame_count - 1;

  auto start_index = layout->animation_start_index[animation][dir];
  
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

static RenderingInfo get_render_info(GameAssets *assets, Entity *e) {
  TextureGroup *group = get_texure_group(assets, e->texture_group_id);
  switch (group->layout) {
    case LAYOUT_CHARACTER : {
      TextureLayout *layout = get_layout(assets, group->layout);

      Direction dir = e->facing_direction; //get_facing_direction(e);
      AnimationType animation = get_current_animation(e);
      float t = e->animation_dt; //get_current_animation_time(e);

      int sprite_index = get_sprite_index(layout, animation, dir, t);

      RenderingInfo result = {};
      result.bitmap_id = group->render_id;
      result.texture_uv = get_sprite_uv(group, sprite_index);
      if (has_normal_map(group)) 
        result.normal_map_uv = get_sprite_uv(group, sprite_index + group->sprite_count / 2);
      result.color = get_color(e);
      result.offset = get_sprite_offset(group);
      result.sprite_depth = get_sprite_depth(group, sprite_index);
      result.scale = get_sprite_scale(e);

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
#endif

static void unpack_assets(GameAssets *assets) {
  // TODO Implement this
  // Stream in "assets/game_assets.packed_assets"
 
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
      work->filename = "face.png";
    } break;
    case BITMAP_TEST_SPRITE : {
      work->filename = "test_sprite.png";
    } break;
    case BITMAP_LINK : {
      work->filename = "lttp_link.png";
    } break;
    case BITMAP_LINK_NORMAL_MAP : {
      work->filename = "lttp_normal_map.png";
    } break;
    case BITMAP_TEST_SPRITE_NORMAL_MAP : {
      work->filename = "test_sprite_normal_map.png";
    } break;
    case BITMAP_BACKGROUND : {
      assert(!"Invalid BitmapID load.");
      return;
    } break;
    case BITMAP_WALL : {
      work->filename = "wall_texture.png";
    } break;
    case BITMAP_WALL_NORMAL_MAP : {
      work->filename = "wall_normal_map.png";
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

static inline FontInfo *get_font(GameAssets *assets, uint32_t font_id) {
  auto font = assets->fonts + font_id;
  if (font->baked_chars) return font;
  //assert(!"Font not loaded.");
  return NULL;
}

static inline FontInfo *get_font_location(GameAssets *assets, uint32_t font_id) {
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
static FontInfo load_font_file(const char* filename, uint32_t text_height, 
                               PushAllocator *perm_allocator, PushAllocator *temp_allocator) {

  // TODO test for different alignments and speed gains/losses
  int alignment = 64;
  auto file_buffer = read_entire_file(filename, temp_allocator, alignment);
  if (!file_buffer) return {};

  // NOTE : We want space to ~, which is 32 to 126
  // 126 - 32 = 95 glyphs
  // sqrt(94) ~ 10
  uint32_t bitmap_dim = next_power_2(10 * text_height);
  int first_glyph = ' ';
  int num_glyphs = 95;
  // TODO test for different alignments and speed gains/losses

  int pixel_bytes = 1;
  auto bitmap = alloc_texture(perm_allocator, bitmap_dim, bitmap_dim, 1);
  if (!bitmap.buffer) {
    assert(!"Failed to allocate font bitmap.");
    return {};
  }

  auto baked_chars = alloc_array(perm_allocator, BakedChar, num_glyphs + 1);
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
      uint32_t new_height = next_power_2(first_unused_row);
      assert(new_height <= bitmap_dim);
      if (new_height < bitmap_dim) {
        bitmap.height = new_height;
        pop_size(perm_allocator, (bitmap_dim - new_height) * bitmap_dim * pixel_bytes);
      }
    }
  }

  // TODO separate this out to a "init font" call
  uint32_t font_id;
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
static void draw_circle_asset(uint8_t *texture_buf, uint8_t *normal_buf, uint32_t diameter) {
  float radius = diameter / 2.0f;
  V2 center = vec2(radius);
  float radius_sq = radius * radius;

  uint32_t pixel_bytes = 4;
  uint32_t pitch = pixel_bytes * diameter;

  for (uint32_t y = 0; y < diameter; y++) {
    uint32_t row = pitch * y;
    for (uint32_t x = 0; x < diameter; x++) {
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

      uint32_t index = row + x * pixel_bytes;
      uint32_t *pixel = (uint32_t *) (texture_buf + index);
      uint32_t *normal_pixel = (uint32_t *) (normal_buf + index);
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
  auto white_pixel = (uint32_t *) white_texture->buffer;
  *white_pixel = 0xffffffff;
  init_texture(assets, BITMAP_WHITE);

  uint32_t circle_diameter = 128;
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
  init_texture(assets, BITMAP_WALL, param);
  init_texture(assets, BITMAP_WALL_NORMAL_MAP, param);
}


