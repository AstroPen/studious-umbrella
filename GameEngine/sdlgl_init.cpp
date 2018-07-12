
struct RenderInfo {
  RenderBuffer render_buffer;
  SDL_Window *window;
  SDL_GLContext glcontext;
};

#if 0
void MessageCallback( GLenum source,
                      GLenum type,
                      GLuint id,
                      GLenum severity,
                      GLsizei length,
                      const GLchar *message,
                      const void *userParam ) {

  fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
          (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
          type, severity, message);
}
#endif

static void init_opengl(RenderInfo *info) {

  info->glcontext = SDL_GL_CreateContext(info->window);
  if (!info->glcontext) {
    printf("ERROR : %s\n", SDL_GetError());
  }
  assert(info->glcontext);

  // TODO Consider passing in -1 to get late swap tearing.
  int error = SDL_GL_SetSwapInterval(1);
  if (error) printf("System does not support vsync.\n");

  //glGenTextures(1, &blit_texture_handle);
 
  GLenum err;
  while ((err = glGetError()) != GL_NO_ERROR) {
    printf("OpenGL error: 0x%x at start-up.\n", err);
  }


  glDepthMask(GL_TRUE);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glEnable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  // NOTE turn this on for gamma correction :
  //glEnable(GL_FRAMEBUFFER_SRGB);

  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  glDepthFunc(GL_LEQUAL);

  while ((err = glGetError()) != GL_NO_ERROR) {
    printf("OpenGL error: 0x%x during initialization.\n", err);
  }
  
  //glEnable(GL_DEBUG_OUTPUT);
  //glDebugMessageCallback((GLDEBUGPROC) MessageCallback, 0);

  char *header = R"(
  #version 150

  )"; 

  char *vertex_shader = 
#include "blinn_phong.vert"

  char *fragment_shader = 
#include "blinn_phong.frag"

  // NOTE : We have to bind a vertex array to avoid an error in the gl3
  // pipeline, but we never need to change it, so we don't save the 
  // vertex array id.
  GLuint vertex_array_id;
  glGenVertexArrays(1, &vertex_array_id);
  glBindVertexArray(vertex_array_id);

  // NOTE : these ids are just the handles, the binding to actual values happens later
  program_id = create_shader_program(header, vertex_shader, fragment_shader);

  //transform_id = glGetUniformLocation(program_id, "transform");
  projection_matrix_id = glGetUniformLocation(program_id, "projection_matrix");
  view_matrix_id = glGetUniformLocation(program_id, "view_matrix");
  gl_check_error();

  texture_sampler_id = glGetUniformLocation(program_id, "texture_sampler");
  gl_check_error();

  auto buf = &info->render_buffer;
  assert(buf->vertices);
  glGenBuffers(1, &vertex_buffer_id);
  gl_check_error();
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_id);
  gl_check_error();
}

static void display_buffer(RenderInfo render_info) {
  TIMED_FUNCTION();

  //int pixel_bytes = 4;
  //int pitch = render_info.width * pixel_bytes;

  //glViewport(0, 0, render_info.width, render_info.height);

  gl_draw_buffer(&render_info.render_buffer);

  static int frame_num = 0;
  GLenum err;
  while ((err = glGetError()) != GL_NO_ERROR) {
    printf("OpenGL error: 0x%x on frame %d\n", err, frame_num);
  }
  frame_num++;


  SDL_GL_SwapWindow(render_info.window);
}

static RenderInfo init_screen(int width, int height) {

  RenderInfo result = {};
  auto buffer = &result.render_buffer;
  buffer->screen_width = width;
  buffer->screen_height = height;

  // TODO maybe pass in the memory from outside?
  buffer->max_vertices = 4096 * 8;
  buffer->vertex_count = 0;
  int vertex_buffer_size = buffer->max_vertices * sizeof(Vertex);
  int total_buffer_size = vertex_buffer_size + 4096 * 64;

  printf("Total buffer size : %d\n", total_buffer_size);
  auto temp = new_push_allocator(total_buffer_size);
  buffer->vertices = alloc_array(&temp, Vertex, buffer->max_vertices);
  assert(buffer->vertices);
  buffer->allocator = new_push_allocator(&temp, remaining_size(&temp));
  assert(is_initialized(&buffer->allocator));

  clear(buffer);



  SDL_Init(SDL_INIT_VIDEO);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  uint32_t window_flags = SDL_WINDOW_OPENGL;
  result.window = SDL_CreateWindow("Box World",
                                    SDL_WINDOWPOS_UNDEFINED,
                                    SDL_WINDOWPOS_UNDEFINED,
                                    width, height, window_flags);

  if (!result.window) {
    perror("Could not create window");
    return {};
  }

  SDL_ShowCursor(SDL_DISABLE);
  // TODO
  //SDL_SetWindowResizable(window, SDL_TRUE);

  init_opengl(&result);

  /*
  int pixel_bytes = 4;
  int window_buffer_size = width * height;
  result.buffer = (uint8_t *) calloc(window_buffer_size, pixel_bytes); 
  assert(result.buffer);
  */

  GLenum err;
  while ((err = glGetError()) != GL_NO_ERROR) {
    printf("OpenGL error: 0x%x after initialization.\n", err);
  }

  return result;
}

