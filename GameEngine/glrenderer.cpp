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

enum TextureFormatSpecifier : uint32_t {
  RGBA = GL_RGBA,
  BGRA = GL_BGRA,
  CLAMP_TO_EDGE = GL_CLAMP_TO_EDGE,
  REPEAT_CLAMPING = GL_REPEAT,
  LINEAR_BLEND = GL_LINEAR,
  NEAREST_BLEND = GL_NEAREST
};

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
  // TODO
  //RenderType_RenderElementClear,
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

// TODO we may also want to clear the depth buffer
struct RenderElementClear {
  RenderElement head;
  V4 color;
};

struct Vertex {
  V4 position;
  V4 color;
  V2 uv;
  V3 normal;
  V3 tangent;
};

struct RenderBuffer {
  PushAllocator allocator;
  GameAssets *assets;
  GameCamera *camera;
  RenderElement *tail;
  Vertex *vertices;
  int element_count;
  int vertex_count;
  int max_vertices;
  int screen_width;
  int screen_height;

  float z_bias_accum;
};

#define Z_BIAS_EPSILON 0.000001f
#define Z_BIAS_HUD_BASE 0.01

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

#define push_element(render_buffer, type) ((type *) push_element_((render_buffer), RenderType_##type, sizeof(type)))
static RenderElement *push_element_(RenderBuffer *buffer, RenderType type, uint32_t size) {
  TIMED_FUNCTION();
  auto elem = (RenderElement *) alloc_size(&buffer->allocator, size);
  if (!elem) {
    assert(false);
    return NULL;
  }

  if (buffer->tail) {
    assert(!buffer->tail->next);
    buffer->tail->next = elem;
  }
  buffer->tail = elem;

  elem->next = NULL;
  elem->type = type;

  buffer->element_count++;
  return elem;
}

static inline void clear(RenderBuffer *buffer) {
  clear(&buffer->allocator);
  buffer->tail = NULL;
  buffer->element_count = 0;
  buffer->vertex_count = 0;
  buffer->z_bias_accum = Z_BIAS_HUD_BASE;
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

// static inline void push_quad_vertices(RenderBuffer *buffer, V3 *p, V2 *t, V4 *c, V3 *n, float *bias = NULL) {
static inline void push_quad_vertices(RenderBuffer *buffer, VertexSOA verts) { //, float *bias = NULL) {
  TIMED_FUNCTION();

  // NOTE Wraps counter clockwise around each triangle
  int idxs[6] = {0,1,2,0,2,3};
  //float default_bias[] = {0,0,0,0};
  //if (!bias) bias = default_bias;

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
  glUniform1i(glGetUniformLocation(program_id, "has_normal_map"), has_normal_map);

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

static inline void append_quads(RenderBuffer *buffer, int count, uint32_t texture_id, uint32_t normal_map_id = 0) {
  TIMED_FUNCTION();
  auto prev = buffer->tail;
  if (prev && prev->type == RenderType_RenderElementTextureQuads) {
    auto old_quads = (RenderElementTextureQuads *) prev;
    if (old_quads->texture_id == texture_id && old_quads->normal_map_id == normal_map_id) {
      old_quads->quad_count += count;
      return;
    }
  }

  auto elem = push_element(buffer, RenderElementTextureQuads);

  elem->texture_id = texture_id;
  elem->normal_map_id = normal_map_id;
  elem->quad_count = count;
  elem->vertex_index = buffer->vertex_count - count * 6;
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

// TODO move this somewhere reasonable
static inline void to_vec4s(V3 *old, V4 *result, uint32_t count) {
  for (uint32_t i = 0; i < count; i++) {
    V4 v = {};
    v.xyz = old[i];
    result[i] = v;
  }
}

static inline void get_faces_v4(AlignedBox box, V4 *result) {
  to_vec4s(top_quad(box).verts, result + 0, 4);
  to_vec4s(front_quad(box).verts, result + 4, 4);
  to_vec4s(right_quad(box).verts, result + 8, 4);
  to_vec4s(left_quad(box).verts, result + 12, 4);
  to_vec4s(back_quad(box).verts, result + 16, 4);
  to_vec4s(bot_quad(box).verts, result + 20, 4);
}

static void push_box(RenderBuffer *buffer, AlignedBox box, V4 color, uint32_t texture_id, float tex_width, float tex_height, uint32_t normal_map_id = 0) {
  TIMED_FUNCTION();
#define DEBUG_COLORS 0
#if DEBUG_COLORS
  V4 debug_colors[6] = {{1,0,0,1},{0,1,0,1},{0,0,1,1},{1,0.5,0,1},{0,1,0.5,1},{0.5,0,1,1}};
#endif

  assert(tex_width);
  assert(tex_height);
  auto b = box;

  V4 vertices[6][4];
  get_faces_v4(box, vertices[0]);

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
    verts.p = vertices[i];
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

  auto quad = to_quad(rect);
  V4 vertices[4];
  to_vec4s(quad.verts, vertices, 4);

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
  verts.p = vertices;
  verts.uv = uv;
  verts.c = c4;
  verts.n = n4;
  verts.t = t4;

  push_quad_vertices(buffer, verts);

  append_quads(buffer, 1, texture_id, normal_map_id);
}

static void push_hud(RenderBuffer *buffer, Rectangle rect, V4 color, uint32_t texture_id, uint32_t normal_map_id = 0) {
  TIMED_FUNCTION();

  auto quad = to_quad(rect);
  V4 vertices[4];
  to_vec4s(quad.verts, vertices, 4);

  V2 uv[4] = {{0,1}, {1,1}, {1,0}, {0,0}};
  auto c = color;
  // NOTE for gamma correction :
  //c.rgb *= c.rgb;
  
  V4 c4[4] = {c,c,c,c};

  V3 n = {0,0,1};
  V3 n4[4] = {n,n,n,n};
  float z_bias = buffer->camera->p.z - buffer->camera->near_dist + buffer->z_bias_accum;
  buffer->z_bias_accum += Z_BIAS_EPSILON;
  //float z_bias = 4.5;
  //float bias[4] = {z_bias, z_bias, z_bias, z_bias};
  for (int i = 0; i < 4; i++) {
    vertices[i].w = z_bias;
  }

  V3 t = {1,0,0};
  V3 t4[4] = {t,t,t,t};

  VertexSOA verts;
  verts.p = vertices;
  verts.uv = uv;
  verts.c = c4;
  verts.n = n4;
  verts.t = t4;

  push_quad_vertices(buffer, verts);

  append_quads(buffer, 1, texture_id, normal_map_id);
}


static void push_hud_text(RenderBuffer *buffer, V2 text_p, const char *text, V4 color, uint32_t font_id) { 
  // TODO figure out a way to time this that doesn't interfere with drawing debug text
  //TIMED_FUNCTION();

  auto assets = buffer->assets;
  int num_quads = 0;
  V2 current_p = text_p; // Upper left corner
  V2 pixel_p = current_p * PIXELS_PER_METER;

  auto font_info = get_font(assets, font_id);
  assert(font_info);
  if (!font_info) return;

  float font_width  = font_info->bitmap.width;
  float font_height = font_info->bitmap.height;
  while (*text) {
    if (*text >= ' ' && *text <= '~') {
      auto b = get_baked_char(font_info, *text);
      auto p = round(pixel_p + V2{b->xoff, -b->yoff}) * METERS_PER_PIXEL;

      float char_width  = (b->x1 - b->x0) * METERS_PER_PIXEL;
      float char_height = (b->y1 - b->y0) * METERS_PER_PIXEL;
      auto quad = to_quad(aligned_rect(p.x, p.y - char_height, p.x + char_width, p.y));
      V4 vertices[4];
      to_vec4s(quad.verts, vertices, 4);

      V2 uv[4] = {
                 {b->x0 / font_width, b->y1 / font_height}, 
                 {b->x1 / font_width, b->y1 / font_height}, 
                 {b->x1 / font_width, b->y0 / font_height}, 
                 {b->x0 / font_width, b->y0 / font_height},
      };

      auto c = color;
      // NOTE for gamma correction :
      //c.rgb *= c.rgb;
      
      V4 c4[4] = {c,c,c,c};

      V3 n = {0,0,1};
      V3 n4[4] = {n,n,n,n};
      //float z_bias = 4.5f;
      float z_bias = buffer->camera->p.z - buffer->camera->near_dist + buffer->z_bias_accum;
      buffer->z_bias_accum += Z_BIAS_EPSILON;
      //float bias[4] = {z_bias, z_bias, z_bias, z_bias};
      for (int i = 0; i < 4; i++) {
        vertices[i].w = z_bias;
      }

      V3 t = {1,0,0};
      V3 t4[4] = {t,t,t,t};

      VertexSOA verts;
      verts.p = vertices;
      verts.uv = uv;
      verts.c = c4;
      verts.n = n4;
      verts.t = t4;

      push_quad_vertices(buffer, verts);
      num_quads++;
      pixel_p.x += b->xadvance; // * METERS_PER_PIXEL;

    } else {
      assert(!"Text character not in range.");
    }
    text++;
  }
  append_quads(buffer, num_quads, font_info->bitmap.texture_id);
}

// TODO I would like this to work with multiple colors...
// Idea : Choose a symbol that can't be drawn with push_hud_text use it to indicate a color change.
static void push_debug_string(RenderBuffer *buffer, V2 cursor_p, char const *format...) {
  char dest[1024 * 16];

  va_list args;
  va_start(args, format);
  vsnprintf(dest, sizeof(dest), format, args);
  va_end(args);

  float const shadow_depth = 0.02;
  push_hud_text(buffer, cursor_p + V2{shadow_depth, -shadow_depth}, dest, V4{0,0,0,1}, FONT_DEBUG);
  push_hud_text(buffer, cursor_p, dest, V4{1,1,1,1}, FONT_DEBUG);
}

// TODO This is a weird place to put this, but right now it is inconvenient to put it anywhere else.
static void draw_frame_records(RenderBuffer *buffer) {
  // NOTE : This is basically copied from print_frame_records.
#define TAB "   "
  V2 cursor_p = { 0.4, 11 };
  float const newline_height = 0.5f;

  static darray<uint32_t> block_ids;

  if (!block_ids) for (uint32_t block_id = 0; block_id < debug_global_memory.record_count; block_id++) {
    push(block_ids, block_id);
  }

  quick_sort<uint32_t, block_id_compare>(block_ids, count(block_ids));

  uint32_t lines = 0;
  uint32_t const max_lines = 26;

  uint32_t current_frame = debug_global_memory.current_frame % NUM_FRAMES_RECORDED;
  push_debug_string(buffer, cursor_p + V2{8, 0} , "Total Frames : %llu", debug_global_memory.current_frame);
  cursor_p.y -= newline_height;
  lines++;

  push_debug_string(buffer, cursor_p, TAB "Thread |    Cycles |  Internal | Hits | Cycles/Hit |     Average");
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

    push_debug_string(buffer, cursor_p, "%-24s in %-24s (line %4u) : ",
                      info->function_name, 
                      info->filename, 
                      info->line_number);
    cursor_p.y -= newline_height;
    lines++;

    for (uint32_t thread_idx = 0; thread_idx < NUM_THREADS && lines < max_lines; thread_idx++) {
      auto frame = frames[thread_idx];
      auto record = records[thread_idx];
      if (!frame->hit_count) continue;

      push_debug_string(buffer, cursor_p, 
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
    cursor_p.y -= newline_height / 2;
  }
#undef TAB
}

static void push_sprite(RenderBuffer *buffer, Rectangle rect, float z_min, float z_max, V4 color, uint32_t texture_id, uint32_t normal_map_id) {
  TIMED_FUNCTION();

  auto quad = to_quad(rect);
  V4 vertices[4];
  to_vec4s(quad.verts, vertices, 4);

  V2 uv[4] = {{0,1}, {1,1}, {1,0}, {0,0}};
  auto c = color;
  // NOTE for gamma correction :
  //c.rgb *= c.rgb;
  
  V4 c4[4] = {c,c,c,c};

  V3 n = {0,0,1};
  V3 n4[4] = {n,n,n,n};
  float bias[4] = {z_min, z_min, z_max, z_max};
  for (int i = 0; i < 4; i++) {
    vertices[i].w = bias[i];
  }

  V3 t = {1,0,0};
  V3 t4[4] = {t,t,t,t};

  VertexSOA verts;
  verts.p = vertices;
  verts.uv = uv;
  verts.c = c4;
  verts.n = n4;
  verts.t = t4;


  push_quad_vertices(buffer, verts);

  append_quads(buffer, 1, texture_id, normal_map_id);
}


static void set_vertex_buffer(RenderBuffer *buffer, int start_idx, int vertex_count) {
  auto vertices = buffer->vertices + start_idx;
  uint32_t size = sizeof(Vertex) * vertex_count;
  glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STREAM_DRAW);
}

static void gl_draw_buffer(RenderBuffer *buffer) {
  TIMED_FUNCTION();

  glViewport(-4, -4, buffer->screen_width + 4, buffer->screen_height + 4);
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

#define ORTHOGRAPHIC 1
#define PERSPECTIVE 2

#define PROJECTION 2

#if PROJECTION == ORTHOGRAPHIC
  float projection_mat[] = {
    e, 0, 0,  0,
    0, q, 0,  0,
    0, 0, -2/(far_d-near_d),  p,
    0, 0, 0,  1,
  };
#elif PROJECTION == PERSPECTIVE
  float projection_mat[] = {
    e, 0, 0,  0,
    0, q, 0,  0,
    0, 0, p, -1,
    0, 0, r,  0,
  };
#else // Infinite far plane
  float projection_mat[] = {
    e, 0, 0,  0,
    0, q, 0,  0,
    0, 0, 0.001f-1, -1,
    0, 0, (0.001f-2)*near_d,  0,
  };
#endif

  // View Matrix (flipped) :

  V3 eye = camera->p;
  V3 up = camera->up;
  V3 target = camera->target;

  // "Forward"
  V3 Z = normalize(eye - target);
  // "Left"
  V3 X = normalize(cross(up, Z)); 
  // "Up" (recalculated so that the camera can tilt up and down)
  V3 Y = normalize(cross(Z,X));

  // "Translation" (dotted because view = translation * rotation)
  V3 T = V3{-dot(X, eye), -dot(Y, eye), -dot(Z, eye)};

  float view_mat[] = {
    X.x, Y.x, Z.x, 0,
    X.y, Y.y, Z.y, 0,
    X.z, Y.z, Z.z, 0,
    T.x, T.y, T.z, 1,
  };


  glClearColor(1,1,0,1);
  gl_check_error();
  glClearDepth(1);
  gl_check_error();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  gl_check_error();

  glUseProgram(program_id);
  gl_check_error();

  glUniformMatrix4fv(projection_matrix_id, 1, GL_FALSE, projection_mat);
  gl_check_error();
  glUniformMatrix4fv(view_matrix_id, 1, GL_FALSE, view_mat);
  gl_check_error();

  auto normal_sampler_id = glGetUniformLocation(program_id, "normal_sampler");

  glUniform1i(texture_sampler_id, 0); // Texture unit 0 is for the texture
  glUniform1i(normal_sampler_id, 1); // Texture unit 1 is for normal maps.

  // DEBUG
  //print_render_buffer(buffer);

  auto front = (RenderElement *) buffer->allocator.memory;
  auto elem = front;
  for (int i = 0; i < buffer->element_count; i++) {
    assert(elem);
    gl_check_error();

    switch (elem->type) {

      case RenderType_RenderElementTextureQuads : {
        auto e = (RenderElementTextureQuads *) elem;
        set_vertex_buffer(buffer, e->vertex_index, e->quad_count * 6);

        draw_vertices(0, e->quad_count * 6, e->texture_id, e->normal_map_id);
      } break;

      default : {
        assert(false);
      };
    }

    // TODO figure out why this crashes when I don't check first?
    if (elem->next) {
      elem = elem->next;
    } else {
      break;
    }
  }
  if (buffer->element_count) assert(!elem->next);

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
  uint32_t min_blend = LINEAR_BLEND;
  uint32_t max_blend = NEAREST_BLEND;
  uint32_t s_clamp = CLAMP_TO_EDGE;
  uint32_t t_clamp = CLAMP_TO_EDGE;
  uint32_t pixel_format = BGRA;
} default_texture_parameters;

static void update_texture(GameAssets *assets, BitmapID bitmap_id, uint32_t pixel_format = BGRA) {
  auto texture = get_bitmap(assets, bitmap_id);
  if (!texture) return;
  assert(texture->texture_id);
  //assert(texture->texture_id < BITMAP_COUNT);
  gl_check_error();
  glBindTexture(GL_TEXTURE_2D, texture->texture_id);
  gl_check_error();
  // NOTE switch to GL_SRGB8_ALPHA8 if you want srgb (gamma correction)
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture->width, texture->height, 0,
               pixel_format, GL_UNSIGNED_BYTE, texture->buffer);
}

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
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, param.min_blend);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, param.max_blend);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, param.s_clamp);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, param.t_clamp);
  gl_check_error();
}


#endif // _NAR_GL_RENDERER_CPP_
