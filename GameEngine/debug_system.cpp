
// 
// Currently, the debug system is laid out like this :
//
// 1. Each TimedBlock pushes an "event" that indicates the start of a block.
// 2. The end of block pushes an event indicating the end of the block.
// 3. Each event contains a thread id that allows you to tell which thread the event came from.
// 4. Later, when debug information is processed, aggregate_debug_events creates a hierarchy of 
//    calls that can destinguish between internal time and total time for the block.
// 5. It also aggregates this information into a set of data about previous frames (max time,
//    min time, average time, etc).
// 6. This information is displayed over the game in a gui that can be toggled with the 1 key.
// 7. push_debug_records is called outside the game in the platform layer at the end of the frame.

// TODO move lighting to uniform so that I can turn it off

#include "dynamic_array.h"
#include "quick_sort.h"

enum DebugEventType : uint8_t {
  BLOCK_START,
  BLOCK_END,
};

#define EVENT_QUEUE_SIZE (4098 * 16)

struct DebugEvent {
  DebugEventType type;
  uint16_t block_id;
  uint32_t thread_id;
  uint64_t cycle_count;
};


static inline uint32_t get_max_counter(); 

#define NUM_FRAMES_RECORDED 60

struct FrameRecord {
  uint32_t total_cycles;
  uint32_t internal_cycles;
  uint32_t hit_count;
};

struct FunctionInfo {
  char const *filename;
  char const *function_name;
  uint32_t line_number;
  uint32_t priority;
};

struct DebugRecord {

  float64 average_internal_cycles;
  uint64_t total_cumulative_hits;

  uint32_t max_cycle_count;
  uint32_t max_internal_cycle_count;
  uint32_t max_hit_count;

  FrameRecord frames[NUM_FRAMES_RECORDED];
};

struct DebugLog {
  darray<DebugEvent> events;
  DebugRecord *records;
};

#define DEBUG_RECORD_MAX 4096
DebugRecord _debug_global_records[NUM_THREADS][DEBUG_RECORD_MAX];
FunctionInfo _debug_global_function_infos[DEBUG_RECORD_MAX];

struct RenderBuffer;

struct DebugState {
  GameState *game_state;
  RenderBuffer *render_buffer;
  FunctionInfo *function_infos;
  uint64_t current_frame;
  DebugLog debug_logs[NUM_THREADS];
  uint32_t thread_ids[NUM_THREADS];
  uint32_t record_count;
  bool display_records;
  bool camera_mode;
} debug_global_memory;

static void init_debug_global_memory(uint32_t *thread_ids, GameMemory *game_memory, RenderBuffer *render_buffer) {
  auto max_count = get_max_counter();
  assert(max_count < DEBUG_RECORD_MAX);
  debug_global_memory.game_state = (GameState *) game_memory->permanent_store;
  debug_global_memory.render_buffer = render_buffer;
  debug_global_memory.record_count = max_count;
  for (int i = 0; i < NUM_THREADS; i++) {
    debug_global_memory.thread_ids[i] = thread_ids[i];
    debug_global_memory.debug_logs[i].records = _debug_global_records[i];
    initialize(debug_global_memory.debug_logs[i].events, 300);
  }
  debug_global_memory.function_infos = _debug_global_function_infos;
}

inline uint32_t get_thread_index(uint32_t thread_id) {
  for (uint32_t i = 0; i < NUM_THREADS; i++) {
    if (thread_id == debug_global_memory.thread_ids[i]) return i;
  }
  assert(!"Invalid thread id.");
  return 0;
}

struct TimedBlock {
  DebugLog *log;
  uint32_t thread_id;
  uint32_t block_id;

  inline TimedBlock(uint32_t block_id_, char const *filename, 
                    uint32_t line_number, char const *function_name,
                    bool *initialized, uint32_t priority) {

    block_id = block_id_;

    // NOTE : create a static local bool with the macro and pass a pointer to
    // it into TimedBlock. The bool indicates whether the function has been
    // registered. If it hasn't, set the filename, function_name, line_number, and
    // anything else in the FunctionInfo.
    if (!(*initialized)) {
      assert(debug_global_memory.record_count > block_id);
      auto info = debug_global_memory.function_infos + block_id;
      info->filename = filename;
      info->function_name = function_name;
      info->line_number = line_number;
      info->priority = priority;
      *initialized = true;
    }

    thread_id = SDL_ThreadID();
    uint32_t thread_idx = get_thread_index(thread_id);
    log = debug_global_memory.debug_logs + thread_idx;

    auto event = push(log->events);
    event->type = BLOCK_START;
    event->block_id = block_id;
    event->thread_id = thread_id;

    event->cycle_count = SDL_GetPerformanceCounter();
  }

  inline void force_end() {
    assert(log);
    uint64_t end_cycles = SDL_GetPerformanceCounter();
    auto event = push(log->events);
    event->type = BLOCK_END;
    event->block_id = block_id;
    event->thread_id = thread_id;
    event->cycle_count = end_cycles;
    log = NULL;
  }

  inline ~TimedBlock() {
    if (log) force_end();
  }
};

// NOTE : all these declarations are necessary to mangle the variable name and
// allow for multiple uses per function.

#define TIMED_BLOCK__(mangle, name, priority) \
  static bool _initialized_##mangle = false; \
  TimedBlock _timed_block_##mangle(__COUNTER__, __FILE__, __LINE__, name, &_initialized_##mangle, priority)

#define TIMED_BLOCK(name) TIMED_BLOCK__(name, #name, 0)

#define END_TIMED_BLOCK(name) _timed_block_##name.force_end()

#define TIMED_FUNCTION_(number, priority) TIMED_BLOCK__(number, __FUNCTION__, priority)
#define TIMED_FUNCTION() TIMED_FUNCTION_(__LINE__, 0)

#define PRIORITY_TIMED_FUNCTION(priority) TIMED_FUNCTION_(__LINE__, priority)


static void aggregate_debug_events() {
  static darray <DebugEvent *> event_stack; 
  static darray <uint64_t> cycle_stack;

  uint64_t total_frames = debug_global_memory.current_frame;

  for (int thread_idx = 0; thread_idx < NUM_THREADS; thread_idx++) {
    auto log = debug_global_memory.debug_logs + thread_idx;
    uint32_t thread_id = debug_global_memory.thread_ids[thread_idx];
    uint64_t total_sibling_cycles = 0;
    uint64_t total_child_cycles = 0;

    for (int i = 0; i < count(log->events); i++) {
      auto event = log->events + i;
      if (event->type == BLOCK_START) {
        assert(event->thread_id == thread_id);
        push(event_stack, event);
        push(cycle_stack, total_sibling_cycles);
        total_sibling_cycles = total_child_cycles;
        total_child_cycles = 0;
      } else {
        assert(event->type == BLOCK_END);
        auto start_event = pop(event_stack);
        uint64_t total_cycles = event->cycle_count - start_event->cycle_count;
        total_sibling_cycles += total_cycles;
        uint64_t internal_cycles = total_cycles - total_child_cycles;

        assert(event->block_id == start_event->block_id);
        assert(event->thread_id == thread_id);

        //
        // Update record ---
        //

        auto record = log->records + event->block_id;
        auto frame = record->frames + (total_frames % NUM_FRAMES_RECORDED);
        frame->total_cycles += total_cycles;
        frame->internal_cycles += internal_cycles;
        frame->hit_count++;

        // Per frame
        record->average_internal_cycles = ((record->average_internal_cycles * total_frames) + internal_cycles) / (total_frames + 1);
        record->total_cumulative_hits++;

        if (record->max_cycle_count < frame->total_cycles) record->max_cycle_count = frame->total_cycles;
        if (record->max_internal_cycle_count < frame->internal_cycles) record->max_internal_cycle_count = frame->internal_cycles;
        if (record->max_hit_count < frame->hit_count) record->max_hit_count = frame->hit_count;

        total_child_cycles = total_sibling_cycles;
        total_sibling_cycles = pop(cycle_stack);
      }
    }
    assert(count(event_stack) == 0);
    assert(count(cycle_stack) == 0);
    clear(log->events);
  }

}

#define BLOCK_ID_COMPARE_AVERAGE 1

inline int block_id_compare(uint32_t *a, uint32_t *b) {
  uint32_t bid_a = *a;
  uint32_t bid_b = *b;
  FunctionInfo *info_a = debug_global_memory.function_infos + bid_a;
  FunctionInfo *info_b = debug_global_memory.function_infos + bid_b;

  if (!info_a->filename && !info_b->filename) return 0;
  else if (!info_a->filename) return 1;
  else if (!info_b->filename) return -1;

  // NOTE : Both are initialized.
  
  int priority_cmp = -compare(&info_a->priority, &info_b->priority);
  if (priority_cmp) return priority_cmp;
 
#if BLOCK_ID_COMPARE_AVERAGE
  float64 internal_cycles_a = 0;
  float64 internal_cycles_b = 0;

  for (uint32_t thread_idx = 0; thread_idx < NUM_THREADS; thread_idx++) {
    auto record_a = debug_global_memory.debug_logs[thread_idx].records + bid_a;
    auto record_b = debug_global_memory.debug_logs[thread_idx].records + bid_b;

    internal_cycles_a += record_a->average_internal_cycles;
    internal_cycles_b += record_b->average_internal_cycles;
  }

  return compare(&internal_cycles_b, &internal_cycles_a);
#else
  uint32_t current_frame = (debug_global_memory.current_frame % NUM_FRAMES_RECORDED);

  uint64_t internal_cycles_a = 0;
  uint64_t internal_cycles_b = 0;

  for (uint32_t thread_idx = 0; thread_idx < NUM_THREADS; thread_idx++) {
    auto record_a = debug_global_memory.debug_logs[thread_idx].records + bid_a;
    auto record_b = debug_global_memory.debug_logs[thread_idx].records + bid_b;

    auto frame_a = &record_a->frames[current_frame];
    auto frame_b = &record_b->frames[current_frame];

    internal_cycles_a += frame_a->internal_cycles;
    internal_cycles_b += frame_b->internal_cycles;
  }

  return compare(&internal_cycles_b, &internal_cycles_a);
#endif
}

static void print_frame_records() {
  static darray<uint32_t> block_ids;

  if (!block_ids) for (uint32_t block_id = 0; block_id < debug_global_memory.record_count; block_id++) {
    push(block_ids, block_id);
  }

  quick_sort<uint32_t, block_id_compare>(block_ids, count(block_ids));

  uint32_t lines = 0;
  uint32_t const max_lines = 40;

  uint32_t current_frame = debug_global_memory.current_frame % NUM_FRAMES_RECORDED;
  printf("Total Frames : %llu\n", debug_global_memory.current_frame);

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

    printf("%-30s in %-30s (line %4u) : \n",  
           info->function_name, 
           info->filename, 
           info->line_number);
    lines++;

    for (uint32_t thread_idx = 0; thread_idx < NUM_THREADS && lines < max_lines; thread_idx++) {
      auto frame = frames[thread_idx];
      auto record = records[thread_idx];
      if (!frame->hit_count) continue;

      printf("\tThread %2u : %9u cycles (%9u internal), %4u hits, %8u cycles/hit\t\t Average : %lf\n",
             thread_idx,
             frame->total_cycles, 
             frame->internal_cycles,
             frame->hit_count,
             frame->total_cycles / frame->hit_count,
             record->average_internal_cycles);
      lines++;
    }
    lines++;
    printf("\n");
  }
  printf("\n");
}

static void clear_frame_records() {
  uint32_t current_frame = debug_global_memory.current_frame % NUM_FRAMES_RECORDED;

  for (uint32_t block_id = 0; block_id < debug_global_memory.record_count; block_id++) {
    FunctionInfo *info = debug_global_memory.function_infos + block_id;
    if (!info->filename) continue;

    for (uint32_t thread_idx = 0; thread_idx < NUM_THREADS; thread_idx++) {
      auto record = debug_global_memory.debug_logs[thread_idx].records + block_id;
      auto frame = record->frames + current_frame;
      *frame = {};
    }
  }
}

static void render_frame_records(RenderBuffer *buffer);

static void push_debug_records() {
  RenderBuffer *buffer = debug_global_memory.render_buffer;
  aggregate_debug_events();
  //print_frame_records();
  if (debug_global_memory.display_records) render_frame_records(buffer);

  debug_global_memory.current_frame++;
  clear_frame_records();
}

#define QUICK_SORT_IMPLEMENTATION
#include "quick_sort.h"

