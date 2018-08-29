#ifndef _NAR_GL_RENDERER_CPP_
#define _NAR_GL_RENDERER_CPP_


//
// Gradient background test code ---
//

static void draw_grad_bg_internal(PixelBuffer buf, u32 game_ticks, int y_start, int y_end) {
  TIMED_FUNCTION();

  assert(buf.buffer);
  assert(buf.texture_id);
  int pixel_bytes = 4;
  int width = buf.width;
  int height = buf.height;
  uint8_t *buffer = buf.buffer;
  uint8_t ticks = game_ticks / 2;

  //assert(width > 16);
  //assert(height > 16);
  assert(y_start < height);
  if (y_end > height) y_end = height;

  int pitch = width * pixel_bytes;
  for(int y = y_start; y < y_end; y++) {
    uint8_t *row = buffer + (y * pitch);
    for(int x = 0; x < width; x++) {
      u32 *pixel = (u32 *) (row + (x * pixel_bytes));

      if(x % 16 && y % 16) {
        Color color = 0;
        color.a = 0xff;
        color.r = y + 2 * ticks;
        color.g = y + 2 * ticks;
        color.b = x + 1 * ticks;

        auto c2 = V3{(float)color.r, (float)color.g, (float)color.b} / 255.0f;

        c2 = lerp(c2, V3{0.95,0.4,0.7}, 0.5);
        c2 = normalize(c2 * c2) * length(c2);
        c2 = clamp(c2, 0, 1);
        color = to_color(c2);

        assert(color.value);
        *pixel = color.value;
      } else {
        *pixel = 0xff000000;
      }
    }
  }
}

struct DrawGradientBackgroundWork {
  PixelBuffer pixel_buffer;
  u32 game_ticks;
  int y_start;
  int y_end;
};

void do_draw_gradient_background_work(DrawGradientBackgroundWork *work) {
  draw_grad_bg_internal(work->pixel_buffer, work->game_ticks, work->y_start, work->y_end);
}

static inline void draw_gradient_background(GameState *g, WorkQueue *queue) {
  auto assets = &g->assets;
  auto background_texture = get_bitmap(assets, BITMAP_BACKGROUND);
  assert(background_texture);

  int num_splits = 8;
  auto bg_args = ALLOC_ARRAY(&g->temp_allocator, DrawGradientBackgroundWork, num_splits);
  assert(bg_args);

  int y_start = 0;
  int y_end = background_texture->height;
  int y_increment = y_end / num_splits;

  // TODO if I actually care to optimize this I could divide the bitmap into 64 byte groups
  for (int i = 0; i < num_splits - 1; i++) {
    bg_args[i].pixel_buffer = *background_texture;
    bg_args[i].game_ticks = g->game_ticks;
    bg_args[i].y_start = y_start;
    y_start += y_increment;
    bg_args[i].y_end = y_start;
    push_work(queue, bg_args + i, (work_queue_callback *) do_draw_gradient_background_work);
  }

  int i = num_splits - 1;
  bg_args[i].pixel_buffer = *background_texture;
  bg_args[i].game_ticks = g->game_ticks;
  bg_args[i].y_start = y_start;
  bg_args[i].y_end = y_end;
  push_work(queue, bg_args + i, (work_queue_callback *) do_draw_gradient_background_work);
}

//
// Renderer type definitions ---
//


#define MAX_RENDERER_VERTICES (4096 * 16)

union RenderType {
  enum VertMode : u8 {
    TRIANGLES,
  };
  enum Command : u8 {
    VERTEX,
    CLEAR,
  };

  struct {
    Command command;
    VertMode mode;
    u16 flags; // RenderInfo::Flags
  };

  u32 value;
};


struct RenderElement {
  RenderElement *next;
  RenderType type;
};

struct VertexInfo {
  int index;
  int count;
};

struct RenderElementVertex : RenderElement {
  VertexInfo vertex;
  TextureIndex texture;
  u32 normal_map; // TODO Remove dumb hack
};

struct RenderElementClear : RenderElement {
  V4 color;
  float depth;
};

inline u32 get_size(RenderType type) {
  switch (type.command) {
    case RenderType::VERTEX : return sizeof(RenderElementVertex);
    case RenderType::CLEAR : return sizeof(RenderElementClear);
    default : INVALID_SWITCH_CASE(type.command);
  }
  return 0;
}

inline TextureIndex get_texture(RenderElement *elem) {
  if (elem->type.command != RenderType::VERTEX) return TEXTURE_INDEX_INVALID;
  auto e = (RenderElementVertex *) elem;
  return e->texture;
}

inline VertexInfo *get_vertices(RenderElement *elem) {
  if (elem->type.command != RenderType::VERTEX) return NULL;
  auto e = (RenderElementVertex *) elem;
  ASSERT(e->vertex.count > 0);
  ASSERT(e->vertex.index >= 0 && e->vertex.index < MAX_RENDERER_VERTICES);
  return &e->vertex;
}

struct Vertex {
  V4 position;
  V4 color;
  V2 uv;
  V3 normal;
  V3 tangent;
};

struct RenderStage {
  enum Flags : u16 {
    ENABLE_DEPTH_TEST = 1,
    CLEAR_DEPTH = (1 << 1),
    CLEAR_COLOR = (1 << 2),
    HAS_LIGHTING = (1 << 3),
  };

  enum ProjectionType : u8 {
    ORTHOGRAPHIC,
    PERSPECTIVE,
    INFINITE_FAR_PLANE,
  };

  enum ViewType : u8 {
    CAMERA_VIEW,
    IDENTITY,
  };

  RenderElement *first;
  RenderElement *tail;
  u32 element_count;
  ProjectionType projection;
  ViewType view;
  Flags flags;
};


static void init_render_stage(RenderStage *stage, RenderStage::ProjectionType projection, RenderStage::ViewType view, u16 flags) {
  *stage = {};
  stage->projection = projection;
  stage->view = view;
  stage->flags = RenderStage::Flags(flags);
}


struct RenderBuffer {
  PushAllocator allocator;
  GameAssets *assets;
  GameCamera *camera;
  Vertex *vertices;

  int vertex_count;
  int max_vertices;
  int screen_width;
  int screen_height;

  float z_bias_accum;

  RenderStage stages[RENDER_STAGE_COUNT];
};

// TODO maybe delete these, they are not currently used I think
#define Z_BIAS_EPSILON 0.000001f
#define Z_BIAS_HUD_BASE 0.01

//
// Basic RenderBuffer operations ---
//

inline TextureInfo *get_texture(RenderBuffer *buffer, TextureIndex index) {
  ASSERT(index > 0 && index < TEXTURE_INDEX_MAX);
  return buffer->assets->texture_infos + index;
}

static inline void clear(RenderBuffer *buffer) {
  clear(&buffer->allocator);
  buffer->vertex_count = 0;
  buffer->z_bias_accum = Z_BIAS_HUD_BASE;

  for (u32 i = 0; i < RENDER_STAGE_COUNT; i++) {
    auto stage = buffer->stages + i;
    stage->first = NULL;
    stage->tail = NULL;
    stage->element_count = 0;
  }
}

static void init_render_buffer(RenderBuffer *buffer, int width, int height) {
  assert(buffer);
  *buffer = {};
  buffer->screen_width = width;
  buffer->screen_height = height;

  // TODO maybe pass in the memory from outside?
  buffer->max_vertices = MAX_RENDERER_VERTICES;
  buffer->vertex_count = 0;
  int vertex_buffer_size = buffer->max_vertices * sizeof(Vertex);
  int total_buffer_size = vertex_buffer_size + 4096 * 128;
  printf("Total render buffer size : %d\n", total_buffer_size);

  auto temp = new_push_allocator(total_buffer_size);
  buffer->vertices = ALLOC_ARRAY(&temp, Vertex, buffer->max_vertices);
  assert(buffer->vertices);
  buffer->allocator = new_push_allocator(&temp, remaining_size(&temp));
  assert(is_initialized(&buffer->allocator));

  init_render_stage(buffer->stages + RENDER_STAGE_BASE, 
      RenderStage::PERSPECTIVE, RenderStage::CAMERA_VIEW, 
      RenderStage::CLEAR_DEPTH | RenderStage::CLEAR_COLOR | 
      RenderStage::ENABLE_DEPTH_TEST | RenderStage::HAS_LIGHTING);

  init_render_stage(buffer->stages + RENDER_STAGE_TRANSPARENT,
      RenderStage::PERSPECTIVE, RenderStage::CAMERA_VIEW,
      RenderStage::ENABLE_DEPTH_TEST | RenderStage::HAS_LIGHTING);

  init_render_stage(buffer->stages + RENDER_STAGE_HUD, 
      RenderStage::ORTHOGRAPHIC, RenderStage::IDENTITY, 
      0);

  clear(buffer);
}

static RenderElement *push_element(RenderBuffer *buffer, RenderType type, u32 stage) {
  TIMED_FUNCTION();

  assert(stage < RENDER_STAGE_COUNT);
  auto elem = (RenderElement *) alloc_size(&buffer->allocator, get_size(type));
  if (!elem) {
    FAILURE("RenderElement allocator is full.");
    return NULL;
  }

  elem->next = NULL;
  elem->type = type;

  auto render_stage = buffer->stages + stage;
  if (!render_stage->first) render_stage->first = elem;
  if (render_stage->tail) {
    ASSERT(!render_stage->tail->next);
    render_stage->tail->next = elem;
  }
  render_stage->tail = elem;
  render_stage->element_count++;

  return elem;
}

//
// Vertex operations ---
//

enum VertexAttribute {
  VERTEX_POSITION,
  VERTEX_UV,
  VERTEX_COLOR,
  VERTEX_NORMAL,
  VERTEX_TANGENT,

  VERTEX_ATTRIBUTE_COUNT,
  VERTEX_FULLY_INITIALIZED,
};



struct VertexArray {
  Vertex *verts = NULL;
  NormalMapInfo *normal_map;
  TextureIndex texture;
  RenderStageNum stage;
  u32 count;
  u32 attribute_flags;
  u32 element_flags;
};

// TODO add VertexTriangle, VertexTriangleFan, etc
struct VertexQuad : VertexArray {
  u32 quad_count = 0;

  // NOTE Wraps counter clockwise around each triangle
  static constexpr u32 COUNT = 6;
  static constexpr int IDXS[COUNT] = {0,1,2,0,2,3};

  VertexQuad() = default;
  inline VertexQuad(VertexArray a) { 
    ASSERT_EQUAL(a.count % COUNT, 0); 
    auto me = (VertexArray *) this;
    *me = a;
    quad_count = a.count / COUNT;
  }
};

constexpr int VertexQuad::IDXS[COUNT];

static inline VertexArray push_vertices(RenderBuffer *buffer, u32 count) {
  if (buffer->max_vertices - buffer->vertex_count < count) {
    FAILURE("Vertex buffer overflowed.", buffer->vertex_count, buffer->max_vertices, count);
    return {};
  }
  auto vert = buffer->vertices + buffer->vertex_count;
  buffer->vertex_count += count;
  VertexArray result = {};
  result.stage = RENDER_STAGE_BASE;
  result.count = count;
  result.verts = vert;
  return result;
}

static inline void push_vertices(RenderBuffer *buffer, VertexArray &verts, u32 count) {
  if (!verts.verts) {
    verts = {};
    verts.stage = RENDER_STAGE_BASE;
  }

  if (buffer->max_vertices - buffer->vertex_count < count) {
    FAILURE("Vertex buffer overflowed.", buffer->vertex_count, buffer->max_vertices, count);
    return;
  }
  auto new_vert = buffer->vertices + buffer->vertex_count;
  buffer->vertex_count += count;

  if (!verts.verts) {
    verts.verts = new_vert;
  } else {
    ASSERT_EQUAL(verts.verts, new_vert - verts.count);
  }
  verts.count += count;
}

static inline void set_colors(VertexArray &verts, V4 color) {
  for (u32 i = 0; i < verts.count; i++) {
    verts.verts[i].color = color;
  }
  SHIFT_SET(verts.attribute_flags, VERTEX_COLOR);
}

static inline void set_uv(VertexArray &verts, V2 uv) {
  for (u32 i = 0; i < verts.count; i++) {
    verts.verts[i].uv = uv;
  }
  SHIFT_SET(verts.attribute_flags, VERTEX_UV);
}

static inline void set_normals(VertexArray &verts, V3 normal) {
  for (u32 i = 0; i < verts.count; i++) {
    verts.verts[i].normal = normal;
  }
  SHIFT_SET(verts.attribute_flags, VERTEX_NORMAL);
}

static inline void set_tangents(VertexArray &verts, V3 tangent) {
  for (u32 i = 0; i < verts.count; i++) {
    verts.verts[i].tangent = tangent;
  }
  SHIFT_SET(verts.attribute_flags, VERTEX_TANGENT);
}

static inline void set_texture(VertexArray &verts, TextureIndex index) {
  verts.texture = index;
}

static inline void set_normal_map(VertexArray &verts, NormalMapInfo *normal_map) {
  verts.normal_map = normal_map;
}

static inline void set_flags(VertexArray &verts, u32 flags) {
  verts.element_flags = flags;
}

static inline void set_stage(VertexArray &verts, RenderStageNum stage) {
  verts.stage = stage;
}


#define SET_QUAD_ATTRIBUTE_ARRAY_WORK(array, member, FLAG) \
  ASSERT(index >= 0 && index < verts.quad_count); \
  int const *idxs = verts.IDXS; \
  for (u32 i = 0; i < verts.COUNT; i++) { \
    verts.verts[i + index * verts.COUNT].member = array[idxs[i]]; \
  } \
  SHIFT_SET(verts.attribute_flags, FLAG);

#define SET_QUAD_ATTRIBUTE_ARRAY(array, member, FLAG) \
  ASSERT(array); \
  SET_QUAD_ATTRIBUTE_ARRAY_WORK(array, member, FLAG);

#define SET_QUAD_ATTRIBUTE(value, member, FLAG) \
  ASSERT(index >= 0 && index < verts.quad_count); \
  for (u32 i = 0; i < verts.COUNT; i++) { \
    verts.verts[i + index * verts.COUNT].member = value; \
  } \
  SHIFT_SET(verts.attribute_flags, FLAG);

static inline void set_colors(VertexQuad &verts, V4 *colors, int index = 0) {
  SET_QUAD_ATTRIBUTE_ARRAY(colors, color, VERTEX_COLOR);
}

static inline void set_colors(VertexQuad &verts, V4 color, int index) {
  SET_QUAD_ATTRIBUTE(color, color, VERTEX_COLOR);
}

static inline void set_uv(VertexQuad &verts, V2 *uv, int index = 0) {
  SET_QUAD_ATTRIBUTE_ARRAY(uv, uv, VERTEX_UV);
}

static inline void set_uv(VertexQuad &verts, V2 uv, int index) {
  SET_QUAD_ATTRIBUTE(uv, uv, VERTEX_UV);
}

inline void uv_to_array(V4 uv, V2 *arr) {
  ASSERT(arr);
  arr[0] = vec2(uv.x, uv.w);
  arr[1] = uv.zw;
  arr[2] = vec2(uv.z, uv.y);
  arr[3] = uv.xy;
}

static inline void set_uv(VertexQuad &verts, V4 uv, int index = 0) {
  ASSERT(index >= 0 && index < verts.quad_count);
  V2 uv4[4];
  uv_to_array(uv, uv4);
  set_uv(verts, uv4, index);
}

static inline void set_normals(VertexQuad &verts, V3 *normals, int index = 0) {
  SET_QUAD_ATTRIBUTE_ARRAY(normals, normal, VERTEX_NORMAL);
}

static inline void set_normals(VertexQuad &verts, V3 normal, int index) {
  SET_QUAD_ATTRIBUTE(normal, normal, VERTEX_NORMAL);
}

static inline void set_tangents(VertexQuad &verts, V3 *tangents, int index = 0) {
  SET_QUAD_ATTRIBUTE_ARRAY(tangents, tangent, VERTEX_TANGENT);
}

static inline void set_tangents(VertexQuad &verts, V3 tangent, int index) {
  SET_QUAD_ATTRIBUTE(tangent, tangent, VERTEX_TANGENT);
}

static inline void set_positions(VertexQuad &verts, Quad4 *quad, int index = 0) {
  ASSERT(quad);
  SET_QUAD_ATTRIBUTE_ARRAY_WORK(quad->verts, position, VERTEX_POSITION);
}

static inline void set_positions(VertexQuad &verts, AlignedRect3 rect, FaceIndex face, int index = 0) {
  Quad4 quad = to_quad4(rect, face);
  SET_QUAD_ATTRIBUTE_ARRAY_WORK(quad.verts, position, VERTEX_POSITION);
}

static inline void push_render_element(RenderBuffer *buffer, VertexArray &verts) {
  if (!verts.count) return;
  if (!verts.verts) return;

  RenderType render_type = {};
  render_type.command = RenderType::VERTEX;
  render_type.mode = RenderType::TRIANGLES;
  render_type.flags = verts.element_flags;

  auto elem = (RenderElementVertex *) push_element(buffer, render_type, verts.stage);

  if (verts.texture) {
    elem->texture = verts.texture;
  } else {
    elem->texture = get_texture_index(BITMAP_WHITE);
  }

  if (verts.normal_map) {
    ASSERT(verts.normal_map->id);
    elem->normal_map = verts.normal_map->id;
  } else {
    elem->normal_map = 0;
  }

  elem->vertex.index = verts.verts - buffer->vertices;
  elem->vertex.count = verts.count;
}

static inline void set_default_attributes(VertexQuad &verts, int index) {
  ASSERT(index >= 0 && index < verts.quad_count);
  if (SHIFT_TEST(verts.attribute_flags, VERTEX_FULLY_INITIALIZED)) return;

  if (!SHIFT_TEST(verts.attribute_flags, VERTEX_POSITION)) 
    FAILURE("Vertex position must be set", verts.attribute_flags);
  if (!SHIFT_TEST(verts.attribute_flags, VERTEX_COLOR)) 
    set_colors(verts, vec4(1), index);
  if (!SHIFT_TEST(verts.attribute_flags, VERTEX_NORMAL)) 
    set_normals(verts, vec3(0,0,1), index);
  if (!SHIFT_TEST(verts.attribute_flags, VERTEX_TANGENT)) 
    set_tangents(verts, vec3(1,0,0), index);

  V2 default_quad_uv[4] = {{0,1}, {1,1}, {1,0}, {0,0}};
  if (!SHIFT_TEST(verts.attribute_flags, VERTEX_UV)) {
    if (verts.count == 6) {
      set_uv(verts, default_quad_uv, index);
    } else {
      set_uv(verts, vec2(0), index);
    }
  }

  SHIFT_SET(verts.attribute_flags, VERTEX_FULLY_INITIALIZED);
}

static inline int push_quad(RenderBuffer *buffer, VertexQuad &verts, u32 quad_count = 1) {
  if (verts.verts) {
    int last_index = verts.quad_count - 1;
    set_default_attributes(verts, last_index);
  }
  push_vertices(buffer, verts, quad_count * verts.COUNT);
  int index = -1;
  if (verts.verts) {
    index = verts.quad_count;
    verts.quad_count += quad_count;
  }
  return index;
}

static inline void finalize(RenderBuffer *buffer, VertexQuad &verts) {
  set_default_attributes(verts, verts.quad_count - 1);
  push_render_element(buffer, verts);
}


//
// High level renderer calls ---
//


// TODO This should be combined with render_tiled_box to some degree. Note that this
// should only be used for untextured boxes.
static void push_box(RenderBuffer *buffer, AlignedBox box, V4 color) {
  TIMED_FUNCTION();
#define DEBUG_COLORS 0
#if DEBUG_COLORS
  V4 debug_colors[6] = {{1,0,0,1},{0,1,0,1},{0,0,1,1},{1,0.5,0,1},{0,1,0.5,1},{0.5,0,1,1}};
#endif

  Quad4 quads[6];
  to_quad4s(box, quads);

  auto c = color;
  
  V3 const *n = aabb_normals;
  V3 const *t = aabb_tangents;

  auto stage = (color.a < 1) ? RENDER_STAGE_TRANSPARENT : RENDER_STAGE_BASE;

  for (int i = 0; i < 6; i++) {

#if DEBUG_COLORS
    c = debug_colors[i];
#endif

    VertexQuad verts = push_vertices(buffer, 6);
    set_positions(verts, quads + i);
    set_colors(verts, c);
    set_normals(verts, n[i]);
    set_tangents(verts, t[i]);
    set_stage(verts, stage);
    finalize(buffer, verts);
  }

  // TODO we should push all of these as one element
#undef DEBUG_COLORS
}

static void push_rectangle(RenderBuffer *buffer, Rectangle rect, V4 color, BitmapID bitmap_id, u32 normal_map_id = 0, bool is_sprite = true) {
  TIMED_FUNCTION();

  auto quad = to_quad4(rect);

  // TODO calculate this for non-axis-aligned rectangles
  V3 n = {0,0,1};
  V3 t = {1,0,0};

  auto stage = (color.a < 1) ? RENDER_STAGE_TRANSPARENT : RENDER_STAGE_BASE;

  VertexQuad verts = push_vertices(buffer, 6);
  set_colors(verts, color);
  set_positions(verts, &quad);
  set_normals(verts, n);
  set_tangents(verts, t);
  set_texture(verts, get_texture_index(bitmap_id));
  auto normal_map = NormalMapInfo{normal_map_id, vec2(0)};
  if (normal_map_id) set_normal_map(verts, &normal_map);
  set_stage(verts, stage);
  if (is_sprite) set_flags(verts, RenderInfo::LOW_RES);
  finalize(buffer, verts);
}

// NOTE : Mostly intended for debugging purposes
static void render_arrow(RenderBuffer *buffer, V3 p1, V3 p2, V4 color, float width) {
  /*
  GameCamera *camera = buffer->camera;

  V3 Z = camera->forward;
  V3 X = camera->right;
  V3 Y = camera->up;
  */

  Rectangle r;
  r.center = (p1 + p2) / 2;
  r.offsets[0] = p2 - r.center;
  // TODO Do some fancy crap here with the camera/cross product to make r face the camera
  r.offsets[1] = vec3(width, 0, 0);
  auto quad = to_quad4(r);

  Rectangle r2 = r;
  r2.offsets[1] = vec3(0, 0, width);
  auto quad2 = to_quad4(r2);

  VertexQuad verts1 = push_vertices(buffer, 6);
  VertexQuad verts2 = push_vertices(buffer, 6);
  V4 colors[4] = {color, color, vec4(1,1,1,1), color};
  set_colors(verts1, colors);
  set_colors(verts2, colors);

  set_positions(verts1, &quad);
  set_positions(verts2, &quad2);
  finalize(buffer, verts1);
  finalize(buffer, verts2);

  // TODO This is broken because finalize doesn't actually use the vertex pointer
  // in verts yet. It still does the thing where it assumes the most recent vertices
  // belong to it.
}

//
// Hud rendering ---
//

static void render_pixel_space(RenderBuffer *buffer, Rectangle rect, V4 color, BitmapID bitmap_id) {
  TIMED_FUNCTION();

  //float z_bias = buffer->camera->p.z - buffer->camera->near_dist + buffer->z_bias_accum;
  //buffer->z_bias_accum += Z_BIAS_EPSILON;

  auto quad = to_quad4(rect);

  VertexQuad verts = push_vertices(buffer, 6);
  set_colors(verts, color);
  set_positions(verts, &quad);
  set_stage(verts, RENDER_STAGE_HUD);
  set_texture(verts, get_texture_index(bitmap_id));
  //set_flags(verts, RenderInfo::LOW_RES);
  finalize(buffer, verts);
}

inline void render_pixel_space(RenderBuffer *buffer, AlignedRect rect, V4 color, BitmapID bitmap_id) {
  render_pixel_space(buffer, rectangle(rect), color, bitmap_id);
}

inline void render_screen_space(RenderBuffer *buffer, Rectangle rect, V4 color, BitmapID bitmap_id) {
  V2 to_pixel = vec2(buffer->screen_width, buffer->screen_height);
  rect.center.xy *= to_pixel;
  render_pixel_space(buffer, rect, color, bitmap_id);
}

inline void render_screen_space(RenderBuffer *buffer, AlignedRect rect, V4 color, BitmapID bitmap_id) {
  render_screen_space(buffer, rectangle(rect), color, bitmap_id);
}

inline void render_circle_screen_space(RenderBuffer *buffer, V2 p, float width, V4 color) {
  render_screen_space(buffer, aligned_rect(p, width, width), color, BITMAP_CIRCLE);
}

//
// Text rendering ---
//

static void render_shadowed_text_pixel_space(RenderBuffer *buffer, V2 text_p, const char *text, V4 color, V4 shadow_color, int shadow_depth, SizedFontID font_id) { 
  // TODO figure out a way to time this that doesn't interfere with drawing debug text
  //TIMED_FUNCTION();

  bool is_shadowed = shadow_color.a > 0;
  V3 shadow_offset;
  if (is_shadowed) shadow_offset = vec3(shadow_depth, -shadow_depth, 0);
  // TODO get rid of all the unnecessary variables from when this was in world space
  auto assets = buffer->assets;
  int num_quads = 0;
  V2 current_p = text_p; // Upper left corner
  V2 pixel_p = current_p;

  auto font_info = get_font(assets, font_id);
  assert(font_info);
  if (!font_info) return;

  float font_width  = font_info->width;
  float font_height = font_info->height;
  auto texture_index = get_texture_index(font_id);
  VertexQuad verts;

  while (*text) {
    auto b = get_baked_char(font_info, *text);
    auto p = round(pixel_p + V2{b->xoff, -b->yoff});

    float char_width  = (b->x1 - b->x0);
    float char_height = (b->y1 - b->y0);

    auto quad = to_quad4(aligned_rect(p.x, p.y - char_height, p.x + char_width, p.y), 0);

    V2 uv[4] = {
      {b->x0 / font_width, b->y1 / font_height}, 
      {b->x1 / font_width, b->y1 / font_height}, 
      {b->x1 / font_width, b->y0 / font_height}, 
      {b->x0 / font_width, b->y0 / font_height},
    };

    if (is_shadowed) {
      translate(&quad, shadow_offset);
      int index = push_quad(buffer, verts);
      ASSERT(index >= 0);
      set_colors(verts, shadow_color, index);
      set_uv(verts, uv, index);
      set_positions(verts, &quad, index);
      num_quads++;
      translate(&quad, -shadow_offset);
    }

    int index = push_quad(buffer, verts);
    ASSERT(index >= 0);
    set_colors(verts, color, index);
    set_uv(verts, uv, index);
    set_positions(verts, &quad, index);

    num_quads++;
    pixel_p.x += b->xadvance;
    text++;
  }

  set_texture(verts, texture_index);
  set_stage(verts, RENDER_STAGE_HUD);
  finalize(buffer, verts);
}

inline void render_text_pixel_space(RenderBuffer *buffer, V2 text_p, const char *text, V4 color, SizedFontID font_id) {
  render_shadowed_text_pixel_space(buffer, text_p, text, color, vec4(0), 0, font_id);
}

inline void render_shadowed_text_screen_space(RenderBuffer *buffer, V2 text_p, const char *text, V4 color, V4 shadow_color, int shadow_depth, SizedFontID font_id) { 
  V2 text_p_pixel_space = text_p * vec2(buffer->screen_width, buffer->screen_height);
  render_shadowed_text_pixel_space(buffer, text_p_pixel_space, text, color, shadow_color, shadow_depth, font_id);
}

inline void render_text_screen_space(RenderBuffer *buffer, V2 text_p, const char *text, V4 color, SizedFontID font_id) { 
  V2 text_p_pixel_space = text_p * vec2(buffer->screen_width, buffer->screen_height);
  render_text_pixel_space(buffer, text_p_pixel_space, text, color, font_id);
}


// TODO I would like this to work with multiple colors...
// Idea : Choose a symbol that can't be drawn with push_hud_text use it to indicate a color change.
static void render_debug_string(RenderBuffer *buffer, V2 cursor_p, char const *format...) {
  char dest[1024 * 16];

  va_list args;
  va_start(args, format);
  vsnprintf(dest, sizeof(dest), format, args);
  va_end(args);

  render_shadowed_text_screen_space(buffer, cursor_p, dest, vec4(1), vec4(0,0,0,1), 2, FONT_COURIER_NEW_BOLD_SIZE_19);
}

// TODO This is a weird place to put this, but right now it is inconvenient to put it anywhere else.
static void render_frame_records(RenderBuffer *buffer) {
  // NOTE : This is basically copied from print_frame_records.
#define TAB "   "
 
  u32 const screen_lines = 28;
  // TODO we could use font_info->font_size;
  float const newline_height = 1.0f / screen_lines;
  float left_margin = 1.f/12;
  u32 const max_lines = 26;

  V2 cursor_p = { left_margin, 1 - newline_height * (screen_lines - max_lines) };

  static darray<u32> block_ids;

  if (!block_ids) for (u32 block_id = 0; block_id < debug_global_memory.record_count; block_id++) {
    push(block_ids, block_id);
  }

  quick_sort<u32, block_id_compare>(block_ids, count(block_ids));

  u32 lines = 0;

  u32 current_frame = debug_global_memory.current_frame % NUM_FRAMES_RECORDED;
  render_debug_string(buffer, cursor_p + V2{7.f/12, 0} , "Total Frames : %llu", debug_global_memory.current_frame);
  cursor_p.y -= newline_height;
  lines++;

  render_debug_string(buffer, cursor_p, TAB "Thread |    Cycles |  Internal | Hits | Cycles/Hit |     Average");
  cursor_p.y -= newline_height;
  lines++;

  for (u32 i = 0; i < count(block_ids) && lines < max_lines - 1; i++) {
    u32 block_id = block_ids[i];
    FunctionInfo *info = debug_global_memory.function_infos + block_id;
    if (!info->filename) continue;
    DebugRecord *records[NUM_THREADS];
    FrameRecord *frames[NUM_THREADS];

    for (u32 thread_idx = 0; thread_idx < NUM_THREADS; thread_idx++) {
      records[thread_idx] = debug_global_memory.debug_logs[thread_idx].records + block_id;
      frames[thread_idx] = &records[thread_idx]->frames[current_frame];
    }

    render_debug_string(buffer, cursor_p, "%-24s in %-24s (line %4u) : ",
                        info->function_name, 
                        info->filename, 
                        info->line_number);
    cursor_p.y -= newline_height;
    lines++;

    for (u32 thread_idx = 0; thread_idx < NUM_THREADS && lines < max_lines; thread_idx++) {
      auto frame = frames[thread_idx];
      auto record = records[thread_idx];
      if (!frame->hit_count) continue;

      render_debug_string(buffer, cursor_p, 
                          TAB "%6u | %9u | %9u | %4u | %10u | %11.1lf",
                          thread_idx,
                          frame->total_cycles, 
                          frame->internal_cycles,
                          frame->hit_count,
                          frame->total_cycles / frame->hit_count,
                          record->average_internal_cycles);
      cursor_p.y -= newline_height;
      lines++;
    }
    lines++;
    cursor_p.y -= newline_height;
    //cursor_p.y -= newline_height / 2;
  }
#undef TAB
}


//
// Entity rendering ---
//

static SpriteRenderInfo get_sprite_render_info(GameAssets *assets, Entity *e);

// TODO possibly factor some of this out into render_entity
static void render_sprite(RenderBuffer *buffer, Entity *e) {
#define DEBUG_CUBES 0
  TIMED_FUNCTION();

  SpriteRenderInfo info = get_sprite_render_info(buffer->assets, e);
  if (info.texture_uv == vec4(0)) return;
  //PRINT_V4(info.texture_uv);
  assert(info.texture.id);

  GameCamera *camera = buffer->camera;

  V3 Z = camera->forward;
  V3 X = camera->right;
  V3 Y = camera->up;

  float width  = info.width  * METERS_PER_PIXEL * info.scale;
  float height = info.height * METERS_PER_PIXEL * info.scale;

  auto box = e->collision_box;
  Rectangle r;
  auto offset = info.offset;
  offset.z = 0;
  r.center = center(box) + info.offset;
  //r.center.z -= box.offset.z;
  r.center.z += info.offset.z;
  r.offsets[1].z = r.offsets[0].z = height;
  r.offsets[0].x = width / 2;
  r.offsets[1].x = -width / 2;
  r.offsets[0].y = height / 2;
  r.offsets[1].y = height / 2;

  r.offsets[0] = X * r.offsets[0].x + Y * r.offsets[0].y;
  r.offsets[1] = X * r.offsets[1].x + Y * r.offsets[1].y;


  auto quad = to_quad4(r);

  V4 uv = info.texture_uv;
  //V2 uv4[4] = {{uv.x,uv.w}, uv.zw, {uv.z,uv.y}, uv.xy};

  V3 n = normalize(Z + vec3(0,-2,0)); // TODO Figure this out for real

  /*
  float z_min = visual_info.offset.z;
  float z_max = visual_info.sprite_height;
  */

  float z_min = 0;
  float z_max = info.sprite_depth;

  float bias[4] = {z_min, z_min, z_max, z_max};
  for (int i = 0; i < 4; i++) {
    quad.verts[i].w = bias[i];
  }

  // NOTE : This is always correct if the sprite isn't skewed.
  V3 t = X; //{1,0,0};


  // DELETEME
  //render_arrow(buffer, r.center, r.center + n, vec4(0.3,0,0,1), 0.1);
  //render_arrow(buffer, r.center, r.center + t, vec4(0,0,0.5,1), 0.1);

  VertexQuad verts = push_vertices(buffer, 6);
  set_colors(verts, info.color);
  set_uv(verts, uv);
  set_normals(verts, n);
  set_tangents(verts, t);
  set_positions(verts, &quad);
  set_stage(verts, info.render_stage);
  set_texture(verts, get_texture_index(e->texture_group_id));
  if (info.normal_map.id) set_normal_map(verts, &info.normal_map);
  set_flags(verts, info.flags);
  finalize(buffer, verts);

#if DEBUG_CUBES
  push_box(buffer, box, vec4(0.9, 1, 0, 0.2));
#endif
#undef DEBUG_CUBES
}

static TileRenderInfo get_tile_render_info(GameAssets *assets, Entity *e, FaceIndex face_idx, u32 tile_idx);

inline void render_tile(RenderBuffer *buffer, Entity *e, VertexQuad &verts, AlignedRect3 tile, FaceIndex face_idx, u32 tile_idx) {

  // TODO pull some of this info out a level
  TileRenderInfo info = get_tile_render_info(buffer->assets, e, face_idx, tile_idx);
  // TODO handle cubes without a texture

  const V3 normal  = aabb_normals[face_idx];
  const V3 tangent = aabb_tangents[face_idx];

  // TODO factor this out more
  int index = push_quad(buffer, verts);
  set_colors(verts, info.color, index);
  set_uv(verts, info.texture_uv, index);
  set_normals(verts, normal, index);
  set_tangents(verts, tangent, index);
  set_positions(verts, tile, face_idx, index);
  //finalize(buffer, verts); 
}

// TODO add debug colors
static void render_tiled_rect(RenderBuffer *buffer, Entity *e, VertexQuad &verts, AlignedRect3 rect, FaceIndex face_idx) {

#define CULL_REVERSE_FACES false
  

  float tile_dim = 1;
  V3 normal = aabb_normals[face_idx];
  V3 tangent = aabb_tangents[face_idx];
  V3 cotangent = aabb_cotangents[face_idx];

  if (CULL_REVERSE_FACES) {
    // TODO What we actually want is to check that the normal is towards
    // the diff vector for the camera and rect, and return otherwise.
    //
    // Also, we may want to do this with transparent cubes as well except that
    // we would just draw the back faces first. For that reason, we may want to
    // put this code in render_tiled_box and create a mask of which faces to draw.
    if (towards(buffer->camera->forward, normal)) return;
  }

  // NOTE : These are the indices of the axes in the array {x,y,z}
  u32 x_axis_index = !normal.x ? 0 : 1;
  u32 y_axis_index = !normal.z ? 2 : 1;
  u32 zero_axis_index = 3 - x_axis_index - y_axis_index;

  V3 tile_offset = (tangent + cotangent) * tile_dim / 2;
  ASSERT_EQUAL(tile_offset.elements[zero_axis_index], 0);

  V3 origin = rect.center - rect.offset + tile_offset;

  AlignedRect3 tile;
  tile.center = origin;
  tile.offset = tile_offset;

  u32 x_count = abs(rect.offset.elements[x_axis_index] * 2 / tile_dim);
  u32 y_count = abs(rect.offset.elements[y_axis_index] * 2 / tile_dim);

  float x_translation = tile_offset.elements[x_axis_index] * 2;
  float y_translation = tile_offset.elements[y_axis_index] * 2;
  float origin_y = origin.elements[y_axis_index];

  for (u32 i = 0; i < x_count; i++) {
    for (u32 j = 0; j < y_count; j++) {
      render_tile(buffer, e, verts, tile, face_idx, i * y_count + j);
      tile.center.elements[y_axis_index] += y_translation;
    }
    tile.center.elements[y_axis_index] = origin_y;
    tile.center.elements[x_axis_index] += x_translation;
  }
}

static void render_tiled_box(RenderBuffer *buffer, Entity *e) {
  TIMED_FUNCTION();

  auto box = e->collision_box;

  VertexQuad verts;

  for (u32 i = 0; i < FACE_COUNT; i++) {
    auto face = get_face(box, FaceIndex(i));
    render_tiled_rect(buffer, e, verts, face, FaceIndex(i));
  }

  // TODO This is dumb, should be a different call
  TileRenderInfo info = get_tile_render_info(buffer->assets, e, FaceIndex(0), 0);

  auto stage = (info.color.a < 1) ? RENDER_STAGE_TRANSPARENT : RENDER_STAGE_BASE;

  set_stage(verts, stage);
  set_texture(verts, get_texture_index(e->texture_group_id));
  set_normal_map(verts, &info.normal_map);
  set_flags(verts, info.flags);

  finalize(buffer, verts);
  // TODO consider pulling out some of the push calls from render_tiled_rect
  // to call them here instead.
}

// TODO switch everthing to use the same path
static void render_entity(RenderBuffer *buffer, Entity *e) {
  TIMED_FUNCTION();
  
  if (e->flags & ENTITY_SPRITE) render_sprite(buffer, e);
  else if (e->flags & ENTITY_CUBOID) render_tiled_box(buffer, e);

}

#if 1
// TODO massively rework this to simplify and get better z biasing.
static void push_sprite(RenderBuffer *buffer, AlignedBox box, VisualInfo visual_info) {
#define DEBUG_CUBES 0
  TIMED_FUNCTION();

  auto texture_index = get_texture_index(visual_info.texture_id);
  auto texture = get_texture(buffer, texture_index);
  auto normal_map_id = get_texture_id(buffer->assets, visual_info.normal_map_id);
  auto normal_map = NormalMapInfo{normal_map_id, vec2(0)};

  GameCamera *camera = buffer->camera;

  V3 Z = camera->forward;
  V3 X = camera->right;
  V3 Y = camera->up;

  float width  = texture->width  * METERS_PER_PIXEL * visual_info.scale;
  float height = texture->height * METERS_PER_PIXEL * visual_info.scale;

  Rectangle r;
  auto offset = visual_info.offset;
  offset.z = 0;
  r.center = center(box) + visual_info.offset;
  //r.center.z -= box.offset.z;
  r.center.z += visual_info.offset.z;
  r.offsets[1].z = r.offsets[0].z = height;
  r.offsets[0].x = width / 2;
  r.offsets[1].x = -width / 2;
  r.offsets[0].y = height / 2;
  r.offsets[1].y = height / 2;

  r.offsets[0] = X * r.offsets[0].x + Y * r.offsets[0].y;
  r.offsets[1] = X * r.offsets[1].x + Y * r.offsets[1].y;


  auto quad = to_quad4(r);

  auto c = visual_info.color; 
  V3 n = -Z;

  /*
  float z_min = visual_info.offset.z;
  float z_max = visual_info.sprite_height;
  */

  float z_min = 0;
  float z_max = visual_info.sprite_depth; // TODO figure this out for real

  float bias[4] = {z_min, z_min, z_max, z_max};
  for (int i = 0; i < 4; i++) {
    quad.verts[i].w = bias[i];
  }

  // NOTE : This is always correct if the sprite isn't skewed.
  V3 t = X; //{1,0,0};

  VertexQuad verts = push_vertices(buffer, 6);
  set_colors(verts, c);
  set_normals(verts, n);
  set_tangents(verts, t);
  set_positions(verts, &quad);
  set_texture(verts, texture_index);
  set_normal_map(verts, &normal_map);
  set_stage(verts, RENDER_STAGE_TRANSPARENT);
  finalize(buffer, verts);

#if DEBUG_CUBES
  push_box(buffer, box, vec4(0.9, 1, 0, 0.5));
#endif
#undef DEBUG_CUBES
}
#endif

#if 1
#define gl_check_error() _gl_check_error(__FILE__, __LINE__)
#else
#define gl_check_error() 
#endif

// TODO pass these around more elegantly?
//static GLuint transform_id;
static GLuint projection_matrix_id;
static GLuint view_matrix_id;
static GLuint program_id;
static GLuint vertex_buffer_id;
static GLuint texture_sampler_id;
static GLuint texture_width_id;
static GLuint texture_height_id;
static GLuint use_low_res_uv_filter_id;
static GLuint normal_map_uv_offset_id;
static GLuint has_normal_map_id;

static inline void _gl_check_error(const char *file_name, int line_num) {
  GLenum err;
  while ((err = glGetError()) != GL_NO_ERROR) {
    printf("OpenGL error: 0x%x in file %s, on line %d\n", err, file_name, line_num);
  }
}

static inline void gl_push_normal_map(NormalMapInfo *normal_map) {
  bool has_normal_map = normal_map ? true : false;
  glUniform1i(has_normal_map_id, has_normal_map);
  if (!normal_map) return;
  glActiveTexture(GL_TEXTURE0 + 1);
  glBindTexture(GL_TEXTURE_2D, normal_map->id);
  glUniform2fv(normal_map_uv_offset_id, 1, normal_map->uv_offset.elements);
  gl_check_error();
}

static inline void gl_push_texture(TextureInfo *texture) {
  if (!texture) return;
  glActiveTexture(GL_TEXTURE0 + 0);
  glBindTexture(GL_TEXTURE_2D, texture->id);
  glUniform1f(texture_width_id, texture->width);
  glUniform1f(texture_height_id, texture->height);
  gl_check_error();
}

static inline void gl_push_flags(u16 flags) {
  bool use_low_res_uv = (flags & RenderInfo::LOW_RES) != 0;
  glUniform1i(use_low_res_uv_filter_id, use_low_res_uv);
  gl_check_error();
}

static inline void gl_draw_vertices(VertexInfo *vertex) {
  if (!vertex) return;
  glDrawArrays(GL_TRIANGLES, vertex->index, vertex->count);
  gl_check_error();
}

static void gl_set_vertex_attribute_pointers() {
  glEnableVertexAttribArray(VERTEX_POSITION);
  glEnableVertexAttribArray(VERTEX_COLOR);
  glEnableVertexAttribArray(VERTEX_UV);
  glEnableVertexAttribArray(VERTEX_NORMAL);
  glEnableVertexAttribArray(VERTEX_TANGENT);

  // TODO Not sure I'm doing this right. Further reading :
  // https://www.khronos.org/opengl/wiki/Vertex_Specification_Best_Practices
  // https://www.khronos.org/opengl/wiki/Buffer_Object_Streaming

  // NOTE : the boolean argument is "normalized" and causes 
  // integers to be mapped to [-1,1] (or [0,1] if unsigned).
  glVertexAttribPointer(VERTEX_POSITION, 4, GL_FLOAT, false, 
      sizeof(Vertex), (void *)(offsetof(Vertex, position)));

  // TODO switch this to GL_UNSIGNED_BYTE
  glVertexAttribPointer(VERTEX_COLOR, 4, GL_FLOAT, false, 
      sizeof(Vertex), (void *)(offsetof(Vertex, color)));

  glVertexAttribPointer(VERTEX_UV, 2, GL_FLOAT, false, 
      sizeof(Vertex), (void *)(offsetof(Vertex, uv)));

  glVertexAttribPointer(VERTEX_NORMAL, 3, GL_FLOAT, false,
      sizeof(Vertex), (void *)(offsetof(Vertex, normal)));

  glVertexAttribPointer(VERTEX_TANGENT, 3, GL_FLOAT, false,
      sizeof(Vertex), (void *)(offsetof(Vertex, tangent)));
  gl_check_error();
}

// TODO Instead of calling glBufferData every frame, consider calling
// it once to allocate, and then call this once per frame :
//    glBufferSubData(GL_ARRAY_BUFFER, 0, total_vertex_size, vertices);
//
// However, this might be slower due to implicit synchronization.
// See : https://www.khronos.org/opengl/wiki/Buffer_Object_Streaming
//
// We should still only call this once per frame though.

static void gl_set_vertex_buffer(Vertex *vertices, int vertex_count) {
  u32 size = sizeof(Vertex) * vertex_count;
  glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STREAM_DRAW);
}

static void gl_draw_buffer(RenderBuffer *buffer) {
  TIMED_FUNCTION();

  // NOTE : Setting the viewport with no margin may cause things to be culled 
  // unexpectedly. However, adding a margin means that the matrices need to be adjusted.
  glViewport(0, 0, buffer->screen_width, buffer->screen_height);
  gl_check_error();

  // NOTE : Due to the way arrays work in C, these matrices are
  // transposed (right is down, down is right).

  auto camera = buffer->camera;
  float far_d = camera->far_dist;
  float near_d = camera->near_dist;

  // WARNING : Do not enable this, it caused all sorts of bugs.
  //glDepthRange(near_d, far_d);
  
  float e = camera->focal_length;     // Typically 2/width
  float q = e / camera->aspect_ratio; // Typically 2/height
  float p = -(far_d + near_d) / (far_d - near_d);
  float r = -2 * far_d * near_d / (far_d - near_d);


  float perspective_proj[] = {
    e, 0, 0,  0,
    0, q, 0,  0,
    0, 0, p, -1,
    0, 0, r,  0,
  };

  float infinite_proj[] = {
    e, 0, 0,  0,
    0, q, 0,  0,
    0, 0, 0.001f-1, -1,
    0, 0, (0.001f-2)*near_d,  0,
  };

  float orthographic_proj[] = {
    2.0f/buffer->screen_width, 0, 0,  0,
    0, 2.0f/buffer->screen_height, 0,  0,
    0, 0, 1,  0,
    -1, -1, 0,  1,
  };


  V3 eye = camera->p;
  //V3 target = camera->target;

  V3 Z = camera->forward;
  V3 X = camera->right;
  V3 Y = camera->up;

  // "Translation" (dotted because view = translation * rotation)
  V3 T = V3{-dot(X, eye), -dot(Y, eye), -dot(Z, eye)};

  float view_mat[] = {
    X.x, Y.x, Z.x, 0,
    X.y, Y.y, Z.y, 0,
    X.z, Y.z, Z.z, 0,
    T.x, T.y, T.z, 1,
  };

  float identity_mat[] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1,
  };

  glClearColor(0.5,0.5,0,1);
  gl_check_error();
  glClearDepth(1);
  gl_check_error();

  glUseProgram(program_id);
  gl_check_error();


  auto normal_sampler_id = glGetUniformLocation(program_id, "NORMAL_SAMPLER");

  glUniform1i(texture_sampler_id, 0); // Texture unit 0 is for the texture
  glUniform1i(normal_sampler_id, 1); // Texture unit 1 is for normal maps.

  auto light_p_id = glGetUniformLocation(program_id, "LIGHT_P");
  V3 light_p = debug_global_memory.game_state->pointer_world_p + vec3(0,0,0.5);
  push_box(buffer, aligned_box(light_p, 1), vec4(1, 1, 0, 0.2));
  
  //V3 light_p = {7,9,2};
  glUniform3fv(light_p_id, 1, light_p.elements);

  // DEBUG
  //print_render_buffer(buffer);

  //
  // Render Game
  //

  gl_set_vertex_buffer(buffer->vertices, buffer->vertex_count);
  gl_set_vertex_attribute_pointers(); // TODO Don't know if I need to call this every frame

  for (u32 i = 0; i < RENDER_STAGE_COUNT; i++) {
    auto stage = buffer->stages + i;

    u32 clear_bits = 0;
    if (stage->flags & RenderStage::CLEAR_COLOR) clear_bits |= GL_COLOR_BUFFER_BIT;
    if (stage->flags & RenderStage::CLEAR_DEPTH) clear_bits |= GL_DEPTH_BUFFER_BIT;
    if (clear_bits) glClear(clear_bits);

    if (!stage->element_count) continue;
    assert(stage->first);
    assert(stage->tail);

    if (stage->flags & RenderStage::ENABLE_DEPTH_TEST) {
      glEnable(GL_DEPTH_TEST);
    } else {
      glDisable(GL_DEPTH_TEST);
    }

    bool has_lighting = (stage->flags & RenderStage::HAS_LIGHTING) != 0;
    glUniform1i(glGetUniformLocation(program_id, "HAS_LIGHTING"), has_lighting);

    switch (stage->projection) {
      case RenderStage::ORTHOGRAPHIC :
        glUniformMatrix4fv(projection_matrix_id, 1, GL_FALSE, orthographic_proj); break;
      case RenderStage::PERSPECTIVE :
        glUniformMatrix4fv(projection_matrix_id, 1, GL_FALSE, perspective_proj); break;
      case RenderStage::INFINITE_FAR_PLANE :
        glUniformMatrix4fv(projection_matrix_id, 1, GL_FALSE, infinite_proj); break;
      default :
        INVALID_SWITCH_CASE(stage->projection);
    }

    switch (stage->view) {
      case RenderStage::IDENTITY :
        glUniformMatrix4fv(view_matrix_id, 1, GL_FALSE, identity_mat); break;
      case RenderStage::CAMERA_VIEW :
        glUniformMatrix4fv(view_matrix_id, 1, GL_FALSE, view_mat); break;
      default :
        INVALID_SWITCH_CASE(stage->view);
    }

    auto elem = stage->first;

    for (int i = 0; i < stage->element_count; i++) {
      assert(elem);
      gl_check_error();

      // TODO inline all these accessors once I'm done experimenting
      gl_push_flags(elem->type.flags);

      if (elem->type.command == RenderType::VERTEX) {
        auto vert_elem = (RenderElementVertex *) elem;
        TextureInfo *texture = get_texture(buffer, vert_elem->texture);
        // TODO remove dumb hack
        NormalMapInfo normal_map = {vert_elem->normal_map};
        gl_push_texture(texture);
        if (texture && texture->id == vert_elem->normal_map) {
          normal_map.uv_offset = vec2(0, 0.5);
        }
        gl_push_normal_map(normal_map.id ? &normal_map : NULL);
        gl_draw_vertices(&vert_elem->vertex);
      }


      // TODO figure out why this crashes when I don't check first?
      if (elem->next) {
        elem = elem->next;
      } else {
        break;
      }
    }
    if (stage->element_count) ASSERT(!elem->next);
  }
}


//static GLuint blit_texture_handle;

static GLuint create_shader_program(char* header, 
                                    char* vertex, 
                                    char* fragment) {


  GLint shader_code_lengths[] = {-1, -1, -1};

  GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
  gl_check_error();
  GLchar *vertex_shader_code[] = { 
    header, vertex
  };
  glShaderSource(vertex_shader_id, count_of(vertex_shader_code),
                 vertex_shader_code, shader_code_lengths);
  
  GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
  GLchar *fragment_shader_code[] = { 
    header, fragment
  };
  glShaderSource(fragment_shader_id, count_of(fragment_shader_code),
                 fragment_shader_code, shader_code_lengths);

  glCompileShader(vertex_shader_id);
  glCompileShader(fragment_shader_id);

  GLuint program_id = glCreateProgram();
  glAttachShader(program_id, vertex_shader_id);
  glAttachShader(program_id, fragment_shader_id);

  glBindAttribLocation(program_id, VERTEX_POSITION, "vertex");
  glBindAttribLocation(program_id, VERTEX_UV, "vertex_uv");
  glBindAttribLocation(program_id, VERTEX_COLOR, "vertex_color");
  glBindAttribLocation(program_id, VERTEX_NORMAL, "vertex_normal");
  glBindAttribLocation(program_id, VERTEX_TANGENT, "vertex_tangent");

  glLinkProgram(program_id);

  glValidateProgram(program_id);
  GLint linked = false;
  glGetProgramiv(program_id, GL_LINK_STATUS, &linked);
  gl_check_error();
  if (!linked) {
    char vertex_errors[256];
    char fragment_errors[256];
    char program_errors[256];
    GLsizei vcount;
    GLsizei fcount;
    GLsizei pcount;
    glGetShaderInfoLog(vertex_shader_id, sizeof(vertex_errors), 
                       &vcount, vertex_errors);
    glGetShaderInfoLog(fragment_shader_id, sizeof(fragment_errors), 
                       &fcount, fragment_errors);
    glGetProgramInfoLog(program_id, sizeof(program_errors),
                        &pcount, program_errors);

    printf("VERTEX ERRORS : \n\n%s\n\n", vertex_errors);
    printf("FRAGMENT ERRORS : \n\n%s\n\n", fragment_errors);
    printf("PROGRAM ERRORS : \n\n%s\n\n", program_errors);
    assert(!"Shader validation failed.");
  }

  return program_id;
}

GLint to_gl_format_specifier(TextureFormatSpecifier spec) {
  switch (spec) {
    case RGBA : return GL_RGBA;
    case BGRA : return GL_BGRA;
    case CLAMP_TO_EDGE : return GL_CLAMP_TO_EDGE;
    case REPEAT_CLAMPING : return GL_REPEAT;
    case LINEAR_BLEND : return GL_LINEAR;
    case NEAREST_BLEND : return GL_NEAREST;
    default : return 0;
  };
}

struct TextureParameters {
  TextureFormatSpecifier min_blend = LINEAR_BLEND;
  TextureFormatSpecifier max_blend = NEAREST_BLEND;
  TextureFormatSpecifier s_clamp = CLAMP_TO_EDGE;
  TextureFormatSpecifier t_clamp = CLAMP_TO_EDGE;
  TextureFormatSpecifier pixel_format = BGRA;
} default_texture_parameters;

// TODO delete this when I finish switching to the new asset path
static void update_texture(GameAssets *assets, BitmapID bitmap_id, TextureFormatSpecifier pixel_format = BGRA) {
  auto texture = get_bitmap(assets, bitmap_id);
  if (!texture) return;
  assert(texture->texture_id);
  //assert(texture->texture_id < BITMAP_COUNT);
  gl_check_error();
  glBindTexture(GL_TEXTURE_2D, texture->texture_id);
  gl_check_error();
  // NOTE switch to GL_SRGB8_ALPHA8 if you want srgb (gamma correction)
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture->width, texture->height, 0,
               to_gl_format_specifier(pixel_format), GL_UNSIGNED_BYTE, texture->buffer);
  
  // NOTE : It is a bit weird to put this here, but it makes things simpler for now.
  TextureIndex index = get_texture_index(bitmap_id);
  assets->texture_infos[index] = {texture->texture_id, float(texture->width), float(texture->height)};
}

static void update_texture(GameAssets *assets, TextureGroupID id, TextureFormatSpecifier pixel_format = BGRA) {
  auto texture = get_texture_group(assets, id);
  if (!texture) return;
  assert(texture->render_id);
  gl_check_error();
  glBindTexture(GL_TEXTURE_2D, texture->render_id);
  gl_check_error();
  // NOTE switch to GL_SRGB8_ALPHA8 if you want srgb (gamma correction)
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture->bitmap.width, texture->bitmap.height, 0,
               to_gl_format_specifier(pixel_format), GL_UNSIGNED_BYTE, texture->bitmap.buffer);
  
  // NOTE : It is a bit weird to put this here, but it makes things simpler for now.
  TextureIndex index = get_texture_index(id);
  assets->texture_infos[index] = {texture->render_id, float(texture->bitmap.width), float(texture->bitmap.height)};
}

// TODO delete this when I finish switching to the new asset path
static void init_texture(GameAssets *assets, BitmapID bitmap_id, TextureParameters param = default_texture_parameters) {
  auto texture = get_bitmap(assets, bitmap_id);
  if (!texture) return;
  u32 texture_id;
  glGenTextures(1, &texture_id);
  assert(texture_id);
  //assert(texture->texture_id < BITMAP_COUNT);
  texture->texture_id = texture_id;
  update_texture(assets, bitmap_id, param.pixel_format);
  gl_check_error();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, to_gl_format_specifier(param.min_blend));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, to_gl_format_specifier(param.max_blend));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, to_gl_format_specifier(param.s_clamp));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, to_gl_format_specifier(param.t_clamp));
  gl_check_error();
}

static void init_texture(GameAssets *assets, TextureGroupID id, TextureParameters param = default_texture_parameters) {
  auto texture = get_texture_group(assets, id);
  if (!texture) return;
  u32 texture_id;
  glGenTextures(1, &texture_id);
  assert(texture_id);
  //assert(texture->texture_id < BITMAP_COUNT);
  texture->render_id = texture_id;
  update_texture(assets, id, param.pixel_format);
  gl_check_error();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, to_gl_format_specifier(param.min_blend));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, to_gl_format_specifier(param.max_blend));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, to_gl_format_specifier(param.s_clamp));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, to_gl_format_specifier(param.t_clamp));
  gl_check_error();
}

static u32 init_font(GameAssets *assets, SizedFontID font_id, PixelBuffer bitmap) {
  u32 texture_id;
  glGenTextures(1, &texture_id);
  ASSERT(texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, bitmap.width, bitmap.height, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap.buffer);
  GLint swizzle_mask[] = {GL_RED, GL_RED, GL_RED, GL_RED};
  glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle_mask);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // NOTE : It is a bit weird to put this here, but it makes things simpler for now.
  TextureIndex index = get_texture_index(font_id);
  assets->texture_infos[index] = {texture_id, float(bitmap.width), float(bitmap.height)};

  return font_id;
}


#endif // _NAR_GL_RENDERER_CPP_
