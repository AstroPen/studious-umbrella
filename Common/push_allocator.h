
#ifndef _PUSH_ALLOCATOR_H_
#define _PUSH_ALLOCATOR_H_

#ifndef TIMED_FUNCTION
#define PUSH_ALLOCATOR_NO_DEFINED_TIMER
#define TIMED_FUNCTION()
#endif

struct PushAllocator {
  u8 *memory;
#ifdef PUSH_ALLOCATOR_MULTITHREADED
  atomic_uint bytes_allocated;
#else
  u32 bytes_allocated;
#endif
  u32 max_size;
};

static inline bool is_initialized(PushAllocator *a) {
  if (!a) return false;
  if (!a->memory) return false;
  if (!a->max_size) return false;
  return true;
}

static inline u32 remaining_size(PushAllocator *allocator) {
  return allocator->max_size - allocator->bytes_allocated;
}

static inline u64 get_alignment_offset(u8 *memory, u32 bytes_allocated, u64 alignment) {
  u64 memory_index = (u64) memory + bytes_allocated;
  auto mask = alignment - 1;
  u64 alignment_offset = 0;
  if (memory_index & mask) {
    alignment_offset = alignment - (memory_index & mask);
  }
  return alignment_offset;
}

// TODO Why is alignment a u64? I have no idea why I decided that. It should probably be a u32.
#ifdef PUSH_ALLOCATOR_MULTITHREADED
static void *alloc_size(PushAllocator *allocator, u32 size, u64 alignment = 1) {
  TIMED_FUNCTION();

  if (!size) return NULL;
  // NOTE : this will loop until the allocator is full or it successfully allocates
  while (true) {
    u32 bytes_allocated = allocator->bytes_allocated;
    auto alignment_offset = get_alignment_offset(allocator->memory, bytes_allocated, alignment);

    if (bytes_allocated + size + alignment_offset > allocator->max_size) {
      FAILURE("PushAllocator out of memory.", allocator->bytes_allocated, allocator->max_size, size);
      return NULL;
    }
    u32 new_allocated = bytes_allocated + size + alignment_offset;
    if (atomic_compare_exchange(&allocator->bytes_allocated, bytes_allocated, new_allocated)) {
      void *result = allocator->memory + bytes_allocated + alignment_offset;
      //assert(((u64)result) % alignment == 0);
      return result;
    }
  }
}
#else
// NOTE: Single threaded version
static void *alloc_size(PushAllocator *allocator, u32 size, u64 alignment = 1) {
  TIMED_FUNCTION();

  if (!size) return NULL;
  u32 bytes_allocated = allocator->bytes_allocated;
  auto alignment_offset = get_alignment_offset(allocator->memory, bytes_allocated, alignment);

  if (bytes_allocated + size + alignment_offset > allocator->max_size) {
    FAILURE("PushAllocator out of memory.", allocator->bytes_allocated, allocator->max_size, size);
    return NULL;
  }
  allocator->bytes_allocated += size + alignment_offset;
  void *result = allocator->memory + bytes_allocated + alignment_offset;
  //assert(((u64)result) % alignment == 0);
  return result;
}
#endif

inline PushAllocator new_push_allocator(u32 size) {
  u8 *memory = (u8 *) calloc(size, 1);
  if (!memory) {
    return {};
  }

  PushAllocator result = {};
  result.memory = memory;
  result.max_size = size;

  return result;
}

inline PushAllocator new_push_allocator(PushAllocator *old, u32 size, u32 alignment = 8) {
  u8 *memory = (u8 *) alloc_size(old, size, alignment);
  if (!memory) {
    return {};
  }

  PushAllocator result = {};
  result.memory = memory;
  result.max_size = size;

  return result;
}

struct TemporaryAllocator : PushAllocator {
  inline TemporaryAllocator() {
    memory = NULL;
    bytes_allocated = 0;
    max_size = 0;
  }
  inline TemporaryAllocator(PushAllocator alloc) {
    memory = alloc.memory;
    bytes_allocated = alloc.bytes_allocated;
    max_size = alloc.max_size;
  }
  inline ~TemporaryAllocator() {
    if (memory) ASSERT(!"Temporary Allocator was not freed.");
  }
};

static inline TemporaryAllocator push_temporary(PushAllocator *old) {
  return new_push_allocator(old, remaining_size(old), 1);
}

static inline void pop_temporary(PushAllocator *old, TemporaryAllocator *temporary) {
  // NOTE : To make sure the args were passed in the right order :
  assert(old->max_size >= temporary->max_size);

  // NOTE : This should hopefully catch cases where the wrong temporary is being freed, or
  // someone is using the old one in another thread or something.
  assert(old->memory + old->bytes_allocated == temporary->memory + temporary->max_size);

  old->bytes_allocated -= temporary->max_size;
  *temporary = {};
}

static inline void clear(PushAllocator *allocator) {
  allocator->bytes_allocated = 0;
}

// TODO make this thread safe somehow?
static inline void *pop_size(PushAllocator *allocator, u32 size) {
  assert(size < allocator->bytes_allocated);
  allocator->bytes_allocated -= size;
  return allocator->memory + allocator->bytes_allocated;
}

#define ALLOC_STRUCT(allocator, type) ((type *) alloc_size((allocator), sizeof(type), alignof(type)))
#define ALLOC_ARRAY(allocator, type, count) ((type *) alloc_size((allocator), sizeof(type) * (count), alignof(type)))

#ifdef PUSH_ALLOCATOR_NO_DEFINED_TIMER
#undef TIMED_FUNCTION
#endif

#ifndef PUSH_ALLOCATOR_MULTITHREADED 
#define _PUSH_ALLOCATOR_SINGLE_THREADED_INCLUDE_
#endif

#else // _PUSH_ALLOCATOR_H_

#if defined(PUSH_ALLOCATOR_MULTITHREADED) && defined(_PUSH_ALLOCATOR_SINGLE_THREADED_INCLUDE_)
#warning "push_allocator.h was previously included as single threaded."
#endif

#endif

