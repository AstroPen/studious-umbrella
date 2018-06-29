
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

// TODO make debug information thread safe
// TODO move debug printing to platform layer
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

struct DebugRecord {
  char const *filename;
  char const *function_name;
  uint32_t cycle_count;
  uint32_t hit_count;
  uint32_t line_number;
};

struct DebugLog {
  darray<DebugEvent> events;
  DebugRecord *records;
};

#define DEBUG_RECORD_MAX 4096
DebugRecord _debug_global_records[NUM_THREADS][DEBUG_RECORD_MAX];

struct DebugGlobalMemory {
  //DebugRecord *records = _debug_global_records[0];
  DebugLog debug_logs[NUM_THREADS];
  uint32_t thread_ids[NUM_THREADS];
  uint32_t record_count = 0; //DEBUG_RECORD_MAX;
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
  DebugRecord *record;
  uint64_t start_cycles;
  uint32_t thread_id;
  uint32_t block_id;

  inline TimedBlock(uint32_t block_id_, char const *filename, 
                    uint32_t line_number, char const *function_name) {

    block_id = block_id_;
    thread_id = SDL_ThreadID();
    uint32_t thread_idx = get_thread_index(thread_id);
    log = debug_global_memory.debug_logs + thread_idx;
    record = log->records + block_id;
    auto event = push(log->events);
    event->type = BLOCK_START;
    event->block_id = block_id;
    event->thread_id = thread_id;

    if (!record->filename) {
      assert(debug_global_memory.record_count > block_id);
      record->filename = filename;
      record->function_name = function_name;
      record->line_number = line_number;
      //assert(!record->hit_count);
      //assert(!record->cycle_count);
    }
    start_cycles = SDL_GetPerformanceCounter();
    event->cycle_count = start_cycles;
  }

  inline void force_end() {
    assert(record);
    uint64_t end_cycles = SDL_GetPerformanceCounter();
    uint64_t cycle_diff = end_cycles - start_cycles;
    auto event = push(log->events);
    event->type = BLOCK_END;
    event->block_id = block_id;
    event->thread_id = thread_id;
    event->cycle_count = end_cycles;
    record->hit_count++;
    record->cycle_count += (uint32_t) cycle_diff;
    record = NULL;
  }

  inline ~TimedBlock() {
    if (record) force_end();
  }
};

// NOTE : all these declarations are necessary to mangle the variable name and
// allow for multiple uses per function.

#define TIMED_BLOCK(name) TimedBlock _timed_block_##name(__COUNTER__, __FILE__, __LINE__, #name)

#define END_TIMED_BLOCK(name) _timed_block_##name.force_end()

#define TIMED_FUNCTION__(number) TimedBlock _timed_block_##number(__COUNTER__, __FILE__, __LINE__, __FUNCTION__)
#define TIMED_FUNCTION_(number) TIMED_FUNCTION__(number)
#define TIMED_FUNCTION() TIMED_FUNCTION_(__LINE__)

static void aggregate_debug_events() {
  darray <DebugEvent *> event_stack; // TODO make this permanent to avoid repeated allocation?

  for (int thread_idx = 0; thread_idx < NUM_THREADS; thread_idx++) {
    auto log = debug_global_memory.debug_logs + thread_idx;
    for (int i = 0; i < count(log->events); i++) {
      auto event = log->events + i;
      if (event->type == BLOCK_START) {
        push(event_stack, event);
      } else {
        assert(event->type == BLOCK_END);
        auto start_event = pop(event_stack);
        uint64_t diff = event->cycle_count - start_event->cycle_count;
        // TODO update record
      }
    }
  }
}

static void push_debug_records() {
  // FIXME This only prints records for the main thread
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
}

