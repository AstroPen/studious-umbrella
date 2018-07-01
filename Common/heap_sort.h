
#ifndef _HEAP_SORT_H_
#define _HEAP_SORT_H_

// NOTE : This was copied from chess.cpp.
inline int compare(int *a, int *b) {
  if (*a < *b) return -1;
  if (*a == *b) return 0;
  return 1;
}

// TODO move this into common.h or something
#define SWAP(a, b) { auto temp = *(a); *(a) = *(b); *(b) = temp; }

// TODO make an overlaod that doesn't take a start_idx that makes the whole thing a heap (without assumptions)?
template <typename T> 
void heapify(T *arr, uint32_t count, uint32_t start_idx = 0) {

  uint32_t min_idx = start_idx;
  uint32_t l = 2*start_idx + 1;
  uint32_t r = 2*start_idx + 2;

  // If left child is larger than root
  if (l < count) {
    if (compare(arr + l, arr + min_idx) < 0) min_idx = l;
  }

  // If right child is larger than largest so far
  if (r < count) {
    if (compare(arr + r, arr + min_idx) < 0) min_idx = r;
  }

  // If largest is not root
  if (min_idx != start_idx) {

    SWAP(arr + start_idx, arr + min_idx);
    /*
    T temp = arr[start_idx];
    arr[start_idx] = arr[min_idx];
    arr[min_idx] = temp;
    */

    // Recursively heapify the affected sub-tree
    heapify(arr, count, min_idx);
  }
}

// TODO move this to a namespace
template <typename T>
bool is_heap(T *arr, uint32_t count, uint32_t start_idx = 0) {
  int l = 2*start_idx + 1;
  int r = 2*start_idx + 2;

  assert(start_idx < count);

  if (l < count) {
    if (compare(arr + l, arr + start_idx) < 0) return false;
    if (!is_heap(arr, count, l)) return false;
  }

  // If right child is larger than largest so far
  if (r < count) {
    if (compare(arr + r, arr + start_idx) < 0) return false;
    if (!is_heap(arr, count, r)) return false;
  }

  return true; 
}
 
template <typename T>
void heap_sort(T *arr, uint32_t count) {
  if (!count) return;

  for (int i = count / 2 - 1; i >= 0; i--) {
    heapify(arr, count, i);
  }

  // TODO comment this out, make a test instead
  assert(is_heap(arr, count));

  // One by one extract an element from heap
  for (uint32_t i = count - 1; i > 0; i--) {
    SWAP(arr, arr + i); // TODO make "pop min" function or something?
    heapify(arr, i, 0);
  }
}

template <typename T>
bool is_sorted(T *arr, uint32_t count) {
  for (uint32_t i = 0; i < count - 1; i++) {
    if (compare(arr + i, arr + i + 1) < 0) return false;
  }
  return true;
}

// TODO
void test_heap_sort() {
  int ints[] = { 3, 7, -1, 8, 4, 10, 1, 3, 100, -100};
  uint32_t count = count_of(ints);
  assert(!is_sorted(ints, count));
  heap_sort(ints, count);
  for (int i = 0; i < count; i++) {
    printf("%d, ", ints[i]);
  }
  printf("\n");
  assert(is_sorted(ints, count));
}

#endif
