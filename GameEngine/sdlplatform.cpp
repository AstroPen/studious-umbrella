
// TODO This is in major need of updating. Not sure if care about maintaining this code at all.
// Most of this is redundant with gl_sdlplatform.cpp

#include <SDL.h>
#include <SDL_opengl.h>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cmath>

#include "vmath.cpp"
#include "game.h"

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"

// TODO remove this, it is unused
static SDL_PixelFormat *GLOBAL_SDL_FORMAT;

// TODO start using stb for text


// Basic usage (see HDR discussion below for HDR usage):
//    int x,y,n;
//    unsigned char *data = stbi_load(filename, &x, &y, &n, 0);
//    // ... process data if not NULL ...
//    // ... x = width, y = height, n = # 8-bit components per pixel ...
//    // ... replace '0' with '1'..'4' to force that many components per pixel
//    // ... but 'n' will always be the number that it would have been if you said 0
//    stbi_image_free(data)
//
// Standard parameters:
//    int *x                 -- outputs image width in pixels
//    int *y                 -- outputs image height in pixels
//    int *channels_in_file  -- outputs # of image components in image file
//    int desired_channels   -- if non-zero, # of image components requested in result
//
// The return value from an image loader is an 'unsigned char *' which points
// to the pixel data, or NULL on an allocation failure or if the image is
// corrupt or invalid. The pixel data consists of *y scanlines of *x pixels,
// with each pixel consisting of N interleaved 8-bit components; the first
// pixel pointed to is top-left-most in the image. There is no padding between
// image scanlines or between pixels, regardless of format. The number of
// components N is 'desired_channels' if desired_channels is non-zero, or
// *channels_in_file otherwise. If desired_channels is non-zero,
// *channels_in_file has the number of components that _would_ have been
// output otherwise. E.g. if you set desired_channels to 4, you will always
// get RGBA output, but you can check *channels_in_file to see if it's trivially
// opaque because e.g. there were only 3 channels in the source image.
//
// An output image with N components has the following components interleaved
// in this order in each pixel:
//
//     N=#comp     components
//       1           grey
//       2           grey, alpha
//       3           red, green, blue
//       4           red, green, blue, alpha
//
// If image loading fails for any reason, the return value will be NULL,
// and *x, *y, *channels_in_file will be unchanged. The function
// stbi_failure_reason() can be queried for an extremely brief, end-user
// unfriendly explanation of why the load failed. Define STBI_NO_FAILURE_STRINGS
// to avoid compiling these strings at all, and STBI_FAILURE_USERMSG to get slightly
// more user-friendly ones.

static PixelBuffer load_texture(const char* filename) {
  assert(filename);
  int width;
  int height;
  int pixel_bytes = 4;
  int original_pixel_bytes;
  uint8_t *data = stbi_load(filename, &width, &height, &original_pixel_bytes, pixel_bytes);

  assert(data);
  PixelBuffer buffer;
  buffer.width = width;
  buffer.height = height;
  buffer.buffer = data;

  return buffer;
}

#if 0
// TODO handle allocation and stuff
static PixelBuffer load_texture(const char* filename) {
  assert(filename);
  SDL_Surface *surface = SDL_LoadBMP(filename);
  if (!surface) printf("Error loading texture %s.\n", filename);

  auto converted_surface = SDL_ConvertSurface(surface, GLOBAL_SDL_FORMAT, 0);
  SDL_FreeSurface(surface);

  PixelBuffer buffer;
  buffer.width = converted_surface->w;
  buffer.height = converted_surface->h;
  buffer.buffer = (uint8_t *) converted_surface->pixels;

  converted_surface->pixels = NULL;
  SDL_FreeSurface(converted_surface);

  return buffer;
}
#endif


#include "game.cpp"

// TODO fix resizing

// TODO actually use these
#define MAX_SCREEN_WIDTH 1800
#define MAX_SCREEN_HEIGHT 800

// TODO move this to a general library file
#define count_of(array) (sizeof(array)/sizeof(*(array)))


static inline ButtonType keycode_to_button_type(SDL_Keycode keycode) {
  switch (keycode) {
    case SDLK_a :
      return BUTTON_LEFT;
    case SDLK_s :
      return BUTTON_DOWN;
    case SDLK_d :
      return BUTTON_RIGHT;
    case SDLK_w :
      return BUTTON_UP;

    case SDLK_SPACE :
      return BUTTON_NONE;
    case SDLK_ESCAPE :
      return BUTTON_QUIT;
    case SDLK_LSHIFT :
      return BUTTON_NONE;
    case SDLK_RSHIFT :
      return BUTTON_NONE;


    case SDLK_LEFT :
      return BUTTON_LEFT;
    case SDLK_RIGHT :
      return BUTTON_RIGHT;
    case SDLK_DOWN :
      return BUTTON_DOWN;
    case SDLK_UP :
      return BUTTON_UP;
  }
  //assert(false);
  return BUTTON_NONE;
}

static inline void set_button_state(ControllerState *controller, ButtonType idx, ButtonEventType event_type) {
  if (idx == BUTTON_NONE) return;
  assert(BUTTON_MAX == count_of(controller->buttons));

  if (idx > BUTTON_MAX) return;
  if (controller->buttons[idx].first_press == NO_EVENTS) {
    controller->buttons[idx].first_press = event_type;
  }
  controller->buttons[idx].last_press = event_type;
  controller->buttons[idx].num_transitions++;
}

static inline ButtonType mouse_button_to_button_type(uint8_t button) {
  switch (button) {
    case SDL_BUTTON_LEFT :
      return BUTTON_MOUSE_LEFT;
    case SDL_BUTTON_RIGHT :
      return BUTTON_MOUSE_RIGHT;
    case SDL_BUTTON_MIDDLE :
      return BUTTON_MOUSE_MIDDLE;
    default :
      return BUTTON_NONE;
  }
  assert(false);
  return BUTTON_NONE;
}

// TODO remove gl code
#define USING_GL 0

struct RenderInfo {
  SDL_Window *window;
  uint8_t *buffer;
#if USING_GL
  SDL_GLContext glcontext;
#else
  SDL_Renderer *renderer;
  SDL_Texture *screen;
#endif
  int width;
  int height;
};

#if USING_GL
static void init_opengl(RenderInfo *info) {
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  info->glcontext = SDL_GL_CreateContext(info->window);

  //glewExperimental = GL_TRUE;
  //glewInit();
}
#endif

static RenderInfo init_screen(int width, int height) {

  RenderInfo result;
  result.width = width;
  result.height = height;

  SDL_Init(SDL_INIT_VIDEO);

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

#if USING_GL
  init_opengl(&result);
#else

  // TODO
  //SDL_SetWindowResizable(window, SDL_TRUE);

  result.renderer = SDL_CreateRenderer(result.window, 0, SDL_RENDERER_PRESENTVSYNC);
  assert(result.renderer);

  // TODO
  SDL_PixelFormat *format = SDL_AllocFormat(SDL_PIXELFORMAT_RGB888);
  assert(format);
  GLOBAL_SDL_FORMAT = format;

  result.screen = SDL_CreateTexture(result.renderer, format->format,
                                    SDL_TEXTUREACCESS_STREAMING,
                                    width, height);
  assert(result.screen);

#endif

  int pixel_bytes = 4;
  int window_buffer_size = width * height;
  result.buffer = (uint8_t *) calloc(window_buffer_size, pixel_bytes); 
  assert(result.buffer);

  return result;
}

static void display_buffer(RenderInfo render_info) {
  int pixel_bytes = 4;
  int pitch = render_info.width * pixel_bytes;

#if USING_GL

  glViewport(0, 0, render_info.width, render_info.height);
  glClearColor(1,1,0,1);
  glClear(GL_COLOR_BUFFER_BIT);
  SDL_GL_SwapWindow(render_info.window);

#else

  SDL_UpdateTexture(render_info.screen, NULL, render_info.buffer, pitch);
  SDL_RenderClear(render_info.renderer);
  SDL_RenderCopy(render_info.renderer, render_info.screen, NULL, NULL);
  SDL_RenderPresent(render_info.renderer);

#endif
}

int main(int argc, char ** argv){

  //
  // Initialization :
  //

  int width = 800;
  int height = 600;

  auto render_info = init_screen(width, height);
  if (!render_info.window) return 1;

  bool window_open = true;
  bool size_changed = false;
  uint8_t ticks = 0;

  GameMemory game_memory = {};
  game_memory.permanent_size = 4096 * 32;
  game_memory.permanent_store = calloc(game_memory.permanent_size, 1);
  game_memory.temporary_size = 4096;
  game_memory.temporary_store = calloc(game_memory.temporary_size, 1);
  ControllerState old_controller_state = {};
  uint32_t previous_time = SDL_GetTicks();

  while (window_open) {

    ControllerState controller_state = {};
    for (int i = 0; i < BUTTON_MAX; i++) {
      // TODO maybe a speed optimization here, need to check disassembly
      bool held;
      if (old_controller_state.buttons[i].last_press == PRESS_EVENT) held = true;
      else if (old_controller_state.buttons[i].last_press == RELEASE_EVENT) held = false;
      else held = old_controller_state.buttons[i].held;
      controller_state.buttons[i].held = held;
    }

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {

      switch (ev.type) {
        case SDL_KEYDOWN : {
          SDL_Keycode code = ev.key.keysym.sym;
          ButtonType b = keycode_to_button_type(code);
          set_button_state(&controller_state, b, PRESS_EVENT);
        } break;

        case SDL_KEYUP : {
          SDL_Keycode code = ev.key.keysym.sym;
          ButtonType b = keycode_to_button_type(code);
          set_button_state(&controller_state, b, RELEASE_EVENT);
        } break;

        case SDL_MOUSEBUTTONDOWN : {
          ButtonType b = mouse_button_to_button_type(ev.button.button);
          set_button_state(&controller_state, b, PRESS_EVENT);
        } break;

        case SDL_MOUSEBUTTONUP : {
          ButtonType b = mouse_button_to_button_type(ev.button.button);
          set_button_state(&controller_state, b, RELEASE_EVENT);
        } break;

        case SDL_MOUSEMOTION : {
          // TODO take the motion of the mouse as well
          controller_state.pointer_x = ev.motion.x;
          controller_state.pointer_y = ev.motion.y;
          controller_state.pointer_moved = true;
        } break;

        case SDL_QUIT : {
          window_open = false;
        } break;

        case SDL_WINDOWEVENT : {
          switch (ev.window.event) {

            case SDL_WINDOWEVENT_RESIZED : {
              width = ev.window.data1;
              height = ev.window.data2;
              size_changed = true;
            } break;

            case SDL_WINDOWEVENT_CLOSE : {
              window_open = false;
            } break;
          }
        } break;
      }
    }

    if (!window_open) break;
    
    if(size_changed) {
      size_changed = false;
      // TODO resize image
      /*
      window_buffer_size = width * height * pixel_bytes;
      buffer  = (uint8_t *) malloc(window_buffer_size);
      assert(buffer);
      
      x_window_buffer = XCreateImage(display, visual_info.visual, 
                                     visual_info.depth,
                                     ZPixmap, 0, (char *) buffer, width, height,
                                     pixel_bits, 0);
                                     */
    }

    controller_state.delta_t = (double) SDL_GetTicks() - (double) previous_time;
    controller_state.delta_t /= 1000.0f;
    controller_state.ticks = ticks;
    uint32_t current_time = SDL_GetTicks();
    //uint32_t time_passed = current_time - previous_time;
    previous_time = current_time;

    window_open = update_and_render(game_memory, 
                                    PixelBuffer{render_info.buffer, width, height}, 
                                    controller_state);
    game_memory.initialized = true;   

    display_buffer(render_info);   

    old_controller_state = controller_state;
    ticks++;
  }

  SDL_Quit();

  return EXIT_SUCCESS;
}

