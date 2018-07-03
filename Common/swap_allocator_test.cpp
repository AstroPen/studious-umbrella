

#include "common.h"
#include "swap_allocator.h"

void swap_allocator_test() {
  printf("SwapAllocator test begin.\n");
  uint32_t buffer_size = 2048;
  assert(SWAP_ALLOCATOR_BUFFER_COUNT == 2);

  PushAllocator mem = new_push_allocator(calc_swap_allocator_memory_size(buffer_size));
  assert(is_initialized(&mem));
  SwapAllocator a_ = new_swap_allocator(&mem, buffer_size);
  auto a = &a_;
  assert(is_initialized(a));
  auto buffers = a->buffers;
  auto buf_1 = buffers;
  auto buf_2 = buffers + 1;

  auto ref = alloc_size(a, buffer_size);
  auto ref_1 = ref;
  assert(ref.memory);
  assert(ref.memory == a->memory);
  assert(ref.buffer_idx == 0);

  assert(buf_1->ref_count == 1);
  assert(buf_1->bytes_allocated == buffer_size);
  assert(buf_2->ref_count == 0);
  assert(buf_2->bytes_allocated == 0);

  ref = alloc_size(a, buffer_size);
  auto ref_2 = ref;
  assert(ref.memory);
  assert(ref_1.memory != ref_2.memory);
  assert(ref_1.buffer_idx != ref_2.buffer_idx);

  assert(buf_1->ref_count == 1);
  assert(buf_1->bytes_allocated == buffer_size);
  assert(buf_2->ref_count == 1);
  assert(buf_2->bytes_allocated == buffer_size);

  ref = alloc_size(a, buffer_size);
  assert(!ref.memory);

  free(a, ref_1);
  assert(buf_1->ref_count == 0);
  assert(buf_1->bytes_allocated == 0);
  assert(buf_2->ref_count == 1);
  assert(buf_2->bytes_allocated == buffer_size);

  ref = alloc_size(a, buffer_size);
  assert(ref.memory);
  assert(ref.memory == ref_1.memory);
  assert(ref.buffer_idx == ref_1.buffer_idx);

  free(a, ref_2.memory);
  assert(buf_1->ref_count == 1);
  assert(buf_1->bytes_allocated == buffer_size);
  assert(buf_2->ref_count == 0);
  assert(buf_2->bytes_allocated == 0);

  SwapAllocatorReference refs[4];
  ref = refs[0] = alloc_size(a, buffer_size / 4);
  assert(ref.memory);
  assert(ref.buffer_idx == 1);
  assert(buf_2->ref_count == 1);

  ref = refs[1] = alloc_size(a, buffer_size / 4);
  assert(ref.memory);
  assert(ref.buffer_idx == 1);
  assert(buf_2->ref_count == 2);

  ref = refs[2] = alloc_size(a, buffer_size / 4);
  assert(ref.memory);
  assert(ref.buffer_idx == 1);
  assert(buf_2->ref_count == 3);

  ref = refs[3] = alloc_size(a, buffer_size / 4);
  assert(ref.memory);
  assert(ref.buffer_idx == 1);
  assert(buf_2->ref_count == 4);

  free(a, refs[2]);
  assert(buf_2->ref_count == 3);
  free(a, refs[0]);
  assert(buf_2->ref_count == 2);
  free(a, refs[3].memory);
  assert(buf_2->ref_count == 1);
  free(a, refs[1]);
  assert(buf_2->ref_count == 0);

  clear(a);
  assert(buf_1->ref_count == 0);
  assert(buf_1->bytes_allocated == 0);
  assert(buf_2->ref_count == 0);
  assert(buf_2->bytes_allocated == 0);

  uint32_t mid_size = buffer_size * 3 / 4;
  ref = alloc_size(a, mid_size);
  ref_1 = ref;
  assert(ref.memory);
  assert(ref.memory == a->memory);
  assert(ref.buffer_idx == 0);

  assert(buf_1->ref_count == 1);
  assert(buf_1->bytes_allocated == mid_size);
  assert(buf_2->ref_count == 0);
  assert(buf_2->bytes_allocated == 0);

  ref = alloc_size(a, mid_size);
  ref_2 = ref;
  assert(ref.memory);
  assert(ref_1.memory != ref_2.memory);
  assert(ref_1.buffer_idx != ref_2.buffer_idx);

  assert(buf_1->ref_count == 1);
  assert(buf_1->bytes_allocated == mid_size);
  assert(buf_2->ref_count == 1);
  assert(buf_2->bytes_allocated == mid_size);

  ref = alloc_size(a, mid_size);
  assert(!ref.memory);

  free(a, ref_1);
  assert(buf_1->ref_count == 0);
  assert(buf_1->bytes_allocated == 0);
  assert(buf_2->ref_count == 1);
  assert(buf_2->bytes_allocated == mid_size);

  ref = alloc_size(a, mid_size);
  assert(ref.memory);
  assert(ref.memory == ref_1.memory);
  assert(ref.buffer_idx == ref_1.buffer_idx);

  free(a, ref_2.memory);
  assert(buf_1->ref_count == 1);
  assert(buf_1->bytes_allocated == mid_size);
  assert(buf_2->ref_count == 0);
  assert(buf_2->bytes_allocated == 0);

  printf("SwapAllocator test successful.\n\n");
}

int main() {
  swap_allocator_test();

  return EXIT_SUCCESS;
}

