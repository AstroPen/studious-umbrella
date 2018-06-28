
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

#define NUM_THREADS 4
#define EVENT_QUEUE_SIZE (4098 * 16)

struct DebugEvent {
  DebugEventType type;
  uint16_t block_id;
  uint32_t thread_id;
  uint32_t cycle_count;
};

// TODO move all the dyanamic array stuff to a new file

// TODO finish this after implementing dynamic arrays
struct EventQueue {
  darray<DebugEvent> events;
};

static inline uint32_t get_max_counter(); 

struct DebugRecord {
  char const *filename;
  char const *function_name;
  uint32_t cycle_count;
  uint32_t hit_count;
  uint32_t line_number;
};

#define DEBUG_RECORD_MAX 4096
DebugRecord _debug_global_records[DEBUG_RECORD_MAX];

struct DebugGlobalMemory {
  DebugRecord *records = _debug_global_records;
  uint32_t record_count = 0; //DEBUG_RECORD_MAX;
} debug_global_memory;

static void init_debug_global_memory() {
  // TODO multiply by number of threads
  auto max_count = get_max_counter();
  assert(max_count < DEBUG_RECORD_MAX);
  debug_global_memory.record_count = max_count;
}

struct TimedBlock {
  DebugRecord *record;
  uint64_t start_cycles;

  inline TimedBlock(uint32_t block_id, char const *filename, 
                    uint32_t line_number, char const *function_name) {

    record = debug_global_memory.records + block_id;
    if (!record->filename) {
      assert(debug_global_memory.record_count > block_id);
      record->filename = filename;
      record->function_name = function_name;
      record->line_number = line_number;
      //assert(!record->hit_count);
      //assert(!record->cycle_count);
    }
    start_cycles = SDL_GetPerformanceCounter();
  }

  inline void force_end() {
    assert(record);
    uint64_t cycle_diff = SDL_GetPerformanceCounter() - start_cycles;
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

static void push_debug_records() {
  assert(debug_global_memory.record_count);
  for (int i = 0; i < debug_global_memory.record_count; i++) {
    auto record = debug_global_memory.records + i;
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

