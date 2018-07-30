#ifndef _POOL_ALLOCATOR_H_
#define _POOL_ALLOCATOR_H_

#include "push_allocator.h"
#include "dynamic_array.h"

// TODO test this

// WARNING : This is not thread safe even if PushAllocator is multithreaded
struct PoolAllocatorNode {
  PushAllocator allocator;
  PoolAllocatorNode *next;
};

struct PoolAllocator {
  darray<PoolAllocatorNode> nodes;
};

#define POOL_MIN_BLOCK_SIZE (2048*32)

// TODO : Investigate possible weirdness with malloc alignments. If malloc gives an
// alignment smaller than the argument passed in, and size is larger than POOL_MIN_BLOCK_SIZE,
// then this will probably fail.
static uint8_t *allocate(PoolAllocator *pool, uint64_t size, uint32_t alignment) {
  assert(alignment <= sizeof(max_align_t));
  auto node = peek(pool->nodes);
  uint8_t *result = NULL;
  if (node) {
    result = alloc_size(node->allocator, size, alignment);
    if (result) return result;
  }
  auto new_node = push(pool->nodes);
  if (!new_node) return NULL;
  *new_node = {};
  new_node->next = node;
  uint64_t block_size = max(size, POOL_MIN_BLOCK_SIZE);
  new_node->allocator = new_push_allocator(block_size);
  if (!is_initialized(new_node->allocator)) return NULL;
  return alloc_size(new_node->allocator, size, alignment);
}

#endif

