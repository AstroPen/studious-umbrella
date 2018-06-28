
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
// TODO test dynamic arrays

struct darray_head {
  uint32_t count;
  uint32_t max_count;
};

template <typename T>
struct darray {
  T *p;
  inline darray() { p = NULL; }
  inline darray(T* ptr) { p = ptr; }
  inline operator T*() { return p; }
};

template <typename T>
inline darray_head *header(darray<T> arr) {
  if (!arr.p) return NULL;
  auto head = (darray_head *) arr.p;
  return head - 1; 
}

template <typename T>
inline uint32_t count(darray<T> arr) {
  if (!arr.p) return 0;
  return header(arr)->count;
}

template <typename T>
inline uint32_t total_size(darray<T> arr, uint32_t count) {
  return sizeof(T) * count + sizeof(darray_head);
}

template <typename T>
inline bool initialize(darray<T> &arr, uint32_t max_count = 10) {
  assert(!arr.p); // TODO possibly remove this?
  if (!max_count) return true;
  auto head = (darray_head *) malloc(total_size(arr, max_count));
  if (!head) return false;
  head->count = 0;
  head->max_count = max_count;
  arr = (T *)(head + 1);
  return true;
}

template <typename T>
inline bool expand(darray<T> &arr, uint32_t amount) {
  if (!amount) return true;
  auto head = header(arr);
  if (!head) return initialize(arr, amount);

  head = (darray_head *) realloc(head, total_size(arr, head->max_count + amount));
  assert(head);
  if (!head) return false;
  head->max_count += amount;
  arr = (T *)(head + 1);
  return true;
}

template <typename T>
inline bool expand(darray<T> &arr) {
  auto head = header(arr);

  if (!head) return initialize(arr);
  return expand(arr, head->max_count);
}

template <typename T>
inline bool push(darray<T> &arr, T &entry) {
  if (!arr.p) arr = expand(arr);
  auto head = header(arr);
  if (!head) return false;

  assert(head->count <= head->max_count);
  // TODO consider expanding by 1 on failure?
  if (head->count == head->max_count)
    if (!expand(arr)) return false;

  head->count++;
  arr.p[head->count] = entry;
  return true;
}

// TODO finish this after implementing dynamic arrays
struct EventQueue {
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

