#ifndef _NAR_SDL_THREAD_CPP_
#define _NAR_SDL_THREAD_CPP_

// NOTE : global_debug_memory is unitialized before init_worker_threads is called

// TODO consider breaking this into atomics/queue separately
#define _NAR_ATOMICS_H_

//
// atomic_int ---
//

struct atomic_int {
  SDL_atomic_t i;

  inline int operator=(int val) {
    return SDL_AtomicSet(&i, val);
  }

  inline operator int() {
    return SDL_AtomicGet(&i);
  }

  inline int operator+=(int b) {
    return SDL_AtomicAdd(&i, b);
  }

  inline int operator++() {
    return SDL_AtomicAdd(&i, 1);
  }

  inline int operator++(int) {
    return SDL_AtomicAdd(&i, 1) + 1;
  }

  inline int operator-=(int b) {
    return *this += -b;
  }

  inline int operator--() {
    return SDL_AtomicAdd(&i, -1);
  }

  inline u32 operator--(int) {
    return SDL_AtomicAdd(&i, -1) - 1;
  }

};

static inline
bool atomic_compare_exchange(atomic_int *a, int expected_val, int new_val) {
  return SDL_AtomicCAS(&a->i, expected_val, new_val);
}  


//
// atomic_uint ---
//

struct atomic_uint {
  SDL_atomic_t i;

  inline u32 operator=(u32 val) {
    return bit32(SDL_AtomicSet(&i, bit32(val)));
  }

  inline operator u32() {
    return bit32(SDL_AtomicGet(&i));
  }

  inline u32 operator+=(u32 b) {
    return bit32(SDL_AtomicAdd(&i, bit32(b)));
  }

  inline u32 operator++() {
    return bit32(SDL_AtomicAdd(&i, 1));
  }

  inline u32 operator++(int) {
    return bit32(SDL_AtomicAdd(&i, 1)).u + 1;
  }

  inline u32 operator-=(u32 b) {
    return *this += -b;
  }

  inline u32 operator--() {
    return bit32(SDL_AtomicAdd(&i, -1));
  }

  inline u32 operator--(int) {
    return bit32(SDL_AtomicAdd(&i, -1)).u - 1;
  }

};

static inline
bool atomic_compare_exchange(atomic_uint *a, u32 expected_val, u32 new_val) {
  return SDL_AtomicCAS(&a->i, bit32(expected_val), bit32(new_val));
}


// TODO atomic pointers/64 bit numbers?

static inline
bool atomic_compare_exchange(void **a, void *expected_val, void *new_val) {
  return SDL_AtomicCASPtr(a, expected_val, new_val);
}


//
// Work Queue Definitions ---
//

typedef void work_queue_callback(void *data);

struct WorkQueueEntry {
  void *data;
  work_queue_callback *callback;
};

struct WorkQueue {
  SDL_sem *semaphore;
  atomic_int next_entry;
  atomic_int next_free;
  atomic_int completed_entry_count;
  atomic_int completion_goal;
  WorkQueueEntry entries[256]; 
};

struct ThreadInfo {
  WorkQueue *queue;
  // TODO make this just a u32?
  SDL_threadID thread_id;
  int volatile received;
};


//
// Helper Calls ---
//

static inline int get_next_entry(WorkQueue *queue) {
  return queue->next_entry;
}

static inline int get_next_free(WorkQueue *queue) {
  return queue->next_free;
}

static inline void mark_completed(WorkQueue *queue) {
  queue->completed_entry_count++;
}

static inline bool is_completed(WorkQueue *queue) {
  return (int)queue->completion_goal <= (int)queue->completed_entry_count;
}

static inline bool update_next_entry(WorkQueue *queue, int next_idx) {
  return atomic_compare_exchange(&queue->next_entry, next_idx, (next_idx + 1) % (int)count_of(queue->entries));
}

static inline void update_next_free(WorkQueue *queue) {
  int next_free = get_next_free(queue);
  queue->next_free = (next_free + 1) % count_of(queue->entries);
  //SDL_AtomicSet(&queue->next_free, next_free + 1);
}

static inline int take_next_entry(WorkQueue *queue) {

  int next_idx = get_next_entry(queue);
  while (next_idx != get_next_free(queue)) {
    if (update_next_entry(queue, next_idx)) {
      return next_idx;
    }
    next_idx = get_next_entry(queue);
  }
  return -1;
}

static inline bool try_complete_next_entry(WorkQueue *queue) {
  int entry_idx = take_next_entry(queue);
  if (entry_idx >= 0) {
    auto entry = queue->entries + entry_idx;

    entry->callback(entry->data);

    mark_completed(queue);
    return true;
  }
  return false;
}

int worker_thread_proc(ThreadInfo *info) {
  auto thread_id = SDL_ThreadID();
  auto queue = info->queue;
  info->thread_id = thread_id;
  SDL_CompilerBarrier();
  info->received = true;

  printf("Created thread : %lu\n", SDL_ThreadID());

  while (true) {
    if (!try_complete_next_entry(queue)) {

      int error = SDL_SemWait(queue->semaphore);
      if (error) {
        printf("Error in thread %lu : %s\n", thread_id, SDL_GetError());
      }
    }
  }
}


//
// Higher level calls for users : ---
//

// NOTE : This also inits the queue and must be called before push_work. Returns the number created.
static inline int init_worker_threads(WorkQueue *queue, int count, u32 *thread_ids = NULL) {
  printf("Main thread is : %lu\n", SDL_ThreadID());
  auto semaphore = SDL_CreateSemaphore(0);
  *queue = {};
  queue->semaphore = semaphore;
  ThreadInfo info[count];

  for (int i = 0; i < count; i++) {
    info[i] = {};
    info[i].queue = queue;
    auto thread = SDL_CreateThread((SDL_ThreadFunction) worker_thread_proc, NULL, (void *) (info + i)); 
    if (!thread) {
      printf("Error creating thread %d : %s\n", i, SDL_GetError());
      count = i;
    }
  }

  // NOTE : this just makes sure that info doesn't leave scope before the threads are initialized.
  for (int i = 0; i < count; i++) {
    while (!info[i].received);
    if (thread_ids) thread_ids[i] = info[i].thread_id;
  }

  return count;
}

static inline void push_work(WorkQueue *queue, WorkQueueEntry work) {
  assert(queue->semaphore);

  // NOTE : because we are just getting the index this way, the queue is single 
  // producer multiple consumer.
  int entry_idx = get_next_free(queue);
  assert(entry_idx < count_of(queue->entries));
  auto entry = queue->entries + entry_idx;
  *entry = work;

  queue->completion_goal++;
  update_next_free(queue);
  int error = SDL_SemPost(queue->semaphore);
  if (error) {
    printf("Error in push_work : %s\n", SDL_GetError());
  }
}

static inline void push_work(WorkQueue *queue, void *data, work_queue_callback *callback) {
  push_work(queue, {data, callback});
}

static inline void complete_all_work(WorkQueue *queue) {
  while (!is_completed(queue)) {
    try_complete_next_entry(queue);
  }
  queue->completion_goal = 0;
  queue->completed_entry_count = 0;
}

//
// Test Code : ---
//

void thread_test_callback(const char *str) {
  auto thread_id = SDL_ThreadID();
  printf("Thread %lu : %s\n", thread_id, str);
}

static inline char *write_test_string(char *strings_head, char letter, int i) {
  assert(i < 100);
  assert(i >= 0);
  *strings_head = letter;
  strings_head++;
  *strings_head = '0' + i / 10;
  strings_head++;
  *strings_head = '0' + i % 10;
  strings_head++;
  *strings_head = '\0';
  strings_head++;
  return strings_head;
}

static inline void print_test_strings(WorkQueue *queue, char *strings, char letter, int max_total_size) {
  char *strings_head = strings;
  for (int i = 0; i < 100; i++) {
    WorkQueueEntry entry;
    char *this_string = strings_head;
    assert(strings_head < strings + max_total_size - 5);
    strings_head = write_test_string(strings_head, letter, i);
    entry.data = (void *) this_string;
    entry.callback = (work_queue_callback *) thread_test_callback;
    push_work(queue, entry);
  }
  complete_all_work(queue);
}

static void test_threads() {
  WorkQueue work_queue;
  auto queue = &work_queue;
  int count = init_worker_threads(queue, 3);
  assert(count == 3);
  char strings[1024];
  for (char letter = 'A'; letter <= 'A'; letter++) {
    print_test_strings(queue, strings, letter, sizeof(strings));
  }

  printf("Main thread is : %lu\n", SDL_ThreadID());
}

#endif // _NAR_SDL_THREAD_CPP_
