
// TODO make this multithreaded

struct SwapAllocatorReference {
  void *memory;
  uint32_t buffer_idx;
};

struct SwapBufferHead {
  uint32_t ref_count;
  uint32_t bytes_allocated;
};

#define SWAP_ALLOCATOR_BUFFER_COUNT 2

struct SwapAllocator {
  uint8_t *memory;
  SwapBufferHead buffers[SWAP_ALLOCATOR_BUFFER_COUNT];
  uint32_t active_buffer;
  uint32_t buffer_size; // NOTE : This is per buffer
};

static SwapAllocatorReference alloc_size(SwapAllocator *allocator, uint32_t size, uint64_t alignement = 1) {

  for (int i = 0; i < SWAP_ALLOCATOR_BUFFER_COUNT; i++) {
    uint32_t buffer_idx = (allocator->active_buffer + i) % SWAP_ALLOCATOR_BUFFER_COUNT;
    auto active_buffer = allocator->buffers + buffer_idx;
    auto active_memory = allocator->memory + allocator->buffer_size * buffer_idx; 

    uint32_t bytes_allocated = active_buffer->bytes_allocated;

    auto alignment_offset = get_alignment_offset(active_memory, bytes_allocated, alignment);

    if (bytes_allocated + size + alignment_offset > allocator->buffer_size) {
      continue;
    }

    // Allocation Success
    allocator->active_buffer = buffer_idx;
    active_buffer->bytes_allocated += size + alignment_offset;

    SwapAllocatorReference result;
    result.memory = active_memory + bytes_allocated + alignment_offset;
    result.buffer_idx = buffer_idx;
    assert(((uint64_t)result) % alignment == 0);
    return result;
  }

  // Allocation Failed
  assert(!"SwapAllocator out of memory.");
  return NULL;
}

static void free(SwapAllocator *allocator, SwapAllocatorReference ref) {
  auto buf = allocator->buffers + ref.buffer_idx;
  buf->ref_count--;
  if (buf->ref_count == 0) {
    buf->bytes_allocated = 0;
  }
}

void swap_allocator_test() {

}


