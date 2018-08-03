#ifndef _NAR_GL_RENDERER_CPP_
#define _NAR_GL_RENDERER_CPP_


#if 1
#define gl_check_error() _gl_check_error(__FILE__, __LINE__)
#else
#define gl_check_error() 
#endif
static inline void _gl_check_error(const char *file_name, int line_num) {
  GLenum err;
  while ((err = glGetError()) != GL_NO_ERROR) {
    printf("OpenGL error: 0x%x in file %s, on line %d\n", err, file_name, line_num);
  }
}

// TODO pass these around more elegantly?
//static GLuint transform_id;
static GLuint projection_matrix_id;
static GLuint view_matrix_id;
static GLuint program_id;
static GLuint vertex_buffer_id;
static GLuint texture_sampler_id;

enum ShaderAttribute {
  VERTEX_POSITION,
  VERTEX_UV,
  VERTEX_COLOR,
  VERTEX_NORMAL,
  VERTEX_TANGENT,
};

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

static void draw_grad_bg_internal(PixelBuffer buf, uint32_t game_ticks, int y_start, int y_end) {
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
      uint32_t *pixel = (uint32_t *) (row + (x * pixel_bytes));

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
  uint32_t game_ticks;
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
  auto bg_args = alloc_array(&g->temp_allocator, DrawGradientBackgroundWork, num_splits);
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

enum RenderType : uint32_t {
  // TODO switch to RenderElementTextureTris
  RenderType_RenderElementTextureQuads,
  RenderType_RenderElementClear,
  RenderType_RenderElementHud,
  // TODO
  //RenderType_RenderElementTransform
};

struct RenderElement {
  RenderElement *next;
  RenderType type;
};

struct RenderElementTextureQuads {
  RenderElement head;
  uint32_t texture_id;
  uint32_t normal_map_id;
  int vertex_index;
  int quad_count;
};

struct RenderElementHud {
  RenderElement head;
  uint32_t texture_id;
  int vertex_index;
  int quad_count;
};


struct RenderElementClear {
  RenderElement head;
  V4 color;
  float depth;
};

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

#define RENDER_STAGE_COUNT 2

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

#define Z_BIAS_EPSILON 0.000001f
#define Z_BIAS_HUD_BASE 0.01

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
  buffer->max_vertices = 4096 * 8;
  buffer->vertex_count = 0;
  int vertex_buffer_size = buffer->max_vertices * sizeof(Vertex);
  int total_buffer_size = vertex_buffer_size + 4096 * 64;
  printf("Total render buffer size : %d\n", total_buffer_size);

  auto temp = new_push_allocator(total_buffer_size);
  buffer->vertices = alloc_array(&temp, Vertex, buffer->max_vertices);
  assert(buffer->vertices);
  buffer->allocator = new_push_allocator(&temp, remaining_size(&temp));
  assert(is_initialized(&buffer->allocator));

  init_render_stage(buffer->stages + 0, 
      RenderStage::PERSPECTIVE, RenderStage::CAMERA_VIEW, 
      RenderStage::CLEAR_DEPTH | RenderStage::CLEAR_COLOR | 
      RenderStage::ENABLE_DEPTH_TEST | RenderStage::HAS_LIGHTING);

  init_render_stage(buffer->stages + 1, 
      RenderStage::ORTHOGRAPHIC, RenderStage::IDENTITY, 
      0);

  clear(buffer);
}

#if 0
static void print_render_buffer(RenderBuffer *buffer) {
  printf("Printing RenderBuffer ::\n\n");
  printf("screen : %d by %d pixels\n", buffer->screen_width, buffer->screen_height);
  printf("camera : %f by %f meters\n", buffer->camera_width, buffer->camera_height);
  printf("element_count : %d\n", buffer->element_count);
  printf("max_size : %d\n", buffer->allocator.max_size);
  printf("bytes_allocated : %d\n\n", buffer->allocator.bytes_allocated);

  auto elem = (RenderElement *) buffer->allocator.memory;

  for (int i = 0; i < buffer->element_count; i++) {
    printf("Element %d, at address %p : \n", i, elem);
    switch (elem->type) {
      case RenderType_RenderElementRect : {
        auto e = (RenderElementRect *) elem;
        printf("Type : RenderElementRect\n");
        /* TODO
        auto r = e->rect;
        printf("Rect : (%f,%f),(%f,%f)\n", min_x(r), min_y(r), max_x(r), max_y(r));
        */
        auto c = e->color;
        printf("Color : (%f, %f, %f, %f)\n", c.r, c.g, c.b, c.a);
      } break;

      case RenderType_RenderElementTexture : {
        auto e = (RenderElementTexture *) elem;
        printf("Type : RenderElementTexture\n");
        /* TODO
        auto r = e->rect;
        printf("Rect : (%f,%f),(%f,%f)\n", min_x(r), min_y(r), max_x(r), max_y(r));
        */
        printf("Texture id : %d, size : %d by %d pixels\n", e->texture.texture_id, e->texture.width,  e->texture.height);
      } break;

      case RenderType_RenderElementBox : {
      } break;

      case RenderType_RenderElementTextureQuads : {
      } break;

    };
    printf("Next : %p\n\n", elem->next);
    elem = elem->next;
  }

  printf("Tail : %p\n\n", buffer->tail);
}
#endif

#define push_element(render_buffer, type) ((type *) push_element_((render_buffer), RenderType_##type, 0, sizeof(type)))
#define push_hud_element(render_buffer, type) ((type *) push_element_((render_buffer), RenderType_##type, 1, sizeof(type)))
static RenderElement *push_element_(RenderBuffer *buffer, RenderType type, u32 stage, u32 size) {
  TIMED_FUNCTION();
  auto elem = (RenderElement *) alloc_size(&buffer->allocator, size);
  if (!elem) {
    assert(!"RenderElement allocator is full.");
    return NULL;
  }

  elem->next = NULL;
  elem->type = type;

  auto render_stage = buffer->stages + stage;
  if (!render_stage->first) render_stage->first = elem;
  if (render_stage->tail) {
    assert(!render_stage->tail->next);
    render_stage->tail->next = elem;
  }
  render_stage->tail = elem;
  render_stage->element_count++;

#if 0
  if (is_hud) {
    if (!buffer->first_hud_element) buffer->first_hud_element = elem;
    if (buffer->hud_tail) {
      assert(!buffer->hud_tail->next);
      buffer->hud_tail->next = elem;
    }
    buffer->hud_tail = elem;
    buffer->hud_element_count++;
  } else {

    if (!buffer->first_element) buffer->first_element = elem;
    if (buffer->tail) {
      assert(!buffer->tail->next);
      buffer->tail->next = elem;
    }
    buffer->tail = elem;
    buffer->element_count++;
  }
#endif

  return elem;
}

static inline void push_vertex(RenderBuffer *buffer, Vertex v) {
  if (buffer->max_vertices - buffer->vertex_count <= 0) {
    assert(!"Vertex buffer overflowed.");
    return;
  }
  auto vert = buffer->vertices + buffer->vertex_count;
  *vert = v;
  buffer->vertex_count++;
}

struct VertexSOA {
  V4 *p, *c;
  V2 *uv;
  V3 *n, *t;
};

static inline Vertex get(VertexSOA verts, uint32_t index) {
  Vertex vert;
  vert.position = verts.p[index];
  vert.color = verts.c[index];
  vert.uv = verts.uv[index];
  vert.normal = verts.n[index];
  vert.tangent = verts.t[index];
  return vert;
}

static inline void push_quad_vertices(RenderBuffer *buffer, VertexSOA verts) {
  TIMED_FUNCTION();

  // NOTE Wraps counter clockwise around each triangle
  int idxs[6] = {0,1,2,0,2,3};

  for (int i = 0; i < 6; i++) {
    int idx = idxs[i];
    push_vertex(buffer, get(verts, idx));
  }
}

static void draw_vertices(int vertex_idx, int count, uint32_t texture_id, uint32_t normal_map_id) {
  TIMED_FUNCTION();

  assert(texture_id);
  //assert(texture_id < BITMAP_COUNT);
  //glUniform1i(texture_sampler_id, texture_id);
  gl_check_error();
  glActiveTexture(GL_TEXTURE0 + 0);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  gl_check_error();

  bool has_normal_map = normal_map_id ? true : false;
  glUniform1i(glGetUniformLocation(program_id, "HAS_NORMAL_MAP"), has_normal_map);

  if (has_normal_map) {
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, normal_map_id);
  }
  
  glEnableVertexAttribArray(VERTEX_POSITION);
  gl_check_error();
  glEnableVertexAttribArray(VERTEX_COLOR);
  gl_check_error();
  glEnableVertexAttribArray(VERTEX_UV);
  gl_check_error();
  glEnableVertexAttribArray(VERTEX_NORMAL);
  gl_check_error();
  glEnableVertexAttribArray(VERTEX_TANGENT);
  gl_check_error();

  // NOTE : the boolean is "normalized"
  glVertexAttribPointer(VERTEX_POSITION, 4, GL_FLOAT, false, 
      sizeof(Vertex), (void *)(offsetof(Vertex, position) + vertex_idx));
  gl_check_error();

  // TODO switch this to GL_UNSIGNED_BYTE
  glVertexAttribPointer(VERTEX_COLOR, 4, GL_FLOAT, false, 
      sizeof(Vertex), (void *)(offsetof(Vertex, color) + vertex_idx));
  gl_check_error();

  glVertexAttribPointer(VERTEX_UV, 2, GL_FLOAT, false, 
      sizeof(Vertex), (void *)(offsetof(Vertex, uv) + vertex_idx));
  gl_check_error();

  glVertexAttribPointer(VERTEX_NORMAL, 3, GL_FLOAT, false,
      sizeof(Vertex), (void *)(offsetof(Vertex, normal) + vertex_idx));
  gl_check_error();

  glVertexAttribPointer(VERTEX_TANGENT, 3, GL_FLOAT, false,
      sizeof(Vertex), (void *)(offsetof(Vertex, tangent) + vertex_idx));
  gl_check_error();

  glDrawArrays(GL_TRIANGLES, 0, count);
  gl_check_error();

  glDisableVertexAttribArray(VERTEX_POSITION);
  gl_check_error();
  glDisableVertexAttribArray(VERTEX_COLOR);
  gl_check_error();
  glDisableVertexAttribArray(VERTEX_UV);
  gl_check_error();
  glDisableVertexAttribArray(VERTEX_NORMAL);
  gl_check_error();
  glDisableVertexAttribArray(VERTEX_TANGENT);
  gl_check_error();
}

// FIXME TODO This is super broken, but I don't care that much at the moment. If you push any non-hud elements after pushing hud elements,
// it will probably break stuff.
// UPDATE : This might be fixed now, should test it soon.
static inline void append_quads(RenderBuffer *buffer, int count, uint32_t texture_id, uint32_t normal_map_id, u32 render_stage = 0) {
  TIMED_FUNCTION();
  auto stage = buffer->stages + render_stage;
  auto prev = stage->tail;
  if (prev && prev->type == RenderType_RenderElementTextureQuads) {
    auto old_quads = (RenderElementTextureQuads *) prev;
    if (old_quads->texture_id == texture_id && old_quads->normal_map_id == normal_map_id) {
      old_quads->quad_count += count;
      return;
    }
  }

  // TODO Clean this up, I think we should have a separate Element type for having no normal map and use it whenever normal_map_id is 0.
  if (render_stage == 1) {
    auto elem = push_hud_element(buffer, RenderElementHud);

    assert(!normal_map_id);
    elem->texture_id = texture_id;
    elem->quad_count = count;
    elem->vertex_index = buffer->vertex_count - count * 6;

  } else {
    auto elem = push_element(buffer, RenderElementTextureQuads);

    elem->texture_id = texture_id;
    elem->normal_map_id = normal_map_id;
    elem->quad_count = count;
    elem->vertex_index = buffer->vertex_count - count * 6;
  }
}

#if 0
// TODO maybe combine this with the other function
static void push_box(RenderBuffer *buffer, AlignedBox box, V4 color, uint32_t texture_id, uint32_t normal_map_id = 0) {
  TIMED_FUNCTION();
#define DEBUG_COLORS 0
#if DEBUG_COLORS
  V4 debug_colors[6] = {{1,0,0,1},{0,1,0,1},{0,0,1,1},{1,0.5,0,1},{0,1,0.5,1},{0.5,0,1,1}};
#endif

  auto b = box;
  Quad faces[6] = {top_quad(b), front_quad(b), right_quad(b), 
                   left_quad(b), back_quad(b), bot_quad(b)};

  auto c = color;
  // NOTE for gamma correction :
  //c.rgb *= c.rgb;
  
  V2 t[4] = {{0,1}, {1,1}, {1,0}, {0,0}};
  V3 n[6] = {{0,0,1},{0,-1,0},{1,0,0},{-1,0,0},{0,1,0},{0,0,-1}};

  for (int i = 0; i < 6; i++) {

#if DEBUG_COLORS
    c = debug_colors[i];
#endif
    V4 c4[4] = {c,c,c,c};
    V3 n4[4] = {n[i],n[i],n[i],n[i]};
    //c.rgb *= 0.8;
    push_quad_vertices(buffer, faces[i].verts, t, c4, n4);
  }
  
  append_quads(buffer, 6, texture_id);
#undef DEBUG_COLORS
}
#endif


static void push_box(RenderBuffer *buffer, AlignedBox box, V4 color, BitmapID texture_asset_id, float scale, uint32_t normal_map_id = 0) {
  TIMED_FUNCTION();
#define DEBUG_COLORS 0
#if DEBUG_COLORS
  V4 debug_colors[6] = {{1,0,0,1},{0,1,0,1},{0,0,1,1},{1,0.5,0,1},{0,1,0.5,1},{0.5,0,1,1}};
#endif

  auto texture = get_bitmap(buffer->assets, texture_asset_id);
  auto texture_id = get_texture_id(buffer->assets, texture_asset_id);
  float tex_width = texture->width * METERS_PER_PIXEL * scale;
  float tex_height = texture->height * METERS_PER_PIXEL * scale;

  assert(tex_width);
  assert(tex_height);
  auto b = box;

  Quad4 quads[6];
  to_quad4s(box, quads);

  AlignedRect rects[6] = {top_rect(b), front_rect(b), right_rect(b), 
                          left_rect(b), back_rect(b), bot_rect(b)};

  auto c = color;
  // NOTE for gamma correction :
  //c.rgb *= c.rgb;
  
  float x_scale = 1;
  float y_scale = 1;

  V3 n[6] = {{0,0,1},{0,-1,0},{1,0,0},{-1,0,0},{0,1,0},{0,0,-1}};

  V3 t[6] = {{1,0,0},{1,0,0},{0,1,0},{0,-1,0},{-1,0,0},{-1,0,0}};

  for (int i = 0; i < 6; i++) {
    auto dim = rects[i].offset;

    if (tex_width) x_scale = abs(dim.x / tex_width);
    if (tex_height) y_scale = abs(dim.y / tex_height);

#if DEBUG_COLORS
    c = debug_colors[i];
#endif

    V2 uv[4] = {{0,y_scale}, {x_scale,y_scale}, {x_scale,0}, {0,0}};
    V4 c4[4] = {c,c,c,c};
    V3 n4[4] = {n[i],n[i],n[i],n[i]};
    V3 t4[4] = {t[i],t[i],t[i],t[i]};

    VertexSOA verts;
    verts.p = quads[i].verts;
    verts.uv = uv;
    verts.c = c4;
    verts.n = n4;
    verts.t = t4;

    push_quad_vertices(buffer, verts);
    //c.rgb *= 0.8;
  }
  
  append_quads(buffer, 6, texture_id, normal_map_id);
#undef DEBUG_COLORS
}

static void push_rectangle(RenderBuffer *buffer, Rectangle rect, V4 color, uint32_t texture_id, uint32_t normal_map_id = 0) {
  TIMED_FUNCTION();

  auto quad = to_quad4(rect);

  V2 uv[4] = {{0,1}, {1,1}, {1,0}, {0,0}};
  auto c = color;
  // NOTE for gamma correction :
  //c.rgb *= c.rgb;
  
  V4 c4[4] = {c,c,c,c};

  // TODO calculate this for non-axis-aligned rectangles
  V3 n = {0,0,1};
  V3 n4[4] = {n,n,n,n};

  V3 t = {1,0,0};
  V3 t4[4] = {t,t,t,t};

  VertexSOA verts;
  verts.p = quad.verts;
  verts.uv = uv;
  verts.c = c4;
  verts.n = n4;
  verts.t = t4;

  push_quad_vertices(buffer, verts);

  append_quads(buffer, 1, texture_id, normal_map_id);
}

static void render_pixel_space(RenderBuffer *buffer, Rectangle rect, V4 color, BitmapID bitmap_id) {
  TIMED_FUNCTION();

  //float z_bias = buffer->camera->p.z - buffer->camera->near_dist + buffer->z_bias_accum;
  //buffer->z_bias_accum += Z_BIAS_EPSILON;

  auto texture_id = get_texture_id(buffer->assets, bitmap_id);
  auto quad = to_quad4(rect);

  V2 uv[4] = {{0,1}, {1,1}, {1,0}, {0,0}};
  auto c = color;
  // NOTE for gamma correction :
  //c.rgb *= c.rgb;
  
  V4 c4[4] = {c,c,c,c};

  V3 n = {0,0,1};
  V3 n4[4] = {n,n,n,n};

  V3 t = {1,0,0};
  V3 t4[4] = {t,t,t,t};

  VertexSOA verts;
  verts.p = quad.verts;
  verts.uv = uv;
  verts.c = c4;
  verts.n = n4;
  verts.t = t4;

  push_quad_vertices(buffer, verts);

  append_quads(buffer, 1, texture_id, 0, true);
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

// Expand on these functions and use them throughout the file
inline void push_quad_vert_helper(RenderBuffer *buffer, Quad4 *quad, V2 *uv, V4 color) {
  auto c = color;
  assert(uv);
  // NOTE for gamma correction :
  //c.rgb *= c.rgb;
  
  V4 c4[4] = {c,c,c,c};

  V3 n = {0,0,1};
  V3 n4[4] = {n,n,n,n};

  V3 t = {1,0,0};
  V3 t4[4] = {t,t,t,t};

  VertexSOA verts;
  verts.p = quad->verts;
  verts.uv = uv;
  verts.c = c4;
  verts.n = n4;
  verts.t = t4;

  push_quad_vertices(buffer, verts);
}

static void render_shadowed_text_pixel_space(RenderBuffer *buffer, V2 text_p, const char *text, V4 color, V4 shadow_color, int shadow_depth, uint32_t font_id) { 
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

  float font_width  = font_info->bitmap.width;
  float font_height = font_info->bitmap.height;
  while (*text) {
    if (*text >= ' ' && *text <= '~') {
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
        push_quad_vert_helper(buffer, &quad, uv, shadow_color);
        num_quads++;
        translate(&quad, -shadow_offset);
      }

      push_quad_vert_helper(buffer, &quad, uv, color);

      num_quads++;
      pixel_p.x += b->xadvance;

    } else {
      assert(!"Text character not in range.");
    }
    text++;
  }
  append_quads(buffer, num_quads, font_info->bitmap.texture_id, 0, true);
}

inline void render_text_pixel_space(RenderBuffer *buffer, V2 text_p, const char *text, V4 color, uint32_t font_id) {
  render_shadowed_text_pixel_space(buffer, text_p, text, color, vec4(0), 0, font_id);
}

inline void render_shadowed_text_screen_space(RenderBuffer *buffer, V2 text_p, const char *text, V4 color, V4 shadow_color, int shadow_depth, uint32_t font_id) { 
  V2 text_p_pixel_space = text_p * vec2(buffer->screen_width, buffer->screen_height);
  render_shadowed_text_pixel_space(buffer, text_p_pixel_space, text, color, shadow_color, shadow_depth, font_id);
}

inline void render_text_screen_space(RenderBuffer *buffer, V2 text_p, const char *text, V4 color, uint32_t font_id) { 
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

  render_shadowed_text_screen_space(buffer, cursor_p, dest, vec4(1), vec4(0,0,0,1), 2, FONT_DEBUG);
}

// TODO This is a weird place to put this, but right now it is inconvenient to put it anywhere else.
static void render_frame_records(RenderBuffer *buffer) {
  // NOTE : This is basically copied from print_frame_records.
#define TAB "   "
 
  u32 const screen_lines = 28;
  float const newline_height = 1.0f / screen_lines;
  float left_margin = 1.f/12;
  u32 const max_lines = 26;

  V2 cursor_p = { left_margin, 1 - newline_height * (screen_lines - max_lines) };

  static darray<uint32_t> block_ids;

  if (!block_ids) for (uint32_t block_id = 0; block_id < debug_global_memory.record_count; block_id++) {
    push(block_ids, block_id);
  }

  quick_sort<uint32_t, block_id_compare>(block_ids, count(block_ids));

  uint32_t lines = 0;

  uint32_t current_frame = debug_global_memory.current_frame % NUM_FRAMES_RECORDED;
  render_debug_string(buffer, cursor_p + V2{7.f/12, 0} , "Total Frames : %llu", debug_global_memory.current_frame);
  cursor_p.y -= newline_height;
  lines++;

  render_debug_string(buffer, cursor_p, TAB "Thread |    Cycles |  Internal | Hits | Cycles/Hit |     Average");
  cursor_p.y -= newline_height;
  lines++;

  for (uint32_t i = 0; i < count(block_ids) && lines < max_lines - 1; i++) {
    uint32_t block_id = block_ids[i];
    FunctionInfo *info = debug_global_memory.function_infos + block_id;
    if (!info->filename) continue;
    DebugRecord *records[NUM_THREADS];
    FrameRecord *frames[NUM_THREADS];

    for (uint32_t thread_idx = 0; thread_idx < NUM_THREADS; thread_idx++) {
      records[thread_idx] = debug_global_memory.debug_logs[thread_idx].records + block_id;
      frames[thread_idx] = &records[thread_idx]->frames[current_frame];
    }

    render_debug_string(buffer, cursor_p, "%-24s in %-24s (line %4u) : ",
                        info->function_name, 
                        info->filename, 
                        info->line_number);
    cursor_p.y -= newline_height;
    lines++;

    for (uint32_t thread_idx = 0; thread_idx < NUM_THREADS && lines < max_lines; thread_idx++) {
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

  u32 bitmap_id = get_texture_id(buffer->assets, BITMAP_WHITE);
  u32 normal_map_id = 0;

  V2 uv4[4] = {{0,1}, {1,1}, {1,0}, {0,0}};
  auto c = color;
  // NOTE for gamma correction :
  //c.rgb *= c.rgb;
  
  V4 c4[4] = {c,c,c,c};

  V3 n = {0,0,1};
  V3 n4[4] = {n,n,n,n};

  V3 t = {1,0,0};
  V3 t4[4] = {t,t,t,t};

  VertexSOA verts;
  verts.p = quad.verts;
  verts.uv = uv4;
  verts.c = c4;
  verts.n = n4;
  verts.t = t4;

  push_quad_vertices(buffer, verts);
  append_quads(buffer, 1, bitmap_id, normal_map_id);

  verts.p = quad2.verts;
  push_quad_vertices(buffer, verts);
  append_quads(buffer, 1, bitmap_id, normal_map_id);
}

static RenderingInfo get_render_info(GameAssets *assets, Entity *e);

// TODO switch everthing to use the same path
static void render_entity(RenderBuffer *buffer, Entity *e) {
#define DEBUG_CUBES 0
  TIMED_FUNCTION();

  RenderingInfo info = get_render_info(buffer->assets, e);
  if (info.texture_uv == vec4(0)) return;
  //PRINT_V4(info.texture_uv);
  assert(info.normal_map_uv == vec4(0,0,0,0));
  assert(info.bitmap_id);


  uint32_t bitmap_id = info.bitmap_id;
  uint32_t normal_map_id = 0;

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
  V2 uv4[4] = {{uv.x,uv.w}, uv.zw, {uv.z,uv.y}, uv.xy};
  auto c = info.color;
  // NOTE for gamma correction :
  //c.rgb *= c.rgb;
  
  V4 c4[4] = {c,c,c,c};

  V3 n = normalize(Z + vec3(0,-2,0)); // TODO Figure this out for real
  V3 n4[4] = {n,n,n,n};

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
  V3 t4[4] = {t,t,t,t};

  VertexSOA verts;
  verts.p = quad.verts;
  verts.uv = uv4;
  verts.c = c4;
  verts.n = n4;
  verts.t = t4;

  // DELETEME
  //render_arrow(buffer, r.center, r.center + n, vec4(0.1,0,0,1), 0.1);
  //render_arrow(buffer, r.center, r.center + t, vec4(0,0,0.3,1), 0.1);

  push_quad_vertices(buffer, verts);

  append_quads(buffer, 1, bitmap_id, normal_map_id);

#if DEBUG_CUBES
  push_box(buffer, box, vec4(0.9, 1, 0, 0.2), BITMAP_WHITE, 1);
#endif
#undef DEBUG_CUBES
}

// TODO massively rework this to simplify and get better z biasing.
static void push_sprite(RenderBuffer *buffer, AlignedBox box, VisualInfo visual_info) {
#define DEBUG_CUBES 0
  TIMED_FUNCTION();

  auto texture = get_bitmap(buffer->assets, visual_info.texture_id);
  auto texture_id = get_texture_id(buffer->assets, visual_info.texture_id);
  auto normal_map_id = get_texture_id(buffer->assets, visual_info.normal_map_id);

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

  V2 uv[4] = {{0,1}, {1,1}, {1,0}, {0,0}};
  auto c = visual_info.color;
  // NOTE for gamma correction :
  //c.rgb *= c.rgb;
  
  V4 c4[4] = {c,c,c,c};

  V3 n = -Z;
  V3 n4[4] = {n,n,n,n};

  /*
  float z_min = visual_info.offset.z;
  float z_max = visual_info.sprite_height;
  */

  float z_min = 0;
  float z_max = visual_info.sprite_depth; //0.3; // TODO figure this out for real

  float bias[4] = {z_min, z_min, z_max, z_max};
  for (int i = 0; i < 4; i++) {
    quad.verts[i].w = bias[i];
  }

  // NOTE : This is always correct if the sprite isn't skewed.
  V3 t = X; //{1,0,0};
  V3 t4[4] = {t,t,t,t};

  VertexSOA verts;
  verts.p = quad.verts;
  verts.uv = uv;
  verts.c = c4;
  verts.n = n4;
  verts.t = t4;


  push_quad_vertices(buffer, verts);

  append_quads(buffer, 1, texture_id, normal_map_id);

#if DEBUG_CUBES
  push_box(buffer, box, vec4(0.9, 1, 0, 0.5), BITMAP_WHITE, 1);
#endif
#undef DEBUG_CUBES
}

static void set_vertex_buffer(RenderBuffer *buffer, int start_idx, int vertex_count) {
  auto vertices = buffer->vertices + start_idx;
  uint32_t size = sizeof(Vertex) * vertex_count;
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

  // TODO make the input coordinates be in the right place...
  float identity_mat[] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1,
    //-8, -6, -10, 1,
  };

  glClearColor(1,1,0,1);
  gl_check_error();
  glClearDepth(1);
  gl_check_error();

  glUseProgram(program_id);
  gl_check_error();


  auto normal_sampler_id = glGetUniformLocation(program_id, "NORMAL_SAMPLER");

  glUniform1i(texture_sampler_id, 0); // Texture unit 0 is for the texture
  glUniform1i(normal_sampler_id, 1); // Texture unit 1 is for normal maps.

  auto light_p_id = glGetUniformLocation(program_id, "LIGHT_P");
  V3 light_p = vec3(debug_global_memory.game_state->pointer_position, 0.5);
  
  //V3 light_p = {7,9,2};
  glUniform3fv(light_p_id, 1, light_p.elements);

  // DEBUG
  //print_render_buffer(buffer);

  //
  // Render Game
  //

  auto default_texture_id = get_texture_id(buffer->assets, BITMAP_WHITE);

  for (u32 i = 0; i < RENDER_STAGE_COUNT; i++) {
    auto stage = buffer->stages + i;
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

    u32 clear_bits = 0;
    if (stage->flags & RenderStage::CLEAR_COLOR) clear_bits |= GL_COLOR_BUFFER_BIT;
    if (stage->flags & RenderStage::CLEAR_DEPTH) clear_bits |= GL_DEPTH_BUFFER_BIT;
    if (clear_bits) glClear(clear_bits);

    switch (stage->projection) {
      case RenderStage::ORTHOGRAPHIC :
        glUniformMatrix4fv(projection_matrix_id, 1, GL_FALSE, orthographic_proj); break;
      case RenderStage::PERSPECTIVE :
        glUniformMatrix4fv(projection_matrix_id, 1, GL_FALSE, perspective_proj); break;
      case RenderStage::INFINITE_FAR_PLANE :
        glUniformMatrix4fv(projection_matrix_id, 1, GL_FALSE, infinite_proj); break;
      default :
        assert(!"Invalid stage projection.");
    }

    switch (stage->view) {
      case RenderStage::IDENTITY :
        glUniformMatrix4fv(view_matrix_id, 1, GL_FALSE, identity_mat); break;
      case RenderStage::CAMERA_VIEW :
        glUniformMatrix4fv(view_matrix_id, 1, GL_FALSE, view_mat); break;
      default :
        assert(!"Invalid stage view.");
    }

    auto elem = stage->first;

    for (int i = 0; i < stage->element_count; i++) {
      assert(elem);
      gl_check_error();

      uint32_t texture_id = default_texture_id;

      switch (elem->type) {

        case RenderType_RenderElementTextureQuads : {
          auto e = (RenderElementTextureQuads *) elem;

          set_vertex_buffer(buffer, e->vertex_index, e->quad_count * 6);
          if (e->texture_id) texture_id = e->texture_id;

          draw_vertices(0, e->quad_count * 6, texture_id, e->normal_map_id);
        } break;

        case RenderType_RenderElementHud : {
          auto e = (RenderElementHud *) elem;

          set_vertex_buffer(buffer, e->vertex_index, e->quad_count * 6);
          if (e->texture_id) texture_id = e->texture_id;

          draw_vertices(0, e->quad_count * 6, texture_id, 0);
        } break;

        case RenderType_RenderElementClear : {
          //auto e = (RenderElementClear *) elem;
          // TODO

        } break;

        default : {
          printf("Invalid element : %d, index : %d\n", elem->type, i);

          assert(!"Invalid element.");
        };
      }

      // TODO figure out why this crashes when I don't check first?
      if (elem->next) {
        elem = elem->next;
      } else {
        break;
      }
    }
    if (stage->element_count) assert(!elem->next);
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
}

// TODO delete this when I finish switching to the new asset path
static void init_texture(GameAssets *assets, BitmapID bitmap_id, TextureParameters param = default_texture_parameters) {
  auto texture = get_bitmap(assets, bitmap_id);
  if (!texture) return;
  uint32_t texture_id;
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
  uint32_t texture_id;
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


#endif // _NAR_GL_RENDERER_CPP_
