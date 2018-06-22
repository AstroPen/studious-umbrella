
#ifndef _PUSH_ALLOCATOR_H_
#define _PUSH_ALLOCATOR_H_

#ifndef TIMED_FUNCTION
#define PUSH_ALLOCATOR_NO_DEFINED_TIMER
#define TIMED_FUNCTION()
#endif

struct PushAllocator {
  uint8_t *memory;
#ifdef PUSH_ALLOCATOR_MULTITHREADED
  atomic_uint bytes_allocated;
#else
  uint32_t bytes_allocated;
#endif
  uint32_t max_size;
};

static inline bool is_initialized(PushAllocator *a) {
  if (!a) return false;
  if (!a->memory) return false;
  if (!a->max_size) return false;
  return true;
}

static inline uint32_t remaining_size(PushAllocator *allocator) {
  return allocator->max_size - allocator->bytes_allocated;
}

static inline uint64_t get_alignment_offset(uint8_t *memory, uint32_t bytes_allocated, uint64_t alignment) {
  uint64_t memory_index = (uint64_t) memory + bytes_allocated;
  auto mask = alignment - 1;
  uint64_t alignment_offset = 0;
  if (memory_index & mask) {
    alignment_offset = alignment - (memory_index & mask);
  }
  return alignment_offset;
}

// TODO Why is alignment a uint64_t? I have no idea why I decided that. It should probably be a uint32_t.
#ifdef PUSH_ALLOCATOR_MULTITHREADED
static inline void *alloc_size(PushAllocator *allocator, uint32_t size, uint64_t alignment = 1) {
  TIMED_FUNCTION();

  // NOTE : this will loop until the allocator is full or it successfully allocates
  while (true) {
    uint32_t bytes_allocated = allocator->bytes_allocated;
    auto alignment_offset = get_alignment_offset(allocator->memory, bytes_allocated, alignment);

    if (bytes_allocated + size + alignment_offset > allocator->max_size) {
      assert(!"PushAllocator out of memory.");
      return NULL;
    }
    uint32_t new_allocated = bytes_allocated + size + alignment_offset;
    if (atomic_compare_exchange(&allocator->bytes_allocated, bytes_allocated, new_allocated)) {
      void *result = allocator->memory + bytes_allocated + alignment_offset;
      //assert(((uint64_t)result) % alignment == 0);
      return result;
    }
  }
}
#else
// NOTE: Single threaded version
static inline void *alloc_size(PushAllocator *allocator, uint32_t size, uint64_t alignment = 1) {
  TIMED_FUNCTION();

  uint32_t bytes_allocated = allocator->bytes_allocated;
  auto alignment_offset = get_alignment_offset(allocator->memory, bytes_allocated, alignment);

  if (bytes_allocated + size + alignment_offset > allocator->max_size) {
    assert(!"PushAllocator out of memory.");
    return NULL;
  }
  allocator->bytes_allocated += size + alignment_offset;
  void *result = allocator->memory + bytes_allocated + alignment_offset;
  //assert(((uint64_t)result) % alignment == 0);
  return result;
}
#endif

static inline PushAllocator new_push_allocator(uint32_t size) {
  uint8_t *memory = (uint8_t *) calloc(size, 1);
  if (!memory) {
    return {};
  }

  PushAllocator result = {};
  result.memory = memory;
  result.max_size = size;

  return result;
}

static inline PushAllocator new_push_allocator(PushAllocator *old, uint32_t size, uint32_t alignment = 8) {
  uint8_t *memory = (uint8_t *) alloc_size(old, size, alignment);
  if (!memory) {
    return {};
  }

  PushAllocator result = {};
  result.memory = memory;
  result.max_size = size;

  return result;
}

static inline void free(PushAllocator *allocator) {
  allocator->bytes_allocated = 0;
}

// TODO make this thread safe somehow?
static inline void *pop_size(PushAllocator *allocator, uint32_t size) {
  assert(size < allocator->bytes_allocated);
  allocator->bytes_allocated -= size;
  return allocator->memory + allocator->bytes_allocated;
}

#define alloc_struct(allocator, type) ((type *) alloc_size((allocator), sizeof(type), alignof(type)))
#define alloc_array(allocator, type, count) ((type *) alloc_size((allocator), sizeof(type) * (count), alignof(type)))

// TODO make this thread safe somehow?
// TODO TODO TODO BUG FIX : 
//    This is being used dangerously, allocator's max size is not modified so this can only be called once.
//    It should either reduce allocator's size or act more like a regular allocation that gets cleared at
//    end of frame with free. Right now the code expects both somehow.
static inline PushAllocator subdivide(PushAllocator *allocator, uint32_t size) {
  if (size > 0) {
    if (size > allocator->max_size) return {};
    if (size + allocator->bytes_allocated > allocator->max_size) return {};
    auto end = allocator->memory + allocator->max_size;
    
    PushAllocator result = {};
    result.memory = end - size;
    result.max_size = size;
    return result;

  } else {
    assert(!"ERROR : Invalid size.");
    return {};
  }
}

#ifdef PUSH_ALLOCATOR_NO_DEFINED_TIMER
#undef TIMED_FUNCTION
#endif

#endif // _PUSH_ALLOCATOR_H_

