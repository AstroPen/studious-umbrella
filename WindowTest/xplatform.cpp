
// TODO This is in major need of updating. Not sure if I care about maintaining this code.

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xatom.h>

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cmath>
#include "vmath.cpp"
#include "game.cpp"

// TODO
// Draw bitmaps
// Hide the mouse immediately
// Figure out why the mouse is so laggy
// Get timing
// Get sound


#if 0
// For struct stat :
#include <sys/stat.h>
// For read :
#include <unistd.h>
// For O_RDONLY:
#include <fcntl.h>

static void load_png(const char* filename) {
  int fd = open(filename, O_RDONLY);
  if (fd < 0) {
    return;
  }

  struct stat file_stat;
  if (fstat(fd, &file_stat) < 0) return;

  uint32_t file_size = file_stat.st_size;
  uint8_t data_buffer[file_size];
  int bytes_read = 0;
  while (bytes_read < file_size) {
    bytes_read += read(fd, data_buffer + bytes_read, file_size - bytes_read);
  }

  const char *png_file_signature = "\211PNG\r\n\032\n";
  if (memcmp(png_file_signature, data_buffer, 8) != 0) return;

  uint8_t *current_chunk = data_buffer + 8;
  while (current_chunk) {
    uint32_t chunk_length = (uint32_t) *current_chunk;
    uint32_t chunk_type   = (uint32_t) *(current_chunk + 4);
    uint8_t *chunk_data   = current_chunk + 8;
  }
}
#endif

static void set_size_limits(Display *display, Window window, 
                       int min_w, int min_h, 
                       int max_w, int max_h) {
  XSizeHints hints = {};
  if(min_w > 0 && min_h > 0) hints.flags |= PMinSize;
  if(max_w > 0 && max_h > 0) hints.flags |= PMaxSize;

  hints.min_width = min_w;
  hints.min_height = min_h;
  hints.max_width = max_w;
  hints.max_height = max_h;

  XSetWMNormalHints(display, window, &hints);
}

static Status toggle_maximize(Display* display, Window window) {
  XClientMessageEvent ev = {};
  Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
  Atom max_h    = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
  Atom max_v    = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);

  if(wm_state == None) return 0;

  ev.type = ClientMessage;
  ev.format = 32;
  ev.window = window;
  ev.message_type = wm_state;
  ev.data.l[0] = 2;
  ev.data.l[1] = max_h;
  ev.data.l[2] = max_v;
  ev.data.l[3] = 1;

  return XSendEvent(display, DefaultRootWindow(display), False,
            SubstructureRedirectMask | SubstructureNotifyMask,
            (XEvent *)&ev);
}

enum KeyCodeConstant : uint8_t {
  KEYCODE_A = 8,
  KEYCODE_S = 9,
  KEYCODE_D = 10,
  KEYCODE_W = 21,
  KEYCODE_SPACE = 57,
  KEYCODE_ESC = 61,
  KEYCODE_SHIFT = 64,
  KEYCODE_LEFT = 131,
  KEYCODE_RIGHT = 132,
  KEYCODE_DOWN = 133,
  KEYCODE_UP = 134
};

static inline ButtonType keycode_to_button_type(KeyCode keycode) {
  switch (keycode) {
    case KEYCODE_A :
      return BUTTON_LEFT;
    case KEYCODE_S :
      return BUTTON_DOWN;
    case KEYCODE_D :
      return BUTTON_RIGHT;
    case KEYCODE_W :
      return BUTTON_UP;

    case KEYCODE_SPACE :
      return BUTTON_NONE;
    case KEYCODE_ESC :
      return BUTTON_QUIT;
    case KEYCODE_SHIFT :
      return BUTTON_NONE;


    case KEYCODE_LEFT :
      return BUTTON_LEFT;
    case KEYCODE_RIGHT :
      return BUTTON_RIGHT;
    case KEYCODE_DOWN :
      return BUTTON_DOWN;
    case KEYCODE_UP :
      return BUTTON_UP;
  }
  //assert(false);
  return BUTTON_NONE;
}

static inline ButtonType mouse_button_to_button_type(uint32_t button) {
  // TODO check mouse button order
  switch (button) {
    case Button1 :
      return BUTTON_MOUSE_LEFT;
    case Button2 :
      return BUTTON_MOUSE_MIDDLE;
    case Button3 :
      return BUTTON_MOUSE_RIGHT;
    case Button4 :
      return BUTTON_NONE;
    case Button5 :
      return BUTTON_NONE;
    default :
      assert(false);
      return BUTTON_NONE;
  }
  assert(false);
  return BUTTON_NONE;
}

// TODO move this to a general library file
#define count_of(array) (sizeof(array)/sizeof(*(array)))

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

#define MAX_SCREEN_WIDTH 1800
#define MAX_SCREEN_HEIGHT 800

int main(int argc, char ** argv){

  //
  // Initialization :
  //

  int width = 800;
  int height = 600;

  Display *display = XOpenDisplay(NULL);
  if (!display) {
    perror("Unable to connect to display");
    return 1;
  }

  int root_screen_num = DefaultRootWindow(display);
  int default_screen_num = DefaultScreen(display);

  int screen_bit_depth = 24;
  XVisualInfo visual_info = {};
  if (!XMatchVisualInfo(display, default_screen_num, screen_bit_depth, TrueColor, &visual_info)) {
    perror("No matching visual info");
    return 1;
  }

  XSetWindowAttributes window_attributes;
  window_attributes.bit_gravity = StaticGravity;
  window_attributes.background_pixel = 0; // Black
  window_attributes.colormap = 
    XCreateColormap(display, root_screen_num, visual_info.visual, AllocNone);
  window_attributes.event_mask = StructureNotifyMask | KeyPressMask | 
                                 KeyReleaseMask | PointerMotionMask |
                                 ButtonPressMask | ButtonReleaseMask;
  unsigned long attribute_mask = CWBitGravity | CWBackPixel | CWColormap | CWEventMask;

  Window window = XCreateWindow(display, root_screen_num, 0, 0, width, height, 0,
                                visual_info.depth, InputOutput,
                                visual_info.visual, attribute_mask, 
                                &window_attributes);

  if (!window) {
    perror("Could not create window");
    return 1;
  }
  

  XColor xcolor;
  char csr_bits[] = {0};
  Pixmap csr = XCreateBitmapFromData(display, window, csr_bits, 1, 1);
  Cursor cursor = XCreatePixmapCursor(display, csr, csr, &xcolor, &xcolor, 1, 1); 
  XDefineCursor(display, window, cursor);
  //XFreeCursor(display, cursor);


  XStoreName(display, window, "Hello, World!");

  set_size_limits(display, window, 400, 300, 0, 0);

  XMapWindow(display, window);

  //toggle_maximize(display, window);

  XFlush(display);

  int pixel_bits = 32;
  int pixel_bytes = pixel_bits / 8;
  int window_buffer_size = width * height * pixel_bytes;
  uint8_t *buffer  = (uint8_t *) malloc(window_buffer_size);
  assert(buffer);

  Pixmap pixmap = XCreatePixmap(display, root_screen_num, MAX_SCREEN_WIDTH, MAX_SCREEN_HEIGHT, visual_info.depth);
  XImage *x_window_buffer = XCreateImage(display, visual_info.visual, 
                                         visual_info.depth,
                                         ZPixmap, 0, (char *) buffer, width, height,
                                         pixel_bits, 0);
  
  GC default_GC = DefaultGC(display, default_screen_num);

  XAutoRepeatOff(display); 

  bool window_open = true;
  bool size_changed = false;
  uint8_t ticks = 0;

  GameMemory game_memory = {};
  game_memory.size = 4096;
  game_memory.memory = calloc(game_memory.size, 1);
  ControllerState old_controller_state = {};

  while (window_open) {
    XEvent ev = {};
    ControllerState controller_state = {};
    for (int i = 0; i < BUTTON_MAX; i++) {
      // TODO maybe a speed optimization here, need to check disassembly
      bool held;
      if (old_controller_state.buttons[i].last_press == PRESS_EVENT) held = true;
      else if (old_controller_state.buttons[i].last_press == RELEASE_EVENT) held = false;
      else held = old_controller_state.buttons[i].held;
      controller_state.buttons[i].held = held;
    }

    // TODO investigate using XEventsQueued() instead
    while (XPending(display) > 0) {
      XNextEvent(display, &ev);
      switch (ev.type) {

        case KeyPress : {
          auto *e = (XKeyPressedEvent *) &ev;
          //printf("keycode %d\n", e->keycode);
          ButtonType b = keycode_to_button_type(e->keycode);
          set_button_state(&controller_state, b, PRESS_EVENT);
        } break;

        case KeyRelease : {
          auto *e = (XKeyPressedEvent *) &ev;
          ButtonType b = keycode_to_button_type(e->keycode);
          set_button_state(&controller_state, b, RELEASE_EVENT);
        } break;

        case ButtonPress : {
          auto *e = (XButtonPressedEvent *) &ev;
          ButtonType b = mouse_button_to_button_type(e->button);
          set_button_state(&controller_state, b, PRESS_EVENT);
        } break;

        case ButtonRelease : {
          auto *e = (XButtonReleasedEvent *) &ev;
          ButtonType b = mouse_button_to_button_type(e->button);
          set_button_state(&controller_state, b, RELEASE_EVENT);
        } break;

        case MotionNotify : {
          XMotionEvent *e = (XMotionEvent *) &ev;
          controller_state.pointer_x = e->x;
          controller_state.pointer_y = e->y;
          controller_state.pointer_moved = true;
        } break;

        case DestroyNotify : {
          // NOTE for some reason we aren't getting this event ever
          XDestroyWindowEvent *e = (XDestroyWindowEvent *) &ev;
          if (e->window == window) {
            window_open = false;
          }
        } break;

        case ConfigureNotify : {
          XConfigureEvent *e = (XConfigureEvent *) &ev;
          width = e->width;
          height = e->height;
          size_changed = true;
        } break;
      }
    }
    
    if(size_changed) {
      size_changed = false;
      // TODO look at the actual output of XCreateImage to do this without
      // malloc
      XDestroyImage(x_window_buffer); // Free's the memory we malloced;
      window_buffer_size = width * height * pixel_bytes;
      buffer  = (uint8_t *) malloc(window_buffer_size);
      assert(buffer);
      
      x_window_buffer = XCreateImage(display, visual_info.visual, 
                                     visual_info.depth,
                                     ZPixmap, 0, (char *) buffer, width, height,
                                     pixel_bits, 0);
    }

    controller_state.ticks = ticks;
    window_open = update_and_render(game_memory, PixelBuffer{buffer, width, height}, controller_state);
    game_memory.initialized = true;

    if (!window_open) {
      XCloseDisplay(display);
      return EXIT_SUCCESS;
    }
   
    XPutImage(display, pixmap,
              default_GC, x_window_buffer, 0, 0, 0, 0,
              width, height);

    XCopyArea(display, pixmap, window, default_GC, 0, 0, width, height, 0, 0);

    old_controller_state = controller_state;
    ticks++;
  }

  return EXIT_SUCCESS;
}

