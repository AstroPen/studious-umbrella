

#ifndef _HEAP_ALLOCATOR_H_
#define _HEAP_ALLOCATOR_H_

#include "push_allocator.h"

// TODO I should rename this to something like "ListAllocator" or something.

// TODO move this definition to some sort of shared "data structures" file?
struct FreeListNode {
  FreeListNode *next_free;
};

// TODO maybe add a way to have the heap automatically expand instead of using create_heap
// We could store a PushAllocator pointer inside and dynamically expand whenever full
struct HeapAllocator {
  FreeListNode *free_list;
  uint32_t element_size;
  uint32_t count; // NOTE : this is currently unused and only here for future debug help
};

#define calc_max_needed_memory_size(count, size, alignment) ((count) * (size) + (alignment))

static inline HeapAllocator create_heap_(PushAllocator *allocator, uint32_t elem_count, uint32_t elem_size, uint64_t alignment) {
  assert(elem_size >= 8);
  if (alignment < 8) alignment = 8;
  auto memory = (uint8_t *) alloc_size(allocator, elem_size * elem_count, alignment);
  if (!memory) return {};

  auto start_ptr = memory + (elem_count - 1) * elem_size;
  FreeListNode *prev = NULL;
  for (auto ptr = start_ptr; ptr >= memory; ptr -= elem_size) {
    auto node = (FreeListNode *) ptr;
    node->next_free = prev;
    prev = node;
  }

  HeapAllocator result;
  result.element_size = elem_size;
  result.free_list = prev;
  result.count = elem_count;
  return result;
}

#define create_heap(allocator, count, type) (create_heap_((allocator), (count), sizeof(type), alignof(type)))

static inline void *alloc_element_(HeapAllocator *allocator, uint32_t size = 0) {
  if (size) assert(size == allocator->element_size);
  while (true) {
    FreeListNode *next = allocator->free_list;
    if (!next) {
      assert(!"HeapAllocator out of memory.");
      return NULL;
    }
    //allocator->free_list = next->next_free;
    if (atomic_compare_exchange((void **) &allocator->free_list, next, next->next_free)) {
      return (void *) next;
    }
  }
}

static inline void free_element_(HeapAllocator *allocator, void *elem) {
  auto new_free = (FreeListNode *) elem;
  int count = 0;
  while (true) {
    auto last_free = allocator->free_list;
    new_free->next_free = last_free;
    if (atomic_compare_exchange((void **) &allocator->free_list, last_free, new_free)) return;
    count++;
    assert(count < 100);
  }
}

#define alloc_element(allocator, type) ((type *) alloc_element_((allocator), sizeof(type)))
#define free_element(allocator, elem) { \
  assert(sizeof(*(elem)) == (allocator)->element_size); \
  free_element_((allocator), (void *) elem); \
}

#endif // _HEAP_ALLOCATOR_H_


