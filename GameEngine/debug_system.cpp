
// TODO Here is the planned rework for the debug system :
// 1. Each TimedBlock will push an "event" that indicates the start of a block.
// 2. The end of block will push an event indicating the end of the block.
// 3. Each event contains a thread id that allows me to tell which thread the event came from.
// 4. Later, when debug information is processed, I will create a hierarchy of calls that can 
//    destinguish between internal time and total time for the block.
// 5. I will also aggregate this information into a set of data about previous frames (max time,
//    min time, average time, etc).
// 6. This information will be displayed over the game in a gui that can be toggled.
// 7. This display should happen outside the game in the platform layer at the end of the frame.

// TODO make debug information thread safe (sort of done?)
// TODO record info over multiple frames
// TODO draw to screen instead of printing
// TODO make debug interface toggleable
//
// TODO move lighting to uniform so that I can turn it off

#include "dynamic_array.h"

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
};

struct DebugRecord {

  uint64_t total_cumulative_cycles;
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

struct DebugGlobalMemory {
  FunctionInfo *function_infos;
  DebugLog debug_logs[NUM_THREADS];
  uint32_t thread_ids[NUM_THREADS];
  uint32_t record_count;
  uint32_t current_frame;
} debug_global_memory;

static void init_debug_global_memory(uint32_t *thread_ids) {
  auto max_count = get_max_counter();
  assert(max_count < DEBUG_RECORD_MAX);
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

  // TODO Idea : create a static local variable with the macro and pass a pointer to
  // it into TimedBlock. The variable is a bool indicating whether the function has been
  // registered. If it hasn't, set the filename, function_name, and line_number.
  inline TimedBlock(uint32_t block_id_, char const *filename, 
                    uint32_t line_number, char const *function_name,
                    bool *initialized) {

    block_id = block_id_;

    if (!(*initialized)) {
      assert(debug_global_memory.record_count > block_id);
      auto info = debug_global_memory.function_infos + block_id;
      info->filename = filename;
      info->function_name = function_name;
      info->line_number = line_number;
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

#define TIMED_BLOCK(name) \
  static bool _initialized_##name = false; \
  TimedBlock _timed_block_##name(__COUNTER__, __FILE__, __LINE__, #name, &_initialized_##name)

#define END_TIMED_BLOCK(name) _timed_block_##name.force_end()

#define TIMED_FUNCTION__(number) \
  static bool _initialized_##number = false; \
  TimedBlock _timed_block_##number(__COUNTER__, __FILE__, __LINE__, __FUNCTION__, &_initialized_##number)
#define TIMED_FUNCTION_(number) TIMED_FUNCTION__(number)
#define TIMED_FUNCTION() TIMED_FUNCTION_(__LINE__)

static void aggregate_debug_events() {
  static darray <DebugEvent *> event_stack; 
  static darray <uint64_t> cycle_stack;

  printf("current_frame : %u, event counts : ", debug_global_memory.current_frame);

  for (int thread_idx = 0; thread_idx < NUM_THREADS; thread_idx++) {
    auto log = debug_global_memory.debug_logs + thread_idx;
    printf("%u, ", count(log->events));
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
        auto frame = record->frames + debug_global_memory.current_frame;
        frame->total_cycles += total_cycles;
        frame->internal_cycles += internal_cycles;
        frame->hit_count++;

        record->total_cumulative_cycles += total_cycles;
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

  printf("\n");
}

static void print_debug_records() {
  static darray<uint32_t> block_ids;

  if (!block_ids) for (uint32_t block_id = 0; block_id < debug_global_memory.record_count; block_id++) {
    push(block_ids, block_id);
  }

  //quick_sort(block_ids

  uint32_t current_frame = debug_global_memory.current_frame;
  for (uint32_t block_id = 0; block_id < debug_global_memory.record_count; block_id++) {
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

    for (uint32_t thread_idx = 0; thread_idx < NUM_THREADS; thread_idx++) {
      auto frame = frames[thread_idx];
      auto record = records[thread_idx];
      if (!frame->hit_count) continue;

      printf("\tThread %2u : %9u cycles (%9u internal), %4u hits, %8u cycles/hit\t \n",
             thread_idx,
             frame->total_cycles, 
             frame->internal_cycles,
             frame->hit_count,
             frame->total_cycles / frame->hit_count);
    }

  }
}

static void push_debug_records() {
  // FIXME I'm in the process of switching over to the new system
  aggregate_debug_events();
  print_debug_records();

  // TODO Decide exactly where this should happen
  debug_global_memory.current_frame = (debug_global_memory.current_frame + 1) % NUM_FRAMES_RECORDED;
#if 0
  assert(debug_global_memory.record_count);
  for (int i = 0; i < debug_global_memory.record_count; i++) {
    //auto record = debug_global_memory.records + i;
    auto record = debug_global_memory.debug_logs[0].records + i;
    if (record->filename) {

      printf("%-20s / %-30s (%4u) : %8u cycles, %4u hits, %8u cycles/hit\n", 
             record->filename, 
             record->function_name, 
             record->line_number,
             record->cycle_count, 
             record->hit_count,
             record->cycle_count / record->hit_count
             );
      *record = {};
    }
  }
  printf("\n");
#endif
}

